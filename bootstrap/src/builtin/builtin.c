#include "builtin.h"

#include <string.h>

static const vitte_builtin_type_t VITTE_BUILTIN_TYPES[] = {
    {"void", VITTE_BUILTIN_TYPE_VOID, 0u, 0u, false, false, false, false, false},
    {"bool", VITTE_BUILTIN_TYPE_BOOL, 1u, 1u, false, true, false, false, false},
    {"int", VITTE_BUILTIN_TYPE_INT, 4u, 4u, true, false, false, false, false},
    {"i64", VITTE_BUILTIN_TYPE_I64, 8u, 8u, true, false, false, false, false},
    {"string", VITTE_BUILTIN_TYPE_STRING, sizeof(const char *), _Alignof(const char *), false, false, true, false, false},
    {"never", VITTE_BUILTIN_TYPE_NEVER, 0u, 0u, false, false, false, true, false},
    {"error", VITTE_BUILTIN_TYPE_ERROR, 0u, 0u, false, false, false, false, true}
};

static const vitte_builtin_function_t VITTE_BUILTIN_FUNCTIONS[] = {
    {"print", VITTE_BUILTIN_TYPE_VOID, VITTE_BUILTIN_TYPE_STRING, 1u, 1u, false, false, false},
    {"println", VITTE_BUILTIN_TYPE_VOID, VITTE_BUILTIN_TYPE_STRING, 1u, 1u, false, false, false},
    {"panic", VITTE_BUILTIN_TYPE_NEVER, VITTE_BUILTIN_TYPE_STRING, 1u, 1u, false, false, true},
    {"assert", VITTE_BUILTIN_TYPE_VOID, VITTE_BUILTIN_TYPE_BOOL, 1u, 1u, false, false, false}
};

static const vitte_builtin_operator_t VITTE_BUILTIN_OPERATORS[] = {
    {"+", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_NUMERIC, VITTE_BUILTIN_TYPE_ERROR, 60u, VITTE_BUILTIN_ASSOC_LEFT},
    {"-", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_NUMERIC, VITTE_BUILTIN_TYPE_ERROR, 60u, VITTE_BUILTIN_ASSOC_LEFT},
    {"*", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_NUMERIC, VITTE_BUILTIN_TYPE_ERROR, 70u, VITTE_BUILTIN_ASSOC_LEFT},
    {"/", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_NUMERIC, VITTE_BUILTIN_TYPE_ERROR, 70u, VITTE_BUILTIN_ASSOC_LEFT},
    {"%", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_NUMERIC, VITTE_BUILTIN_TYPE_ERROR, 70u, VITTE_BUILTIN_ASSOC_LEFT},
    {"==", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_ANY, VITTE_BUILTIN_TYPE_BOOL, 40u, VITTE_BUILTIN_ASSOC_LEFT},
    {"!=", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_ANY, VITTE_BUILTIN_TYPE_BOOL, 40u, VITTE_BUILTIN_ASSOC_LEFT},
    {"<", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_NUMERIC, VITTE_BUILTIN_TYPE_BOOL, 50u, VITTE_BUILTIN_ASSOC_LEFT},
    {"<=", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_NUMERIC, VITTE_BUILTIN_TYPE_BOOL, 50u, VITTE_BUILTIN_ASSOC_LEFT},
    {">", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_NUMERIC, VITTE_BUILTIN_TYPE_BOOL, 50u, VITTE_BUILTIN_ASSOC_LEFT},
    {">=", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_NUMERIC, VITTE_BUILTIN_TYPE_BOOL, 50u, VITTE_BUILTIN_ASSOC_LEFT},
    {"&&", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_BOOLEAN, VITTE_BUILTIN_TYPE_BOOL, 30u, VITTE_BUILTIN_ASSOC_LEFT},
    {"||", VITTE_BUILTIN_OPERATOR_BINARY, VITTE_BUILTIN_TYPE_CLASS_BOOLEAN, VITTE_BUILTIN_TYPE_BOOL, 20u, VITTE_BUILTIN_ASSOC_LEFT},
    {"!", VITTE_BUILTIN_OPERATOR_UNARY, VITTE_BUILTIN_TYPE_CLASS_BOOLEAN, VITTE_BUILTIN_TYPE_BOOL, 80u, VITTE_BUILTIN_ASSOC_RIGHT}
};

static void vitte_builtin_set_error(
    vitte_builtin_registry_t *registry,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (registry != NULL) {
        vitte_error_set_details(&registry->last_error, status, code, message, details);
    }
}

static bool vitte_builtin_name_is_valid(const char *name) {
    return name != NULL && name[0] != '\0';
}

void vitte_builtin_registry_init(vitte_builtin_registry_t *registry) {
    if (registry == NULL) {
        return;
    }

    memset(registry, 0, sizeof(*registry));
    registry->initialized = true;
    registry->types = VITTE_BUILTIN_TYPES;
    registry->type_count = sizeof(VITTE_BUILTIN_TYPES) / sizeof(VITTE_BUILTIN_TYPES[0]);
    registry->functions = VITTE_BUILTIN_FUNCTIONS;
    registry->function_count = sizeof(VITTE_BUILTIN_FUNCTIONS) / sizeof(VITTE_BUILTIN_FUNCTIONS[0]);
    registry->operators = VITTE_BUILTIN_OPERATORS;
    registry->operator_count = sizeof(VITTE_BUILTIN_OPERATORS) / sizeof(VITTE_BUILTIN_OPERATORS[0]);
    vitte_error_init(&registry->last_error);
}

void vitte_builtin_registry_reset(vitte_builtin_registry_t *registry) {
    if (registry == NULL) {
        return;
    }

    vitte_builtin_registry_init(registry);
}

bool vitte_builtin_registry_is_initialized(const vitte_builtin_registry_t *registry) {
    return registry != NULL &&
        registry->initialized &&
        registry->types != NULL &&
        registry->functions != NULL &&
        registry->operators != NULL;
}

const vitte_error_t *vitte_builtin_registry_last_error(const vitte_builtin_registry_t *registry) {
    return registry != NULL ? &registry->last_error : vitte_error_last();
}

bool vitte_builtin_type_kind_is_valid(vitte_builtin_type_kind_t kind) {
    return kind >= VITTE_BUILTIN_TYPE_VOID && kind < VITTE_BUILTIN_TYPE_COUNT;
}

static bool vitte_builtin_type_class_accepts(vitte_builtin_type_class_t type_class, vitte_builtin_type_kind_t kind) {
    switch (type_class) {
        case VITTE_BUILTIN_TYPE_CLASS_ANY:
            return vitte_builtin_type_kind_is_valid(kind) && kind != VITTE_BUILTIN_TYPE_VOID && kind != VITTE_BUILTIN_TYPE_NEVER && kind != VITTE_BUILTIN_TYPE_ERROR;
        case VITTE_BUILTIN_TYPE_CLASS_NUMERIC:
            return vitte_builtin_type_is_numeric(kind);
        case VITTE_BUILTIN_TYPE_CLASS_BOOLEAN:
            return vitte_builtin_type_is_boolean(kind);
        case VITTE_BUILTIN_TYPE_CLASS_TEXTUAL:
            return kind == VITTE_BUILTIN_TYPE_STRING;
        default:
            return false;
    }
}

vitte_status_t vitte_builtin_registry_validate(vitte_builtin_registry_t *registry) {
    size_t index;
    size_t other;

    if (!vitte_builtin_registry_is_initialized(registry)) {
        vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_BUILTIN_E_REGISTRY", "builtin registry is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (registry->type_count != (size_t)VITTE_BUILTIN_TYPE_COUNT) {
        vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INTERNAL, "VITTE_BUILTIN_E_REGISTRY", "builtin type table does not match type enum count", NULL);
        return VITTE_STATUS_ERROR_INTERNAL;
    }

    for (index = 0u; index < registry->type_count; index++) {
        const vitte_builtin_type_t *type = &registry->types[index];
        if (!vitte_builtin_name_is_valid(type->name) || !vitte_builtin_type_kind_is_valid(type->kind)) {
            vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INTERNAL, "VITTE_BUILTIN_E_TYPE", "invalid builtin type entry", type->name);
            return VITTE_STATUS_ERROR_INTERNAL;
        }
        for (other = index + 1u; other < registry->type_count; other++) {
            if (type->kind == registry->types[other].kind || strcmp(type->name, registry->types[other].name) == 0) {
                vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INTERNAL, "VITTE_BUILTIN_E_DUPLICATE", "duplicate builtin type", type->name);
                return VITTE_STATUS_ERROR_INTERNAL;
            }
        }
    }

    for (index = 0u; index < registry->function_count; index++) {
        const vitte_builtin_function_t *function = &registry->functions[index];
        if (!vitte_builtin_name_is_valid(function->name) ||
            !vitte_builtin_type_kind_is_valid(function->return_type) ||
            !vitte_builtin_type_kind_is_valid(function->parameter_type) ||
            function->min_arity > function->max_arity) {
            vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INTERNAL, "VITTE_BUILTIN_E_FUNCTION", "invalid builtin function entry", function->name);
            return VITTE_STATUS_ERROR_INTERNAL;
        }
        for (other = index + 1u; other < registry->function_count; other++) {
            if (strcmp(function->name, registry->functions[other].name) == 0) {
                vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INTERNAL, "VITTE_BUILTIN_E_DUPLICATE", "duplicate builtin function", function->name);
                return VITTE_STATUS_ERROR_INTERNAL;
            }
        }
    }

    for (index = 0u; index < registry->operator_count; index++) {
        const vitte_builtin_operator_t *operator_info = &registry->operators[index];
        if (!vitte_builtin_name_is_valid(operator_info->token) ||
            !vitte_builtin_type_kind_is_valid(operator_info->return_type) ||
            (operator_info->kind != VITTE_BUILTIN_OPERATOR_UNARY && operator_info->kind != VITTE_BUILTIN_OPERATOR_BINARY)) {
            vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INTERNAL, "VITTE_BUILTIN_E_OPERATOR", "invalid builtin operator entry", operator_info->token);
            return VITTE_STATUS_ERROR_INTERNAL;
        }
    }

    vitte_error_reset(&registry->last_error);
    return VITTE_STATUS_OK;
}

size_t vitte_builtin_type_count(const vitte_builtin_registry_t *registry) {
    return vitte_builtin_registry_is_initialized(registry) ? registry->type_count : 0u;
}

size_t vitte_builtin_function_count(const vitte_builtin_registry_t *registry) {
    return vitte_builtin_registry_is_initialized(registry) ? registry->function_count : 0u;
}

size_t vitte_builtin_operator_count(const vitte_builtin_registry_t *registry) {
    return vitte_builtin_registry_is_initialized(registry) ? registry->operator_count : 0u;
}

const vitte_builtin_type_t *vitte_builtin_type_at(const vitte_builtin_registry_t *registry, size_t index) {
    return vitte_builtin_registry_is_initialized(registry) && index < registry->type_count ? &registry->types[index] : NULL;
}

const vitte_builtin_function_t *vitte_builtin_function_at(const vitte_builtin_registry_t *registry, size_t index) {
    return vitte_builtin_registry_is_initialized(registry) && index < registry->function_count ? &registry->functions[index] : NULL;
}

const vitte_builtin_operator_t *vitte_builtin_operator_at(const vitte_builtin_registry_t *registry, size_t index) {
    return vitte_builtin_registry_is_initialized(registry) && index < registry->operator_count ? &registry->operators[index] : NULL;
}

const vitte_builtin_type_t *vitte_builtin_lookup_type(vitte_builtin_registry_t *registry, const char *name) {
    size_t index;

    if (!vitte_builtin_registry_is_initialized(registry) || !vitte_builtin_name_is_valid(name)) {
        vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_BUILTIN_E_ARGUMENT", "invalid builtin type lookup", name);
        return NULL;
    }

    for (index = 0u; index < registry->type_count; index++) {
        if (strcmp(registry->types[index].name, name) == 0) {
            vitte_error_reset(&registry->last_error);
            return &registry->types[index];
        }
    }

    vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_BUILTIN_E_NOT_FOUND", "builtin type not found", name);
    return NULL;
}

const vitte_builtin_function_t *vitte_builtin_lookup_function(vitte_builtin_registry_t *registry, const char *name) {
    size_t index;

    if (!vitte_builtin_registry_is_initialized(registry) || !vitte_builtin_name_is_valid(name)) {
        vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_BUILTIN_E_ARGUMENT", "invalid builtin function lookup", name);
        return NULL;
    }

    for (index = 0u; index < registry->function_count; index++) {
        if (strcmp(registry->functions[index].name, name) == 0) {
            vitte_error_reset(&registry->last_error);
            return &registry->functions[index];
        }
    }

    vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_BUILTIN_E_NOT_FOUND", "builtin function not found", name);
    return NULL;
}

const vitte_builtin_operator_t *vitte_builtin_lookup_operator(
    vitte_builtin_registry_t *registry,
    const char *token,
    vitte_builtin_operator_kind_t kind
) {
    size_t index;

    if (!vitte_builtin_registry_is_initialized(registry) || !vitte_builtin_name_is_valid(token)) {
        vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_BUILTIN_E_ARGUMENT", "invalid builtin operator lookup", token);
        return NULL;
    }

    for (index = 0u; index < registry->operator_count; index++) {
        if (registry->operators[index].kind == kind && strcmp(registry->operators[index].token, token) == 0) {
            vitte_error_reset(&registry->last_error);
            return &registry->operators[index];
        }
    }

    vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_BUILTIN_E_NOT_FOUND", "builtin operator not found", token);
    return NULL;
}

vitte_builtin_lookup_result_t vitte_builtin_lookup(vitte_builtin_registry_t *registry, const char *name) {
    vitte_builtin_lookup_result_t result;
    const vitte_builtin_type_t *type;
    const vitte_builtin_function_t *function;

    memset(&result, 0, sizeof(result));
    result.status = VITTE_STATUS_ERROR_INVALID_ARGUMENT;

    type = vitte_builtin_lookup_type(registry, name);
    if (type != NULL) {
        result.status = VITTE_STATUS_OK;
        result.symbol.kind = VITTE_BUILTIN_KIND_TYPE;
        result.symbol.name = type->name;
        result.symbol.as.type = type;
        return result;
    }

    function = vitte_builtin_lookup_function(registry, name);
    if (function != NULL) {
        result.status = VITTE_STATUS_OK;
        result.symbol.kind = VITTE_BUILTIN_KIND_FUNCTION;
        result.symbol.name = function->name;
        result.symbol.as.function = function;
        return result;
    }

    vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_BUILTIN_E_NOT_FOUND", "builtin symbol not found", name);
    return result;
}

const char *vitte_builtin_kind_name(vitte_builtin_kind_t kind) {
    switch (kind) {
        case VITTE_BUILTIN_KIND_TYPE:
            return "type";
        case VITTE_BUILTIN_KIND_FUNCTION:
            return "function";
        case VITTE_BUILTIN_KIND_OPERATOR:
            return "operator";
        default:
            return "unknown";
    }
}

const char *vitte_builtin_type_kind_name(vitte_builtin_type_kind_t kind) {
    switch (kind) {
        case VITTE_BUILTIN_TYPE_VOID:
            return "void";
        case VITTE_BUILTIN_TYPE_BOOL:
            return "bool";
        case VITTE_BUILTIN_TYPE_INT:
            return "int";
        case VITTE_BUILTIN_TYPE_I64:
            return "i64";
        case VITTE_BUILTIN_TYPE_STRING:
            return "string";
        case VITTE_BUILTIN_TYPE_NEVER:
            return "never";
        case VITTE_BUILTIN_TYPE_ERROR:
            return "error";
        case VITTE_BUILTIN_TYPE_COUNT:
        default:
            return "unknown";
    }
}

const vitte_builtin_type_t *vitte_builtin_type_by_kind(vitte_builtin_registry_t *registry, vitte_builtin_type_kind_t kind) {
    size_t index;

    if (!vitte_builtin_registry_is_initialized(registry) || !vitte_builtin_type_kind_is_valid(kind)) {
        vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_BUILTIN_E_TYPE", "invalid builtin type kind", NULL);
        return NULL;
    }

    for (index = 0u; index < registry->type_count; index++) {
        if (registry->types[index].kind == kind) {
            vitte_error_reset(&registry->last_error);
            return &registry->types[index];
        }
    }

    vitte_builtin_set_error(registry, VITTE_STATUS_ERROR_INTERNAL, "VITTE_BUILTIN_E_TYPE", "builtin type kind is missing from registry", vitte_builtin_type_kind_name(kind));
    return NULL;
}

bool vitte_builtin_type_is_numeric(vitte_builtin_type_kind_t kind) {
    return kind == VITTE_BUILTIN_TYPE_INT || kind == VITTE_BUILTIN_TYPE_I64;
}

bool vitte_builtin_type_is_boolean(vitte_builtin_type_kind_t kind) {
    return kind == VITTE_BUILTIN_TYPE_BOOL;
}

bool vitte_builtin_function_accepts_arity(const vitte_builtin_function_t *function, size_t arity) {
    if (function == NULL) {
        return false;
    }
    if (arity < function->min_arity) {
        return false;
    }
    return function->variadic || arity <= function->max_arity;
}

bool vitte_builtin_operator_accepts(
    const vitte_builtin_operator_t *operator_info,
    vitte_builtin_type_kind_t left,
    vitte_builtin_type_kind_t right
) {
    if (operator_info == NULL || !vitte_builtin_type_kind_is_valid(left)) {
        return false;
    }
    if (operator_info->kind == VITTE_BUILTIN_OPERATOR_UNARY) {
        return vitte_builtin_type_class_accepts(operator_info->operand_class, left);
    }
    if (!vitte_builtin_type_kind_is_valid(right)) {
        return false;
    }
    if ((operator_info->return_type == VITTE_BUILTIN_TYPE_BOOL && operator_info->operand_class == VITTE_BUILTIN_TYPE_CLASS_ANY) &&
        left != right) {
        return false;
    }
    return vitte_builtin_type_class_accepts(operator_info->operand_class, left) &&
        vitte_builtin_type_class_accepts(operator_info->operand_class, right);
}
