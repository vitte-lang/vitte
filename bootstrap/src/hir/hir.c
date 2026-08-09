#include "hir.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static bool vitte_hir_depth_ok(vitte_hir_lowering_t *lowering, size_t depth);
static vitte_hir_type_t *vitte_hir_lower_type(vitte_hir_lowering_t *lowering, const vitte_ast_node_t *node, size_t depth);

static void vitte_hir_set_error(
    vitte_hir_t *hir,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (hir != NULL) {
        vitte_error_set_details(&hir->last_error, status, code, message, details);
    }
}

static void vitte_hir_lowering_set_error(
    vitte_hir_lowering_t *lowering,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (lowering != NULL) {
        vitte_error_set_details(&lowering->last_error, status, code, message, details);
        vitte_hir_set_error(lowering->hir, status, code, message, details);
    }
}

void vitte_hir_list_init(vitte_hir_list_t *list) {
    if (list == NULL) {
        return;
    }
    list->first = NULL;
    list->last = NULL;
    list->count = 0u;
}

bool vitte_hir_list_append(vitte_hir_list_t *list, vitte_hir_node_t *node) {
    if (list == NULL || node == NULL) {
        return false;
    }
    node->next = NULL;
    if (list->last != NULL) {
        list->last->next = node;
    } else {
        list->first = node;
    }
    list->last = node;
    list->count++;
    return true;
}

vitte_status_t vitte_hir_init(vitte_hir_t *hir, vitte_arena_t *arena) {
    if (hir == NULL || !vitte_arena_is_initialized(arena)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    memset(hir, 0, sizeof(*hir));
    hir->initialized = true;
    hir->owns_arena = false;
    hir->arena = arena;
    hir->next_id = 1u;
    vitte_error_init(&hir->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_hir_init_owned(vitte_hir_t *hir, const vitte_arena_config_t *config) {
    vitte_status_t status;

    if (hir == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    memset(hir, 0, sizeof(*hir));
    status = vitte_arena_init(&hir->owned_arena, config);
    if (status != VITTE_STATUS_OK) {
        vitte_error_init(&hir->last_error);
        vitte_hir_set_error(hir, status, "VITTE_HIR_E_ARENA", "failed to initialize HIR arena", NULL);
        return status;
    }
    hir->initialized = true;
    hir->owns_arena = true;
    hir->arena = &hir->owned_arena;
    hir->next_id = 1u;
    vitte_error_init(&hir->last_error);
    return VITTE_STATUS_OK;
}

void vitte_hir_destroy(vitte_hir_t *hir) {
    if (hir == NULL) {
        return;
    }
    if (hir->owns_arena) {
        vitte_arena_destroy(&hir->owned_arena);
    }
    memset(hir, 0, sizeof(*hir));
}

bool vitte_hir_is_initialized(const vitte_hir_t *hir) {
    return hir != NULL && hir->initialized && vitte_arena_is_initialized(hir->arena);
}

const vitte_error_t *vitte_hir_last_error(const vitte_hir_t *hir) {
    return hir != NULL ? &hir->last_error : vitte_error_last();
}

void vitte_hir_clear_error(vitte_hir_t *hir) {
    if (hir != NULL) {
        vitte_error_reset(&hir->last_error);
    }
}

bool vitte_hir_kind_is_valid(vitte_hir_kind_t kind) {
    return kind >= VITTE_HIR_ERROR && kind < VITTE_HIR_KIND_COUNT;
}

vitte_hir_node_t *vitte_hir_alloc_node(
    vitte_hir_t *hir,
    vitte_hir_kind_t kind,
    const vitte_ast_node_t *source
) {
    vitte_hir_node_t *node;

    if (!vitte_hir_is_initialized(hir)) {
        vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_STATE", "HIR is not initialized", NULL);
        return NULL;
    }
    if (!vitte_hir_kind_is_valid(kind)) {
        vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_HIR_E_KIND", "invalid HIR node kind", NULL);
        return NULL;
    }
    if (hir->next_id == 0u) {
        vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INTERNAL, "VITTE_HIR_E_ID", "HIR node id overflow", NULL);
        return NULL;
    }

    node = (vitte_hir_node_t *)vitte_arena_alloc_zeroed(hir->arena, sizeof(*node), _Alignof(vitte_hir_node_t));
    if (node == NULL) {
        vitte_error_copy(&hir->last_error, vitte_arena_last_error(hir->arena));
        return NULL;
    }
    node->id = hir->next_id;
    hir->next_id++;
    node->kind = kind;
    node->source = source;
    hir->node_count++;
    return node;
}

const char *vitte_hir_kind_name(vitte_hir_kind_t kind) {
    switch (kind) {
        case VITTE_HIR_ERROR:
            return "error";
        case VITTE_HIR_MODULE:
            return "module";
        case VITTE_HIR_FUNCTION:
            return "function";
        case VITTE_HIR_CONST_DECL:
            return "const";
        case VITTE_HIR_PICK_DECL:
            return "pick";
        case VITTE_HIR_PICK_VARIANT:
            return "pick_variant";
        case VITTE_HIR_FORM_DECL:
            return "form";
        case VITTE_HIR_FORM_FIELD:
            return "form_field";
        case VITTE_HIR_LIST_EXPR:
            return "list";
        case VITTE_HIR_RECORD_EXPR:
            return "record";
        case VITTE_HIR_RECORD_FIELD:
            return "record_field";
        case VITTE_HIR_BLOCK:
            return "block";
        case VITTE_HIR_RETURN_STMT:
            return "return";
        case VITTE_HIR_LET_STMT:
            return "let";
        case VITTE_HIR_ASSIGN_STMT:
            return "assign";
        case VITTE_HIR_EXPR_STMT:
            return "expr_stmt";
        case VITTE_HIR_IF_STMT:
            return "if";
        case VITTE_HIR_WHILE_STMT:
            return "while";
        case VITTE_HIR_INTEGER_LITERAL:
            return "integer";
        case VITTE_HIR_STRING_LITERAL:
            return "string";
        case VITTE_HIR_VARIABLE:
            return "variable";
        case VITTE_HIR_BINARY_EXPR:
            return "binary";
        case VITTE_HIR_CALL_EXPR:
            return "call";
        case VITTE_HIR_IF_EXPR:
            return "if_expr";
        case VITTE_HIR_TYPE_NAME:
            return "type";
        case VITTE_HIR_KIND_COUNT:
        default:
            return "unknown";
    }
}

const char *vitte_hir_node_label(const vitte_hir_node_t *node) {
    if (node == NULL) {
        return "<null>";
    }
    switch (node->kind) {
        case VITTE_HIR_MODULE:
            return node->as.module.name != NULL ? node->as.module.name : "<module>";
        case VITTE_HIR_FUNCTION:
            return node->as.function.name != NULL ? node->as.function.name : "<function>";
        case VITTE_HIR_CONST_DECL:
            return node->as.const_decl.name != NULL ? node->as.const_decl.name : "<const>";
        case VITTE_HIR_PICK_DECL:
            return node->as.pick_decl.name != NULL ? node->as.pick_decl.name : "<pick>";
        case VITTE_HIR_PICK_VARIANT:
            return node->as.pick_variant.name != NULL ? node->as.pick_variant.name : "<variant>";
        case VITTE_HIR_FORM_DECL:
            return node->as.form_decl.name != NULL ? node->as.form_decl.name : "<form>";
        case VITTE_HIR_FORM_FIELD:
            return node->as.form_field.name != NULL ? node->as.form_field.name : "<field>";
        case VITTE_HIR_RECORD_EXPR:
            return node->as.record_expr.type_name != NULL ? node->as.record_expr.type_name : "<record>";
        case VITTE_HIR_RECORD_FIELD:
            return node->as.record_field.name != NULL ? node->as.record_field.name : "<field>";
        case VITTE_HIR_LET_STMT:
            return node->as.let_stmt.name != NULL ? node->as.let_stmt.name : "<let>";
        case VITTE_HIR_ASSIGN_STMT:
            return node->as.assign_stmt.name != NULL ? node->as.assign_stmt.name : "<assign>";
        case VITTE_HIR_STRING_LITERAL:
            return node->as.string_literal.value != NULL ? node->as.string_literal.value : "";
        case VITTE_HIR_VARIABLE:
            return node->as.variable.name != NULL ? node->as.variable.name : "<var>";
        case VITTE_HIR_BINARY_EXPR:
            return node->as.binary_expr.operator_text != NULL ? node->as.binary_expr.operator_text : "<op>";
        case VITTE_HIR_TYPE_NAME:
            return node->as.type_name.name != NULL ? node->as.type_name.name : "<type>";
        case VITTE_HIR_ERROR:
            return node->as.error_node.message != NULL ? node->as.error_node.message : "<error>";
        default:
            return vitte_hir_kind_name(node->kind);
    }
}

static bool vitte_hir_text_equal(const char *left, const char *right) {
    if (left == right) {
        return true;
    }
    if (left == NULL || right == NULL) {
        return false;
    }
    return strcmp(left, right) == 0;
}

static const char *vitte_hir_source_decl_name(const vitte_ast_node_t *source) {
    if (source == NULL) {
        return NULL;
    }
    if (source->kind == VITTE_AST_NODE_PROC_DECL) {
        return source->as.proc_decl.name;
    }
    if (source->kind == VITTE_AST_NODE_CONST_DECL) {
        return source->as.const_decl.name;
    }
    return NULL;
}

static const char *vitte_hir_source_decl_lowered_name(const vitte_ast_node_t *source) {
    if (source == NULL) {
        return NULL;
    }
    if (source->kind == VITTE_AST_NODE_PROC_DECL) {
        return source->as.proc_decl.lowered_name;
    }
    if (source->kind == VITTE_AST_NODE_CONST_DECL) {
        return source->as.const_decl.lowered_name;
    }
    return NULL;
}

static const char *vitte_hir_source_variable_name(const vitte_ast_node_t *source) {
    if (source == NULL) {
        return NULL;
    }
    if (source->kind == VITTE_AST_NODE_IDENTIFIER) {
        return source->as.identifier.name;
    }
    if (source->kind == VITTE_AST_NODE_PARAM_DECL) {
        return source->as.param_decl.name;
    }
    return NULL;
}

static const char *vitte_hir_source_variable_lowered_name(const vitte_ast_node_t *source) {
    if (source != NULL && source->kind == VITTE_AST_NODE_IDENTIFIER) {
        return source->as.identifier.lowered_name;
    }
    return NULL;
}

size_t vitte_hir_node_count(const vitte_hir_t *hir) {
    return hir != NULL ? hir->node_count : 0u;
}

size_t vitte_hir_function_count(const vitte_hir_t *hir) {
    const vitte_hir_node_t *decl;
    size_t count = 0u;

    if (hir == NULL || hir->root == NULL) {
        return 0u;
    }
    for (decl = hir->root->as.module.declarations.first; decl != NULL; decl = decl->next) {
        if (decl->kind == VITTE_HIR_FUNCTION) {
            count++;
        }
    }
    return count;
}

void vitte_hir_builder_init(vitte_hir_builder_t *builder, vitte_hir_t *hir) {
    if (builder == NULL) {
        return;
    }
    builder->hir = hir;
}

vitte_hir_module_t *vitte_hir_make_module(vitte_hir_builder_t *builder, const char *name, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node;

    if (builder == NULL || builder->hir == NULL) {
        return NULL;
    }
    node = vitte_hir_alloc_node(builder->hir, VITTE_HIR_MODULE, source);
    if (node == NULL) {
        return NULL;
    }
    node->as.module.name = name;
    vitte_hir_list_init(&node->as.module.declarations);
    return node;
}

vitte_hir_function_t *vitte_hir_make_function(vitte_hir_builder_t *builder, const char *name, vitte_hir_type_t *return_type, vitte_hir_block_t *body, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node;

    if (builder == NULL || builder->hir == NULL) {
        return NULL;
    }
    node = vitte_hir_alloc_node(builder->hir, VITTE_HIR_FUNCTION, source);
    if (node == NULL) {
        return NULL;
    }
    node->as.function.name = name;
    node->as.function.source_name = vitte_hir_source_decl_name(source);
    node->as.function.lowered_name = vitte_hir_source_decl_lowered_name(source);
    vitte_hir_list_init(&node->as.function.parameters);
    node->as.function.return_type = return_type;
    node->as.function.body = body;
    return node;
}

vitte_hir_decl_t *vitte_hir_make_const(vitte_hir_builder_t *builder, const char *name, vitte_hir_type_t *type, vitte_hir_expr_t *value, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node;

    if (builder == NULL || builder->hir == NULL || name == NULL || value == NULL) {
        return NULL;
    }
    node = vitte_hir_alloc_node(builder->hir, VITTE_HIR_CONST_DECL, source);
    if (node == NULL) {
        return NULL;
    }
    node->as.const_decl.name = name;
    node->as.const_decl.source_name = vitte_hir_source_decl_name(source);
    node->as.const_decl.lowered_name = vitte_hir_source_decl_lowered_name(source);
    node->as.const_decl.declared_type = type;
    node->as.const_decl.value = value;
    return node;
}

vitte_hir_decl_t *vitte_hir_make_pick(vitte_hir_builder_t *builder, const char *name, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node;

    if (builder == NULL || builder->hir == NULL || name == NULL) {
        return NULL;
    }
    node = vitte_hir_alloc_node(builder->hir, VITTE_HIR_PICK_DECL, source);
    if (node == NULL) {
        return NULL;
    }
    node->as.pick_decl.name = name;
    vitte_hir_list_init(&node->as.pick_decl.variants);
    return node;
}

vitte_hir_node_t *vitte_hir_make_pick_variant(vitte_hir_builder_t *builder, const char *name, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_PICK_VARIANT, source) : NULL;
    if (node != NULL) {
        node->as.pick_variant.name = name;
    }
    return node;
}

vitte_hir_decl_t *vitte_hir_make_form(vitte_hir_builder_t *builder, const char *name, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL && name != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_FORM_DECL, source) : NULL;
    if (node != NULL) {
        node->as.form_decl.name = name;
        vitte_hir_list_init(&node->as.form_decl.fields);
    }
    return node;
}

vitte_hir_node_t *vitte_hir_make_form_field(vitte_hir_builder_t *builder, const char *name, vitte_hir_type_t *type, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL && name != NULL && type != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_FORM_FIELD, source) : NULL;
    if (node != NULL) {
        node->as.form_field.name = name;
        node->as.form_field.type = type;
    }
    return node;
}

vitte_hir_expr_t *vitte_hir_make_list(vitte_hir_builder_t *builder, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_LIST_EXPR, source) : NULL;
    if (node != NULL) vitte_hir_list_init(&node->as.list_expr.elements);
    return node;
}

vitte_hir_expr_t *vitte_hir_make_record(vitte_hir_builder_t *builder, const char *type_name, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_RECORD_EXPR, source) : NULL;
    if (node != NULL) { node->as.record_expr.type_name = type_name; vitte_hir_list_init(&node->as.record_expr.fields); }
    return node;
}

vitte_hir_node_t *vitte_hir_make_record_field(vitte_hir_builder_t *builder, const char *name, vitte_hir_expr_t *value, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_RECORD_FIELD, source) : NULL;
    if (node != NULL) { node->as.record_field.name = name; node->as.record_field.value = value; }
    return node;
}

vitte_hir_block_t *vitte_hir_make_block(vitte_hir_builder_t *builder, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node;

    if (builder == NULL || builder->hir == NULL) {
        return NULL;
    }
    node = vitte_hir_alloc_node(builder->hir, VITTE_HIR_BLOCK, source);
    if (node == NULL) {
        return NULL;
    }
    vitte_hir_list_init(&node->as.block.statements);
    return node;
}

vitte_hir_stmt_t *vitte_hir_make_return(vitte_hir_builder_t *builder, vitte_hir_expr_t *value, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_RETURN_STMT, source) : NULL;
    if (node != NULL) {
        node->as.return_stmt.value = value;
    }
    return node;
}

vitte_hir_stmt_t *vitte_hir_make_let(vitte_hir_builder_t *builder, const char *name, vitte_hir_type_t *type, vitte_hir_expr_t *value, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_LET_STMT, source) : NULL;
    if (node != NULL) {
        node->as.let_stmt.name = name;
        node->as.let_stmt.declared_type = type;
        node->as.let_stmt.value = value;
    }
    return node;
}

vitte_hir_stmt_t *vitte_hir_make_assign(vitte_hir_builder_t *builder, const char *name, vitte_hir_expr_t *value, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_ASSIGN_STMT, source) : NULL;
    if (node != NULL) {
        node->as.assign_stmt.name = name;
        node->as.assign_stmt.value = value;
    }
    return node;
}

vitte_hir_stmt_t *vitte_hir_make_expr_stmt(vitte_hir_builder_t *builder, vitte_hir_expr_t *value, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_EXPR_STMT, source) : NULL;
    if (node != NULL) {
        node->as.expr_stmt.value = value;
    }
    return node;
}

vitte_hir_stmt_t *vitte_hir_make_if(vitte_hir_builder_t *builder, vitte_hir_expr_t *condition, vitte_hir_stmt_t *then_branch, vitte_hir_stmt_t *else_branch, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_IF_STMT, source) : NULL;
    if (node != NULL) {
        node->as.if_stmt.condition = condition;
        node->as.if_stmt.then_branch = then_branch;
        node->as.if_stmt.else_branch = else_branch;
    }
    return node;
}

vitte_hir_expr_t *vitte_hir_make_integer_literal(vitte_hir_builder_t *builder, int64_t value, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_INTEGER_LITERAL, source) : NULL;
    if (node != NULL) {
        node->as.integer_literal.value = value;
    }
    return node;
}

vitte_hir_expr_t *vitte_hir_make_string_literal(vitte_hir_builder_t *builder, const char *value, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_STRING_LITERAL, source) : NULL;
    if (node != NULL) {
        node->as.string_literal.value = value;
    }
    return node;
}

vitte_hir_expr_t *vitte_hir_make_variable(vitte_hir_builder_t *builder, const char *name, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_VARIABLE, source) : NULL;
    if (node != NULL) {
        node->as.variable.name = name;
        node->as.variable.source_name = vitte_hir_source_variable_name(source);
        node->as.variable.lowered_name = vitte_hir_source_variable_lowered_name(source);
    }
    return node;
}

vitte_hir_expr_t *vitte_hir_make_binary(vitte_hir_builder_t *builder, const char *operator_text, vitte_hir_expr_t *left, vitte_hir_expr_t *right, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_BINARY_EXPR, source) : NULL;
    if (node != NULL) {
        node->as.binary_expr.operator_text = operator_text;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
    }
    return node;
}

vitte_hir_expr_t *vitte_hir_make_call(vitte_hir_builder_t *builder, vitte_hir_expr_t *callee, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_CALL_EXPR, source) : NULL;
    if (node != NULL) {
        node->as.call_expr.callee = callee;
        vitte_hir_list_init(&node->as.call_expr.arguments);
    }
    return node;
}

vitte_hir_expr_t *vitte_hir_make_if_expr(vitte_hir_builder_t *builder, vitte_hir_expr_t *condition, vitte_hir_expr_t *then_value, vitte_hir_expr_t *else_value, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_IF_EXPR, source) : NULL;
    if (node != NULL) {
        node->as.if_expr.condition = condition;
        node->as.if_expr.then_value = then_value;
        node->as.if_expr.else_value = else_value;
    }
    return node;
}

vitte_hir_stmt_t *vitte_hir_make_while(vitte_hir_builder_t *builder, vitte_hir_expr_t *condition, vitte_hir_stmt_t *body, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_WHILE_STMT, source) : NULL;
    if (node != NULL) { node->as.while_stmt.condition = condition; node->as.while_stmt.body = body; }
    return node;
}

vitte_hir_type_t *vitte_hir_make_type_name(vitte_hir_builder_t *builder, const char *name, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_TYPE_NAME, source) : NULL;
    if (node != NULL) {
        node->as.type_name.name = name;
    }
    return node;
}

vitte_hir_node_t *vitte_hir_make_error(vitte_hir_builder_t *builder, const char *message, const vitte_ast_node_t *source) {
    vitte_hir_node_t *node = builder != NULL ? vitte_hir_alloc_node(builder->hir, VITTE_HIR_ERROR, source) : NULL;
    if (node != NULL) {
        node->as.error_node.message = message;
    }
    return node;
}

bool vitte_hir_module_add_decl(vitte_hir_module_t *module, vitte_hir_decl_t *decl) {
    if (module == NULL || module->kind != VITTE_HIR_MODULE || decl == NULL) {
        return false;
    }
    if (decl->kind != VITTE_HIR_FUNCTION && decl->kind != VITTE_HIR_CONST_DECL && decl->kind != VITTE_HIR_PICK_DECL && decl->kind != VITTE_HIR_FORM_DECL) {
        return false;
    }
    return vitte_hir_list_append(&module->as.module.declarations, decl);
}

static vitte_hir_decl_t *vitte_hir_lower_pick(vitte_hir_lowering_t *lowering, const vitte_ast_node_t *decl, size_t depth) {
    vitte_hir_builder_t builder;
    vitte_hir_decl_t *pick;
    const vitte_ast_node_t *variant;

    if (!vitte_hir_depth_ok(lowering, depth) || decl == NULL || decl->kind != VITTE_AST_NODE_PICK_DECL) {
        vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_HIR_E_DECL", "HIR lowering expects pick declarations", decl != NULL ? vitte_ast_node_kind_name(decl->kind) : NULL);
        return NULL;
    }
    vitte_hir_builder_init(&builder, lowering->hir);
    pick = vitte_hir_make_pick(&builder, decl->as.pick_decl.name, decl);
    if (pick == NULL) {
        return NULL;
    }
    for (variant = decl->as.pick_decl.variants.first; variant != NULL; variant = variant->next) {
        vitte_hir_node_t *hir_variant;
        if (variant->kind != VITTE_AST_NODE_PICK_VARIANT) {
            vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_HIR_E_PICK", "unsupported AST pick variant", vitte_ast_node_kind_name(variant->kind));
            return NULL;
        }
        hir_variant = vitte_hir_make_pick_variant(&builder, variant->as.pick_variant.name, variant);
        if (hir_variant == NULL || !vitte_hir_list_append(&pick->as.pick_decl.variants, hir_variant)) {
            vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INTERNAL, "VITTE_HIR_E_PICK", "failed to append lowered pick variant", variant->as.pick_variant.name);
            return NULL;
        }
    }
    return pick;
}

static vitte_hir_decl_t *vitte_hir_lower_form(vitte_hir_lowering_t *lowering, const vitte_ast_node_t *decl, size_t depth) {
    vitte_hir_builder_t builder;
    vitte_hir_decl_t *form;
    const vitte_ast_node_t *field;
    if (!vitte_hir_depth_ok(lowering, depth) || decl == NULL || decl->kind != VITTE_AST_NODE_FORM_DECL) {
        vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_HIR_E_DECL", "HIR lowering expects form declarations", NULL);
        return NULL;
    }
    vitte_hir_builder_init(&builder, lowering->hir);
    form = vitte_hir_make_form(&builder, decl->as.form_decl.name, decl);
    if (form == NULL) return NULL;
    for (field = decl->as.form_decl.fields.first; field != NULL; field = field->next) {
        vitte_hir_type_t *type;
        vitte_hir_node_t *hir_field;
        if (field->kind != VITTE_AST_NODE_FORM_FIELD) return NULL;
        type = vitte_hir_lower_type(lowering, field->as.form_field.type, depth + 1u);
        hir_field = type != NULL ? vitte_hir_make_form_field(&builder, field->as.form_field.name, type, field) : NULL;
        if (hir_field == NULL || !vitte_hir_list_append(&form->as.form_decl.fields, hir_field)) return NULL;
    }
    return form;
}

bool vitte_hir_module_add_function(vitte_hir_module_t *module, vitte_hir_function_t *function) {
    if (function == NULL || function->kind != VITTE_HIR_FUNCTION) {
        return false;
    }
    return vitte_hir_module_add_decl(module, function);
}

bool vitte_hir_module_add_const(vitte_hir_module_t *module, vitte_hir_decl_t *decl) {
    if (decl == NULL || decl->kind != VITTE_HIR_CONST_DECL) {
        return false;
    }
    return vitte_hir_module_add_decl(module, decl);
}

bool vitte_hir_function_add_param(vitte_hir_function_t *function, vitte_hir_node_t *param) {
    if (function == NULL || function->kind != VITTE_HIR_FUNCTION || param == NULL ||
        param->kind != VITTE_HIR_VARIABLE || param->type == NULL) {
        return false;
    }
    return vitte_hir_list_append(&function->as.function.parameters, param);
}

bool vitte_hir_block_add_stmt(vitte_hir_block_t *block, vitte_hir_stmt_t *stmt) {
    if (block == NULL || block->kind != VITTE_HIR_BLOCK || stmt == NULL) {
        return false;
    }
    return vitte_hir_list_append(&block->as.block.statements, stmt);
}

bool vitte_hir_call_add_arg(vitte_hir_expr_t *call, vitte_hir_expr_t *argument) {
    if (call == NULL || call->kind != VITTE_HIR_CALL_EXPR || argument == NULL) {
        return false;
    }
    return vitte_hir_list_append(&call->as.call_expr.arguments, argument);
}

void vitte_hir_lowering_init(vitte_hir_lowering_t *lowering, vitte_hir_t *hir) {
    if (lowering == NULL) {
        return;
    }
    memset(lowering, 0, sizeof(*lowering));
    lowering->hir = hir;
    lowering->max_depth = VITTE_HIR_DEFAULT_MAX_DEPTH;
    vitte_error_init(&lowering->last_error);
}

const vitte_error_t *vitte_hir_lowering_last_error(const vitte_hir_lowering_t *lowering) {
    return lowering != NULL ? &lowering->last_error : vitte_error_last();
}

static vitte_hir_type_t *vitte_hir_lower_type(vitte_hir_lowering_t *lowering, const vitte_ast_node_t *node, size_t depth);
static vitte_hir_expr_t *vitte_hir_lower_expr(vitte_hir_lowering_t *lowering, const vitte_ast_node_t *node, size_t depth);
static vitte_hir_stmt_t *vitte_hir_lower_stmt(vitte_hir_lowering_t *lowering, const vitte_ast_node_t *node, size_t depth);

static const char *vitte_hir_ast_decl_lowered_name(const vitte_ast_node_t *decl) {
    if (decl == NULL) {
        return NULL;
    }
    if (decl->kind == VITTE_AST_NODE_PROC_DECL) {
        return decl->as.proc_decl.lowered_name != NULL ? decl->as.proc_decl.lowered_name : decl->as.proc_decl.name;
    }
    if (decl->kind == VITTE_AST_NODE_CONST_DECL) {
        return decl->as.const_decl.lowered_name != NULL ? decl->as.const_decl.lowered_name : decl->as.const_decl.name;
    }
    return NULL;
}

static const char *vitte_hir_ast_identifier_lowered_name(const vitte_ast_node_t *node) {
    if (node == NULL || node->kind != VITTE_AST_NODE_IDENTIFIER) {
        return NULL;
    }
    return node->as.identifier.lowered_name != NULL ? node->as.identifier.lowered_name : node->as.identifier.name;
}

static bool vitte_hir_depth_ok(vitte_hir_lowering_t *lowering, size_t depth) {
    if (lowering == NULL || depth > lowering->max_depth) {
        vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_DEPTH", "HIR lowering exceeded maximum depth", NULL);
        return false;
    }
    return true;
}

static vitte_hir_type_t *vitte_hir_lower_type(vitte_hir_lowering_t *lowering, const vitte_ast_node_t *node, size_t depth) {
    vitte_hir_builder_t builder;

    if (!vitte_hir_depth_ok(lowering, depth)) {
        return NULL;
    }
    vitte_hir_builder_init(&builder, lowering->hir);
    if (node == NULL) {
        return vitte_hir_make_type_name(&builder, "int", NULL);
    }
    if (node->kind == VITTE_AST_NODE_TYPE_NAME) {
        return vitte_hir_make_type_name(&builder, node->as.type_name.name, node);
    }
    vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_HIR_E_TYPE", "unsupported AST type for HIR lowering", vitte_ast_node_kind_name(node->kind));
    return NULL;
}

static vitte_hir_expr_t *vitte_hir_lower_expr(vitte_hir_lowering_t *lowering, const vitte_ast_node_t *node, size_t depth) {
    vitte_hir_builder_t builder;

    if (!vitte_hir_depth_ok(lowering, depth)) {
        return NULL;
    }
    if (node == NULL) {
        vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_HIR_E_EXPR", "missing AST expression for HIR lowering", NULL);
        return NULL;
    }
    vitte_hir_builder_init(&builder, lowering->hir);
    switch (node->kind) {
        case VITTE_AST_NODE_INTEGER_LITERAL:
            return vitte_hir_make_integer_literal(&builder, node->as.integer_literal.value, node);
        case VITTE_AST_NODE_STRING_LITERAL:
            return vitte_hir_make_string_literal(&builder, node->as.string_literal.value, node);
        case VITTE_AST_NODE_LIST_EXPR: {
            const vitte_ast_node_t *element;
            vitte_hir_expr_t *list = vitte_hir_make_list(&builder, node);
            if (list == NULL) return NULL;
            for (element = node->as.list_expr.elements.first; element != NULL; element = element->next) {
                vitte_hir_expr_t *item = vitte_hir_lower_expr(lowering, element, depth + 1u);
                if (item == NULL || !vitte_hir_list_append(&list->as.list_expr.elements, item)) return NULL;
            }
            return list;
        }
        case VITTE_AST_NODE_RECORD_EXPR: {
            const vitte_ast_node_t *field;
            vitte_hir_expr_t *record = vitte_hir_make_record(&builder, node->as.record_expr.type_name, node);
            if (record == NULL) return NULL;
            for (field = node->as.record_expr.fields.first; field != NULL; field = field->next) {
                vitte_hir_expr_t *value = vitte_hir_lower_expr(lowering, field->as.record_field.value, depth + 1u);
                vitte_hir_node_t *hir_field = value != NULL ? vitte_hir_make_record_field(&builder, field->as.record_field.name, value, field) : NULL;
                if (hir_field == NULL || !vitte_hir_list_append(&record->as.record_expr.fields, hir_field)) return NULL;
            }
            return record;
        }
        case VITTE_AST_NODE_IDENTIFIER:
            return vitte_hir_make_variable(&builder, vitte_hir_ast_identifier_lowered_name(node), node);
        case VITTE_AST_NODE_BINARY_EXPR: {
            vitte_hir_expr_t *left = vitte_hir_lower_expr(lowering, node->as.binary_expr.left, depth + 1u);
            vitte_hir_expr_t *right = vitte_hir_lower_expr(lowering, node->as.binary_expr.right, depth + 1u);
            if (left == NULL || right == NULL) {
                return NULL;
            }
            return vitte_hir_make_binary(&builder, node->as.binary_expr.operator_text, left, right, node);
        }
        case VITTE_AST_NODE_CALL_EXPR: {
            const vitte_ast_node_t *arg;
            vitte_hir_expr_t *callee = vitte_hir_lower_expr(lowering, node->as.call_expr.callee, depth + 1u);
            vitte_hir_expr_t *call;
            if (callee == NULL) {
                return NULL;
            }
            call = vitte_hir_make_call(&builder, callee, node);
            if (call == NULL) {
                return NULL;
            }
            for (arg = node->as.call_expr.arguments.first; arg != NULL; arg = arg->next) {
                vitte_hir_expr_t *hir_arg = vitte_hir_lower_expr(lowering, arg, depth + 1u);
                if (hir_arg == NULL || !vitte_hir_call_add_arg(call, hir_arg)) {
                    vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INTERNAL, "VITTE_HIR_E_CALL", "failed to lower call argument", NULL);
                    return NULL;
                }
            }
            return call;
        }
        case VITTE_AST_NODE_IF_EXPR: {
            vitte_hir_expr_t *condition = vitte_hir_lower_expr(lowering, node->as.if_expr.condition, depth + 1u);
            vitte_hir_expr_t *then_value = vitte_hir_lower_expr(lowering, node->as.if_expr.then_value, depth + 1u);
            vitte_hir_expr_t *else_value = vitte_hir_lower_expr(lowering, node->as.if_expr.else_value, depth + 1u);
            if (condition == NULL || then_value == NULL || else_value == NULL) return NULL;
            return vitte_hir_make_if_expr(&builder, condition, then_value, else_value, node);
        }
        case VITTE_AST_NODE_ERROR:
            return vitte_hir_make_error(&builder, node->as.error_node.message, node);
        default:
            vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_HIR_E_EXPR", "unsupported AST expression for HIR lowering", vitte_ast_node_kind_name(node->kind));
            return NULL;
    }
}

static vitte_hir_stmt_t *vitte_hir_lower_stmt(vitte_hir_lowering_t *lowering, const vitte_ast_node_t *node, size_t depth) {
    vitte_hir_builder_t builder;

    if (!vitte_hir_depth_ok(lowering, depth)) {
        return NULL;
    }
    if (node == NULL) {
        vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_HIR_E_STMT", "missing AST statement for HIR lowering", NULL);
        return NULL;
    }
    vitte_hir_builder_init(&builder, lowering->hir);
    switch (node->kind) {
        case VITTE_AST_NODE_BLOCK_STMT: {
            const vitte_ast_node_t *stmt;
            vitte_hir_block_t *block = vitte_hir_make_block(&builder, node);
            if (block == NULL) {
                return NULL;
            }
            for (stmt = node->as.block_stmt.statements.first; stmt != NULL; stmt = stmt->next) {
                vitte_hir_stmt_t *hir_stmt = vitte_hir_lower_stmt(lowering, stmt, depth + 1u);
                if (hir_stmt == NULL || !vitte_hir_block_add_stmt(block, hir_stmt)) {
                    vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INTERNAL, "VITTE_HIR_E_BLOCK", "failed to lower block statement", NULL);
                    return NULL;
                }
            }
            return block;
        }
        case VITTE_AST_NODE_GIVE_STMT: {
            vitte_hir_expr_t *value = node->as.give_stmt.value != NULL ?
                vitte_hir_lower_expr(lowering, node->as.give_stmt.value, depth + 1u) :
                NULL;
            if (node->as.give_stmt.value != NULL && value == NULL) {
                return NULL;
            }
            return vitte_hir_make_return(&builder, value, node);
        }
        case VITTE_AST_NODE_LET_STMT: {
            vitte_hir_type_t *type = node->as.let_stmt.type != NULL ?
                vitte_hir_lower_type(lowering, node->as.let_stmt.type, depth + 1u) :
                NULL;
            vitte_hir_expr_t *value = node->as.let_stmt.value != NULL ?
                vitte_hir_lower_expr(lowering, node->as.let_stmt.value, depth + 1u) :
                NULL;
            if ((node->as.let_stmt.type != NULL && type == NULL) ||
                (node->as.let_stmt.value != NULL && value == NULL)) {
                return NULL;
            }
            return vitte_hir_make_let(&builder, node->as.let_stmt.name, type, value, node);
        }
        case VITTE_AST_NODE_ASSIGN_STMT: {
            vitte_hir_expr_t *value = vitte_hir_lower_expr(lowering, node->as.assign_stmt.value, depth + 1u);
            if (value == NULL || node->as.assign_stmt.target == NULL || node->as.assign_stmt.target->kind != VITTE_AST_NODE_IDENTIFIER) {
                vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_HIR_E_ASSIGN", "assignment target must be an identifier", NULL);
                return NULL;
            }
            return vitte_hir_make_assign(&builder, node->as.assign_stmt.target->as.identifier.name, value, node);
        }
        case VITTE_AST_NODE_EXPR_STMT: {
            vitte_hir_expr_t *value = vitte_hir_lower_expr(lowering, node->as.expr_stmt.value, depth + 1u);
            if (value == NULL) {
                return NULL;
            }
            return vitte_hir_make_expr_stmt(&builder, value, node);
        }
        case VITTE_AST_NODE_IF_STMT: {
            vitte_hir_expr_t *condition = vitte_hir_lower_expr(lowering, node->as.if_stmt.condition, depth + 1u);
            vitte_hir_stmt_t *then_branch = vitte_hir_lower_stmt(lowering, node->as.if_stmt.then_branch, depth + 1u);
            vitte_hir_stmt_t *else_branch = node->as.if_stmt.else_branch != NULL ?
                vitte_hir_lower_stmt(lowering, node->as.if_stmt.else_branch, depth + 1u) :
                NULL;
            if (condition == NULL || then_branch == NULL ||
                (node->as.if_stmt.else_branch != NULL && else_branch == NULL)) {
                return NULL;
            }
            return vitte_hir_make_if(&builder, condition, then_branch, else_branch, node);
        }
        case VITTE_AST_NODE_WHILE_STMT: {
            vitte_hir_expr_t *condition = vitte_hir_lower_expr(lowering, node->as.while_stmt.condition, depth + 1u);
            vitte_hir_stmt_t *body = vitte_hir_lower_stmt(lowering, node->as.while_stmt.body, depth + 1u);
            if (condition == NULL || body == NULL) return NULL;
            return vitte_hir_make_while(&builder, condition, body, node);
        }
        case VITTE_AST_NODE_ERROR:
            return vitte_hir_make_error(&builder, node->as.error_node.message, node);
        default:
            vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_HIR_E_STMT", "unsupported AST statement for HIR lowering", vitte_ast_node_kind_name(node->kind));
            return NULL;
    }
}

static vitte_hir_function_t *vitte_hir_lower_function(vitte_hir_lowering_t *lowering, const vitte_ast_node_t *decl, size_t depth) {
    vitte_hir_builder_t builder;
    const vitte_ast_node_t *param;
    vitte_hir_function_t *function;
    vitte_hir_type_t *return_type;
    vitte_hir_stmt_t *body;

    if (!vitte_hir_depth_ok(lowering, depth)) {
        return NULL;
    }
    if (decl == NULL || decl->kind != VITTE_AST_NODE_PROC_DECL) {
        vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_HIR_E_DECL", "HIR lowering expects proc declarations", decl != NULL ? vitte_ast_node_kind_name(decl->kind) : NULL);
        return NULL;
    }
    return_type = vitte_hir_lower_type(lowering, decl->as.proc_decl.return_type, depth + 1u);
    body = vitte_hir_lower_stmt(lowering, decl->as.proc_decl.body, depth + 1u);
    if (return_type == NULL || body == NULL) {
        return NULL;
    }
    vitte_hir_builder_init(&builder, lowering->hir);
    function = vitte_hir_make_function(&builder, vitte_hir_ast_decl_lowered_name(decl), return_type, body, decl);
    if (function == NULL) {
        return NULL;
    }
    for (param = decl->as.proc_decl.parameters.first; param != NULL; param = param->next) {
        vitte_hir_type_t *param_type;
        vitte_hir_node_t *hir_param;

        if (param->kind != VITTE_AST_NODE_PARAM_DECL) {
            vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_HIR_E_PARAM", "unsupported AST parameter node for HIR lowering", vitte_ast_node_kind_name(param->kind));
            return NULL;
        }
        param_type = vitte_hir_lower_type(lowering, param->as.param_decl.type, depth + 1u);
        if (param_type == NULL) {
            return NULL;
        }
        hir_param = vitte_hir_make_variable(&builder, param->as.param_decl.name, param);
        if (hir_param == NULL) {
            return NULL;
        }
        hir_param->type = param_type;
        if (!vitte_hir_function_add_param(function, hir_param)) {
            vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INTERNAL, "VITTE_HIR_E_PARAM", "failed to append lowered function parameter", param->as.param_decl.name);
            return NULL;
        }
    }
    return function;
}

static vitte_hir_decl_t *vitte_hir_lower_const(vitte_hir_lowering_t *lowering, const vitte_ast_node_t *decl, size_t depth) {
    vitte_hir_builder_t builder;
    vitte_hir_type_t *declared_type = NULL;
    vitte_hir_expr_t *value;

    if (!vitte_hir_depth_ok(lowering, depth)) {
        return NULL;
    }
    if (decl == NULL || decl->kind != VITTE_AST_NODE_CONST_DECL) {
        vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_HIR_E_DECL", "HIR lowering expects const declarations", decl != NULL ? vitte_ast_node_kind_name(decl->kind) : NULL);
        return NULL;
    }
    if (decl->as.const_decl.type != NULL) {
        declared_type = vitte_hir_lower_type(lowering, decl->as.const_decl.type, depth + 1u);
        if (declared_type == NULL) {
            return NULL;
        }
    }
    value = vitte_hir_lower_expr(lowering, decl->as.const_decl.value, depth + 1u);
    if (value == NULL) {
        return NULL;
    }
    vitte_hir_builder_init(&builder, lowering->hir);
    return vitte_hir_make_const(&builder, vitte_hir_ast_decl_lowered_name(decl), declared_type, value, decl);
}

vitte_status_t vitte_hir_lower_ast_with_options(
    vitte_hir_lowering_t *lowering,
    const vitte_ast_t *ast
) {
    const vitte_ast_node_t *decl;
    vitte_hir_builder_t builder;
    vitte_hir_module_t *module;

    if (lowering == NULL || !vitte_hir_is_initialized(lowering->hir) ||
        ast == NULL || !vitte_ast_is_initialized(ast) || ast->root == NULL) {
        vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_HIR_E_ARGUMENT", "missing initialized AST or HIR for lowering", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_hir_builder_init(&builder, lowering->hir);
    module = vitte_hir_make_module(&builder, ast->root->as.module.name, ast->root);
    if (module == NULL) {
        vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_OUT_OF_MEMORY, "VITTE_HIR_E_ALLOC", "failed to allocate HIR module", NULL);
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }
    lowering->hir->root = module;

    for (decl = ast->root->as.module.declarations.first; decl != NULL; decl = decl->next) {
        if (decl->kind == VITTE_AST_NODE_PROC_DECL) {
            vitte_hir_function_t *function = vitte_hir_lower_function(lowering, decl, 1u);
            if (function == NULL || !vitte_hir_module_add_function(module, function)) {
                if (vitte_error_is_ok(&lowering->last_error)) {
                    vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INTERNAL, "VITTE_HIR_E_MODULE", "failed to append lowered function", NULL);
                }
                return lowering->last_error.status;
            }
        } else if (decl->kind == VITTE_AST_NODE_CONST_DECL) {
            vitte_hir_decl_t *const_decl = vitte_hir_lower_const(lowering, decl, 1u);
            if (const_decl == NULL || !vitte_hir_module_add_const(module, const_decl)) {
                if (vitte_error_is_ok(&lowering->last_error)) {
                    vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INTERNAL, "VITTE_HIR_E_MODULE", "failed to append lowered const declaration", NULL);
                }
                return lowering->last_error.status;
            }
        } else if (decl->kind == VITTE_AST_NODE_PICK_DECL) {
            vitte_hir_decl_t *pick = vitte_hir_lower_pick(lowering, decl, 1u);
            if (pick == NULL || !vitte_hir_module_add_decl(module, pick)) {
                if (vitte_error_is_ok(&lowering->last_error)) {
                    vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_INTERNAL, "VITTE_HIR_E_MODULE", "failed to append lowered pick declaration", NULL);
                }
                return lowering->last_error.status;
            }
        } else if (decl->kind == VITTE_AST_NODE_FORM_DECL) {
            vitte_hir_decl_t *form = vitte_hir_lower_form(lowering, decl, 1u);
            if (form == NULL || !vitte_hir_module_add_decl(module, form)) return VITTE_STATUS_ERROR_INVALID_STATE;
        } else {
            vitte_hir_lowering_set_error(lowering, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_HIR_E_DECL", "unsupported AST declaration for HIR lowering", vitte_ast_node_kind_name(decl->kind));
            return VITTE_STATUS_ERROR_UNSUPPORTED;
        }
    }

    vitte_error_reset(&lowering->last_error);
    vitte_hir_clear_error(lowering->hir);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_hir_lower_ast(vitte_hir_t *hir, const vitte_ast_t *ast) {
    vitte_hir_lowering_t lowering;

    vitte_hir_lowering_init(&lowering, hir);
    return vitte_hir_lower_ast_with_options(&lowering, ast);
}

static size_t vitte_hir_list_count_nodes(const vitte_hir_list_t *list) {
    const vitte_hir_node_t *node;
    size_t count = 0u;

    if (list == NULL) {
        return 0u;
    }
    for (node = list->first; node != NULL; node = node->next) {
        count++;
    }
    return count;
}

static vitte_status_t vitte_hir_validate_node(vitte_hir_t *hir, const vitte_hir_node_t *node, size_t depth, size_t max_depth, size_t *visited);

static vitte_status_t vitte_hir_validate_lowered_symbol(
    vitte_hir_t *hir,
    const char *name,
    const char *source_name,
    const char *lowered_name,
    const char *code,
    const char *message
) {
    if (name == NULL || name[0] == '\0') {
        vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, code, message, NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (source_name != NULL && source_name[0] == '\0') {
        vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, code, "HIR source symbol name is empty", name);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (lowered_name != NULL) {
        if (lowered_name[0] == '\0') {
            vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, code, "HIR lowered symbol name is empty", name);
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
        if (!vitte_hir_text_equal(name, lowered_name)) {
            vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, code, "HIR effective name does not match lowered symbol name", name);
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_hir_validate_list(vitte_hir_t *hir, const vitte_hir_list_t *list, size_t depth, size_t max_depth, size_t *visited) {
    const vitte_hir_node_t *node;

    if (list == NULL) {
        return VITTE_STATUS_OK;
    }
    if (vitte_hir_list_count_nodes(list) != list->count) {
        vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_LIST", "HIR list count is inconsistent", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    for (node = list->first; node != NULL; node = node->next) {
        vitte_status_t status = vitte_hir_validate_node(hir, node, depth + 1u, max_depth, visited);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_hir_validate_node(vitte_hir_t *hir, const vitte_hir_node_t *node, size_t depth, size_t max_depth, size_t *visited) {
    vitte_status_t status;

    if (node == NULL) {
        vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_NODE", "missing HIR node", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (depth > max_depth) {
        vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_DEPTH", "HIR validation exceeded maximum depth", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (node->id == 0u || !vitte_hir_kind_is_valid(node->kind)) {
        vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_NODE", "invalid HIR node id or kind", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (visited != NULL) {
        (*visited)++;
    }

    switch (node->kind) {
        case VITTE_HIR_MODULE:
            if (node->as.module.name == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_MODULE", "HIR module has no name", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            return vitte_hir_validate_list(hir, &node->as.module.declarations, depth, max_depth, visited);
        case VITTE_HIR_FUNCTION:
            status = vitte_hir_validate_lowered_symbol(
                hir,
                node->as.function.name,
                node->as.function.source_name,
                node->as.function.lowered_name,
                "VITTE_HIR_E_FUNCTION",
                "HIR function requires name and body"
            );
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            if (node->as.function.body == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_FUNCTION", "HIR function requires name and body", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            status = vitte_hir_validate_list(hir, &node->as.function.parameters, depth, max_depth, visited);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            for (const vitte_hir_node_t *param = node->as.function.parameters.first; param != NULL; param = param->next) {
                if (param->kind != VITTE_HIR_VARIABLE || param->as.variable.name == NULL || param->type == NULL) {
                    vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_PARAM", "HIR function parameter is invalid", node->as.function.name);
                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
                status = vitte_hir_validate_node(hir, param->type, depth + 1u, max_depth, visited);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            status = vitte_hir_validate_node(hir, node->as.function.return_type, depth + 1u, max_depth, visited);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_hir_validate_node(hir, node->as.function.body, depth + 1u, max_depth, visited);
        case VITTE_HIR_CONST_DECL:
            status = vitte_hir_validate_lowered_symbol(
                hir,
                node->as.const_decl.name,
                node->as.const_decl.source_name,
                node->as.const_decl.lowered_name,
                "VITTE_HIR_E_CONST",
                "HIR const requires name and value"
            );
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            if (node->as.const_decl.value == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_CONST", "HIR const requires name and value", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            if (node->as.const_decl.declared_type != NULL) {
                status = vitte_hir_validate_node(hir, node->as.const_decl.declared_type, depth + 1u, max_depth, visited);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            return vitte_hir_validate_node(hir, node->as.const_decl.value, depth + 1u, max_depth, visited);
        case VITTE_HIR_PICK_DECL:
            if (node->as.pick_decl.name == NULL || node->as.pick_decl.variants.count == 0u) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_PICK", "HIR pick requires a name and variants", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            status = vitte_hir_validate_list(hir, &node->as.pick_decl.variants, depth, max_depth, visited);
            return status;
        case VITTE_HIR_PICK_VARIANT:
            if (node->as.pick_variant.name == NULL || node->as.pick_variant.name[0] == '\0') {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_PICK", "HIR pick variant requires a name", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            return VITTE_STATUS_OK;
        case VITTE_HIR_FORM_DECL:
            if (node->as.form_decl.name == NULL || node->as.form_decl.fields.count == 0u) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_FORM", "HIR form requires name and fields", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            return vitte_hir_validate_list(hir, &node->as.form_decl.fields, depth, max_depth, visited);
        case VITTE_HIR_FORM_FIELD:
            if (node->as.form_field.name == NULL || node->as.form_field.type == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_FORM", "HIR form field requires name and type", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            return vitte_hir_validate_node(hir, node->as.form_field.type, depth + 1u, max_depth, visited);
        case VITTE_HIR_LIST_EXPR:
            return vitte_hir_validate_list(hir, &node->as.list_expr.elements, depth, max_depth, visited);
        case VITTE_HIR_RECORD_EXPR:
            if (node->as.record_expr.type_name == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_RECORD", "HIR record requires a type name", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            return vitte_hir_validate_list(hir, &node->as.record_expr.fields, depth, max_depth, visited);
        case VITTE_HIR_RECORD_FIELD:
            if (node->as.record_field.name == NULL || node->as.record_field.value == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_RECORD", "HIR record field requires name and value", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            return vitte_hir_validate_node(hir, node->as.record_field.value, depth + 1u, max_depth, visited);
        case VITTE_HIR_BLOCK:
            return vitte_hir_validate_list(hir, &node->as.block.statements, depth, max_depth, visited);
        case VITTE_HIR_RETURN_STMT:
            return node->as.return_stmt.value != NULL ?
                vitte_hir_validate_node(hir, node->as.return_stmt.value, depth + 1u, max_depth, visited) :
                VITTE_STATUS_OK;
        case VITTE_HIR_LET_STMT:
            if (node->as.let_stmt.name == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_LET", "HIR let requires a name", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            if (node->as.let_stmt.declared_type != NULL) {
                status = vitte_hir_validate_node(hir, node->as.let_stmt.declared_type, depth + 1u, max_depth, visited);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            return node->as.let_stmt.value != NULL ?
                vitte_hir_validate_node(hir, node->as.let_stmt.value, depth + 1u, max_depth, visited) :
                VITTE_STATUS_OK;
        case VITTE_HIR_EXPR_STMT:
        case VITTE_HIR_ASSIGN_STMT:
            if (node->kind == VITTE_HIR_ASSIGN_STMT) {
                if (node->as.assign_stmt.name == NULL || node->as.assign_stmt.value == NULL) {
                    vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_ASSIGN", "HIR assignment requires name and value", NULL);
                    return VITTE_STATUS_ERROR_INVALID_STATE;
                }
                return vitte_hir_validate_node(hir, node->as.assign_stmt.value, depth + 1u, max_depth, visited);
            }
            if (node->as.expr_stmt.value == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_EXPR_STMT", "HIR expression statement requires value", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            return vitte_hir_validate_node(hir, node->as.expr_stmt.value, depth + 1u, max_depth, visited);
        case VITTE_HIR_IF_STMT:
            if (node->as.if_stmt.condition == NULL || node->as.if_stmt.then_branch == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_IF", "HIR if requires condition and then branch", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            status = vitte_hir_validate_node(hir, node->as.if_stmt.condition, depth + 1u, max_depth, visited);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_hir_validate_node(hir, node->as.if_stmt.then_branch, depth + 1u, max_depth, visited);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return node->as.if_stmt.else_branch != NULL ?
                vitte_hir_validate_node(hir, node->as.if_stmt.else_branch, depth + 1u, max_depth, visited) :
                VITTE_STATUS_OK;
        case VITTE_HIR_BINARY_EXPR:
            if (node->as.binary_expr.operator_text == NULL || node->as.binary_expr.left == NULL || node->as.binary_expr.right == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_BINARY", "HIR binary expression is incomplete", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            status = vitte_hir_validate_node(hir, node->as.binary_expr.left, depth + 1u, max_depth, visited);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_hir_validate_node(hir, node->as.binary_expr.right, depth + 1u, max_depth, visited);
        case VITTE_HIR_CALL_EXPR:
            if (node->as.call_expr.callee == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_CALL", "HIR call requires callee", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            status = vitte_hir_validate_node(hir, node->as.call_expr.callee, depth + 1u, max_depth, visited);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_hir_validate_list(hir, &node->as.call_expr.arguments, depth, max_depth, visited);
        case VITTE_HIR_IF_EXPR:
            if (node->as.if_expr.condition == NULL || node->as.if_expr.then_value == NULL || node->as.if_expr.else_value == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_IF_EXPR", "HIR conditional expression is incomplete", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            status = vitte_hir_validate_node(hir, node->as.if_expr.condition, depth + 1u, max_depth, visited);
            if (status != VITTE_STATUS_OK) return status;
            status = vitte_hir_validate_node(hir, node->as.if_expr.then_value, depth + 1u, max_depth, visited);
            if (status != VITTE_STATUS_OK) return status;
            return vitte_hir_validate_node(hir, node->as.if_expr.else_value, depth + 1u, max_depth, visited);
        case VITTE_HIR_TYPE_NAME:
            if (node->as.type_name.name == NULL) {
                vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_TYPE", "HIR type name requires name", NULL);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            return VITTE_STATUS_OK;
        case VITTE_HIR_VARIABLE:
            return vitte_hir_validate_lowered_symbol(
                hir,
                node->as.variable.name,
                node->as.variable.source_name,
                node->as.variable.lowered_name,
                "VITTE_HIR_E_VARIABLE",
                "HIR variable requires name"
            );
        case VITTE_HIR_INTEGER_LITERAL:
        case VITTE_HIR_STRING_LITERAL:
        case VITTE_HIR_ERROR:
            return VITTE_STATUS_OK;
        case VITTE_HIR_KIND_COUNT:
        default:
            vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_KIND", "invalid HIR kind", NULL);
            return VITTE_STATUS_ERROR_INVALID_STATE;
    }
}

vitte_status_t vitte_hir_validate(vitte_hir_t *hir) {
    size_t visited = 0u;
    vitte_status_t status;

    if (!vitte_hir_is_initialized(hir)) {
        vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_STATE", "HIR is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (hir->root == NULL || hir->root->kind != VITTE_HIR_MODULE) {
        vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_ROOT", "HIR root module is missing", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    status = vitte_hir_validate_node(hir, hir->root, 0u, VITTE_HIR_DEFAULT_MAX_DEPTH, &visited);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    if (visited > hir->node_count) {
        vitte_hir_set_error(hir, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_HIR_E_COUNT", "HIR visited more nodes than allocated", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    vitte_hir_clear_error(hir);
    return VITTE_STATUS_OK;
}

static size_t vitte_hir_visit_node(vitte_hir_node_t *node, vitte_hir_visit_fn callback, void *user, size_t depth, size_t max_depth) {
    size_t count = 0u;
    vitte_hir_node_t *child;

    if (node == NULL || depth > max_depth) {
        return 0u;
    }
    if (callback != NULL && !callback(node, user)) {
        return 0u;
    }
    count++;

    switch (node->kind) {
        case VITTE_HIR_MODULE:
            for (child = node->as.module.declarations.first; child != NULL; child = child->next) {
                count += vitte_hir_visit_node(child, callback, user, depth + 1u, max_depth);
            }
            break;
        case VITTE_HIR_FUNCTION:
            for (child = node->as.function.parameters.first; child != NULL; child = child->next) {
                count += vitte_hir_visit_node(child, callback, user, depth + 1u, max_depth);
            }
            count += vitte_hir_visit_node(node->as.function.return_type, callback, user, depth + 1u, max_depth);
            count += vitte_hir_visit_node(node->as.function.body, callback, user, depth + 1u, max_depth);
            break;
        case VITTE_HIR_CONST_DECL:
            count += vitte_hir_visit_node(node->as.const_decl.declared_type, callback, user, depth + 1u, max_depth);
            count += vitte_hir_visit_node(node->as.const_decl.value, callback, user, depth + 1u, max_depth);
            break;
        case VITTE_HIR_BLOCK:
            for (child = node->as.block.statements.first; child != NULL; child = child->next) {
                count += vitte_hir_visit_node(child, callback, user, depth + 1u, max_depth);
            }
            break;
        case VITTE_HIR_RETURN_STMT:
            count += vitte_hir_visit_node(node->as.return_stmt.value, callback, user, depth + 1u, max_depth);
            break;
        case VITTE_HIR_LET_STMT:
            count += vitte_hir_visit_node(node->as.let_stmt.declared_type, callback, user, depth + 1u, max_depth);
            count += vitte_hir_visit_node(node->as.let_stmt.value, callback, user, depth + 1u, max_depth);
            break;
        case VITTE_HIR_ASSIGN_STMT:
            count += vitte_hir_visit_node(node->as.assign_stmt.value, callback, user, depth + 1u, max_depth);
            break;
        case VITTE_HIR_EXPR_STMT:
            count += vitte_hir_visit_node(node->as.expr_stmt.value, callback, user, depth + 1u, max_depth);
            break;
        case VITTE_HIR_IF_STMT:
            count += vitte_hir_visit_node(node->as.if_stmt.condition, callback, user, depth + 1u, max_depth);
            count += vitte_hir_visit_node(node->as.if_stmt.then_branch, callback, user, depth + 1u, max_depth);
            count += vitte_hir_visit_node(node->as.if_stmt.else_branch, callback, user, depth + 1u, max_depth);
            break;
        case VITTE_HIR_BINARY_EXPR:
            count += vitte_hir_visit_node(node->as.binary_expr.left, callback, user, depth + 1u, max_depth);
            count += vitte_hir_visit_node(node->as.binary_expr.right, callback, user, depth + 1u, max_depth);
            break;
        case VITTE_HIR_CALL_EXPR:
            count += vitte_hir_visit_node(node->as.call_expr.callee, callback, user, depth + 1u, max_depth);
            for (child = node->as.call_expr.arguments.first; child != NULL; child = child->next) {
                count += vitte_hir_visit_node(child, callback, user, depth + 1u, max_depth);
            }
            break;
        case VITTE_HIR_IF_EXPR:
            count += vitte_hir_visit_node(node->as.if_expr.condition, callback, user, depth + 1u, max_depth);
            count += vitte_hir_visit_node(node->as.if_expr.then_value, callback, user, depth + 1u, max_depth);
            count += vitte_hir_visit_node(node->as.if_expr.else_value, callback, user, depth + 1u, max_depth);
            break;
        default:
            break;
    }
    return count;
}

size_t vitte_hir_visit(vitte_hir_node_t *node, vitte_hir_visit_fn callback, void *user, size_t max_depth) {
    if (max_depth == 0u) {
        max_depth = VITTE_HIR_DEFAULT_MAX_DEPTH;
    }
    return vitte_hir_visit_node(node, callback, user, 0u, max_depth);
}

static void vitte_hir_dump_node(const vitte_hir_node_t *node, FILE *stream, size_t depth, size_t max_depth) {
    size_t index;
    const vitte_hir_node_t *child;

    if (node == NULL || stream == NULL || depth > max_depth) {
        return;
    }
    for (index = 0u; index < depth; index++) {
        (void)fputs("  ", stream);
    }
    if (node->kind == VITTE_HIR_INTEGER_LITERAL) {
        (void)fprintf(stream, "#%" PRIu32 " %s %" PRId64 "\n", node->id, vitte_hir_kind_name(node->kind), node->as.integer_literal.value);
    } else {
        (void)fprintf(stream, "#%" PRIu32 " %s %s\n", node->id, vitte_hir_kind_name(node->kind), vitte_hir_node_label(node));
    }

    switch (node->kind) {
        case VITTE_HIR_MODULE:
            for (child = node->as.module.declarations.first; child != NULL; child = child->next) {
                vitte_hir_dump_node(child, stream, depth + 1u, max_depth);
            }
            break;
        case VITTE_HIR_FUNCTION:
            for (child = node->as.function.parameters.first; child != NULL; child = child->next) {
                vitte_hir_dump_node(child, stream, depth + 1u, max_depth);
            }
            vitte_hir_dump_node(node->as.function.return_type, stream, depth + 1u, max_depth);
            vitte_hir_dump_node(node->as.function.body, stream, depth + 1u, max_depth);
            break;
        case VITTE_HIR_CONST_DECL:
            vitte_hir_dump_node(node->as.const_decl.declared_type, stream, depth + 1u, max_depth);
            vitte_hir_dump_node(node->as.const_decl.value, stream, depth + 1u, max_depth);
            break;
        case VITTE_HIR_BLOCK:
            for (child = node->as.block.statements.first; child != NULL; child = child->next) {
                vitte_hir_dump_node(child, stream, depth + 1u, max_depth);
            }
            break;
        case VITTE_HIR_RETURN_STMT:
            vitte_hir_dump_node(node->as.return_stmt.value, stream, depth + 1u, max_depth);
            break;
        case VITTE_HIR_LET_STMT:
            vitte_hir_dump_node(node->as.let_stmt.declared_type, stream, depth + 1u, max_depth);
            vitte_hir_dump_node(node->as.let_stmt.value, stream, depth + 1u, max_depth);
            break;
        case VITTE_HIR_ASSIGN_STMT:
            vitte_hir_dump_node(node->as.assign_stmt.value, stream, depth + 1u, max_depth);
            break;
        case VITTE_HIR_EXPR_STMT:
            vitte_hir_dump_node(node->as.expr_stmt.value, stream, depth + 1u, max_depth);
            break;
        case VITTE_HIR_IF_STMT:
            vitte_hir_dump_node(node->as.if_stmt.condition, stream, depth + 1u, max_depth);
            vitte_hir_dump_node(node->as.if_stmt.then_branch, stream, depth + 1u, max_depth);
            vitte_hir_dump_node(node->as.if_stmt.else_branch, stream, depth + 1u, max_depth);
            break;
        case VITTE_HIR_BINARY_EXPR:
            vitte_hir_dump_node(node->as.binary_expr.left, stream, depth + 1u, max_depth);
            vitte_hir_dump_node(node->as.binary_expr.right, stream, depth + 1u, max_depth);
            break;
        case VITTE_HIR_CALL_EXPR:
            vitte_hir_dump_node(node->as.call_expr.callee, stream, depth + 1u, max_depth);
            for (child = node->as.call_expr.arguments.first; child != NULL; child = child->next) {
                vitte_hir_dump_node(child, stream, depth + 1u, max_depth);
            }
            break;
        case VITTE_HIR_IF_EXPR:
            vitte_hir_dump_node(node->as.if_expr.condition, stream, depth + 1u, max_depth);
            vitte_hir_dump_node(node->as.if_expr.then_value, stream, depth + 1u, max_depth);
            vitte_hir_dump_node(node->as.if_expr.else_value, stream, depth + 1u, max_depth);
            break;
        default:
            break;
    }
}

void vitte_hir_dump(const vitte_hir_node_t *node, FILE *stream, size_t max_depth) {
    if (stream == NULL) {
        return;
    }
    if (max_depth == 0u) {
        max_depth = VITTE_HIR_DEFAULT_MAX_DEPTH;
    }
    vitte_hir_dump_node(node, stream, 0u, max_depth);
}
