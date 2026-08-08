#include "constant.h"

#include <limits.h>
#include <string.h>

static const size_t VITTE_CONSTANT_DEFAULT_MAX_DEPTH = 256u;

static void vitte_constant_set_error(
    vitte_constant_folder_t *folder,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (folder != NULL) {
        vitte_error_set_details(&folder->last_error, status, code, message, details);
        folder->stats.error_count++;
    }
}

void vitte_constant_options_init(vitte_constant_options_t *options) {
    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->max_depth = VITTE_CONSTANT_DEFAULT_MAX_DEPTH;
    options->allow_builtin_constants = true;
    options->allow_string_concat = false;
    options->detect_overflow = true;
}

void vitte_constant_value_init(vitte_constant_value_t *value) {
    if (value == NULL) {
        return;
    }

    memset(value, 0, sizeof(*value));
    value->kind = VITTE_CONSTANT_NONE;
    value->type = VITTE_BUILTIN_TYPE_ERROR;
}

void vitte_constant_result_init(vitte_constant_result_t *result) {
    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->status = VITTE_STATUS_OK;
    vitte_constant_value_init(&result->value);
}

static vitte_constant_value_t vitte_constant_bool(bool value) {
    vitte_constant_value_t constant;

    vitte_constant_value_init(&constant);
    constant.kind = VITTE_CONSTANT_BOOL;
    constant.type = VITTE_BUILTIN_TYPE_BOOL;
    constant.valid = true;
    constant.as.boolean = value;
    return constant;
}

static vitte_constant_value_t vitte_constant_int(int64_t value) {
    vitte_constant_value_t constant;

    vitte_constant_value_init(&constant);
    constant.kind = VITTE_CONSTANT_I64;
    constant.type = VITTE_BUILTIN_TYPE_I64;
    constant.valid = true;
    constant.as.integer = value;
    return constant;
}

static vitte_constant_value_t vitte_constant_string(const char *value) {
    vitte_constant_value_t constant;

    vitte_constant_value_init(&constant);
    constant.kind = VITTE_CONSTANT_STRING;
    constant.type = VITTE_BUILTIN_TYPE_STRING;
    constant.valid = true;
    constant.as.string = value != NULL ? value : "";
    return constant;
}

vitte_status_t vitte_constant_folder_init(
    vitte_constant_folder_t *folder,
    const vitte_constant_options_t *options
) {
    vitte_constant_options_t defaults;

    if (folder == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(folder, 0, sizeof(*folder));
    folder->options = options != NULL ? *options : (vitte_constant_options_init(&defaults), defaults);
    if (folder->options.max_depth == 0u) {
        folder->options.max_depth = VITTE_CONSTANT_DEFAULT_MAX_DEPTH;
    }
    vitte_error_init(&folder->last_error);
    vitte_builtin_registry_init(&folder->builtins);
    if (vitte_builtin_registry_validate(&folder->builtins) != VITTE_STATUS_OK) {
        vitte_error_copy(&folder->last_error, vitte_builtin_registry_last_error(&folder->builtins));
        return VITTE_STATUS_ERROR_INTERNAL;
    }
    folder->initialized = true;
    return VITTE_STATUS_OK;
}

void vitte_constant_folder_destroy(vitte_constant_folder_t *folder) {
    if (folder == NULL) {
        return;
    }

    memset(folder, 0, sizeof(*folder));
}

bool vitte_constant_folder_is_initialized(const vitte_constant_folder_t *folder) {
    return folder != NULL && folder->initialized;
}

const vitte_error_t *vitte_constant_folder_last_error(const vitte_constant_folder_t *folder) {
    return folder != NULL ? &folder->last_error : vitte_error_last();
}

void vitte_constant_folder_clear_error(vitte_constant_folder_t *folder) {
    if (folder != NULL) {
        vitte_error_reset(&folder->last_error);
    }
}

const vitte_constant_stats_t *vitte_constant_folder_stats(const vitte_constant_folder_t *folder) {
    return folder != NULL ? &folder->stats : NULL;
}

const char *vitte_constant_kind_name(vitte_constant_kind_t kind) {
    switch (kind) {
        case VITTE_CONSTANT_NONE:
            return "none";
        case VITTE_CONSTANT_BOOL:
            return "bool";
        case VITTE_CONSTANT_INT:
            return "int";
        case VITTE_CONSTANT_I64:
            return "i64";
        case VITTE_CONSTANT_STRING:
            return "string";
        case VITTE_CONSTANT_ERROR:
            return "error";
        default:
            return "unknown";
    }
}

bool vitte_constant_value_is_truthy(const vitte_constant_value_t *value) {
    if (value == NULL || !value->valid) {
        return false;
    }
    switch (value->kind) {
        case VITTE_CONSTANT_BOOL:
            return value->as.boolean;
        case VITTE_CONSTANT_INT:
        case VITTE_CONSTANT_I64:
            return value->as.integer != 0;
        case VITTE_CONSTANT_STRING:
            return value->as.string != NULL && value->as.string[0] != '\0';
        default:
            return false;
    }
}

bool vitte_constant_values_equal(const vitte_constant_value_t *left, const vitte_constant_value_t *right) {
    if (left == NULL || right == NULL || !left->valid || !right->valid || left->kind != right->kind) {
        return false;
    }
    switch (left->kind) {
        case VITTE_CONSTANT_BOOL:
            return left->as.boolean == right->as.boolean;
        case VITTE_CONSTANT_INT:
        case VITTE_CONSTANT_I64:
            return left->as.integer == right->as.integer;
        case VITTE_CONSTANT_STRING:
            return strcmp(left->as.string != NULL ? left->as.string : "", right->as.string != NULL ? right->as.string : "") == 0;
        default:
            return false;
    }
}

static bool vitte_constant_is_integer(const vitte_constant_value_t *value) {
    return value != NULL &&
        value->valid &&
        (value->kind == VITTE_CONSTANT_INT || value->kind == VITTE_CONSTANT_I64);
}

static bool vitte_constant_add_overflows(int64_t left, int64_t right, int64_t *out) {
    if ((right > 0 && left > INT64_MAX - right) ||
        (right < 0 && left < INT64_MIN - right)) {
        return true;
    }
    *out = left + right;
    return false;
}

static bool vitte_constant_sub_overflows(int64_t left, int64_t right, int64_t *out) {
    if ((right < 0 && left > INT64_MAX + right) ||
        (right > 0 && left < INT64_MIN + right)) {
        return true;
    }
    *out = left - right;
    return false;
}

static bool vitte_constant_mul_overflows(int64_t left, int64_t right, int64_t *out) {
    if (left == 0 || right == 0) {
        *out = 0;
        return false;
    }
    if (left == -1 && right == INT64_MIN) {
        return true;
    }
    if (right == -1 && left == INT64_MIN) {
        return true;
    }
    if (left > 0) {
        if ((right > 0 && left > INT64_MAX / right) ||
            (right < 0 && right < INT64_MIN / left)) {
            return true;
        }
    } else {
        if ((right > 0 && left < INT64_MIN / right) ||
            (right < 0 && left < INT64_MAX / right)) {
            return true;
        }
    }
    *out = left * right;
    return false;
}

static vitte_status_t vitte_constant_fail(
    vitte_constant_folder_t *folder,
    vitte_constant_result_t *result,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (result != NULL) {
        vitte_constant_result_init(result);
        result->status = status;
        result->diagnostic = message;
    }
    if (status == VITTE_STATUS_ERROR_UNSUPPORTED && folder != NULL) {
        folder->stats.unsupported_count++;
    }
    vitte_constant_set_error(folder, status, code, message, details);
    return status;
}

static vitte_status_t vitte_constant_eval_node(
    vitte_constant_folder_t *folder,
    const vitte_ast_expr_t *expr,
    vitte_constant_result_t *result,
    size_t depth
);

static vitte_status_t vitte_constant_eval_identifier(
    vitte_constant_folder_t *folder,
    const vitte_ast_expr_t *expr,
    vitte_constant_result_t *result
) {
    const vitte_builtin_constant_t *constant;

    if (expr->as.identifier.name == NULL || !folder->options.allow_builtin_constants) {
        return vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_CONSTANT_E_UNSUPPORTED", "identifier is not a constant", expr->as.identifier.name);
    }

    constant = vitte_builtin_lookup_constant(&folder->builtins, expr->as.identifier.name);
    if (constant == NULL) {
        return vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_CONSTANT_E_UNSUPPORTED", "identifier is not a known builtin constant", expr->as.identifier.name);
    }
    if (constant->type == VITTE_BUILTIN_TYPE_BOOL) {
        result->value = vitte_constant_bool(constant->bool_value);
    } else if (vitte_builtin_type_is_integer(constant->type)) {
        result->value = vitte_constant_int(constant->int_value);
    } else if (constant->type == VITTE_BUILTIN_TYPE_STRING) {
        result->value = vitte_constant_string(constant->text_value);
    } else {
        return vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_CONSTANT_E_UNSUPPORTED", "unsupported builtin constant type", expr->as.identifier.name);
    }

    result->status = VITTE_STATUS_OK;
    result->folded = true;
    result->source = expr;
    folder->stats.builtin_constants_resolved++;
    folder->stats.expressions_folded++;
    vitte_error_reset(&folder->last_error);
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_constant_eval_binary_integer(
    vitte_constant_folder_t *folder,
    const char *operator_text,
    const vitte_constant_value_t *left,
    const vitte_constant_value_t *right,
    vitte_constant_result_t *result
) {
    int64_t value = 0;
    bool overflow = false;

    if (strcmp(operator_text, "+") == 0) {
        overflow = folder->options.detect_overflow && vitte_constant_add_overflows(left->as.integer, right->as.integer, &value);
    } else if (strcmp(operator_text, "-") == 0) {
        overflow = folder->options.detect_overflow && vitte_constant_sub_overflows(left->as.integer, right->as.integer, &value);
    } else if (strcmp(operator_text, "*") == 0) {
        overflow = folder->options.detect_overflow && vitte_constant_mul_overflows(left->as.integer, right->as.integer, &value);
    } else if (strcmp(operator_text, "/") == 0) {
        if (right->as.integer == 0) {
            folder->stats.divide_by_zero_count++;
            return vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONSTANT_E_DIV_ZERO", "division by zero in constant expression", operator_text);
        }
        if (folder->options.detect_overflow && left->as.integer == INT64_MIN && right->as.integer == -1) {
            overflow = true;
        } else {
            value = left->as.integer / right->as.integer;
        }
    } else if (strcmp(operator_text, "%") == 0) {
        if (right->as.integer == 0) {
            folder->stats.divide_by_zero_count++;
            return vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONSTANT_E_DIV_ZERO", "modulo by zero in constant expression", operator_text);
        }
        if (folder->options.detect_overflow && left->as.integer == INT64_MIN && right->as.integer == -1) {
            overflow = true;
        } else {
            value = left->as.integer % right->as.integer;
        }
    } else if (strcmp(operator_text, "==") == 0) {
        result->value = vitte_constant_bool(left->as.integer == right->as.integer);
        return VITTE_STATUS_OK;
    } else if (strcmp(operator_text, "!=") == 0) {
        result->value = vitte_constant_bool(left->as.integer != right->as.integer);
        return VITTE_STATUS_OK;
    } else if (strcmp(operator_text, "<") == 0) {
        result->value = vitte_constant_bool(left->as.integer < right->as.integer);
        return VITTE_STATUS_OK;
    } else if (strcmp(operator_text, "<=") == 0) {
        result->value = vitte_constant_bool(left->as.integer <= right->as.integer);
        return VITTE_STATUS_OK;
    } else if (strcmp(operator_text, ">") == 0) {
        result->value = vitte_constant_bool(left->as.integer > right->as.integer);
        return VITTE_STATUS_OK;
    } else if (strcmp(operator_text, ">=") == 0) {
        result->value = vitte_constant_bool(left->as.integer >= right->as.integer);
        return VITTE_STATUS_OK;
    } else {
        return vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_CONSTANT_E_OPERATOR", "unsupported integer constant operator", operator_text);
    }

    if (overflow) {
        folder->stats.overflow_count++;
        return vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONSTANT_E_OVERFLOW", "integer overflow in constant expression", operator_text);
    }
    result->value = vitte_constant_int(value);
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_constant_eval_binary(
    vitte_constant_folder_t *folder,
    const vitte_ast_expr_t *expr,
    vitte_constant_result_t *result,
    size_t depth
) {
    vitte_constant_result_t left;
    vitte_constant_result_t right;
    const char *operator_text = expr->as.binary_expr.operator_text;
    vitte_status_t status;

    if (operator_text == NULL) {
        return vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONSTANT_E_OPERATOR", "missing constant operator", NULL);
    }

    vitte_constant_result_init(&left);
    vitte_constant_result_init(&right);
    status = vitte_constant_eval_node(folder, expr->as.binary_expr.left, &left, depth + 1u);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_constant_eval_node(folder, expr->as.binary_expr.right, &right, depth + 1u);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    if (vitte_constant_is_integer(&left.value) && vitte_constant_is_integer(&right.value)) {
        status = vitte_constant_eval_binary_integer(folder, operator_text, &left.value, &right.value, result);
    } else if (left.value.kind == VITTE_CONSTANT_BOOL && right.value.kind == VITTE_CONSTANT_BOOL) {
        if (strcmp(operator_text, "&&") == 0) {
            result->value = vitte_constant_bool(left.value.as.boolean && right.value.as.boolean);
            status = VITTE_STATUS_OK;
        } else if (strcmp(operator_text, "||") == 0) {
            result->value = vitte_constant_bool(left.value.as.boolean || right.value.as.boolean);
            status = VITTE_STATUS_OK;
        } else if (strcmp(operator_text, "==") == 0) {
            result->value = vitte_constant_bool(left.value.as.boolean == right.value.as.boolean);
            status = VITTE_STATUS_OK;
        } else if (strcmp(operator_text, "!=") == 0) {
            result->value = vitte_constant_bool(left.value.as.boolean != right.value.as.boolean);
            status = VITTE_STATUS_OK;
        } else {
            status = vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_CONSTANT_E_OPERATOR", "unsupported boolean constant operator", operator_text);
        }
    } else if (left.value.kind == VITTE_CONSTANT_STRING && right.value.kind == VITTE_CONSTANT_STRING) {
        if (strcmp(operator_text, "==") == 0) {
            result->value = vitte_constant_bool(vitte_constant_values_equal(&left.value, &right.value));
            status = VITTE_STATUS_OK;
        } else if (strcmp(operator_text, "!=") == 0) {
            result->value = vitte_constant_bool(!vitte_constant_values_equal(&left.value, &right.value));
            status = VITTE_STATUS_OK;
        } else if (strcmp(operator_text, "+") == 0 && folder->options.allow_string_concat) {
            status = vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_CONSTANT_E_UNSUPPORTED", "string concatenation requires allocation and is unsupported in bootstrap constant fold", operator_text);
        } else {
            status = vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_CONSTANT_E_OPERATOR", "unsupported string constant operator", operator_text);
        }
    } else {
        status = vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONSTANT_E_TYPE", "constant operator operand types do not match", operator_text);
    }

    if (status == VITTE_STATUS_OK) {
        result->status = VITTE_STATUS_OK;
        result->folded = true;
        result->source = expr;
        folder->stats.expressions_folded++;
        vitte_error_reset(&folder->last_error);
    }
    return status;
}

static vitte_status_t vitte_constant_eval_node(
    vitte_constant_folder_t *folder,
    const vitte_ast_expr_t *expr,
    vitte_constant_result_t *result,
    size_t depth
) {
    if (result != NULL) {
        vitte_constant_result_init(result);
        result->source = expr;
    }
    if (expr == NULL) {
        return vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONSTANT_E_ARGUMENT", "missing expression for constant evaluation", NULL);
    }
    if (depth > folder->options.max_depth) {
        return vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONSTANT_E_DEPTH", "constant evaluation depth exceeded", NULL);
    }

    folder->stats.expressions_seen++;
    switch (expr->kind) {
        case VITTE_AST_NODE_INTEGER_LITERAL:
            result->value = vitte_constant_int(expr->as.integer_literal.value);
            result->status = VITTE_STATUS_OK;
            result->folded = true;
            folder->stats.expressions_folded++;
            vitte_error_reset(&folder->last_error);
            return VITTE_STATUS_OK;
        case VITTE_AST_NODE_STRING_LITERAL:
            result->value = vitte_constant_string(expr->as.string_literal.value);
            result->status = VITTE_STATUS_OK;
            result->folded = true;
            folder->stats.expressions_folded++;
            vitte_error_reset(&folder->last_error);
            return VITTE_STATUS_OK;
        case VITTE_AST_NODE_IDENTIFIER:
            return vitte_constant_eval_identifier(folder, expr, result);
        case VITTE_AST_NODE_BINARY_EXPR:
            return vitte_constant_eval_binary(folder, expr, result, depth);
        case VITTE_AST_NODE_ERROR:
            result->value.kind = VITTE_CONSTANT_ERROR;
            return vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONSTANT_E_ARGUMENT", "error node cannot be folded", expr->as.error_node.message);
        default:
            return vitte_constant_fail(folder, result, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_CONSTANT_E_UNSUPPORTED", "AST node is not constant-foldable", vitte_ast_node_kind_name(expr->kind));
    }
}

vitte_status_t vitte_constant_eval_expr(
    vitte_constant_folder_t *folder,
    const vitte_ast_expr_t *expr,
    vitte_constant_result_t *result
) {
    if (!vitte_constant_folder_is_initialized(folder)) {
        if (result != NULL) {
            vitte_constant_result_init(result);
            result->status = VITTE_STATUS_ERROR_INVALID_STATE;
        }
        vitte_constant_set_error(folder, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CONSTANT_E_STATE", "constant folder is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_constant_eval_node(folder, expr, result, 0u);
}
