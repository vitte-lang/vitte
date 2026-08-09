#include "ast.h"

#include <inttypes.h>
#include <string.h>

static const size_t VITTE_AST_DEFAULT_MAX_DEPTH = 256u;

static void vitte_ast_set_error(
    vitte_ast_t *ast,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (ast == NULL) {
        vitte_error_t error;
        vitte_error_set_details(&error, status, code, message, details);
        return;
    }

    vitte_error_set_details(&ast->last_error, status, code, message, details);
}

void vitte_ast_span_init(vitte_ast_span_t *span) {
    if (span == NULL) {
        return;
    }

    memset(span, 0, sizeof(*span));
}

bool vitte_ast_span_is_valid(const vitte_ast_span_t *span) {
    if (span == NULL || !span->valid) {
        return false;
    }

    return span->start_offset <= span->end_offset &&
        span->start_line <= span->end_line;
}

bool vitte_ast_span_merge(
    const vitte_ast_span_t *left,
    const vitte_ast_span_t *right,
    vitte_ast_span_t *out
) {
    if (out == NULL || !vitte_ast_span_is_valid(left) || !vitte_ast_span_is_valid(right)) {
        return false;
    }

    *out = *left;
    if (right->start_offset < out->start_offset) {
        out->start_offset = right->start_offset;
        out->start_line = right->start_line;
        out->start_column = right->start_column;
    }
    if (right->end_offset > out->end_offset) {
        out->end_offset = right->end_offset;
        out->end_line = right->end_line;
        out->end_column = right->end_column;
    }
    if (out->source_name == NULL) {
        out->source_name = right->source_name;
    }
    out->valid = true;
    return true;
}

void vitte_ast_list_init(vitte_ast_list_t *list) {
    if (list == NULL) {
        return;
    }

    memset(list, 0, sizeof(*list));
}

bool vitte_ast_list_append(vitte_ast_list_t *list, vitte_ast_node_t *node) {
    if (list == NULL || node == NULL) {
        return false;
    }

    node->next = NULL;
    if (list->last == NULL) {
        list->first = node;
        list->last = node;
    } else {
        list->last->next = node;
        list->last = node;
    }
    list->count++;
    return true;
}

vitte_status_t vitte_ast_init(vitte_ast_t *ast, vitte_arena_t *arena) {
    if (ast == NULL || !vitte_arena_is_initialized(arena)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(ast, 0, sizeof(*ast));
    ast->initialized = true;
    ast->owns_arena = false;
    ast->arena = arena;
    vitte_error_init(&ast->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_ast_init_owned(vitte_ast_t *ast, const vitte_arena_config_t *config) {
    vitte_status_t status;

    if (ast == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(ast, 0, sizeof(*ast));
    status = vitte_arena_init(&ast->owned_arena, config);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    ast->initialized = true;
    ast->owns_arena = true;
    ast->arena = &ast->owned_arena;
    vitte_error_init(&ast->last_error);
    return VITTE_STATUS_OK;
}

void vitte_ast_destroy(vitte_ast_t *ast) {
    if (ast == NULL) {
        return;
    }

    if (ast->owns_arena) {
        vitte_arena_destroy(&ast->owned_arena);
    }
    memset(ast, 0, sizeof(*ast));
}

bool vitte_ast_is_initialized(const vitte_ast_t *ast) {
    return ast != NULL && ast->initialized && vitte_arena_is_initialized(ast->arena);
}

vitte_ast_node_t *vitte_ast_alloc_node(
    vitte_ast_t *ast,
    vitte_ast_node_kind_t kind,
    vitte_ast_span_t span
) {
    vitte_ast_node_t *node;

    if (!vitte_ast_is_initialized(ast) || !vitte_ast_node_kind_is_valid(kind)) {
        vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_KIND", "invalid AST node allocation", NULL);
        return NULL;
    }

    node = (vitte_ast_node_t *)vitte_arena_alloc_zeroed(ast->arena, sizeof(*node), _Alignof(vitte_ast_node_t));
    if (node == NULL) {
        vitte_ast_set_error(ast, VITTE_STATUS_ERROR_OUT_OF_MEMORY, "VITTE_AST_E_OOM", "unable to allocate AST node", NULL);
        return NULL;
    }

    node->kind = kind;
    node->span = span;
    ast->node_count++;
    return node;
}

const vitte_error_t *vitte_ast_last_error(const vitte_ast_t *ast) {
    return ast != NULL ? &ast->last_error : vitte_error_last();
}

void vitte_ast_clear_error(vitte_ast_t *ast) {
    if (ast != NULL) {
        vitte_error_reset(&ast->last_error);
    }
}

const char *vitte_ast_node_kind_name(vitte_ast_node_kind_t kind) {
    switch (kind) {
        case VITTE_AST_NODE_ERROR:
            return "error";
        case VITTE_AST_NODE_MODULE:
            return "module";
        case VITTE_AST_NODE_IMPORT_DECL:
            return "import_decl";
        case VITTE_AST_NODE_EXPORT_DECL:
            return "export_decl";
        case VITTE_AST_NODE_PROC_DECL:
            return "proc_decl";
        case VITTE_AST_NODE_PARAM_DECL:
            return "param_decl";
        case VITTE_AST_NODE_CONST_DECL:
            return "const_decl";
        case VITTE_AST_NODE_PICK_DECL:
            return "pick_decl";
        case VITTE_AST_NODE_PICK_VARIANT:
            return "pick_variant";
        case VITTE_AST_NODE_FORM_DECL:
            return "form_decl";
        case VITTE_AST_NODE_FORM_FIELD:
            return "form_field";
        case VITTE_AST_NODE_BLOCK_STMT:
            return "block_stmt";
        case VITTE_AST_NODE_GIVE_STMT:
            return "give_stmt";
        case VITTE_AST_NODE_LET_STMT:
            return "let_stmt";
        case VITTE_AST_NODE_ASSIGN_STMT:
            return "assign_stmt";
        case VITTE_AST_NODE_EXPR_STMT:
            return "expr_stmt";
        case VITTE_AST_NODE_IF_STMT:
            return "if_stmt";
        case VITTE_AST_NODE_INTEGER_LITERAL:
            return "integer_literal";
        case VITTE_AST_NODE_STRING_LITERAL:
            return "string_literal";
        case VITTE_AST_NODE_IDENTIFIER:
            return "identifier";
        case VITTE_AST_NODE_BINARY_EXPR:
            return "binary_expr";
        case VITTE_AST_NODE_CALL_EXPR:
            return "call_expr";
        case VITTE_AST_NODE_LIST_EXPR:
            return "list_expr";
        case VITTE_AST_NODE_RECORD_EXPR:
            return "record_expr";
        case VITTE_AST_NODE_RECORD_FIELD:
            return "record_field";
        case VITTE_AST_NODE_CAST_EXPR:
            return "cast_expr";
        case VITTE_AST_NODE_INDEX_EXPR:
            return "index_expr";
        case VITTE_AST_NODE_TYPE_NAME:
            return "type_name";
        case VITTE_AST_NODE_COUNT:
        default:
            return "unknown";
    }
}

bool vitte_ast_node_kind_is_valid(vitte_ast_node_kind_t kind) {
    return kind >= VITTE_AST_NODE_ERROR && kind < VITTE_AST_NODE_COUNT;
}

const char *vitte_ast_node_label(const vitte_ast_node_t *node) {
    if (node == NULL) {
        return NULL;
    }

    switch (node->kind) {
        case VITTE_AST_NODE_MODULE:
            return node->as.module.name;
        case VITTE_AST_NODE_IMPORT_DECL:
            return node->as.import_decl.path;
        case VITTE_AST_NODE_EXPORT_DECL:
            return node->as.export_decl.export_name;
        case VITTE_AST_NODE_PROC_DECL:
            return node->as.proc_decl.name;
        case VITTE_AST_NODE_PARAM_DECL:
            return node->as.param_decl.name;
        case VITTE_AST_NODE_CONST_DECL:
            return node->as.const_decl.name;
        case VITTE_AST_NODE_PICK_DECL:
            return node->as.pick_decl.name;
        case VITTE_AST_NODE_PICK_VARIANT:
            return node->as.pick_variant.name;
        case VITTE_AST_NODE_FORM_DECL:
            return node->as.form_decl.name;
        case VITTE_AST_NODE_FORM_FIELD:
            return node->as.form_field.name;
        case VITTE_AST_NODE_LET_STMT:
            return node->as.let_stmt.name;
        case VITTE_AST_NODE_ASSIGN_STMT:
            return node->as.assign_stmt.target != NULL ? vitte_ast_node_label(node->as.assign_stmt.target) : NULL;
        case VITTE_AST_NODE_STRING_LITERAL:
            return node->as.string_literal.value;
        case VITTE_AST_NODE_IDENTIFIER:
            return node->as.identifier.name;
        case VITTE_AST_NODE_BINARY_EXPR:
            return node->as.binary_expr.operator_text;
        case VITTE_AST_NODE_TYPE_NAME:
            return node->as.type_name.name;
        case VITTE_AST_NODE_RECORD_EXPR:
            return node->as.record_expr.type_name;
        case VITTE_AST_NODE_RECORD_FIELD:
            return node->as.record_field.name;
        case VITTE_AST_NODE_CAST_EXPR:
            return node->as.cast_expr.type != NULL ? node->as.cast_expr.type->as.type_name.name : NULL;
        case VITTE_AST_NODE_INDEX_EXPR:
            return "[]";
        case VITTE_AST_NODE_ERROR:
            return node->as.error_node.message;
        default:
            return NULL;
    }
}

const char *vitte_ast_decl_name(const vitte_ast_decl_t *decl) {
    if (decl == NULL) {
        return NULL;
    }
    if (decl->kind == VITTE_AST_NODE_PROC_DECL) {
        return decl->as.proc_decl.name;
    }
    if (decl->kind == VITTE_AST_NODE_CONST_DECL) {
        return decl->as.const_decl.name;
    }
    return NULL;
}

const vitte_ast_decl_t *vitte_ast_module_find_decl(const vitte_ast_module_t *module, const char *name) {
    const vitte_ast_node_t *decl;

    if (module == NULL || module->kind != VITTE_AST_NODE_MODULE || name == NULL) {
        return NULL;
    }
    for (decl = module->as.module.declarations.first; decl != NULL; decl = decl->next) {
        const char *decl_name = vitte_ast_decl_name(decl);
        if (decl_name != NULL && strcmp(decl_name, name) == 0) {
            return decl;
        }
    }
    return NULL;
}

const vitte_ast_decl_t *vitte_ast_export_decl_target(
    const vitte_ast_module_t *module,
    const vitte_ast_decl_t *export_decl
) {
    if (module == NULL || export_decl == NULL || export_decl->kind != VITTE_AST_NODE_EXPORT_DECL ||
        export_decl->as.export_decl.local_name == NULL) {
        return NULL;
    }
    return vitte_ast_module_find_decl(module, export_decl->as.export_decl.local_name);
}

bool vitte_ast_module_decl_is_exported(
    const vitte_ast_module_t *module,
    const vitte_ast_decl_t *decl
) {
    const vitte_ast_node_t *export_decl;
    const char *decl_name;

    if (module == NULL || module->kind != VITTE_AST_NODE_MODULE || decl == NULL) {
        return false;
    }
    if (decl->kind == VITTE_AST_NODE_PROC_DECL) {
        if (decl->as.proc_decl.exported || module->as.module.export_all) {
            return true;
        }
    } else if (decl->kind == VITTE_AST_NODE_CONST_DECL) {
        if (decl->as.const_decl.exported || module->as.module.export_all) {
            return true;
        }
    } else {
        return false;
    }

    decl_name = vitte_ast_decl_name(decl);
    if (decl_name == NULL) {
        return false;
    }
    for (export_decl = module->as.module.exports.first; export_decl != NULL; export_decl = export_decl->next) {
        if (export_decl->kind == VITTE_AST_NODE_EXPORT_DECL &&
            export_decl->as.export_decl.local_name != NULL &&
            strcmp(export_decl->as.export_decl.local_name, decl_name) == 0) {
            return true;
        }
    }
    return false;
}

const vitte_ast_decl_t *vitte_ast_module_find_exported_decl(
    const vitte_ast_module_t *module,
    const char *export_name
) {
    const vitte_ast_node_t *export_decl;
    const vitte_ast_decl_t *decl;

    if (module == NULL || module->kind != VITTE_AST_NODE_MODULE || export_name == NULL) {
        return NULL;
    }
    for (export_decl = module->as.module.exports.first; export_decl != NULL; export_decl = export_decl->next) {
        const vitte_ast_decl_t *target;

        if (export_decl->kind != VITTE_AST_NODE_EXPORT_DECL ||
            export_decl->as.export_decl.export_name == NULL ||
            strcmp(export_decl->as.export_decl.export_name, export_name) != 0) {
            continue;
        }
        target = vitte_ast_export_decl_target(module, export_decl);
        if (target != NULL) {
            return target;
        }
    }

    decl = vitte_ast_module_find_decl(module, export_name);
    if (decl != NULL && vitte_ast_module_decl_is_exported(module, decl)) {
        return decl;
    }
    return NULL;
}

static bool vitte_ast_export_matches(
    const vitte_ast_decl_t *decl,
    const char *public_name,
    const vitte_ast_decl_t *other_decl,
    const char *other_public_name
) {
    return decl == other_decl &&
        public_name != NULL &&
        other_public_name != NULL &&
        strcmp(public_name, other_public_name) == 0;
}

static bool vitte_ast_module_export_is_duplicate(
    const vitte_ast_module_t *module,
    const vitte_ast_decl_t *export_decl,
    const vitte_ast_decl_t *target_decl,
    const char *public_name
) {
    const char *decl_name;
    const vitte_ast_node_t *previous;

    if (module == NULL || export_decl == NULL || target_decl == NULL || public_name == NULL) {
        return false;
    }
    decl_name = vitte_ast_decl_name(target_decl);
    if (decl_name != NULL &&
        vitte_ast_module_decl_is_exported(module, target_decl) &&
        vitte_ast_export_matches(target_decl, public_name, target_decl, decl_name)) {
        return true;
    }
    for (previous = module->as.module.exports.first; previous != NULL && previous != export_decl; previous = previous->next) {
        const vitte_ast_decl_t *previous_target;
        const char *previous_public_name;

        if (previous->kind != VITTE_AST_NODE_EXPORT_DECL) {
            continue;
        }
        previous_target = vitte_ast_export_decl_target(module, previous);
        previous_public_name = previous->as.export_decl.export_name;
        if (vitte_ast_export_matches(target_decl, public_name, previous_target, previous_public_name)) {
            return true;
        }
    }
    return false;
}

size_t vitte_ast_module_visit_exports(
    const vitte_ast_module_t *module,
    vitte_ast_export_visit_fn callback,
    void *user
) {
    const vitte_ast_node_t *decl;
    const vitte_ast_node_t *export_decl;
    size_t count = 0u;

    if (module == NULL || module->kind != VITTE_AST_NODE_MODULE || callback == NULL) {
        return 0u;
    }

    for (decl = module->as.module.declarations.first; decl != NULL; decl = decl->next) {
        const char *decl_name = vitte_ast_decl_name(decl);

        if (decl_name == NULL || !vitte_ast_module_decl_is_exported(module, decl)) {
            continue;
        }
        if (!callback(decl, decl_name, user)) {
            return count;
        }
        count++;
    }

    for (export_decl = module->as.module.exports.first; export_decl != NULL; export_decl = export_decl->next) {
        const vitte_ast_decl_t *target_decl;
        const char *public_name;

        if (export_decl->kind != VITTE_AST_NODE_EXPORT_DECL) {
            continue;
        }
        target_decl = vitte_ast_export_decl_target(module, export_decl);
        public_name = export_decl->as.export_decl.export_name;
        if (target_decl == NULL || public_name == NULL ||
            vitte_ast_module_export_is_duplicate(module, export_decl, target_decl, public_name)) {
            continue;
        }
        if (!callback(target_decl, public_name, user)) {
            return count;
        }
        count++;
    }

    return count;
}

void vitte_ast_builder_init(vitte_ast_builder_t *builder, vitte_ast_t *ast) {
    if (builder == NULL) {
        return;
    }

    builder->ast = ast;
}

vitte_ast_module_t *vitte_ast_make_module(vitte_ast_builder_t *builder, const char *name, vitte_ast_span_t span) {
    vitte_ast_node_t *node;

    if (builder == NULL) {
        return NULL;
    }

    node = vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_MODULE, span);
    if (node == NULL) {
        return NULL;
    }
    node->as.module.name = name;
    vitte_ast_list_init(&node->as.module.imports);
    vitte_ast_list_init(&node->as.module.exports);
    vitte_ast_list_init(&node->as.module.declarations);
    node->as.module.export_all = false;
    builder->ast->root = node;
    return node;
}

vitte_ast_decl_t *vitte_ast_make_pick_decl(vitte_ast_builder_t *builder, const char *name, bool exported, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_PICK_DECL, span) : NULL;
    if (node != NULL) {
        node->as.pick_decl.name = name;
        node->as.pick_decl.exported = exported;
        vitte_ast_list_init(&node->as.pick_decl.variants);
    }
    return node;
}

vitte_ast_node_t *vitte_ast_make_pick_variant(vitte_ast_builder_t *builder, const char *name, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_PICK_VARIANT, span) : NULL;
    if (node != NULL) {
        node->as.pick_variant.name = name;
    }
    return node;
}

vitte_ast_decl_t *vitte_ast_make_form_decl(vitte_ast_builder_t *builder, const char *name, bool exported, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_FORM_DECL, span) : NULL;
    if (node != NULL) {
        node->as.form_decl.name = name;
        node->as.form_decl.exported = exported;
        vitte_ast_list_init(&node->as.form_decl.fields);
    }
    return node;
}

vitte_ast_node_t *vitte_ast_make_form_field(vitte_ast_builder_t *builder, const char *name, vitte_ast_type_ref_t *type, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_FORM_FIELD, span) : NULL;
    if (node != NULL) {
        node->as.form_field.name = name;
        node->as.form_field.type = type;
    }
    return node;
}

vitte_ast_decl_t *vitte_ast_make_import_decl(
    vitte_ast_builder_t *builder,
    const char *path,
    const char *alias,
    bool relative,
    vitte_ast_import_kind_t import_kind,
    vitte_ast_span_t span
) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_IMPORT_DECL, span) : NULL;
    if (node != NULL) {
        node->as.import_decl.path = path;
        node->as.import_decl.alias = alias;
        node->as.import_decl.relative = relative;
        node->as.import_decl.import_kind = import_kind;
    }
    return node;
}

vitte_ast_decl_t *vitte_ast_make_export_decl(
    vitte_ast_builder_t *builder,
    const char *local_name,
    const char *export_name,
    vitte_ast_span_t span
) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_EXPORT_DECL, span) : NULL;
    if (node != NULL) {
        node->as.export_decl.local_name = local_name;
        node->as.export_decl.export_name = export_name;
    }
    return node;
}

vitte_ast_decl_t *vitte_ast_make_proc_decl(vitte_ast_builder_t *builder, const char *name, bool exported, vitte_ast_type_ref_t *return_type, vitte_ast_stmt_t *body, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_PROC_DECL, span) : NULL;
    if (node != NULL) {
        node->as.proc_decl.name = name;
        node->as.proc_decl.exported = exported;
        vitte_ast_list_init(&node->as.proc_decl.parameters);
        node->as.proc_decl.return_type = return_type;
        node->as.proc_decl.body = body;
    }
    return node;
}

vitte_ast_node_t *vitte_ast_make_param_decl(vitte_ast_builder_t *builder, const char *name, vitte_ast_type_ref_t *type, bool mutable_value, bool by_ref, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_PARAM_DECL, span) : NULL;
    if (node != NULL) {
        node->as.param_decl.name = name;
        node->as.param_decl.type = type;
        node->as.param_decl.mutable_value = mutable_value;
        node->as.param_decl.by_ref = by_ref;
    }
    return node;
}

vitte_ast_decl_t *vitte_ast_make_const_decl(vitte_ast_builder_t *builder, const char *name, bool exported, vitte_ast_type_ref_t *type, vitte_ast_expr_t *value, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_CONST_DECL, span) : NULL;
    if (node != NULL) {
        node->as.const_decl.name = name;
        node->as.const_decl.exported = exported;
        node->as.const_decl.type = type;
        node->as.const_decl.value = value;
    }
    return node;
}

vitte_ast_stmt_t *vitte_ast_make_block_stmt(vitte_ast_builder_t *builder, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_BLOCK_STMT, span) : NULL;
    if (node != NULL) {
        vitte_ast_list_init(&node->as.block_stmt.statements);
    }
    return node;
}

vitte_ast_stmt_t *vitte_ast_make_give_stmt(vitte_ast_builder_t *builder, vitte_ast_expr_t *value, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_GIVE_STMT, span) : NULL;
    if (node != NULL) {
        node->as.give_stmt.value = value;
    }
    return node;
}

vitte_ast_stmt_t *vitte_ast_make_let_stmt(vitte_ast_builder_t *builder, const char *name, vitte_ast_type_ref_t *type, vitte_ast_expr_t *value, bool mutable_value, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_LET_STMT, span) : NULL;
    if (node != NULL) {
        node->as.let_stmt.name = name;
        node->as.let_stmt.type = type;
        node->as.let_stmt.value = value;
        node->as.let_stmt.mutable_value = mutable_value;
    }
    return node;
}

vitte_ast_stmt_t *vitte_ast_make_assign_stmt(vitte_ast_builder_t *builder, vitte_ast_expr_t *target, vitte_ast_expr_t *value, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_ASSIGN_STMT, span) : NULL;
    if (node != NULL) {
        node->as.assign_stmt.target = target;
        node->as.assign_stmt.value = value;
    }
    return node;
}

vitte_ast_stmt_t *vitte_ast_make_expr_stmt(vitte_ast_builder_t *builder, vitte_ast_expr_t *value, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_EXPR_STMT, span) : NULL;
    if (node != NULL) {
        node->as.expr_stmt.value = value;
    }
    return node;
}

vitte_ast_stmt_t *vitte_ast_make_if_stmt(vitte_ast_builder_t *builder, vitte_ast_expr_t *condition, vitte_ast_stmt_t *then_branch, vitte_ast_stmt_t *else_branch, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_IF_STMT, span) : NULL;
    if (node != NULL) {
        node->as.if_stmt.condition = condition;
        node->as.if_stmt.then_branch = then_branch;
        node->as.if_stmt.else_branch = else_branch;
    }
    return node;
}

vitte_ast_expr_t *vitte_ast_make_integer_literal(vitte_ast_builder_t *builder, int64_t value, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_INTEGER_LITERAL, span) : NULL;
    if (node != NULL) {
        node->as.integer_literal.value = value;
    }
    return node;
}

vitte_ast_expr_t *vitte_ast_make_string_literal(vitte_ast_builder_t *builder, const char *value, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_STRING_LITERAL, span) : NULL;
    if (node != NULL) {
        node->as.string_literal.value = value;
    }
    return node;
}

vitte_ast_expr_t *vitte_ast_make_identifier(vitte_ast_builder_t *builder, const char *name, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_IDENTIFIER, span) : NULL;
    if (node != NULL) {
        node->as.identifier.name = name;
    }
    return node;
}

vitte_ast_expr_t *vitte_ast_make_binary_expr(vitte_ast_builder_t *builder, const char *operator_text, vitte_ast_expr_t *left, vitte_ast_expr_t *right, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_BINARY_EXPR, span) : NULL;
    if (node != NULL) {
        node->as.binary_expr.operator_text = operator_text;
        node->as.binary_expr.left = left;
        node->as.binary_expr.right = right;
    }
    return node;
}

vitte_ast_expr_t *vitte_ast_make_call_expr(vitte_ast_builder_t *builder, vitte_ast_expr_t *callee, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_CALL_EXPR, span) : NULL;
    if (node != NULL) {
        node->as.call_expr.callee = callee;
        vitte_ast_list_init(&node->as.call_expr.arguments);
    }
    return node;
}

vitte_ast_expr_t *vitte_ast_make_list_expr(vitte_ast_builder_t *builder, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_LIST_EXPR, span) : NULL;
    if (node != NULL) vitte_ast_list_init(&node->as.list_expr.elements);
    return node;
}

vitte_ast_expr_t *vitte_ast_make_record_expr(vitte_ast_builder_t *builder, const char *type_name, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_RECORD_EXPR, span) : NULL;
    if (node != NULL) {
        node->as.record_expr.type_name = type_name;
        vitte_ast_list_init(&node->as.record_expr.fields);
    }
    return node;
}

vitte_ast_node_t *vitte_ast_make_record_field(vitte_ast_builder_t *builder, const char *name, vitte_ast_expr_t *value, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_RECORD_FIELD, span) : NULL;
    if (node != NULL) {
        node->as.record_field.name = name;
        node->as.record_field.value = value;
    }
    return node;
}

vitte_ast_expr_t *vitte_ast_make_cast_expr(vitte_ast_builder_t *builder, vitte_ast_expr_t *value, vitte_ast_type_ref_t *type, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_CAST_EXPR, span) : NULL;
    if (node != NULL) { node->as.cast_expr.value = value; node->as.cast_expr.type = type; }
    return node;
}

vitte_ast_expr_t *vitte_ast_make_index_expr(vitte_ast_builder_t *builder, vitte_ast_expr_t *base, vitte_ast_expr_t *index, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_INDEX_EXPR, span) : NULL;
    if (node != NULL) { node->as.index_expr.base = base; node->as.index_expr.index = index; }
    return node;
}

vitte_ast_type_ref_t *vitte_ast_make_type_name(vitte_ast_builder_t *builder, const char *name, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_TYPE_NAME, span) : NULL;
    if (node != NULL) {
        node->as.type_name.name = name;
    }
    return node;
}

vitte_ast_node_t *vitte_ast_make_error(vitte_ast_builder_t *builder, const char *message, vitte_ast_span_t span) {
    vitte_ast_node_t *node = builder != NULL ? vitte_ast_alloc_node(builder->ast, VITTE_AST_NODE_ERROR, span) : NULL;
    if (node != NULL) {
        node->as.error_node.message = message;
    }
    return node;
}

bool vitte_ast_module_add_decl(vitte_ast_module_t *module, vitte_ast_decl_t *decl) {
    return module != NULL &&
        module->kind == VITTE_AST_NODE_MODULE &&
        decl != NULL &&
        (decl->kind == VITTE_AST_NODE_PROC_DECL || decl->kind == VITTE_AST_NODE_CONST_DECL || decl->kind == VITTE_AST_NODE_PICK_DECL || decl->kind == VITTE_AST_NODE_FORM_DECL) &&
        vitte_ast_list_append(&module->as.module.declarations, decl);
}

bool vitte_ast_module_add_import(vitte_ast_module_t *module, vitte_ast_decl_t *import_decl) {
    return module != NULL &&
        module->kind == VITTE_AST_NODE_MODULE &&
        import_decl != NULL &&
        import_decl->kind == VITTE_AST_NODE_IMPORT_DECL &&
        vitte_ast_list_append(&module->as.module.imports, import_decl);
}

bool vitte_ast_module_add_export(vitte_ast_module_t *module, vitte_ast_decl_t *export_decl) {
    return module != NULL &&
        module->kind == VITTE_AST_NODE_MODULE &&
        export_decl != NULL &&
        export_decl->kind == VITTE_AST_NODE_EXPORT_DECL &&
        vitte_ast_list_append(&module->as.module.exports, export_decl);
}

void vitte_ast_module_set_export_all(vitte_ast_module_t *module, bool enabled) {
    if (module == NULL || module->kind != VITTE_AST_NODE_MODULE) {
        return;
    }
    module->as.module.export_all = enabled;
}

bool vitte_ast_proc_add_param(vitte_ast_decl_t *proc, vitte_ast_node_t *param) {
    return proc != NULL &&
        proc->kind == VITTE_AST_NODE_PROC_DECL &&
        param != NULL &&
        param->kind == VITTE_AST_NODE_PARAM_DECL &&
        vitte_ast_list_append(&proc->as.proc_decl.parameters, param);
}

bool vitte_ast_block_add_stmt(vitte_ast_stmt_t *block, vitte_ast_stmt_t *stmt) {
    return block != NULL &&
        block->kind == VITTE_AST_NODE_BLOCK_STMT &&
        stmt != NULL &&
        vitte_ast_list_append(&block->as.block_stmt.statements, stmt);
}

bool vitte_ast_call_add_arg(vitte_ast_expr_t *call, vitte_ast_expr_t *argument) {
    return call != NULL &&
        call->kind == VITTE_AST_NODE_CALL_EXPR &&
        argument != NULL &&
        vitte_ast_list_append(&call->as.call_expr.arguments, argument);
}

static bool vitte_ast_list_is_coherent(const vitte_ast_list_t *list) {
    const vitte_ast_node_t *node;
    const vitte_ast_node_t *last = NULL;
    size_t count = 0u;

    if (list == NULL) {
        return false;
    }
    if (list->count == 0u) {
        return list->first == NULL && list->last == NULL;
    }
    if (list->first == NULL || list->last == NULL) {
        return false;
    }

    for (node = list->first; node != NULL; node = node->next) {
        count++;
        last = node;
        if (count > list->count) {
            return false;
        }
    }

    return count == list->count && last == list->last;
}

static vitte_status_t vitte_ast_validate_list(
    vitte_ast_t *ast,
    const vitte_ast_list_t *list,
    const char *code,
    const char *message
) {
    if (!vitte_ast_list_is_coherent(list)) {
        vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, code, message, NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_ast_validate_node(vitte_ast_t *ast, const vitte_ast_node_t *node, size_t depth) {
    const vitte_ast_node_t *child;
    vitte_status_t status;

    if (node == NULL) {
        vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_NULL", "null AST node", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (depth > VITTE_AST_DEFAULT_MAX_DEPTH) {
        vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_DEPTH", "AST validation depth exceeded", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_ast_node_kind_is_valid(node->kind)) {
        vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_KIND", "invalid AST node kind", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    switch (node->kind) {
        case VITTE_AST_NODE_MODULE:
            status = vitte_ast_validate_list(ast, &node->as.module.imports, "VITTE_AST_E_LIST", "module import list is incoherent");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_ast_validate_list(ast, &node->as.module.exports, "VITTE_AST_E_LIST", "module export list is incoherent");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_ast_validate_list(ast, &node->as.module.declarations, "VITTE_AST_E_LIST", "module declaration list is incoherent");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            for (child = node->as.module.imports.first; child != NULL; child = child->next) {
                status = vitte_ast_validate_node(ast, child, depth + 1u);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            for (child = node->as.module.exports.first; child != NULL; child = child->next) {
                status = vitte_ast_validate_node(ast, child, depth + 1u);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            for (child = node->as.module.declarations.first; child != NULL; child = child->next) {
                status = vitte_ast_validate_node(ast, child, depth + 1u);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            break;
        case VITTE_AST_NODE_IMPORT_DECL:
            if (node->as.import_decl.path == NULL ||
                node->as.import_decl.import_kind < VITTE_AST_IMPORT_MODULE ||
                node->as.import_decl.import_kind > VITTE_AST_IMPORT_GLOB) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_IMPORT", "import declaration requires a path", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            break;
        case VITTE_AST_NODE_EXPORT_DECL:
            if (node->as.export_decl.local_name == NULL || node->as.export_decl.export_name == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_EXPORT", "export declaration requires local and visible names", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            break;
        case VITTE_AST_NODE_PROC_DECL:
            if (node->as.proc_decl.name == NULL || node->as.proc_decl.body == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_PROC", "procedure declaration requires name and body", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            status = vitte_ast_validate_list(ast, &node->as.proc_decl.parameters, "VITTE_AST_E_LIST", "procedure parameter list is incoherent");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            for (child = node->as.proc_decl.parameters.first; child != NULL; child = child->next) {
                status = vitte_ast_validate_node(ast, child, depth + 1u);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            if (node->as.proc_decl.return_type != NULL) {
                status = vitte_ast_validate_node(ast, node->as.proc_decl.return_type, depth + 1u);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            return vitte_ast_validate_node(ast, node->as.proc_decl.body, depth + 1u);
        case VITTE_AST_NODE_PARAM_DECL:
            if (node->as.param_decl.name == NULL || node->as.param_decl.type == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_PARAM", "parameter requires name and type", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            return vitte_ast_validate_node(ast, node->as.param_decl.type, depth + 1u);
        case VITTE_AST_NODE_CONST_DECL:
            if (node->as.const_decl.name == NULL || node->as.const_decl.value == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_CONST", "const declaration requires name and value", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            if (node->as.const_decl.type != NULL) {
                status = vitte_ast_validate_node(ast, node->as.const_decl.type, depth + 1u);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            return vitte_ast_validate_node(ast, node->as.const_decl.value, depth + 1u);
        case VITTE_AST_NODE_PICK_DECL:
            if (node->as.pick_decl.name == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_PICK", "pick declaration requires a name", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            status = vitte_ast_validate_list(ast, &node->as.pick_decl.variants, "VITTE_AST_E_LIST", "pick variant list is incoherent");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            for (child = node->as.pick_decl.variants.first; child != NULL; child = child->next) {
                status = vitte_ast_validate_node(ast, child, depth + 1u);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            return VITTE_STATUS_OK;
        case VITTE_AST_NODE_FORM_DECL:
            if (node->as.form_decl.name == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_FORM", "form declaration requires a name", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            status = vitte_ast_validate_list(ast, &node->as.form_decl.fields, "VITTE_AST_E_LIST", "form field list is incoherent");
            if (status != VITTE_STATUS_OK) return status;
            for (child = node->as.form_decl.fields.first; child != NULL; child = child->next) {
                status = vitte_ast_validate_node(ast, child, depth + 1u);
                if (status != VITTE_STATUS_OK) return status;
            }
            return VITTE_STATUS_OK;
        case VITTE_AST_NODE_FORM_FIELD:
            if (node->as.form_field.name == NULL || node->as.form_field.type == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_FORM", "form field requires name and type", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            return vitte_ast_validate_node(ast, node->as.form_field.type, depth + 1u);
        case VITTE_AST_NODE_CAST_EXPR:
            if (node->as.cast_expr.value == NULL || node->as.cast_expr.type == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_CAST", "cast expression requires value and type", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            status = vitte_ast_validate_node(ast, node->as.cast_expr.value, depth + 1u);
            if (status != VITTE_STATUS_OK) return status;
            return vitte_ast_validate_node(ast, node->as.cast_expr.type, depth + 1u);
        case VITTE_AST_NODE_INDEX_EXPR:
            if (node->as.index_expr.base == NULL || node->as.index_expr.index == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_INDEX", "index expression requires base and index", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            status = vitte_ast_validate_node(ast, node->as.index_expr.base, depth + 1u);
            if (status != VITTE_STATUS_OK) return status;
            return vitte_ast_validate_node(ast, node->as.index_expr.index, depth + 1u);
        case VITTE_AST_NODE_PICK_VARIANT:
            if (node->as.pick_variant.name == NULL || node->as.pick_variant.name[0] == '\0') {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_PICK", "pick variant requires a name", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            return VITTE_STATUS_OK;
        case VITTE_AST_NODE_BLOCK_STMT:
            status = vitte_ast_validate_list(ast, &node->as.block_stmt.statements, "VITTE_AST_E_LIST", "block statement list is incoherent");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            for (child = node->as.block_stmt.statements.first; child != NULL; child = child->next) {
                status = vitte_ast_validate_node(ast, child, depth + 1u);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            break;
        case VITTE_AST_NODE_GIVE_STMT:
            return node->as.give_stmt.value != NULL
                ? vitte_ast_validate_node(ast, node->as.give_stmt.value, depth + 1u)
                : VITTE_STATUS_OK;
        case VITTE_AST_NODE_LET_STMT:
            if (node->as.let_stmt.name == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_LET", "let statement requires name", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            if (node->as.let_stmt.type != NULL) {
                status = vitte_ast_validate_node(ast, node->as.let_stmt.type, depth + 1u);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            if (node->as.let_stmt.value != NULL) {
                status = vitte_ast_validate_node(ast, node->as.let_stmt.value, depth + 1u);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            break;
        case VITTE_AST_NODE_ASSIGN_STMT:
            if (node->as.assign_stmt.target == NULL || node->as.assign_stmt.value == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_ASSIGN", "assignment requires target and value", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            status = vitte_ast_validate_node(ast, node->as.assign_stmt.target, depth + 1u);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_ast_validate_node(ast, node->as.assign_stmt.value, depth + 1u);
        case VITTE_AST_NODE_EXPR_STMT:
            if (node->as.expr_stmt.value == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_EXPR_STMT", "expression statement requires value", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            return vitte_ast_validate_node(ast, node->as.expr_stmt.value, depth + 1u);
        case VITTE_AST_NODE_IF_STMT:
            if (node->as.if_stmt.condition == NULL || node->as.if_stmt.then_branch == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_IF", "if statement requires condition and then branch", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            status = vitte_ast_validate_node(ast, node->as.if_stmt.condition, depth + 1u);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_ast_validate_node(ast, node->as.if_stmt.then_branch, depth + 1u);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            if (node->as.if_stmt.else_branch != NULL) {
                status = vitte_ast_validate_node(ast, node->as.if_stmt.else_branch, depth + 1u);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            break;
        case VITTE_AST_NODE_BINARY_EXPR:
            if (node->as.binary_expr.left == NULL || node->as.binary_expr.right == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_BINARY", "binary expression requires operands", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            status = vitte_ast_validate_node(ast, node->as.binary_expr.left, depth + 1u);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            return vitte_ast_validate_node(ast, node->as.binary_expr.right, depth + 1u);
        case VITTE_AST_NODE_CALL_EXPR:
            if (node->as.call_expr.callee == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_CALL", "call expression requires callee", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            status = vitte_ast_validate_list(ast, &node->as.call_expr.arguments, "VITTE_AST_E_LIST", "call argument list is incoherent");
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_ast_validate_node(ast, node->as.call_expr.callee, depth + 1u);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            for (child = node->as.call_expr.arguments.first; child != NULL; child = child->next) {
                status = vitte_ast_validate_node(ast, child, depth + 1u);
                if (status != VITTE_STATUS_OK) {
                    return status;
                }
            }
            break;
        case VITTE_AST_NODE_IDENTIFIER:
            if (node->as.identifier.name == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_IDENT", "identifier requires name", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            break;
        case VITTE_AST_NODE_TYPE_NAME:
            if (node->as.type_name.name == NULL) {
                vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_AST_E_TYPE", "type name requires name", NULL);
                return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            }
            break;
        default:
            break;
    }

    return VITTE_STATUS_OK;
}

vitte_status_t vitte_ast_validate(vitte_ast_t *ast) {
    if (!vitte_ast_is_initialized(ast)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (ast->root == NULL) {
        vitte_ast_set_error(ast, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_AST_E_ROOT", "AST root module is missing", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    return vitte_ast_validate_node(ast, ast->root, 0u);
}

static bool vitte_ast_visit_child(
    vitte_ast_node_t *node,
    vitte_ast_visit_fn callback,
    void *user,
    size_t depth,
    size_t max_depth,
    size_t *count
);

static bool vitte_ast_visit_children(
    vitte_ast_list_t *list,
    vitte_ast_visit_fn callback,
    void *user,
    size_t depth,
    size_t max_depth,
    size_t *count
) {
    vitte_ast_node_t *child;

    if (list == NULL) {
        return true;
    }

    for (child = list->first; child != NULL; child = child->next) {
        if (!vitte_ast_visit_child(child, callback, user, depth, max_depth, count)) {
            return false;
        }
    }

    return true;
}

static bool vitte_ast_visit_child(
    vitte_ast_node_t *node,
    vitte_ast_visit_fn callback,
    void *user,
    size_t depth,
    size_t max_depth,
    size_t *count
) {
    if (node == NULL) {
        return true;
    }
    if (callback == NULL || count == NULL || depth > max_depth) {
        return false;
    }
    if (!callback(node, user)) {
        return false;
    }

    (*count)++;
    switch (node->kind) {
        case VITTE_AST_NODE_MODULE:
            return vitte_ast_visit_children(&node->as.module.imports, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_children(&node->as.module.exports, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_children(&node->as.module.declarations, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_IMPORT_DECL:
        case VITTE_AST_NODE_EXPORT_DECL:
            return true;
        case VITTE_AST_NODE_PROC_DECL:
            return vitte_ast_visit_children(&node->as.proc_decl.parameters, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_child(node->as.proc_decl.return_type, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_child(node->as.proc_decl.body, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_PARAM_DECL:
            return vitte_ast_visit_child(node->as.param_decl.type, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_CONST_DECL:
            return vitte_ast_visit_child(node->as.const_decl.type, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_child(node->as.const_decl.value, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_PICK_DECL:
            return vitte_ast_visit_children(&node->as.pick_decl.variants, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_PICK_VARIANT:
            return true;
        case VITTE_AST_NODE_FORM_DECL:
            return vitte_ast_visit_children(&node->as.form_decl.fields, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_FORM_FIELD:
            return vitte_ast_visit_child(node->as.form_field.type, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_CAST_EXPR:
            return vitte_ast_visit_child(node->as.cast_expr.value, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_child(node->as.cast_expr.type, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_INDEX_EXPR:
            return vitte_ast_visit_child(node->as.index_expr.base, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_child(node->as.index_expr.index, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_BLOCK_STMT:
            return vitte_ast_visit_children(&node->as.block_stmt.statements, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_GIVE_STMT:
            return vitte_ast_visit_child(node->as.give_stmt.value, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_LET_STMT:
            return vitte_ast_visit_child(node->as.let_stmt.type, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_child(node->as.let_stmt.value, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_ASSIGN_STMT:
            return vitte_ast_visit_child(node->as.assign_stmt.target, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_child(node->as.assign_stmt.value, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_EXPR_STMT:
            return vitte_ast_visit_child(node->as.expr_stmt.value, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_IF_STMT:
            return vitte_ast_visit_child(node->as.if_stmt.condition, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_child(node->as.if_stmt.then_branch, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_child(node->as.if_stmt.else_branch, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_BINARY_EXPR:
            return vitte_ast_visit_child(node->as.binary_expr.left, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_child(node->as.binary_expr.right, callback, user, depth + 1u, max_depth, count);
        case VITTE_AST_NODE_CALL_EXPR:
            return vitte_ast_visit_child(node->as.call_expr.callee, callback, user, depth + 1u, max_depth, count) &&
                vitte_ast_visit_children(&node->as.call_expr.arguments, callback, user, depth + 1u, max_depth, count);
        default:
            return true;
    }
}

static void vitte_ast_dump_indent(FILE *stream, size_t depth) {
    size_t index;

    for (index = 0u; index < depth; index++) {
        (void)fputs("  ", stream);
    }
}

static void vitte_ast_dump_child(const vitte_ast_node_t *node, FILE *stream, size_t depth, size_t max_depth);

static void vitte_ast_dump_children(const vitte_ast_list_t *list, FILE *stream, size_t depth, size_t max_depth) {
    const vitte_ast_node_t *child;

    if (list == NULL) {
        return;
    }

    for (child = list->first; child != NULL; child = child->next) {
        vitte_ast_dump_child(child, stream, depth, max_depth);
    }
}

static void vitte_ast_dump_child(const vitte_ast_node_t *node, FILE *stream, size_t depth, size_t max_depth) {
    const char *label;

    if (node == NULL || stream == NULL) {
        return;
    }

    vitte_ast_dump_indent(stream, depth);
    (void)fputs(vitte_ast_node_kind_name(node->kind), stream);
    label = vitte_ast_node_label(node);
    if (label != NULL) {
        (void)fprintf(stream, " %s", label);
    } else if (node->kind == VITTE_AST_NODE_INTEGER_LITERAL) {
        (void)fprintf(stream, " %" PRId64, node->as.integer_literal.value);
    }
    (void)fputc('\n', stream);

    if (depth >= max_depth) {
        vitte_ast_dump_indent(stream, depth + 1u);
        (void)fputs("...\n", stream);
        return;
    }

    switch (node->kind) {
        case VITTE_AST_NODE_MODULE:
            vitte_ast_dump_children(&node->as.module.imports, stream, depth + 1u, max_depth);
            vitte_ast_dump_children(&node->as.module.exports, stream, depth + 1u, max_depth);
            vitte_ast_dump_children(&node->as.module.declarations, stream, depth + 1u, max_depth);
            break;
        case VITTE_AST_NODE_IMPORT_DECL:
        case VITTE_AST_NODE_EXPORT_DECL:
            break;
        case VITTE_AST_NODE_PROC_DECL:
            vitte_ast_dump_children(&node->as.proc_decl.parameters, stream, depth + 1u, max_depth);
            vitte_ast_dump_child(node->as.proc_decl.return_type, stream, depth + 1u, max_depth);
            vitte_ast_dump_child(node->as.proc_decl.body, stream, depth + 1u, max_depth);
            break;
        case VITTE_AST_NODE_PARAM_DECL:
            vitte_ast_dump_child(node->as.param_decl.type, stream, depth + 1u, max_depth);
            break;
        case VITTE_AST_NODE_CONST_DECL:
            vitte_ast_dump_child(node->as.const_decl.type, stream, depth + 1u, max_depth);
            vitte_ast_dump_child(node->as.const_decl.value, stream, depth + 1u, max_depth);
            break;
        case VITTE_AST_NODE_BLOCK_STMT:
            vitte_ast_dump_children(&node->as.block_stmt.statements, stream, depth + 1u, max_depth);
            break;
        case VITTE_AST_NODE_GIVE_STMT:
            vitte_ast_dump_child(node->as.give_stmt.value, stream, depth + 1u, max_depth);
            break;
        case VITTE_AST_NODE_LET_STMT:
            vitte_ast_dump_child(node->as.let_stmt.type, stream, depth + 1u, max_depth);
            vitte_ast_dump_child(node->as.let_stmt.value, stream, depth + 1u, max_depth);
            break;
        case VITTE_AST_NODE_ASSIGN_STMT:
            vitte_ast_dump_child(node->as.assign_stmt.target, stream, depth + 1u, max_depth);
            vitte_ast_dump_child(node->as.assign_stmt.value, stream, depth + 1u, max_depth);
            break;
        case VITTE_AST_NODE_EXPR_STMT:
            vitte_ast_dump_child(node->as.expr_stmt.value, stream, depth + 1u, max_depth);
            break;
        case VITTE_AST_NODE_IF_STMT:
            vitte_ast_dump_child(node->as.if_stmt.condition, stream, depth + 1u, max_depth);
            vitte_ast_dump_child(node->as.if_stmt.then_branch, stream, depth + 1u, max_depth);
            vitte_ast_dump_child(node->as.if_stmt.else_branch, stream, depth + 1u, max_depth);
            break;
        case VITTE_AST_NODE_BINARY_EXPR:
            vitte_ast_dump_child(node->as.binary_expr.left, stream, depth + 1u, max_depth);
            vitte_ast_dump_child(node->as.binary_expr.right, stream, depth + 1u, max_depth);
            break;
        case VITTE_AST_NODE_CALL_EXPR:
            vitte_ast_dump_child(node->as.call_expr.callee, stream, depth + 1u, max_depth);
            vitte_ast_dump_children(&node->as.call_expr.arguments, stream, depth + 1u, max_depth);
            break;
        default:
            break;
    }
}

void vitte_ast_dump(const vitte_ast_node_t *node, FILE *stream, size_t max_depth) {
    if (max_depth == 0u) {
        max_depth = VITTE_AST_DEFAULT_MAX_DEPTH;
    }
    vitte_ast_dump_child(node, stream, 0u, max_depth);
}

static size_t vitte_ast_visit_node(vitte_ast_node_t *node, vitte_ast_visit_fn callback, void *user, size_t max_depth) {
    size_t count = 0u;

    (void)vitte_ast_visit_child(node, callback, user, 0u, max_depth, &count);
    return count;
}

size_t vitte_ast_visit(vitte_ast_node_t *node, vitte_ast_visit_fn callback, void *user, size_t max_depth) {
    if (max_depth == 0u) {
        max_depth = VITTE_AST_DEFAULT_MAX_DEPTH;
    }
    return vitte_ast_visit_node(node, callback, user, max_depth);
}
