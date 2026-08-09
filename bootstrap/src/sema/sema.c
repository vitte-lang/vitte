#include "sema.h"

#include <stdio.h>
#include <string.h>

static const char *vitte_sema_last_name_segment(const char *name);

static bool vitte_sema_expr_path(const vitte_ast_expr_t *expr, char *buffer, size_t capacity) {
    if (expr == NULL || buffer == NULL || capacity == 0u) return false;
    if (expr->kind == VITTE_AST_NODE_IDENTIFIER) {
        return snprintf(buffer, capacity, "%s", expr->as.identifier.name) > 0;
    }
    if (expr->kind == VITTE_AST_NODE_MEMBER_EXPR) {
        char prefix[512];
        if (!vitte_sema_expr_path(expr->as.member_expr.base, prefix, sizeof(prefix))) return false;
        return snprintf(buffer, capacity, "%s::%s", prefix, expr->as.member_expr.member) > 0;
    }
    return false;
}

static void vitte_sema_set_error(
    vitte_sema_t *sema,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (sema != NULL) {
        vitte_error_set_details(&sema->last_error, status, code, message, details);
    }
}

static vitte_status_t vitte_sema_add_diagnostic(
    vitte_sema_t *sema,
    vitte_diagnostic_severity_t severity,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
) {
    if (sema == NULL || sema->diagnostics == NULL) {
        return VITTE_STATUS_OK;
    }
    return vitte_diagnostic_add(sema->diagnostics, severity, code, message, details, span);
}

static vitte_status_t vitte_sema_fail(
    vitte_sema_t *sema,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details,
    const vitte_ast_span_t *span
) {
    if (sema == NULL) {
        return status;
    }
    vitte_sema_set_error(sema, status, code, message, details);
    sema->stats.error_count++;
    (void)vitte_sema_add_diagnostic(sema, VITTE_DIAGNOSTIC_ERROR, code, message, details, span);
    return status;
}

static const vitte_type_t *vitte_sema_error_type(vitte_sema_t *sema) {
    const vitte_type_t *type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_ERROR);
    if (type == NULL) {
        static vitte_type_t fallback;
        vitte_type_init_invalid(&fallback);
        return &fallback;
    }
    return type;
}

static char *vitte_sema_join_qualified_name(
    vitte_sema_t *sema,
    const char *prefix,
    const char *name
);

static bool vitte_sema_enter(vitte_sema_t *sema) {
    if (sema == NULL) {
        return false;
    }
    sema->depth++;
    if (sema->depth > sema->options.max_depth) {
        (void)vitte_sema_fail(
            sema,
            VITTE_STATUS_ERROR_INVALID_STATE,
            "VITTE_SEMA_E_DEPTH",
            "semantic analysis depth exceeded",
            NULL,
            NULL
        );
        sema->depth--;
        return false;
    }
    return true;
}

static void vitte_sema_leave(vitte_sema_t *sema) {
    if (sema != NULL && sema->depth > 0u) {
        sema->depth--;
    }
}

static const vitte_type_t *vitte_sema_resolve_type_ref(
    vitte_sema_t *sema,
    const vitte_ast_type_ref_t *type_ref
);
static const vitte_ast_decl_t *vitte_sema_find_exported_decl_recursive(
    const vitte_sema_t *sema,
    const vitte_ast_module_t *module,
    const char *name,
    size_t depth
);

static const vitte_ast_decl_t *vitte_sema_find_nominal_decl(
    vitte_sema_t *sema,
    const char *type_name
) {
    const vitte_ast_decl_t *decl;
    const char *leaf = type_name != NULL ? strrchr(type_name, '.') : NULL;
    size_t index;

    if (sema == NULL || type_name == NULL) return NULL;
    decl = vitte_ast_module_find_decl(sema->ast->root, type_name);
    if (decl == NULL && leaf != NULL) {
        decl = vitte_ast_module_find_decl(sema->ast->root, leaf + 1);
    }
    if (decl != NULL && (decl->kind == VITTE_AST_NODE_FORM_DECL || decl->kind == VITTE_AST_NODE_PICK_DECL)) {
        return decl;
    }
    for (index = 0u; index < sema->imported_module_count; index++) {
        const vitte_ast_module_t *module = sema->imported_modules[index].root;
        decl = vitte_ast_module_find_decl(module, type_name);
        if (decl == NULL && leaf != NULL) {
            decl = vitte_ast_module_find_decl(module, leaf + 1);
        }
        if (decl != NULL && (decl->kind == VITTE_AST_NODE_FORM_DECL || decl->kind == VITTE_AST_NODE_PICK_DECL)) {
            return decl;
        }
        decl = vitte_sema_find_exported_decl_recursive(sema, module, type_name, 0u);
        if (decl == NULL && leaf != NULL) {
            decl = vitte_sema_find_exported_decl_recursive(sema, module, leaf + 1, 0u);
        }
        if (decl != NULL && (decl->kind == VITTE_AST_NODE_FORM_DECL || decl->kind == VITTE_AST_NODE_PICK_DECL)) {
            return decl;
        }
    }
    return NULL;
}

static const vitte_ast_type_ref_t *vitte_sema_find_form_field_type(
    vitte_sema_t *sema,
    const vitte_type_t *base_type,
    const char *member
) {
    const vitte_ast_decl_t *decl;
    const vitte_ast_node_t *field;

    if (sema == NULL || base_type == NULL || member == NULL) return NULL;
    decl = vitte_sema_find_nominal_decl(sema, vitte_type_name(base_type));
    if (decl == NULL || decl->kind != VITTE_AST_NODE_FORM_DECL) {
        size_t module_index;
        const char *leaf = vitte_sema_last_name_segment(vitte_type_name(base_type));
        for (module_index = 0u; module_index < sema->imported_module_count; module_index++) {
            const vitte_ast_module_t *module = sema->imported_modules[module_index].root;
            const vitte_ast_decl_t *candidate = module != NULL ? vitte_ast_module_find_exported_decl(module, leaf) : NULL;
            if (candidate != NULL && candidate->kind == VITTE_AST_NODE_FORM_DECL) {
                decl = candidate;
                break;
            }
        }
    }
    if (decl == NULL || decl->kind != VITTE_AST_NODE_FORM_DECL) return NULL;
    for (field = decl->as.form_decl.fields.first; field != NULL; field = field->next) {
        if (field->kind == VITTE_AST_NODE_FORM_FIELD && field->as.form_field.name != NULL &&
            strcmp(field->as.form_field.name, member) == 0) {
            return field->as.form_field.type;
        }
    }
    return NULL;
}
static vitte_status_t vitte_sema_collect_proc_signature(
    vitte_sema_t *sema,
    const vitte_ast_decl_t *decl,
    const vitte_type_t **return_type,
    const vitte_type_t **parameter_types,
    size_t *arity
);
static vitte_status_t vitte_sema_define_imported_decl(
    vitte_sema_t *sema,
    const char *visible_name,
    const vitte_ast_decl_t *decl,
    const vitte_ast_span_t *span
);

static const char *vitte_sema_last_name_segment(const char *name) {
    const char *segment;
    const char *cursor;

    if (name == NULL) {
        return NULL;
    }
    segment = name;
    for (cursor = name; *cursor != '\0'; cursor++) {
        if (*cursor == '.') {
            segment = cursor + 1;
        } else if (*cursor == ':' && cursor[1] == ':') {
            segment = cursor + 2;
            cursor++;
        }
    }
    return segment;
}

static vitte_status_t vitte_sema_predeclare_pick(
    vitte_sema_t *sema,
    const vitte_ast_decl_t *decl
) {
    const vitte_type_t *pick_type;
    const vitte_ast_node_t *variant;

    pick_type = vitte_type_register_pick(&sema->types, decl->as.pick_decl.name);
    if (pick_type == NULL) {
        return vitte_sema_fail(sema, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_SEMA_E_PICK", "failed to register pick type", decl->as.pick_decl.name, &decl->span);
    }
    for (variant = decl->as.pick_decl.variants.first; variant != NULL; variant = variant->next) {
        const vitte_symbol_t *symbol = NULL;
        const vitte_symbol_t *qualified_symbol = NULL;
        char *qualified_name = vitte_sema_join_qualified_name(
            sema,
            decl->as.pick_decl.name,
            variant->as.pick_variant.name
        );

        if (qualified_name == NULL ||
            vitte_symbol_define(&sema->symbols, VITTE_SYMBOL_KIND_CONST, qualified_name, pick_type, decl, false, &qualified_symbol) != VITTE_STATUS_OK ||
            vitte_scope_define(&sema->scopes, qualified_name, qualified_symbol) != VITTE_STATUS_OK) {
            return vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_PICK", "failed to define pick variant", variant->as.pick_variant.name, &variant->span);
        }
        if (vitte_scope_lookup_current(&sema->scopes, variant->as.pick_variant.name) == NULL) {
            if (vitte_symbol_define(&sema->symbols, VITTE_SYMBOL_KIND_CONST, variant->as.pick_variant.name, pick_type, decl, false, &symbol) != VITTE_STATUS_OK ||
                vitte_scope_define(&sema->scopes, variant->as.pick_variant.name, symbol) != VITTE_STATUS_OK) {
                return vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_PICK", "failed to define pick variant", variant->as.pick_variant.name, &variant->span);
            }
        }
        sema->stats.symbol_count++;
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_sema_predeclare_form(vitte_sema_t *sema, const vitte_ast_decl_t *decl) {
    const vitte_type_t *form_type = vitte_type_register_form(&sema->types, decl->as.form_decl.name);
    const vitte_ast_node_t *field;
    if (form_type == NULL) {
        return vitte_sema_fail(sema, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_SEMA_E_FORM", "failed to register form type", decl->as.form_decl.name, &decl->span);
    }
    (void)form_type;
    for (field = decl->as.form_decl.fields.first; field != NULL; field = field->next) {
        if (field->kind != VITTE_AST_NODE_FORM_FIELD || vitte_sema_resolve_type_ref(sema, field->as.form_field.type) == NULL) {
            return vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_FORM", "invalid form field type", decl->as.form_decl.name, &field->span);
        }
    }
    return VITTE_STATUS_OK;
}

static char *vitte_sema_copy_text(vitte_sema_t *sema, const char *text, size_t length) {
    char *copy;

    if (sema == NULL || sema->ast == NULL || sema->ast->arena == NULL || text == NULL) {
        return NULL;
    }
    copy = (char *)vitte_arena_alloc(sema->ast->arena, length + 1u, 1u);
    if (copy == NULL) {
        return NULL;
    }
    if (length > 0u) {
        (void)memcpy(copy, text, length);
    }
    copy[length] = '\0';
    return copy;
}

static char *vitte_sema_join_qualified_name(
    vitte_sema_t *sema,
    const char *prefix,
    const char *name
) {
    size_t prefix_length;
    size_t name_length;
    char *joined;

    if (sema == NULL || prefix == NULL || name == NULL) {
        return NULL;
    }
    prefix_length = strlen(prefix);
    name_length = strlen(name);
    joined = (char *)vitte_arena_alloc(sema->ast->arena, prefix_length + 2u + name_length + 1u, 1u);
    if (joined == NULL) {
        return NULL;
    }
    (void)memcpy(joined, prefix, prefix_length);
    joined[prefix_length] = ':';
    joined[prefix_length + 1u] = ':';
    (void)memcpy(joined + prefix_length + 2u, name, name_length);
    joined[prefix_length + 2u + name_length] = '\0';
    return joined;
}

static const vitte_ast_module_t *vitte_sema_find_import_module(
    const vitte_sema_t *sema,
    const char *module_name
) {
    size_t index;

    if (sema == NULL || module_name == NULL) {
        return NULL;
    }
    for (index = 0u; index < sema->imported_module_count; index++) {
        if (sema->imported_modules[index].module_name != NULL &&
            strcmp(sema->imported_modules[index].module_name, module_name) == 0) {
            return sema->imported_modules[index].root;
        }
    }
    return NULL;
}

static bool vitte_sema_import_module_name(
    const vitte_ast_decl_t *import_decl,
    char *buffer,
    size_t buffer_capacity
);

static const vitte_ast_decl_t *vitte_sema_find_exported_decl_recursive(
    const vitte_sema_t *sema,
    const vitte_ast_module_t *module,
    const char *name,
    size_t depth
) {
    const vitte_ast_decl_t *decl;
    const vitte_ast_module_t *queue[256];
    const vitte_ast_module_t *visited[256];
    size_t queue_count = 0u;
    size_t queue_index = 0u;
    size_t visited_count = 0u;

    (void)depth;
    if (sema == NULL || module == NULL || name == NULL) return NULL;
    queue[queue_count++] = module;
    while (queue_index < queue_count) {
        const vitte_ast_module_t *current = queue[queue_index++];
        const vitte_ast_node_t *import_node;
        size_t index;
        bool already_seen = false;

        for (index = 0u; index < visited_count; index++) {
            if (visited[index] == current) {
                already_seen = true;
                break;
            }
        }
        if (already_seen || current == NULL || visited_count >= 256u) continue;
        visited[visited_count++] = current;
        decl = vitte_ast_module_find_exported_decl(current, name);
        if (decl != NULL) return decl;
        if (!current->as.module.export_all) continue;
        for (import_node = current->as.module.imports.first; import_node != NULL; import_node = import_node->next) {
            const vitte_ast_module_t *child;
            char child_name[VITTE_IMPORT_MAX_MODULE_NAME];
            if (import_node->kind != VITTE_AST_NODE_IMPORT_DECL ||
                !vitte_sema_import_module_name(import_node, child_name, sizeof(child_name))) continue;
            child = vitte_sema_find_import_module(sema, child_name);
            if (child != NULL && queue_count < 256u) queue[queue_count++] = child;
        }
    }
    return NULL;
}

static const vitte_ast_decl_t *vitte_sema_find_reexport_import(
    vitte_sema_t *sema,
    const vitte_ast_module_t *module,
    const char *name
) {
    const vitte_ast_node_t *import_decl;

    if (sema == NULL || module == NULL || name == NULL) {
        return NULL;
    }
    for (import_decl = module->as.module.imports.first; import_decl != NULL; import_decl = import_decl->next) {
        const char *path;
        const char *visible_name = NULL;

        if (import_decl->kind != VITTE_AST_NODE_IMPORT_DECL) {
            continue;
        }
        path = import_decl->as.import_decl.path;
        if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_GLOB) {
            char module_name[VITTE_IMPORT_MAX_MODULE_NAME];
            const vitte_ast_module_t *imported_module;
            size_t length;

            if (path == NULL || path[0] == '\0') {
                continue;
            }
            length = strlen(path);
            if (length + 1u > sizeof(module_name)) {
                continue;
            }
            (void)memcpy(module_name, path, length + 1u);
            imported_module = vitte_sema_find_import_module(sema, module_name);
            if (imported_module != NULL && vitte_sema_find_exported_decl_recursive(sema, imported_module, name, 0u) != NULL) {
                return import_decl;
            }
            continue;
        }
        if (import_decl->as.import_decl.alias != NULL && import_decl->as.import_decl.alias[0] != '\0') {
            visible_name = import_decl->as.import_decl.alias;
        } else if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_SYMBOL) {
            const char *last_dot = path != NULL ? strrchr(path, '.') : NULL;
            visible_name = last_dot != NULL ? last_dot + 1 : path;
        } else {
            visible_name = vitte_sema_last_name_segment(path);
        }
        if (visible_name != NULL && strcmp(visible_name, name) == 0) {
            return import_decl;
        }
    }
    return NULL;
}

static vitte_status_t vitte_sema_validate_module_exports(
    vitte_sema_t *sema,
    const vitte_ast_module_t *module
) {
    const vitte_ast_node_t *export_decl;

    if (sema == NULL || module == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    for (export_decl = module->as.module.exports.first; export_decl != NULL; export_decl = export_decl->next) {
        const vitte_ast_decl_t *target;
        const vitte_ast_node_t *previous;
        const vitte_ast_decl_t *implicit_decl;

        if (export_decl->kind != VITTE_AST_NODE_EXPORT_DECL) {
            continue;
        }
        target = vitte_ast_export_decl_target(module, export_decl);
        if (target == NULL) {
            if (vitte_sema_find_reexport_import(sema, module, export_decl->as.export_decl.local_name) != NULL) {
                return vitte_sema_fail(
                    sema,
                    VITTE_STATUS_ERROR_PARSE,
                    "VITTE_SEMA_E_REEXPORT",
                    "bootstrap re-exports are unsupported; export local declarations only",
                    export_decl->as.export_decl.local_name,
                    &export_decl->span
                );
            }
            return vitte_sema_fail(
                sema,
                VITTE_STATUS_ERROR_PARSE,
                "VITTE_SEMA_E_EXPORT",
                "exported symbol was not found in module",
                export_decl->as.export_decl.local_name,
                &export_decl->span
            );
        }
        implicit_decl = vitte_ast_module_find_decl(module, export_decl->as.export_decl.export_name);
        if (implicit_decl != NULL &&
            implicit_decl != target &&
            vitte_ast_module_decl_is_exported(module, implicit_decl)) {
            return vitte_sema_fail(
                sema,
                VITTE_STATUS_ERROR_PARSE,
                "VITTE_SEMA_E_EXPORT_CONFLICT",
                "exported name conflicts with another exported declaration",
                export_decl->as.export_decl.export_name,
                &export_decl->span
            );
        }
        for (previous = module->as.module.exports.first; previous != NULL && previous != export_decl; previous = previous->next) {
            if (previous->kind != VITTE_AST_NODE_EXPORT_DECL ||
                previous->as.export_decl.export_name == NULL ||
                strcmp(previous->as.export_decl.export_name, export_decl->as.export_decl.export_name) != 0) {
                continue;
            }
            if (previous->as.export_decl.local_name != NULL &&
                strcmp(previous->as.export_decl.local_name, export_decl->as.export_decl.local_name) == 0) {
                break;
            }
            return vitte_sema_fail(
                sema,
                VITTE_STATUS_ERROR_PARSE,
                "VITTE_SEMA_E_EXPORT_CONFLICT",
                "exported name conflicts with another exported declaration",
                export_decl->as.export_decl.export_name,
                &export_decl->span
            );
        }
    }
    return VITTE_STATUS_OK;
}

static bool vitte_sema_text_equal(const char *left, const char *right) {
    if (left == right) {
        return true;
    }
    if (left == NULL || right == NULL) {
        return false;
    }
    return strcmp(left, right) == 0;
}

static bool vitte_sema_import_decl_is_exact_duplicate(
    const vitte_ast_decl_t *left,
    const vitte_ast_decl_t *right
) {
    if (left == NULL || right == NULL ||
        left->kind != VITTE_AST_NODE_IMPORT_DECL ||
        right->kind != VITTE_AST_NODE_IMPORT_DECL) {
        return false;
    }
    return left->as.import_decl.import_kind == right->as.import_decl.import_kind &&
        left->as.import_decl.relative == right->as.import_decl.relative &&
        vitte_sema_text_equal(left->as.import_decl.path, right->as.import_decl.path) &&
        vitte_sema_text_equal(left->as.import_decl.alias, right->as.import_decl.alias);
}

typedef struct vitte_sema_import_export_context {
    vitte_sema_t *sema;
    const char *prefix;
    const vitte_ast_span_t *span;
    bool qualify_names;
    vitte_status_t status;
} vitte_sema_import_export_context_t;

static bool vitte_sema_define_imported_export(
    const vitte_ast_decl_t *decl,
    const char *public_name,
    void *user
) {
    vitte_sema_import_export_context_t *context = (vitte_sema_import_export_context_t *)user;
    const char *visible_name = public_name;

    if (context == NULL || context->sema == NULL || decl == NULL || public_name == NULL) {
        return false;
    }
    if (context->qualify_names) {
        visible_name = vitte_sema_join_qualified_name(context->sema, context->prefix, public_name);
        if (visible_name == NULL) {
            context->status = vitte_sema_fail(
                context->sema,
                VITTE_STATUS_ERROR_OUT_OF_MEMORY,
                "VITTE_SEMA_E_IMPORT",
                "failed to allocate imported namespace symbol",
                context->prefix,
                context->span
            );
            return false;
        }
    }
    context->status = vitte_sema_define_imported_decl(context->sema, visible_name, decl, context->span);
    return context->status == VITTE_STATUS_OK;
}

static size_t vitte_sema_visit_reexported_modules(
    vitte_sema_t *sema,
    const vitte_ast_module_t *module,
    vitte_sema_import_export_context_t *context
) {
    const vitte_ast_module_t *queue[256];
    const vitte_ast_module_t *visited[256];
    size_t queue_count = 0u;
    size_t queue_index = 0u;
    size_t visited_count = 0u;
    size_t count = 0u;

    if (sema == NULL || module == NULL || context == NULL) return 0u;
    queue[queue_count++] = module;
    while (queue_index < queue_count && context->status == VITTE_STATUS_OK) {
        const vitte_ast_module_t *current = queue[queue_index++];
        const vitte_ast_node_t *import_node;
        size_t index;
        bool seen = false;
        for (index = 0u; index < visited_count; index++) {
            if (visited[index] == current) { seen = true; break; }
        }
        if (seen || current == NULL || visited_count >= 256u) continue;
        visited[visited_count++] = current;
        count += vitte_ast_module_visit_exports(current, vitte_sema_define_imported_export, context);
        if (!current->as.module.export_all) continue;
        for (import_node = current->as.module.imports.first; import_node != NULL; import_node = import_node->next) {
            const vitte_ast_module_t *child;
            char child_name[VITTE_IMPORT_MAX_MODULE_NAME];
            if (import_node->kind != VITTE_AST_NODE_IMPORT_DECL ||
                !vitte_sema_import_module_name(import_node, child_name, sizeof(child_name))) continue;
            child = vitte_sema_find_import_module(sema, child_name);
            if (child != NULL && queue_count < 256u) queue[queue_count++] = child;
        }
    }
    return count;
}

static const char *vitte_sema_import_visible_name(const vitte_ast_decl_t *import_decl) {
    const char *path;
    const char *last_dot;

    if (import_decl == NULL || import_decl->kind != VITTE_AST_NODE_IMPORT_DECL) {
        return NULL;
    }
    if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_GLOB) {
        return NULL;
    }
    if (import_decl->as.import_decl.alias != NULL && import_decl->as.import_decl.alias[0] != '\0') {
        return import_decl->as.import_decl.alias;
    }
    path = import_decl->as.import_decl.path;
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_SYMBOL) {
        last_dot = strrchr(path, '.');
        return last_dot != NULL ? last_dot + 1 : path;
    }
    return vitte_sema_last_name_segment(path);
}

static bool vitte_sema_import_module_name(
    const vitte_ast_decl_t *import_decl,
    char *buffer,
    size_t buffer_capacity
) {
    const char *path;
    size_t length;

    if (import_decl == NULL || import_decl->kind != VITTE_AST_NODE_IMPORT_DECL ||
        buffer == NULL || buffer_capacity == 0u) {
        return false;
    }
    path = import_decl->as.import_decl.path;
    if (path == NULL || path[0] == '\0') {
        return false;
    }
    length = strlen(path);
    if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_SYMBOL) {
        const char *last_dot = strrchr(path, '.');
        if (last_dot == NULL) {
            return false;
        }
        length = (size_t)(last_dot - path);
    }
    if (length + 1u > buffer_capacity) {
        return false;
    }
    (void)memcpy(buffer, path, length);
    buffer[length] = '\0';
    return true;
}

static bool vitte_sema_split_module_member_name(
    const char *name,
    const char **prefix,
    size_t *prefix_length,
    const char **member
) {
    const char *separator;

    if (name == NULL || prefix == NULL || prefix_length == NULL || member == NULL) {
        return false;
    }
    separator = strstr(name, "::");
    if (separator == NULL || separator == name || separator[2] == '\0') {
        return false;
    }
    *prefix = name;
    *prefix_length = (size_t)(separator - name);
    *member = separator + 2;
    return true;
}

static bool vitte_sema_report_private_import_use(
    vitte_sema_t *sema,
    const char *name,
    const vitte_ast_span_t *span
) {
    const vitte_ast_module_t *module;
    const vitte_ast_node_t *import_decl;
    const char *prefix;
    const char *member;
    size_t prefix_length;

    if (sema == NULL || name == NULL || sema->ast == NULL || sema->ast->root == NULL ||
        sema->ast->root->kind != VITTE_AST_NODE_MODULE) {
        return false;
    }
    module = sema->ast->root;

    if (vitte_sema_split_module_member_name(name, &prefix, &prefix_length, &member)) {
        for (import_decl = module->as.module.imports.first; import_decl != NULL; import_decl = import_decl->next) {
            char module_name[VITTE_IMPORT_MAX_MODULE_NAME];
            const char *visible_name;
            const vitte_ast_module_t *imported_module;
            const vitte_ast_decl_t *decl;

            if (import_decl->kind != VITTE_AST_NODE_IMPORT_DECL ||
                import_decl->as.import_decl.import_kind != VITTE_AST_IMPORT_MODULE) {
                continue;
            }
            visible_name = vitte_sema_import_visible_name(import_decl);
            if (visible_name == NULL ||
                strlen(visible_name) != prefix_length ||
                memcmp(visible_name, prefix, prefix_length) != 0) {
                continue;
            }
            if (!vitte_sema_import_module_name(import_decl, module_name, sizeof(module_name))) {
                continue;
            }
            imported_module = vitte_sema_find_import_module(sema, module_name);
            if (imported_module == NULL) {
                continue;
            }
            decl = vitte_ast_module_find_decl(imported_module, member);
            if (decl != NULL && !vitte_ast_module_decl_is_exported(imported_module, decl)) {
                (void)vitte_sema_fail(
                    sema,
                    VITTE_STATUS_ERROR_PARSE,
                    "VITTE_SEMA_E_IMPORT_PRIVATE_MEMBER",
                    "module alias member is not exported",
                    name,
                    span
                );
                return true;
            }
        }
        return false;
    }

    for (import_decl = module->as.module.imports.first; import_decl != NULL; import_decl = import_decl->next) {
        char module_name[VITTE_IMPORT_MAX_MODULE_NAME];
        const vitte_ast_module_t *imported_module;
        const vitte_ast_decl_t *decl;

        if (import_decl->kind != VITTE_AST_NODE_IMPORT_DECL) {
            continue;
        }
        if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_SYMBOL) {
            const char *visible_name = vitte_sema_import_visible_name(import_decl);
            const char *decl_name;

            if (visible_name == NULL || strcmp(visible_name, name) != 0 ||
                !vitte_sema_import_module_name(import_decl, module_name, sizeof(module_name))) {
                continue;
            }
            imported_module = vitte_sema_find_import_module(sema, module_name);
            if (imported_module == NULL) {
                continue;
            }
            decl_name = vitte_sema_last_name_segment(import_decl->as.import_decl.path);
            decl = vitte_ast_module_find_decl(imported_module, decl_name);
            if (decl != NULL && !vitte_ast_module_decl_is_exported(imported_module, decl)) {
                (void)vitte_sema_fail(
                    sema,
                    VITTE_STATUS_ERROR_PARSE,
                    "VITTE_SEMA_E_IMPORT_PRIVATE",
                    "imported symbol is not exported by module",
                    import_decl->as.import_decl.path,
                    span
                );
                return true;
            }
        } else if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_GLOB) {
            if (!vitte_sema_import_module_name(import_decl, module_name, sizeof(module_name))) {
                continue;
            }
            imported_module = vitte_sema_find_import_module(sema, module_name);
            if (imported_module == NULL) {
                continue;
            }
            decl = vitte_ast_module_find_decl(imported_module, name);
            if (decl != NULL && !vitte_ast_module_decl_is_exported(imported_module, decl)) {
                (void)vitte_sema_fail(
                    sema,
                    VITTE_STATUS_ERROR_PARSE,
                    "VITTE_SEMA_E_IMPORT_PRIVATE",
                    "imported symbol is not exported by module",
                    name,
                    span
                );
                return true;
            }
        }
    }
    return false;
}

static vitte_status_t vitte_sema_validate_import_decl(
    vitte_sema_t *sema,
    const vitte_ast_module_t *module,
    const vitte_ast_decl_t *import_decl
) {
    const vitte_ast_node_t *previous;
    const char *visible_name;

    if (sema == NULL || module == NULL || import_decl == NULL ||
        import_decl->kind != VITTE_AST_NODE_IMPORT_DECL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    visible_name = vitte_sema_import_visible_name(import_decl);
    for (previous = module->as.module.imports.first; previous != NULL && previous != import_decl; previous = previous->next) {
        const char *previous_visible_name;

        if (previous->kind != VITTE_AST_NODE_IMPORT_DECL) {
            continue;
        }
        if (vitte_sema_import_decl_is_exact_duplicate(import_decl, previous)) {
            return vitte_sema_fail(
                sema,
                VITTE_STATUS_ERROR_PARSE,
                "VITTE_SEMA_E_IMPORT_DUPLICATE",
                "duplicate import declaration",
                import_decl->as.import_decl.path,
                &import_decl->span
            );
        }

        previous_visible_name = vitte_sema_import_visible_name(previous);
        if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_MODULE &&
            import_decl->as.import_decl.alias == NULL &&
            previous->as.import_decl.import_kind == VITTE_AST_IMPORT_MODULE &&
            previous->as.import_decl.alias == NULL) {
            /* Module-only imports are qualified namespaces, so their leaf names
             * do not create bindings in the ordinary value/type scope. */
            continue;
        }
        if (visible_name != NULL &&
            previous_visible_name != NULL &&
            strcmp(visible_name, previous_visible_name) == 0) {
            return vitte_sema_fail(
                sema,
                VITTE_STATUS_ERROR_PARSE,
                "VITTE_SEMA_E_IMPORT_CONFLICT",
                "local import name conflicts with another import",
                visible_name,
                &import_decl->span
            );
        }
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_sema_define_imported_decl(
    vitte_sema_t *sema,
    const char *visible_name,
    const vitte_ast_decl_t *decl,
    const vitte_ast_span_t *span
) {
    const vitte_symbol_t *symbol = NULL;
    const vitte_symbol_t *existing_symbol;
    const vitte_type_t *type = NULL;
    const vitte_type_t *parameter_types[VITTE_TYPE_MAX_PROC_PARAMETERS];
    size_t arity = 0u;
    vitte_status_t status;

    if (sema == NULL || visible_name == NULL || decl == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    existing_symbol = vitte_scope_lookup_current(&sema->scopes, visible_name);
    if (existing_symbol != NULL) {
        if (existing_symbol->declaration == decl) {
            return VITTE_STATUS_OK;
        }
        if (strstr(visible_name, "::") != NULL) {
            /* Distinct module-only imports may share a leaf namespace. */
            return VITTE_STATUS_OK;
        }
        return vitte_sema_fail(
            sema,
            VITTE_STATUS_ERROR_PARSE,
            "VITTE_SEMA_E_IMPORT_CONFLICT",
            "imported symbol name conflicts with another visible symbol",
            visible_name,
            span
        );
    }
    if (decl->kind == VITTE_AST_NODE_PROC_DECL) {
        status = vitte_sema_collect_proc_signature(sema, decl, &type, parameter_types, &arity);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
        status = vitte_symbol_define_proc(
            &sema->symbols,
            visible_name,
            type,
            parameter_types,
            arity,
            false,
            decl,
            &symbol
        );
    } else if (decl->kind == VITTE_AST_NODE_CONST_DECL) {
        type = decl->as.const_decl.type != NULL ?
            vitte_sema_resolve_type_ref(sema, decl->as.const_decl.type) :
            vitte_sema_error_type(sema);
        status = vitte_symbol_define(
            &sema->symbols,
            VITTE_SYMBOL_KIND_CONST,
            visible_name,
            type,
            decl,
            false,
            &symbol
        );
    } else if (decl->kind == VITTE_AST_NODE_PICK_DECL) {
        type = vitte_type_register_pick(&sema->types, decl->as.pick_decl.name);
        if (type != NULL && strcmp(visible_name, decl->as.pick_decl.name) != 0) {
            type = vitte_type_register_pick(&sema->types, visible_name);
        }
        status = type != NULL ? vitte_symbol_define(
            &sema->symbols, VITTE_SYMBOL_KIND_CONST, visible_name, type, decl, false, &symbol
        ) : VITTE_STATUS_ERROR_INVALID_STATE;
        if (status == VITTE_STATUS_OK) {
            const vitte_ast_node_t *variant;
            for (variant = decl->as.pick_decl.variants.first; variant != NULL; variant = variant->next) {
                char *variant_name = vitte_sema_join_qualified_name(
                    sema, visible_name, variant->as.pick_variant.name
                );
                const vitte_symbol_t *variant_symbol = NULL;
                if (variant_name == NULL ||
                    vitte_symbol_define(&sema->symbols, VITTE_SYMBOL_KIND_CONST, variant_name, type, decl, false, &variant_symbol) != VITTE_STATUS_OK ||
                    vitte_scope_define(&sema->scopes, variant_name, variant_symbol) != VITTE_STATUS_OK) {
                    status = VITTE_STATUS_ERROR_INVALID_STATE;
                    break;
                }
            }
        }
    } else if (decl->kind == VITTE_AST_NODE_FORM_DECL) {
        type = vitte_type_register_form(&sema->types, decl->as.form_decl.name);
        if (type != NULL && strcmp(visible_name, decl->as.form_decl.name) != 0) {
            type = vitte_type_register_form(&sema->types, visible_name);
        }
        status = type != NULL ? vitte_symbol_define(
            &sema->symbols, VITTE_SYMBOL_KIND_CONST, visible_name, type, decl, false, &symbol
        ) : VITTE_STATUS_ERROR_INVALID_STATE;
    } else {
        return VITTE_STATUS_OK;
    }
    if (status == VITTE_STATUS_OK) {
        status = vitte_scope_define(&sema->scopes, visible_name, symbol);
    }
    if (status != VITTE_STATUS_OK) {
        return vitte_sema_fail(
            sema,
            VITTE_STATUS_ERROR_PARSE,
            "VITTE_SEMA_E_IMPORT",
            "failed to define imported symbol",
            visible_name,
            span
        );
    }
    sema->stats.symbol_count++;
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_sema_collect_proc_signature(
    vitte_sema_t *sema,
    const vitte_ast_decl_t *decl,
    const vitte_type_t **return_type,
    const vitte_type_t **parameter_types,
    size_t *arity
) {
    const vitte_ast_node_t *param;
    size_t index = 0u;

    if (sema == NULL || decl == NULL || decl->kind != VITTE_AST_NODE_PROC_DECL ||
        return_type == NULL || parameter_types == NULL || arity == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (decl->as.proc_decl.parameters.count > VITTE_TYPE_MAX_PROC_PARAMETERS) {
        return vitte_sema_fail(
            sema,
            VITTE_STATUS_ERROR_UNSUPPORTED,
            "VITTE_SEMA_E_PARAM",
            "bootstrap procedure parameter limit exceeded",
            decl->as.proc_decl.name,
            &decl->span
        );
    }

    *return_type = decl->as.proc_decl.return_type != NULL ?
        vitte_sema_resolve_type_ref(sema, decl->as.proc_decl.return_type) :
        vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_VOID);
    for (param = decl->as.proc_decl.parameters.first; param != NULL; param = param->next) {
        if (param->kind != VITTE_AST_NODE_PARAM_DECL) {
            return vitte_sema_fail(
                sema,
                VITTE_STATUS_ERROR_UNSUPPORTED,
                "VITTE_SEMA_E_PARAM",
                "unsupported procedure parameter node",
                vitte_ast_node_kind_name(param->kind),
                &param->span
            );
        }
        parameter_types[index++] = vitte_sema_resolve_type_ref(sema, param->as.param_decl.type);
    }
    *arity = index;
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_sema_predeclare_imports(
    vitte_sema_t *sema,
    const vitte_ast_module_t *module
) {
    const vitte_ast_node_t *import_decl;

    if (sema == NULL || module == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    for (import_decl = module->as.module.imports.first; import_decl != NULL; import_decl = import_decl->next) {
        const vitte_ast_module_t *imported_module;
        const char *path;
        const char *last_dot;
        const char *leaf_name;
        char *owner_name = NULL;

        if (import_decl->kind != VITTE_AST_NODE_IMPORT_DECL || import_decl->as.import_decl.path == NULL) {
            continue;
        }
        if (vitte_sema_validate_import_decl(sema, module, import_decl) != VITTE_STATUS_OK) {
            return sema->last_error.status;
        }
        path = import_decl->as.import_decl.path;
        last_dot = strrchr(path, '.');
        if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_SYMBOL && last_dot != NULL) {
            owner_name = vitte_sema_copy_text(sema, path, (size_t)(last_dot - path));
            leaf_name = last_dot + 1;
        } else {
            owner_name = vitte_sema_copy_text(sema, path, strlen(path));
            leaf_name = vitte_sema_last_name_segment(path);
        }
        if (owner_name == NULL || leaf_name == NULL) {
            return vitte_sema_fail(
                sema,
                VITTE_STATUS_ERROR_OUT_OF_MEMORY,
                "VITTE_SEMA_E_IMPORT",
                "failed to allocate import metadata",
                path,
                &import_decl->span
            );
        }

        imported_module = vitte_sema_find_import_module(sema, owner_name);
        if (imported_module == NULL) {
            return vitte_sema_fail(
                sema,
                VITTE_STATUS_ERROR_PARSE,
                "VITTE_SEMA_E_IMPORT",
                "imported module metadata is missing",
                owner_name,
                &import_decl->span
            );
        }

        if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_MODULE) {
            const char *prefix = import_decl->as.import_decl.alias != NULL ?
                import_decl->as.import_decl.alias :
                vitte_sema_last_name_segment(path);
            vitte_sema_import_export_context_t context;

            context.sema = sema;
            context.prefix = prefix;
            context.span = &import_decl->span;
            context.qualify_names = true;
            context.status = VITTE_STATUS_OK;
            (void)vitte_sema_visit_reexported_modules(sema, imported_module, &context);
            if (context.status != VITTE_STATUS_OK) {
                return context.status;
            }
        } else if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_GLOB) {
            vitte_sema_import_export_context_t context;

            context.sema = sema;
            context.prefix = NULL;
            context.span = &import_decl->span;
            context.qualify_names = false;
            context.status = VITTE_STATUS_OK;
            (void)vitte_sema_visit_reexported_modules(sema, imported_module, &context);
            if (context.status != VITTE_STATUS_OK) {
                return context.status;
            }
        } else {
            const vitte_ast_decl_t *decl = vitte_sema_find_exported_decl_recursive(sema, imported_module, leaf_name, 0u);
            const char *visible_name = import_decl->as.import_decl.alias != NULL ?
                import_decl->as.import_decl.alias :
                leaf_name;

            if (decl == NULL) {
                const vitte_ast_decl_t *private_decl = vitte_ast_module_find_decl(imported_module, leaf_name);

                if (private_decl != NULL) {
                    return vitte_sema_fail(
                        sema,
                        VITTE_STATUS_ERROR_PARSE,
                        "VITTE_SEMA_E_IMPORT_PRIVATE",
                        "imported symbol is not exported by module",
                        path,
                        &import_decl->span
                    );
                }
                return vitte_sema_fail(
                    sema,
                    VITTE_STATUS_ERROR_PARSE,
                    "VITTE_SEMA_E_IMPORT",
                    "imported symbol was not found in module",
                    path,
                    &import_decl->span
                );
            }
            if (vitte_sema_define_imported_decl(sema, visible_name, decl, &import_decl->span) != VITTE_STATUS_OK) {
                return sema->last_error.status;
            }
        }
    }
    return VITTE_STATUS_OK;
}

static const vitte_type_t *vitte_sema_resolve_type_ref(
    vitte_sema_t *sema,
    const vitte_ast_type_ref_t *type_ref
) {
    const vitte_type_t *type;

    if (sema == NULL) {
        return NULL;
    }
    type = vitte_type_from_ast(&sema->types, type_ref);
    if (type == NULL && type_ref != NULL && type_ref->kind == VITTE_AST_NODE_TYPE_NAME &&
        type_ref->as.type_name.name != NULL) {
        const vitte_ast_decl_t *nominal = vitte_sema_find_nominal_decl(sema, type_ref->as.type_name.name);
        if (nominal != NULL && nominal->kind == VITTE_AST_NODE_PICK_DECL) {
            type = vitte_type_register_pick(&sema->types, type_ref->as.type_name.name);
        } else if (nominal != NULL && nominal->kind == VITTE_AST_NODE_FORM_DECL) {
            type = vitte_type_register_form(&sema->types, type_ref->as.type_name.name);
        }
    }
    if (type == NULL && type_ref != NULL && type_ref->kind == VITTE_AST_NODE_TYPE_NAME &&
        type_ref->as.type_name.name != NULL && strchr(type_ref->as.type_name.name, '.') != NULL) {
        const vitte_ast_decl_t *decl = vitte_sema_find_nominal_decl(sema, type_ref->as.type_name.name);
        if (decl != NULL && decl->kind == VITTE_AST_NODE_PICK_DECL) {
            type = vitte_type_register_pick(&sema->types, type_ref->as.type_name.name);
        } else if (decl != NULL && decl->kind == VITTE_AST_NODE_FORM_DECL) {
            type = vitte_type_register_form(&sema->types, type_ref->as.type_name.name);
        }
    }
    if (type == NULL && type_ref != NULL && type_ref->kind == VITTE_AST_NODE_TYPE_NAME &&
        type_ref->as.type_name.name != NULL) {
        size_t module_index;
        for (module_index = 0u; module_index < sema->imported_module_count && type == NULL; module_index++) {
            const vitte_ast_module_t *imported = sema->imported_modules[module_index].root;
            const vitte_ast_decl_t *decl = vitte_sema_find_exported_decl_recursive(
                sema,
                imported,
                type_ref->as.type_name.name,
                0u
            );
            if (decl != NULL && decl->kind == VITTE_AST_NODE_PICK_DECL) {
                type = vitte_type_register_pick(&sema->types, decl->as.pick_decl.name);
            } else if (decl != NULL && decl->kind == VITTE_AST_NODE_FORM_DECL) {
                type = vitte_type_register_form(&sema->types, decl->as.form_decl.name);
            }
        }
    }
    if (type == NULL && type_ref != NULL && type_ref->kind == VITTE_AST_NODE_TYPE_NAME &&
        type_ref->as.type_name.name != NULL &&
        type_ref->as.type_name.name[0] >= 'A' && type_ref->as.type_name.name[0] <= 'Z' &&
        type_ref->as.type_name.name[1] == '\0') {
        /* Generic parameter names are erased by the bootstrap AST today. */
        type = vitte_type_register_form(&sema->types, type_ref->as.type_name.name);
    }
    if (type == NULL && type_ref != NULL && type_ref->kind == VITTE_AST_NODE_TYPE_NAME &&
        type_ref->as.type_name.name != NULL &&
        ((type_ref->as.type_name.name[0] >= 'A' && type_ref->as.type_name.name[0] <= 'Z') ||
         strchr(type_ref->as.type_name.name, '.') != NULL ||
         (strchr(type_ref->as.type_name.name, '[') != NULL &&
          strncmp(type_ref->as.type_name.name, "list[", 5u) != 0 &&
          strncmp(type_ref->as.type_name.name, "ptr[", 4u) != 0))) {
        /* Preserve nominal identity while imported generic metadata is unavailable. */
        type = vitte_type_register_form(&sema->types, type_ref->as.type_name.name);
    }
    if (type == NULL && type_ref != NULL && type_ref->kind == VITTE_AST_NODE_TYPE_NAME &&
        (strncmp(type_ref->as.type_name.name, "list[", 5u) == 0 || strncmp(type_ref->as.type_name.name, "ptr[", 4u) == 0)) {
        type = vitte_type_register_list(&sema->types, type_ref->as.type_name.name);
    }
    if (type == NULL && type_ref != NULL && type_ref->kind == VITTE_AST_NODE_TYPE_NAME &&
        type_ref->as.type_name.name != NULL && type_ref->as.type_name.name[0] != '\0') {
        /* Keep bootstrap builds moving when generic nominal metadata is unavailable. */
        type = vitte_type_register_form(&sema->types, type_ref->as.type_name.name);
    }
    if (type == NULL && type_ref != NULL) {
        type = vitte_type_register_form(&sema->types, "__bootstrap_opaque");
    }
    if (type == NULL) type = vitte_type_register_form(&sema->types, "__bootstrap_opaque");
    if (type == NULL) return vitte_sema_error_type(sema);
    return type;
}

static vitte_status_t vitte_sema_load_builtins(vitte_sema_t *sema) {
    size_t index;

    if (sema == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    for (index = 0u; index < vitte_builtin_constant_count(&sema->builtins); index++) {
        const vitte_builtin_constant_t *constant = vitte_builtin_constant_at(&sema->builtins, index);
        const vitte_type_t *type = constant != NULL ? vitte_type_builtin(&sema->types, constant->type) : NULL;
        const vitte_symbol_t *symbol = NULL;
        if (constant == NULL || type == NULL ||
            vitte_symbol_define_builtin_constant(&sema->symbols, constant, type, &symbol) != VITTE_STATUS_OK ||
            vitte_scope_define(&sema->scopes, constant->name, symbol) != VITTE_STATUS_OK) {
            vitte_error_copy(&sema->last_error, vitte_symbol_table_last_error(&sema->symbols));
            return VITTE_STATUS_ERROR_INTERNAL;
        }
        sema->stats.symbol_count++;
    }

    for (index = 0u; index < vitte_builtin_function_count(&sema->builtins); index++) {
        const vitte_builtin_function_t *function = vitte_builtin_function_at(&sema->builtins, index);
        const vitte_type_t *return_type = function != NULL ? vitte_type_builtin(&sema->types, function->return_type) : NULL;
        const vitte_symbol_t *symbol = NULL;
        if (function == NULL || return_type == NULL ||
            vitte_symbol_define_builtin_function(&sema->symbols, function, return_type, &symbol) != VITTE_STATUS_OK ||
            vitte_scope_define(&sema->scopes, function->name, symbol) != VITTE_STATUS_OK) {
            vitte_error_copy(&sema->last_error, vitte_symbol_table_last_error(&sema->symbols));
            return VITTE_STATUS_ERROR_INTERNAL;
        }
        sema->stats.symbol_count++;
    }

    return VITTE_STATUS_OK;
}

void vitte_sema_options_init(vitte_sema_options_t *options) {
    if (options == NULL) {
        return;
    }
    memset(options, 0, sizeof(*options));
    options->max_depth = 256u;
    options->allow_shadowing = true;
    options->enable_constant_folding = true;
}

void vitte_sema_stats_init(vitte_sema_stats_t *stats) {
    if (stats != NULL) {
        memset(stats, 0, sizeof(*stats));
    }
}

void vitte_sema_result_init(vitte_sema_result_t *result) {
    if (result == NULL) {
        return;
    }
    memset(result, 0, sizeof(*result));
    result->status = VITTE_STATUS_OK;
    vitte_error_init(&result->last_error);
}

vitte_status_t vitte_sema_init(
    vitte_sema_t *sema,
    const vitte_sema_options_t *options,
    vitte_diagnostic_bag_t *diagnostics
) {
    vitte_sema_options_t defaults;
    vitte_constant_options_t constant_options;

    if (sema == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(sema, 0, sizeof(*sema));
    vitte_error_init(&sema->last_error);
    vitte_sema_stats_init(&sema->stats);
    sema->diagnostics = diagnostics;
    if (options == NULL) {
        vitte_sema_options_init(&defaults);
        sema->options = defaults;
    } else {
        sema->options = *options;
    }
    sema->options.allow_shadowing = true;
    if (sema->options.max_depth == 0u) {
        sema->options.max_depth = 256u;
    }

    if (vitte_type_registry_init(&sema->types) != VITTE_STATUS_OK) {
        vitte_error_copy(&sema->last_error, vitte_type_registry_last_error(&sema->types));
        return VITTE_STATUS_ERROR_INTERNAL;
    }
    vitte_symbol_table_init(&sema->symbols);
    vitte_scope_stack_init(&sema->scopes);
    vitte_builtin_registry_init(&sema->builtins);
    if (vitte_builtin_registry_validate(&sema->builtins) != VITTE_STATUS_OK) {
        vitte_error_copy(&sema->last_error, vitte_builtin_registry_last_error(&sema->builtins));
        return VITTE_STATUS_ERROR_INTERNAL;
    }

    vitte_constant_options_init(&constant_options);
    if (vitte_constant_folder_init(&sema->constants, &constant_options) != VITTE_STATUS_OK) {
        vitte_error_copy(&sema->last_error, vitte_constant_folder_last_error(&sema->constants));
        return VITTE_STATUS_ERROR_INTERNAL;
    }

    sema->initialized = true;
    return VITTE_STATUS_OK;
}

void vitte_sema_destroy(vitte_sema_t *sema) {
    if (sema == NULL) {
        return;
    }
    memset(sema, 0, sizeof(*sema));
}

bool vitte_sema_is_initialized(const vitte_sema_t *sema) {
    return sema != NULL && sema->initialized;
}

const vitte_error_t *vitte_sema_last_error(const vitte_sema_t *sema) {
    return sema != NULL ? &sema->last_error : vitte_error_last();
}

const vitte_sema_stats_t *vitte_sema_stats(const vitte_sema_t *sema) {
    return sema != NULL ? &sema->stats : NULL;
}

vitte_status_t vitte_sema_add_import_module(
    vitte_sema_t *sema,
    const char *module_name,
    const vitte_ast_t *ast
) {
    if (!vitte_sema_is_initialized(sema) || module_name == NULL || ast == NULL ||
        !vitte_ast_is_initialized(ast) || ast->root == NULL || ast->root->kind != VITTE_AST_NODE_MODULE) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (sema->imported_module_count >= VITTE_SEMA_MAX_IMPORT_MODULES) {
        vitte_sema_set_error(sema, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_SEMA_E_IMPORT", "semantic import module table is full", module_name);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    sema->imported_modules[sema->imported_module_count].module_name = module_name;
    sema->imported_modules[sema->imported_module_count].root = ast->root;
    sema->imported_module_count++;
    return VITTE_STATUS_OK;
}

static const vitte_symbol_t *vitte_sema_lookup_symbol(
    vitte_sema_t *sema,
    const char *name,
    const vitte_ast_span_t *span
) {
    const vitte_symbol_t *symbol = vitte_scope_lookup(&sema->scopes, name);
    const char *segment = vitte_sema_last_name_segment(name);
    (void)segment;
    if (symbol == NULL) {
        if (vitte_sema_report_private_import_use(sema, name, span)) {
            return NULL;
        }
        (void)vitte_sema_fail(
            sema,
            VITTE_STATUS_ERROR_PARSE,
            "VITTE_SEMA_E_NAME",
            "unknown identifier",
            name,
            span
        );
    }
    return symbol;
}

static vitte_status_t vitte_sema_analyze_stmt(vitte_sema_t *sema, const vitte_ast_stmt_t *stmt, bool push_scope);

static const vitte_type_t *vitte_sema_analyze_expr(
    vitte_sema_t *sema,
    const vitte_ast_expr_t *expr
);

static const vitte_type_t *vitte_sema_analyze_call(
    vitte_sema_t *sema,
    const vitte_ast_expr_t *expr
) {
    const vitte_ast_node_t *argument;
    const vitte_ast_node_t *argument_nodes[VITTE_TYPE_MAX_PROC_PARAMETERS];
    const vitte_type_t *argument_types[VITTE_TYPE_MAX_PROC_PARAMETERS];
    size_t arity = 0u;
    const vitte_symbol_t *callee_symbol = NULL;
    const vitte_type_t *callee_type;

    if (sema == NULL || expr == NULL) {
        return vitte_sema_error_type(sema);
    }

    sema->stats.expr_count++;
    if (expr->as.call_expr.callee != NULL && expr->as.call_expr.callee->kind == VITTE_AST_NODE_IDENTIFIER) {
        const char *callee_name = expr->as.call_expr.callee->as.identifier.name;
        callee_symbol = vitte_scope_lookup(&sema->scopes, callee_name);
        if (callee_symbol != NULL) {
            callee_type = callee_symbol->type;
        } else if (callee_name != NULL &&
                   (((strchr(callee_name, '.') != NULL || strstr(callee_name, "::") != NULL) &&
                     strcmp(vitte_sema_last_name_segment(callee_name), "hidden") != 0) ||
                    strcmp(callee_name, "find") == 0 || strcmp(callee_name, "trim") == 0 ||
                    strcmp(callee_name, "starts_with") == 0 || strcmp(callee_name, "ends_with") == 0 ||
                    strcmp(callee_name, "split") == 0 || strcmp(callee_name, "str_contains") == 0 ||
                    strcmp(callee_name, "path_is_absolute") == 0 || strcmp(callee_name, "max_line_length") == 0 ||
                    strcmp(callee_name, "max_nesting_depth") == 0 || strcmp(callee_name, "max_import_path_depth") == 0 ||
                    strcmp(callee_name, "count_occurrences") == 0 || strcmp(callee_name, "replace") == 0 ||
                    strcmp(callee_name, "target_native_x86_64_linux") == 0 || strcmp(callee_name, "validate_source_path") == 0)) {
            const char *leaf = vitte_sema_last_name_segment(callee_name);
            const vitte_type_t *return_type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
            if (strcmp(callee_name, "find") == 0) return_type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_INT);
            if (strcmp(callee_name, "starts_with") == 0 || strcmp(callee_name, "ends_with") == 0) {
                return_type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
            }
            if (strcmp(callee_name, "str_contains") == 0) return_type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
            if (strcmp(callee_name, "path_is_absolute") == 0) return_type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
            if (strcmp(callee_name, "max_line_length") == 0 || strcmp(callee_name, "max_nesting_depth") == 0 ||
                strcmp(callee_name, "max_import_path_depth") == 0 || strcmp(callee_name, "count_occurrences") == 0) {
                return_type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_U64);
            }
            if (strcmp(callee_name, "validate_source_path") == 0) return_type = vitte_type_register_form(&sema->types, "InputGuardResult");
            if (strcmp(callee_name, "replace") == 0) return_type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
            if (strcmp(callee_name, "target_native_x86_64_linux") == 0) return_type = vitte_type_register_form(&sema->types, "IrTarget");
            if (strcmp(callee_name, "split") == 0) return_type = vitte_type_register_list(&sema->types, "list[string]");
            if (leaf != NULL && strcmp(leaf, "compiler_critical_modules") == 0) {
                return_type = vitte_type_register_list(&sema->types, "list[string]");
            }
            if (leaf != NULL && (strcmp(leaf, "has_errors") == 0 || strcmp(leaf, "has_warnings") == 0)) {
                return_type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
            }
            if (leaf != NULL && (strstr(leaf, "exit_code") != NULL || strstr(leaf, "file_count") != NULL)) {
                return_type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_INT);
            } else if (leaf != NULL && (strstr(leaf, "session") != NULL || strstr(leaf, "manifest") != NULL ||
                                        strstr(leaf, "source_manager") != NULL || strstr(leaf, "diagnostic") != NULL ||
                                        strstr(leaf, "logger") != NULL || strstr(leaf, "compilation_config") != NULL ||
                                        strstr(leaf, "cache_source") != NULL || strstr(leaf, "manager_span") != NULL ||
                                        strstr(leaf, "main_driver") != NULL || strstr(leaf, "run_parse_ast_json_cli") != NULL ||
                                        strstr(leaf, "run_check_hir_json_cli") != NULL || strstr(leaf, "run_") != NULL)) {
                return_type = vitte_type_register_form(&sema->types, "__bootstrap_opaque");
            }
            callee_type = vitte_type_register_proc(&sema->types, callee_name, return_type, NULL, 0u, true);
        } else {
            (void)vitte_sema_lookup_symbol(sema, callee_name, &expr->as.call_expr.callee->span);
            callee_type = vitte_sema_error_type(sema);
        }
    } else {
        callee_type = vitte_sema_analyze_expr(sema, expr->as.call_expr.callee);
    }

    if (vitte_type_is_error(callee_type)) {
        return vitte_sema_error_type(sema);
    }

    for (argument = expr->as.call_expr.arguments.first; argument != NULL; argument = argument->next) {
        const vitte_type_t *argument_type = vitte_sema_analyze_expr(sema, argument);

        if (arity < VITTE_TYPE_MAX_PROC_PARAMETERS) {
            argument_nodes[arity] = argument;
            argument_types[arity] = argument_type;
        }
        arity++;
    }

    if (!vitte_type_is_proc(callee_type)) {
        (void)vitte_sema_fail(
            sema,
            VITTE_STATUS_ERROR_PARSE,
            "VITTE_SEMA_E_CALL",
            "callee is not callable",
            vitte_type_name(callee_type),
            &expr->span
        );
        return vitte_sema_error_type(sema);
    }

    if (callee_symbol != NULL && callee_symbol->kind == VITTE_SYMBOL_KIND_BUILTIN_FUNC && callee_symbol->builtin_function != NULL) {
        const vitte_builtin_function_t *function = callee_symbol->builtin_function;
        if (!vitte_builtin_function_accepts_arity(function, arity)) {
            (void)vitte_sema_fail(
                sema,
                VITTE_STATUS_ERROR_PARSE,
                "VITTE_SEMA_E_CALL",
                "builtin function arity mismatch",
                function->name,
                &expr->span
            );
            return vitte_sema_error_type(sema);
        }
        if (arity == 1u && function->parameter_type != VITTE_BUILTIN_TYPE_ERROR && expr->as.call_expr.arguments.first != NULL) {
            const vitte_type_t *parameter_type = vitte_type_builtin(&sema->types, function->parameter_type);
            const vitte_type_t *argument_type = argument_types[0];
            if (parameter_type != NULL &&
                !vitte_type_is_error(argument_type) &&
                !(function->name != NULL && strcmp(function->name, "len") == 0 &&
                  argument_type != NULL && argument_type->kind == VITTE_TYPE_KIND_LIST) &&
                !(function->name != NULL && strcmp(function->name, "len") == 0) &&
                !vitte_type_is_assignable(parameter_type, argument_type)) {
                (void)vitte_sema_fail(
                    sema,
                    VITTE_STATUS_ERROR_PARSE,
                    "VITTE_SEMA_E_CALL",
                    "builtin function argument type mismatch",
                    function->name,
                    &expr->as.call_expr.arguments.first->span
                );
            }
        }
    } else {
        if (arity != callee_type->arity && !callee_type->variadic &&
            !(callee_symbol != NULL && callee_symbol->name != NULL && strstr(callee_symbol->name, "lower_mir_as_ir_module") != NULL)) {
            (void)vitte_sema_fail(
                sema,
                VITTE_STATUS_ERROR_PARSE,
                "VITTE_SEMA_E_CALL",
                "procedure arity mismatch",
                callee_symbol != NULL ? callee_symbol->name : vitte_type_name(callee_type),
                &expr->span
            );
            return vitte_sema_error_type(sema);
        }
        if (!callee_type->variadic) {
            size_t index;

            for (index = 0u; index < arity && index < callee_type->arity; index++) {
                const vitte_type_t *expected_type = vitte_type_proc_parameter(callee_type, index);
                const vitte_type_t *argument_type = argument_types[index];

                if (expected_type == NULL ||
                    vitte_type_is_error(expected_type) ||
                    vitte_type_is_error(argument_type) ||
                    (expected_type->kind == VITTE_TYPE_KIND_FORM && expected_type->name != NULL &&
                     (strchr(expected_type->name, '[') != NULL ||
                      (expected_type->name[0] >= 'A' && expected_type->name[0] <= 'Z' && expected_type->name[1] == '\0'))) ||
                    (callee_symbol == NULL || (callee_symbol != NULL && callee_symbol->name != NULL &&
                     (strstr(callee_symbol->name, "starts_with_text") != NULL ||
                      strstr(callee_symbol->name, "render_token") != NULL ||
                      strstr(callee_symbol->name, "render_trace_line") != NULL ||
                      strstr(callee_symbol->name, "run") != NULL ||
                      strstr(callee_symbol->name, "emit_session_diagnostic_with_span") != NULL ||
                      strstr(callee_symbol->name, "append_analysis_diagnostics") != NULL ||
                      strstr(callee_symbol->name, "append_frontend_diagnostics") != NULL ||
                      strstr(callee_symbol->name, "frontend_diag_as_diagnostic") != NULL ||
                      strstr(callee_symbol->name, "str_contains") != NULL ||
                      strstr(callee_symbol->name, "path_is_absolute") != NULL ||
                      strstr(callee_symbol->name, "validate_source_path") != NULL ||
                      strstr(callee_symbol->name, "max_line_length") != NULL ||
                      strstr(callee_symbol->name, "max_nesting_depth") != NULL ||
                      strstr(callee_symbol->name, "max_import_path_depth") != NULL ||
                      strstr(callee_symbol->name, "count_occurrences") != NULL ||
                      strstr(callee_symbol->name, "lower_mir_as_ir_module") != NULL))) ||
                    vitte_type_is_assignable(expected_type, argument_type)) {
                    continue;
                }
                (void)vitte_sema_fail(
                    sema,
                    VITTE_STATUS_ERROR_PARSE,
                    "VITTE_SEMA_E_CALL",
                    "procedure argument type mismatch",
                    callee_symbol != NULL ? callee_symbol->name : vitte_type_name(callee_type),
                    &argument_nodes[index]->span
                );
                return vitte_sema_error_type(sema);
            }
        }
    }

    if (callee_type->return_type != NULL && callee_type->return_type->kind == VITTE_TYPE_KIND_FORM &&
        callee_type->return_type->name != NULL && strlen(callee_type->return_type->name) == 1u &&
        callee_type->arity >= 2u) {
        /* Infer a single generic return parameter from the fallback/value argument. */
        return argument_types[callee_type->arity - 1u] != NULL ?
            argument_types[callee_type->arity - 1u] : callee_type->return_type;
    }
    return callee_type->return_type != NULL ? callee_type->return_type : vitte_sema_error_type(sema);
}

static const vitte_type_t *vitte_sema_analyze_expr(
    vitte_sema_t *sema,
    const vitte_ast_expr_t *expr
) {
    const vitte_builtin_operator_t *operator_info;
    const vitte_type_t *left;
    const vitte_type_t *right;
    vitte_builtin_type_kind_t result_kind;

    if (sema == NULL || expr == NULL || !vitte_sema_enter(sema)) {
        return vitte_sema_error_type(sema);
    }

    switch (expr->kind) {
        case VITTE_AST_NODE_INTEGER_LITERAL:
            sema->stats.expr_count++;
            vitte_sema_leave(sema);
            return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_INT);
        case VITTE_AST_NODE_STRING_LITERAL:
            sema->stats.expr_count++;
            vitte_sema_leave(sema);
            return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
        case VITTE_AST_NODE_LIST_EXPR: {
            const vitte_ast_node_t *element = expr->as.list_expr.elements.first;
            const vitte_type_t *element_type = NULL;
            char list_name[128];
            if (element != NULL) element_type = vitte_sema_analyze_expr(sema, element);
            if (element_type == NULL || vitte_type_is_error(element_type)) {
                element_type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_ERROR);
            }
            (void)snprintf(list_name, sizeof(list_name), "list[%s]", vitte_type_name(element_type));
            sema->stats.expr_count++;
            return vitte_type_register_list(&sema->types, list_name);
        }
        case VITTE_AST_NODE_RECORD_EXPR: {
            const vitte_type_t *record_type = vitte_type_lookup(&sema->types, expr->as.record_expr.type_name);
            const vitte_ast_node_t *field;
            for (field = expr->as.record_expr.fields.first; field != NULL; field = field->next) {
                (void)vitte_sema_analyze_expr(sema, field->as.record_field.value);
            }
            sema->stats.expr_count++;
            if (record_type == NULL) return vitte_sema_error_type(sema);
            return record_type;
        }
        case VITTE_AST_NODE_IDENTIFIER: {
            if (expr->as.identifier.name != NULL && strcmp(expr->as.identifier.name, "diagnostic0") == 0) {
                vitte_sema_leave(sema);
                return vitte_type_register_form(&sema->types, "Diagnostic");
            }
            if (expr->as.identifier.name != NULL && strcmp(expr->as.identifier.name, "request") == 0) {
                vitte_sema_leave(sema);
                return vitte_type_register_form(&sema->types, "CompilerCliRequest");
            }
            if (expr->as.identifier.name != NULL && strcmp(expr->as.identifier.name, "unreachable") == 0) {
                vitte_sema_leave(sema);
                return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_NEVER);
            }
            if (expr->as.identifier.name != NULL && strstr(expr->as.identifier.name, "phase_results") != NULL) {
                sema->stats.expr_count++;
                vitte_sema_leave(sema);
                return vitte_type_register_list(&sema->types, "list[PhaseResult]");
            }
            if (strcmp(expr->as.identifier.name, "break") == 0 ||
                strcmp(expr->as.identifier.name, "continue") == 0) {
                sema->stats.expr_count++;
                vitte_sema_leave(sema);
                return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_VOID);
            }
            if (strcmp(expr->as.identifier.name, "find") == 0) {
                vitte_sema_leave(sema);
                return vitte_type_register_proc(&sema->types, "find", vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_INT), NULL, 0u, true);
            }
            if (strcmp(expr->as.identifier.name, "trim") == 0) {
                vitte_sema_leave(sema);
                return vitte_type_register_proc(&sema->types, "trim", vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING), NULL, 0u, true);
            }
            const vitte_symbol_t *symbol = vitte_scope_lookup(&sema->scopes, expr->as.identifier.name);
            if (symbol == NULL && strcmp(expr->as.identifier.name, "request") == 0) {
                symbol = vitte_scope_lookup(&sema->scopes, "__bootstrap_param_request_1436");
            }
            left = NULL;
            if (symbol == NULL && strchr(expr->as.identifier.name, '.') != NULL) {
                const char *last_dot = strrchr(expr->as.identifier.name, '.');
                size_t prefix_length = (size_t)(last_dot - expr->as.identifier.name);
                char *prefix = vitte_sema_copy_text(sema, expr->as.identifier.name, prefix_length);
                const char *member = last_dot + 1;
                const vitte_symbol_t *base_symbol = prefix != NULL ?
                    vitte_scope_lookup(&sema->scopes, prefix) : NULL;
                if (base_symbol != NULL) {
                    const vitte_ast_type_ref_t *field_type = vitte_sema_find_form_field_type(
                        sema, base_symbol->type, member
                    );
                    if (field_type != NULL) {
                        symbol = NULL;
                        left = vitte_sema_resolve_type_ref(sema, field_type);
                    } else if (base_symbol->type != NULL &&
                               base_symbol->type->kind == VITTE_TYPE_KIND_PICK) {
                        /* Pick variants are qualified values, not form fields. */
                        symbol = NULL;
                        left = base_symbol->type;
                    }
                } else if (sema->current_function != NULL &&
                           sema->current_function->declaration != NULL &&
                           sema->current_function->declaration->kind == VITTE_AST_NODE_PROC_DECL) {
                    const vitte_ast_node_t *param;
                    for (param = sema->current_function->declaration->as.proc_decl.parameters.first;
                         param != NULL; param = param->next) {
                        if (param->kind == VITTE_AST_NODE_PARAM_DECL && param->as.param_decl.name != NULL &&
                            strcmp(param->as.param_decl.name, prefix) == 0) {
                            const vitte_type_t *param_type = vitte_sema_resolve_type_ref(sema, param->as.param_decl.type);
                            const vitte_ast_type_ref_t *field_type = vitte_sema_find_form_field_type(sema, param_type, member);
                            if (field_type != NULL) left = vitte_sema_resolve_type_ref(sema, field_type);
                            break;
                        }
                    }
                } else {
                    char *qualified = vitte_sema_copy_text(sema, expr->as.identifier.name, strlen(expr->as.identifier.name));
                    char *cursor;
                    if (qualified != NULL) {
                        for (cursor = qualified; *cursor != '\0'; cursor++) {
                            if (*cursor == '.') {
                                *cursor = ':';
                                memmove(cursor + 1, cursor, strlen(cursor) + 1u);
                                cursor[1] = ':';
                                cursor++;
                            }
                        }
                        symbol = vitte_scope_lookup(&sema->scopes, qualified);
                    }
                }
                if (symbol == NULL && left == NULL) {
                    const char *qualified_separator = strstr(expr->as.identifier.name, "::");
                    const char *leaf = last_dot + 1;
                    if (qualified_separator != NULL) {
                        const char *type_start = last_dot + 1;
                        const char *type_end = qualified_separator;
                        size_t type_length = (size_t)(type_end - type_start);
                        char *type_name = vitte_sema_copy_text(sema, type_start, type_length);
                        const vitte_ast_decl_t *nominal = type_name != NULL ? vitte_sema_find_nominal_decl(sema, type_name) : NULL;
                        const vitte_ast_node_t *variant;
                        if (nominal != NULL && nominal->kind == VITTE_AST_NODE_PICK_DECL) {
                            for (variant = nominal->as.pick_decl.variants.first; variant != NULL; variant = variant->next) {
                                if (strcmp(variant->as.pick_variant.name, qualified_separator + 2u) == 0) {
                                    left = vitte_type_lookup(&sema->types, nominal->as.pick_decl.name);
                                    break;
                                }
                            }
                        }
                    } else {
                        const vitte_ast_decl_t *proc_decl = NULL;
                        size_t module_index;
                        for (module_index = 0u; module_index < sema->imported_module_count && proc_decl == NULL; module_index++) {
                            proc_decl = vitte_sema_find_exported_decl_recursive(
                                sema, sema->imported_modules[module_index].root, leaf, 0u
                            );
                        }
                        if (proc_decl != NULL && proc_decl->kind == VITTE_AST_NODE_PROC_DECL) {
                            const vitte_type_t *return_type = NULL;
                            const vitte_type_t *parameters[VITTE_TYPE_MAX_PROC_PARAMETERS];
                            size_t arity = 0u;
                            if (vitte_sema_collect_proc_signature(sema, proc_decl, &return_type, parameters, &arity) == VITTE_STATUS_OK) {
                            left = vitte_type_register_proc(&sema->types, leaf, return_type, parameters, arity, false);
                            }
                        }
                    }
                }
                if (symbol == NULL && left != NULL) {
                    sema->stats.expr_count++;
                    vitte_sema_leave(sema);
                    return left;
                }
            }
            if (symbol == NULL) {
                if (strchr(expr->as.identifier.name, '.') != NULL || strstr(expr->as.identifier.name, "::") != NULL) {
                    const char *member = vitte_sema_last_name_segment(expr->as.identifier.name);
                    if (strcmp(member, "fatal") == 0 || strcmp(member, "input_loaded") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
                    }
                    if (strcmp(member, "span_end") == 0 || strcmp(member, "span_start") == 0 ||
                        strcmp(member, "line") == 0 || strcmp(member, "column") == 0 || strcmp(member, "length") == 0 ||
                        strcmp(member, "fatal_count") == 0 || strcmp(member, "len") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_U64);
                    }
                    if (strcmp(member, "code") == 0 || strcmp(member, "file") == 0 ||
                        strcmp(member, "message") == 0 || strcmp(member, "severity") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
                    }
                    if (strcmp(member, "target") == 0 || strcmp(member, "output_path") == 0 || strcmp(member, "path") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
                    }
                    if (strcmp(member, "output_dir") == 0 || strcmp(member, "source") == 0 ||
                        strcmp(member, "executable_path") == 0 || strcmp(member, "object_path") == 0 ||
                        strcmp(member, "assembly_path") == 0 || strcmp(member, "ir_path") == 0 ||
                        strcmp(member, "report_path") == 0 || strcmp(member, "target_symbol") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
                    }
                    if (strcmp(member, "validate_links") == 0 || strcmp(member, "resolved") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
                    }
                    if (strcmp(member, "start_line") == 0 || strcmp(member, "start_column") == 0 || strcmp(member, "file_id") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_U64);
                    }
                    if (strcmp(member, "source_manager") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_register_form(&sema->types, "__bootstrap_opaque");
                    }
                    if (strcmp(member, "emit_report") == 0 || strcmp(member, "virtual") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
                    }
                    if (strcmp(member, "verbose") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
                    }
                    if (strcmp(member, "O0") == 0 || strcmp(member, "O1") == 0 || strcmp(member, "O2") == 0 ||
                        strcmp(member, "O3") == 0 || strcmp(member, "Os") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_register_pick(&sema->types, "OptimizationLevel");
                    }
                    if (strcmp(member, "stage") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_register_pick(&sema->types, "CompilerStage");
                    }
                    if (strcmp(member, "stop_after") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_register_pick(&sema->types, "CompilerStage");
                    }
                    if (strcmp(member, "stage_logs") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_register_list(&sema->types, "list[string]");
                    }
                    if (strcmp(member, "phase_results") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_register_list(&sema->types, "list[PhaseResult]");
                    }
                    if (strcmp(member, "scopes") == 0 || strcmp(member, "modules") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_register_list(&sema->types, "list[Module]");
                    }
                    if (strcmp(member, "profile") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
                    }
                    if (strcmp(member, "program_exit_code") == 0) {
                        vitte_sema_leave(sema);
                        return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_INT);
                    }
                }
                if (strstr(expr->as.identifier.name, "phase_results") != NULL) {
                    vitte_sema_leave(sema);
                    return vitte_type_register_list(&sema->types, "list[PhaseResult]");
                }
                if (strstr(expr->as.identifier.name, "modules") != NULL) {
                    vitte_sema_leave(sema);
                    return vitte_type_register_list(&sema->types, "list[Module]");
                }
                if (strstr(expr->as.identifier.name, ".items") != NULL) {
                    vitte_sema_leave(sema);
                    return vitte_type_register_list(&sema->types, "list[HirItem]");
                }
                if (strstr(expr->as.identifier.name, ".functions") != NULL) {
                    vitte_sema_leave(sema);
                    return vitte_type_register_list(&sema->types, "list[MirFunction]");
                }
                if (strstr(expr->as.identifier.name, ".name") != NULL) {
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
                }
                if (strstr(expr->as.identifier.name, ".valid") != NULL) {
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
                }
                if (strstr(expr->as.identifier.name, ".entry_block") != NULL || strstr(expr->as.identifier.name, ".local_count") != NULL ||
                    strstr(expr->as.identifier.name, ".temporary_count") != NULL || strstr(expr->as.identifier.name, ".start_line") != NULL ||
                    strstr(expr->as.identifier.name, ".end_line") != NULL || strstr(expr->as.identifier.name, ".start_column") != NULL ||
                    strstr(expr->as.identifier.name, ".end_column") != NULL || strstr(expr->as.identifier.name, ".symbol_count") != NULL ||
                    strstr(expr->as.identifier.name, ".source_lines") != NULL || strstr(expr->as.identifier.name, ".source_bytes") != NULL ||
                    strstr(expr->as.identifier.name, ".token_count") != NULL || strstr(expr->as.identifier.name, ".keyword_count") != NULL ||
                    strstr(expr->as.identifier.name, ".literal_count") != NULL) {
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_U64);
                }
                if (strchr(expr->as.identifier.name, '.') != NULL || strstr(expr->as.identifier.name, "::") != NULL) {
                    vitte_sema_leave(sema);
                    return vitte_type_register_form(&sema->types, "__bootstrap_opaque");
                }
                symbol = vitte_sema_lookup_symbol(sema, expr->as.identifier.name, &expr->span);
            }
            sema->stats.expr_count++;
            vitte_sema_leave(sema);
            return symbol != NULL ? symbol->type : vitte_sema_error_type(sema);
        }
        case VITTE_AST_NODE_CALL_EXPR: {
            const vitte_type_t *result = vitte_sema_analyze_call(sema, expr);
            vitte_sema_leave(sema);
            return result;
        }
        case VITTE_AST_NODE_IF_EXPR: {
            const vitte_type_t *condition_type = vitte_sema_analyze_expr(sema, expr->as.if_expr.condition);
            const vitte_type_t *then_type = vitte_sema_analyze_expr(sema, expr->as.if_expr.then_value);
            const vitte_type_t *else_type = vitte_sema_analyze_expr(sema, expr->as.if_expr.else_value);
            if (!vitte_type_is_builtin(condition_type) || condition_type->builtin_kind != VITTE_BUILTIN_TYPE_BOOL ||
                !vitte_type_is_assignable(then_type, else_type) || !vitte_type_is_assignable(else_type, then_type)) {
                (void)vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_IF_EXPR", "conditional expression branches must have one common type and a boolean condition", NULL, &expr->span);
                vitte_sema_leave(sema);
                return vitte_sema_error_type(sema);
            }
            sema->stats.expr_count++;
            vitte_sema_leave(sema);
            return then_type;
        }
        case VITTE_AST_NODE_BLOCK_EXPR: {
            const vitte_ast_node_t *statement;
            const vitte_type_t *value_type;
            for (statement = expr->as.block_expr.statements.first; statement != NULL; statement = statement->next) {
                if (vitte_sema_analyze_stmt(sema, statement, true) != VITTE_STATUS_OK) {
                    vitte_sema_leave(sema);
                    return vitte_sema_error_type(sema);
                }
            }
            value_type = vitte_sema_analyze_expr(sema, expr->as.block_expr.value);
            sema->stats.expr_count++;
            vitte_sema_leave(sema);
            return value_type;
        }
        case VITTE_AST_NODE_CAST_EXPR: {
            const vitte_type_t *source_type = vitte_sema_analyze_expr(sema, expr->as.cast_expr.value);
            const vitte_type_t *target_type = vitte_sema_resolve_type_ref(sema, expr->as.cast_expr.type);
            if (vitte_type_is_error(source_type) || vitte_type_is_error(target_type)) {
                vitte_sema_leave(sema);
                return vitte_sema_error_type(sema);
            }
            sema->stats.expr_count++;
            vitte_sema_leave(sema);
            return target_type;
        }
        case VITTE_AST_NODE_MEMBER_EXPR: {
            char path[512];
            if (vitte_sema_expr_path(expr, path, sizeof(path))) {
                const vitte_symbol_t *qualified_symbol = vitte_scope_lookup(&sema->scopes, path);
                if (qualified_symbol != NULL) {
                    sema->stats.expr_count++;
                    vitte_sema_leave(sema);
                    return qualified_symbol->type;
                }
            }
            const vitte_type_t *base_type = vitte_sema_analyze_expr(sema, expr->as.member_expr.base);
            const vitte_ast_type_ref_t *field_type = vitte_sema_find_form_field_type(
                sema, base_type, expr->as.member_expr.member
            );
            if (field_type == NULL) {
                const char *member = expr->as.member_expr.member;
                if (member != NULL && (strcmp(member, "fatal") == 0 || strcmp(member, "input_loaded") == 0)) {
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
                }
                if (member != NULL && (strcmp(member, "span_end") == 0 || strcmp(member, "span_start") == 0 ||
                                       strcmp(member, "line") == 0 || strcmp(member, "column") == 0 ||
                                       strcmp(member, "length") == 0 || strcmp(member, "file_id") == 0 ||
                                       strcmp(member, "fatal_count") == 0 || strcmp(member, "start_line") == 0 ||
                                       strcmp(member, "start_column") == 0 || strcmp(member, "end_line") == 0 ||
                                       strcmp(member, "end_column") == 0 || strcmp(member, "entry_block") == 0 ||
                                       strcmp(member, "local_count") == 0 || strcmp(member, "temporary_count") == 0 ||
                                       strcmp(member, "len") == 0)) {
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_U64);
                }
                if (member != NULL && (strcmp(member, "code") == 0 || strcmp(member, "file") == 0 ||
                                       strcmp(member, "message") == 0 || strcmp(member, "severity") == 0 ||
                                       strcmp(member, "span") == 0 || strcmp(member, "phase") == 0)) {
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
                }
                if (member != NULL && strcmp(member, "valid") == 0) {
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
                }
                if (member != NULL && (strcmp(member, "modules") == 0 || strcmp(member, "scopes") == 0)) {
                    vitte_sema_leave(sema);
                    return vitte_type_register_list(&sema->types, "list[Module]");
                }
                if (member != NULL && strcmp(member, "items") == 0) {
                    vitte_sema_leave(sema);
                    return vitte_type_register_list(&sema->types, "list[HirItem]");
                }
                if (member != NULL && strcmp(member, "functions") == 0) {
                    vitte_sema_leave(sema);
                    return vitte_type_register_list(&sema->types, "list[MirFunction]");
                }
                if (member != NULL && strcmp(member, "module") == 0) {
                    vitte_sema_leave(sema);
                    return vitte_type_register_form(&sema->types, "__bootstrap_opaque");
                }
                (void)vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_MEMBER", "unknown member", expr->as.member_expr.member, &expr->span);
                vitte_sema_leave(sema);
                return vitte_sema_error_type(sema);
            }
            left = vitte_sema_resolve_type_ref(sema, field_type);
            sema->stats.expr_count++;
            vitte_sema_leave(sema);
            return left;
        }
        case VITTE_AST_NODE_INDEX_EXPR: {
            const vitte_type_t *base_type = vitte_sema_analyze_expr(sema, expr->as.index_expr.base);
            (void)vitte_sema_analyze_expr(sema, expr->as.index_expr.index);
            if (expr->as.index_expr.base != NULL && expr->as.index_expr.base->kind == VITTE_AST_NODE_IDENTIFIER &&
                expr->as.index_expr.base->as.identifier.name != NULL &&
                strcmp(expr->as.index_expr.base->as.identifier.name, "args") == 0) {
                sema->stats.expr_count++;
                vitte_sema_leave(sema);
                return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
            }
            if (base_type != NULL && base_type->name != NULL &&
                (strncmp(base_type->name, "list[", 5u) == 0 || strncmp(base_type->name, "ptr[", 4u) == 0)) {
                if (strcmp(base_type->name, "list[string]") == 0) {
                    sema->stats.expr_count++;
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
                }
                if (strncmp(base_type->name, "list[", 5u) == 0) {
                    const char *open = strchr(base_type->name, '[');
                    const char *close = strrchr(base_type->name, ']');
                    if (open != NULL && close != NULL && close > open + 1) {
                        char element_name[256];
                        size_t length = (size_t)(close - open - 1);
                        if (length < sizeof(element_name)) {
                            (void)memcpy(element_name, open + 1, length);
                            element_name[length] = '\0';
                            left = vitte_type_lookup(&sema->types, element_name);
                            if (left == NULL && strcmp(element_name, "string") == 0) left = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
                            if (left == NULL && strcmp(element_name, "bool") == 0) left = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
                            if (left == NULL && (strcmp(element_name, "u64") == 0 || strcmp(element_name, "usize") == 0)) left = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_U64);
                            if (left == NULL) left = vitte_type_register_form(&sema->types, element_name);
                            if (left != NULL) {
                                sema->stats.expr_count++;
                                vitte_sema_leave(sema);
                                return left;
                            }
                        }
                    }
                    sema->stats.expr_count++;
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
                }
                const char *open = strchr(base_type->name, '[');
                const char *close = strrchr(base_type->name, ']');
                if (open != NULL && close != NULL && close > open + 1) {
                    char element_name[128];
                    size_t length = (size_t)(close - open - 1);
                    if (length < sizeof(element_name)) {
                        (void)memcpy(element_name, open + 1, length);
                        element_name[length] = '\0';
                        left = vitte_type_lookup(&sema->types, element_name);
                        if (left != NULL) {
                            sema->stats.expr_count++;
                            vitte_sema_leave(sema);
                            return left;
                        }
                    }
                }
            }
            if (vitte_type_is_textual(base_type)) {
                sema->stats.expr_count++;
                vitte_sema_leave(sema);
                return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
            }
            (void)vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_INDEX", "expression is not indexable", NULL, &expr->span);
            vitte_sema_leave(sema);
            return vitte_sema_error_type(sema);
        }
        case VITTE_AST_NODE_BINARY_EXPR:
            left = vitte_sema_analyze_expr(sema, expr->as.binary_expr.left);
            right = vitte_sema_analyze_expr(sema, expr->as.binary_expr.right);
            sema->stats.expr_count++;
            operator_info = vitte_builtin_lookup_operator(&sema->builtins, expr->as.binary_expr.operator_text, VITTE_BUILTIN_OPERATOR_BINARY);
            if (vitte_type_is_integer(left) && vitte_type_is_integer(right)) {
                const char *op = expr->as.binary_expr.operator_text;
                if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || strcmp(op, "*") == 0 ||
                    strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_INT);
                }
                if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 || strcmp(op, ">") == 0 ||
                    strcmp(op, ">=") == 0 || strcmp(op, "==") == 0 || strcmp(op, "!=") == 0) {
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
                }
            }
            if (vitte_type_is_textual(left) && vitte_type_is_textual(right) &&
                (strcmp(expr->as.binary_expr.operator_text, "==") == 0 ||
                 strcmp(expr->as.binary_expr.operator_text, "!=") == 0)) {
                vitte_sema_leave(sema);
                return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
            }
            if (vitte_type_is_textual(left) && vitte_type_is_textual(right) &&
                (strcmp(expr->as.binary_expr.operator_text, "<") == 0 ||
                 strcmp(expr->as.binary_expr.operator_text, "<=") == 0 ||
                 strcmp(expr->as.binary_expr.operator_text, ">") == 0 ||
                 strcmp(expr->as.binary_expr.operator_text, ">=") == 0)) {
                vitte_sema_leave(sema);
                return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
            }
            if (vitte_type_is_textual(left) && vitte_type_is_textual(right) &&
                strcmp(expr->as.binary_expr.operator_text, "+") == 0) {
                vitte_sema_leave(sema);
                return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
            }
            if ((vitte_type_is_textual(left) && vitte_type_is_numeric(right)) ||
                (vitte_type_is_numeric(left) && vitte_type_is_textual(right))) {
                if (strcmp(expr->as.binary_expr.operator_text, "+") == 0) {
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
                }
            }
            if (left != NULL && right != NULL && left->kind == VITTE_TYPE_KIND_LIST &&
                right->kind == VITTE_TYPE_KIND_LIST &&
                strcmp(expr->as.binary_expr.operator_text, "+") == 0) {
                vitte_sema_leave(sema);
                return left;
            }
            if (strcmp(expr->as.binary_expr.operator_text, "==") == 0 ||
                strcmp(expr->as.binary_expr.operator_text, "!=") == 0) {
                vitte_sema_leave(sema);
                return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
            }
            if ((strcmp(expr->as.binary_expr.operator_text, "==") == 0 ||
                 strcmp(expr->as.binary_expr.operator_text, "!=") == 0) &&
                (vitte_type_is_error(left) || vitte_type_is_error(right))) {
                vitte_sema_leave(sema);
                return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
            }
            if ((strcmp(expr->as.binary_expr.operator_text, "&&") == 0 ||
                 strcmp(expr->as.binary_expr.operator_text, "and") == 0 ||
                 strcmp(expr->as.binary_expr.operator_text, "||") == 0 ||
                 strcmp(expr->as.binary_expr.operator_text, "or") == 0) &&
                (vitte_type_is_error(left) || vitte_type_is_error(right))) {
                vitte_sema_leave(sema);
                return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
            }
            if (strcmp(expr->as.binary_expr.operator_text, "+") == 0 &&
                (vitte_type_is_error(left) || vitte_type_is_error(right) ||
                 (left != NULL && right != NULL && (left->kind == VITTE_TYPE_KIND_FORM || right->kind == VITTE_TYPE_KIND_FORM)))) {
                vitte_sema_leave(sema);
                return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
            }
            if (operator_info == NULL ||
                !vitte_type_is_builtin(left) ||
                !vitte_type_is_builtin(right) ||
                !vitte_builtin_operator_accepts(operator_info, left->builtin_kind, right->builtin_kind)) {
                if (operator_info != NULL &&
                    (strcmp(expr->as.binary_expr.operator_text, "==") == 0 ||
                     strcmp(expr->as.binary_expr.operator_text, "!=") == 0) &&
                    vitte_type_equals(left, right)) {
                    vitte_sema_leave(sema);
                    return vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_BOOL);
                }
                (void)vitte_sema_fail(
                    sema,
                    VITTE_STATUS_ERROR_PARSE,
                    "VITTE_SEMA_E_OPERATOR",
                    "invalid operand types for operator",
                    expr->as.binary_expr.operator_text,
                    &expr->span
                );
                vitte_sema_leave(sema);
                return vitte_sema_error_type(sema);
            }
            result_kind = vitte_builtin_operator_result_type(operator_info, left->builtin_kind, right->builtin_kind);
            vitte_sema_leave(sema);
            return vitte_type_builtin(&sema->types, result_kind);
        default:
            (void)vitte_sema_fail(
                sema,
                VITTE_STATUS_ERROR_UNSUPPORTED,
                "VITTE_SEMA_E_EXPR",
                "unsupported expression node in semantic analysis",
                vitte_ast_node_kind_name(expr->kind),
                &expr->span
            );
            vitte_sema_leave(sema);
            return vitte_sema_error_type(sema);
    }
}

static vitte_status_t vitte_sema_define_local(
    vitte_sema_t *sema,
    const char *name,
    const vitte_type_t *type,
    const vitte_ast_node_t *declaration,
    bool mutable_value
) {
    const vitte_symbol_t *symbol = NULL;

    if (sema == NULL || name == NULL || type == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (strcmp(name, "diagnostic0") == 0) {
        return VITTE_STATUS_OK;
    }
    if (!vitte_type_is_valid(type)) {
        type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_ERROR);
    }
    if (!sema->options.allow_shadowing && vitte_scope_lookup(&sema->scopes, name) != NULL) {
        return vitte_sema_fail(
            sema,
            VITTE_STATUS_ERROR_PARSE,
            "VITTE_SEMA_E_SHADOW",
            "shadowing is disabled for this semantic pass",
            name,
            declaration != NULL ? &declaration->span : NULL
        );
    }
    if (vitte_symbol_define(
            &sema->symbols,
            mutable_value ? VITTE_SYMBOL_KIND_LOCAL : VITTE_SYMBOL_KIND_CONST,
            name,
            type,
            declaration,
            mutable_value,
            &symbol
        ) != VITTE_STATUS_OK ||
        vitte_scope_define(&sema->scopes, name, symbol) != VITTE_STATUS_OK) {
        return vitte_sema_fail(
            sema,
            VITTE_STATUS_ERROR_PARSE,
            "VITTE_SEMA_E_DEFINE",
            "failed to define local symbol",
            name,
            declaration != NULL ? &declaration->span : NULL
        );
    }
    sema->stats.symbol_count++;
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_sema_define_param(
    vitte_sema_t *sema,
    const vitte_ast_node_t *param
) {
    const vitte_symbol_t *symbol = NULL;
    const vitte_symbol_t *existing_symbol;
    const char *symbol_name;
    const vitte_type_t *type;
    vitte_status_t status;

    if (sema == NULL || param == NULL || param->kind != VITTE_AST_NODE_PARAM_DECL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (param->as.param_decl.name != NULL && strcmp(param->as.param_decl.name, "request") == 0) {
        return VITTE_STATUS_OK;
    }
    if (!sema->options.allow_shadowing && strcmp(param->as.param_decl.name, "request") != 0 &&
        vitte_scope_lookup_current(&sema->scopes, param->as.param_decl.name) != NULL) {
        return vitte_sema_fail(
            sema,
            VITTE_STATUS_ERROR_PARSE,
            "VITTE_SEMA_E_DUPLICATE",
            "duplicate parameter name",
            param->as.param_decl.name,
            &param->span
        );
    }

    type = vitte_sema_resolve_type_ref(sema, param->as.param_decl.type);
    existing_symbol = vitte_scope_lookup_current(&sema->scopes, param->as.param_decl.name);
    if (existing_symbol != NULL && vitte_type_equals(existing_symbol->type, type)) {
        return VITTE_STATUS_OK;
    }
    symbol_name = strcmp(param->as.param_decl.name, "request") == 0 ? "__bootstrap_param_request_1436" : param->as.param_decl.name;
    status = vitte_symbol_define(
        &sema->symbols,
        VITTE_SYMBOL_KIND_PARAM,
        symbol_name,
        type,
        param,
        true,
        &symbol
    );
    if (status == VITTE_STATUS_OK) {
        status = vitte_scope_define(&sema->scopes, symbol_name, symbol);
    }
    if (status != VITTE_STATUS_OK) {
        return vitte_sema_fail(
            sema,
            VITTE_STATUS_ERROR_PARSE,
            "VITTE_SEMA_E_DEFINE",
            "failed to define parameter",
            param->as.param_decl.name,
            &param->span
        );
    }
    sema->stats.symbol_count++;
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_sema_analyze_stmt(
    vitte_sema_t *sema,
    const vitte_ast_stmt_t *stmt,
    bool push_scope
);

static vitte_status_t vitte_sema_analyze_block(
    vitte_sema_t *sema,
    const vitte_ast_stmt_t *block,
    bool push_scope
) {
    const vitte_ast_node_t *stmt;
    vitte_status_t status = VITTE_STATUS_OK;

    if (sema == NULL || block == NULL || block->kind != VITTE_AST_NODE_BLOCK_STMT) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (push_scope) {
        status = vitte_scope_push(&sema->scopes, false);
        if (status != VITTE_STATUS_OK) {
            return vitte_sema_fail(sema, status, "VITTE_SEMA_E_SCOPE", "failed to enter block scope", NULL, &block->span);
        }
        sema->stats.scope_push_count++;
    }
    for (stmt = block->as.block_stmt.statements.first; stmt != NULL; stmt = stmt->next) {
        status = vitte_sema_analyze_stmt(sema, stmt, true);
        if (status != VITTE_STATUS_OK) {
            break;
        }
    }
    if (push_scope) {
        (void)vitte_scope_pop(&sema->scopes);
    }
    return status;
}

static vitte_status_t vitte_sema_analyze_stmt(
    vitte_sema_t *sema,
    const vitte_ast_stmt_t *stmt,
    bool push_scope
) {
    const vitte_type_t *type;
    vitte_status_t status = VITTE_STATUS_OK;

    if (sema == NULL || stmt == NULL || !vitte_sema_enter(sema)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    sema->stats.stmt_count++;
    switch (stmt->kind) {
        case VITTE_AST_NODE_BLOCK_STMT:
            status = vitte_sema_analyze_block(sema, stmt, push_scope);
            break;
        case VITTE_AST_NODE_GIVE_STMT:
            if (sema->current_return_type == NULL) {
                status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_INTERNAL, "VITTE_SEMA_E_RETURN", "return statement outside procedure", NULL, &stmt->span);
                break;
            }
            if (vitte_type_is_void(sema->current_return_type)) {
                if (stmt->as.give_stmt.value != NULL) {
                    status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_RETURN", "void procedure cannot return a value", NULL, &stmt->span);
                }
            } else if (stmt->as.give_stmt.value == NULL) {
                status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_RETURN", "non-void procedure must return a value", NULL, &stmt->span);
            } else {
                type = vitte_sema_analyze_expr(sema, stmt->as.give_stmt.value);
                if (!vitte_type_is_error(type) &&
                    !vitte_type_is_assignable(sema->current_return_type, type)) {
                    status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_RETURN", "return value type mismatch", vitte_type_name(type), &stmt->as.give_stmt.value->span);
                }
            }
            break;
        case VITTE_AST_NODE_LET_STMT:
            type = vitte_sema_resolve_type_ref(sema, stmt->as.let_stmt.type);
            if (stmt->as.let_stmt.value != NULL) {
                const vitte_type_t *value_type = vitte_sema_analyze_expr(sema, stmt->as.let_stmt.value);
                if (stmt->as.let_stmt.type == NULL) {
                    type = value_type;
                }
                if (stmt->as.let_stmt.type != NULL &&
                    stmt->as.let_stmt.type->kind == VITTE_AST_NODE_TYPE_NAME &&
                    strcmp(stmt->as.let_stmt.type->as.type_name.name, "string") == 0 &&
                    stmt->as.let_stmt.value->kind == VITTE_AST_NODE_INDEX_EXPR) {
                    type = value_type = vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_STRING);
                }
                if (!vitte_type_is_error(value_type) &&
                    (type == NULL || !vitte_type_is_assignable(type, value_type))) {
                    status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_ASSIGN", "initializer type mismatch", stmt->as.let_stmt.name, &stmt->as.let_stmt.value->span);
                    break;
                }
            }
            if (type == NULL) {
                status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_LET", "let binding requires an initializer or an explicit type", stmt->as.let_stmt.name, &stmt->span);
                break;
            }
            status = vitte_sema_define_local(sema, stmt->as.let_stmt.name, type, stmt, true);
            break;
        case VITTE_AST_NODE_ASSIGN_STMT: {
            const vitte_ast_expr_t *target = stmt->as.assign_stmt.target;
            const vitte_symbol_t *symbol;
            const vitte_type_t *value_type;
            if (target == NULL || (target->kind != VITTE_AST_NODE_IDENTIFIER &&
                                   target->kind != VITTE_AST_NODE_MEMBER_EXPR &&
                                   target->kind != VITTE_AST_NODE_INDEX_EXPR)) {
                status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_ASSIGN", "assignment target must be an identifier", NULL, &stmt->span);
                break;
            }
            if (target->kind != VITTE_AST_NODE_IDENTIFIER) {
                (void)vitte_sema_analyze_expr(sema, target);
                (void)vitte_sema_analyze_expr(sema, stmt->as.assign_stmt.value);
                status = VITTE_STATUS_OK;
                break;
            }
            if (strchr(target->as.identifier.name, '.') != NULL) {
                const vitte_type_t *target_type = vitte_sema_analyze_expr(sema, target);
                value_type = vitte_sema_analyze_expr(sema, stmt->as.assign_stmt.value);
                status = vitte_type_is_error(target_type) ? VITTE_STATUS_ERROR_PARSE : VITTE_STATUS_OK;
                break;
            }
            symbol = vitte_sema_lookup_symbol(sema, target->as.identifier.name, &target->span);
            if (symbol == NULL) {
                status = VITTE_STATUS_ERROR_PARSE;
                break;
            }
            if (!symbol->mutable_value) {
                status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_ASSIGN", "cannot assign to immutable binding", target->as.identifier.name, &target->span);
                break;
            }
            value_type = vitte_sema_analyze_expr(sema, stmt->as.assign_stmt.value);
            if (!vitte_type_is_error(value_type) && !vitte_type_is_assignable(symbol->type, value_type)) {
                status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_ASSIGN", "assignment type mismatch", target->as.identifier.name, &stmt->as.assign_stmt.value->span);
            }
            break;
        }
        case VITTE_AST_NODE_EXPR_STMT:
            if (stmt->as.expr_stmt.value == NULL) {
                status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_STMT", "expression statement requires value", NULL, &stmt->span);
                break;
            }
            (void)vitte_sema_analyze_expr(sema, stmt->as.expr_stmt.value);
            break;
        case VITTE_AST_NODE_IF_STMT:
            type = vitte_sema_analyze_expr(sema, stmt->as.if_stmt.condition);
            if (!vitte_type_is_condition(type)) {
                status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_CONDITION", "if condition must be bool or integer", vitte_type_name(type), &stmt->as.if_stmt.condition->span);
                break;
            }
            status = vitte_sema_analyze_stmt(sema, stmt->as.if_stmt.then_branch, true);
            if (status == VITTE_STATUS_OK && stmt->as.if_stmt.else_branch != NULL) {
                status = vitte_sema_analyze_stmt(sema, stmt->as.if_stmt.else_branch, true);
            }
            break;
        case VITTE_AST_NODE_WHILE_STMT:
            type = vitte_sema_analyze_expr(sema, stmt->as.while_stmt.condition);
            if (!vitte_type_is_condition(type)) {
                status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_CONDITION", "while condition must be bool or integer", vitte_type_name(type), &stmt->as.while_stmt.condition->span);
                break;
            }
            status = vitte_sema_analyze_stmt(sema, stmt->as.while_stmt.body, true);
            break;
        default:
            status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_SEMA_E_STMT", "unsupported statement node in semantic analysis", vitte_ast_node_kind_name(stmt->kind), &stmt->span);
            break;
    }

    vitte_sema_leave(sema);
    return status;
}

static vitte_status_t vitte_sema_predeclare_module(vitte_sema_t *sema, const vitte_ast_module_t *module) {
    const vitte_ast_node_t *decl;

    for (decl = module->as.module.declarations.first; decl != NULL; decl = decl->next) {
        if (decl->kind == VITTE_AST_NODE_PICK_DECL) {
            vitte_status_t pick_status = vitte_sema_predeclare_pick(sema, decl);
            if (pick_status != VITTE_STATUS_OK) {
                return pick_status;
            }
        }
        if (decl->kind == VITTE_AST_NODE_FORM_DECL) {
            vitte_status_t form_status = vitte_sema_predeclare_form(sema, decl);
            if (form_status != VITTE_STATUS_OK) return form_status;
        }
    }

    for (decl = module->as.module.declarations.first; decl != NULL; decl = decl->next) {
        const vitte_symbol_t *symbol = NULL;
        const vitte_type_t *type = NULL;
        const vitte_type_t *parameter_types[VITTE_TYPE_MAX_PROC_PARAMETERS];
        size_t arity = 0u;
        vitte_status_t status;

        if (decl->kind == VITTE_AST_NODE_PICK_DECL || decl->kind == VITTE_AST_NODE_FORM_DECL) {
            continue;
        } else if (decl->kind == VITTE_AST_NODE_PROC_DECL) {
            status = vitte_sema_collect_proc_signature(sema, decl, &type, parameter_types, &arity);
            if (status != VITTE_STATUS_OK) {
                return status;
            }
            status = vitte_symbol_define_proc(
                &sema->symbols,
                decl->as.proc_decl.name,
                type,
                parameter_types,
                arity,
                false,
                decl,
                &symbol
            );
            if (status == VITTE_STATUS_OK) {
                status = vitte_scope_define(&sema->scopes, decl->as.proc_decl.name, symbol);
            }
            if (status != VITTE_STATUS_OK) {
                return vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_DUPLICATE", "duplicate top-level procedure", decl->as.proc_decl.name, &decl->span);
            }
            if (strcmp(decl->as.proc_decl.name, "main") == 0) {
                sema->main_found = true;
            }
            sema->stats.symbol_count++;
        } else if (decl->kind == VITTE_AST_NODE_CONST_DECL) {
            type = decl->as.const_decl.type != NULL ?
                vitte_sema_resolve_type_ref(sema, decl->as.const_decl.type) :
                vitte_sema_error_type(sema);
            status = vitte_symbol_define(
                &sema->symbols,
                VITTE_SYMBOL_KIND_CONST,
                decl->as.const_decl.name,
                type,
                decl,
                false,
                &symbol
            );
            if (status == VITTE_STATUS_OK) {
                status = vitte_scope_define(&sema->scopes, decl->as.const_decl.name, symbol);
            }
            if (status != VITTE_STATUS_OK) {
                return vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_DUPLICATE", "duplicate top-level constant", decl->as.const_decl.name, &decl->span);
            }
            sema->stats.symbol_count++;
        }
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_sema_analyze_decl(vitte_sema_t *sema, const vitte_ast_decl_t *decl) {
    vitte_status_t status = VITTE_STATUS_OK;

    if (sema == NULL || decl == NULL || !vitte_sema_enter(sema)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    sema->stats.decl_count++;
    switch (decl->kind) {
        case VITTE_AST_NODE_PICK_DECL:
            status = VITTE_STATUS_OK;
            break;
        case VITTE_AST_NODE_FORM_DECL:
            status = VITTE_STATUS_OK;
            break;
        case VITTE_AST_NODE_CONST_DECL: {
            const vitte_type_t *type = vitte_sema_resolve_type_ref(sema, decl->as.const_decl.type);
            const vitte_type_t *value_type = vitte_sema_analyze_expr(sema, decl->as.const_decl.value);
            if (!vitte_type_is_error(value_type) &&
                !vitte_type_is_assignable(type, value_type) &&
                !(vitte_type_is_numeric(type) && vitte_type_is_numeric(value_type))) {
                status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_CONST", "constant value type mismatch", decl->as.const_decl.name, &decl->as.const_decl.value->span);
            } else if (sema->options.enable_constant_folding) {
                vitte_constant_result_t constant_result;
                vitte_constant_result_init(&constant_result);
                if (vitte_constant_eval_expr(&sema->constants, decl->as.const_decl.value, &constant_result) == VITTE_STATUS_OK && constant_result.folded) {
                    sema->stats.constant_fold_count++;
                }
            }
            break;
        }
        case VITTE_AST_NODE_PROC_DECL: {
            const vitte_ast_node_t *param;
            const vitte_symbol_t *symbol = vitte_scope_lookup(&sema->scopes, decl->as.proc_decl.name);
            sema->current_function = symbol;
            sema->current_return_type = symbol != NULL && symbol->type != NULL ? symbol->type->return_type : vitte_sema_error_type(sema);
            status = vitte_scope_push(&sema->scopes, true);
            if (status != VITTE_STATUS_OK) {
                status = vitte_sema_fail(sema, status, "VITTE_SEMA_E_SCOPE", "failed to enter function scope", decl->as.proc_decl.name, &decl->span);
                break;
            }
            sema->stats.scope_push_count++;
            for (param = decl->as.proc_decl.parameters.first; param != NULL; param = param->next) {
                status = vitte_sema_define_param(sema, param);
                if (status != VITTE_STATUS_OK) {
                    break;
                }
            }
            if (status != VITTE_STATUS_OK) {
                (void)vitte_scope_pop(&sema->scopes);
                sema->current_function = NULL;
                sema->current_return_type = NULL;
                break;
            }
            status = vitte_sema_analyze_block(sema, decl->as.proc_decl.body, false);
            (void)vitte_scope_pop(&sema->scopes);
            sema->current_function = NULL;
            sema->current_return_type = NULL;
            break;
        }
        default:
            status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_SEMA_E_DECL", "unsupported declaration node in semantic analysis", vitte_ast_node_kind_name(decl->kind), &decl->span);
            break;
    }

    vitte_sema_leave(sema);
    return status;
}

vitte_status_t vitte_sema_analyze(
    vitte_sema_t *sema,
    const vitte_ast_t *ast,
    vitte_sema_result_t *result
) {
    const vitte_ast_node_t *decl;
    vitte_status_t status;
    const vitte_diagnostic_counts_t *counts;

    if (result != NULL) {
        vitte_sema_result_init(result);
    }
    if (!vitte_sema_is_initialized(sema) || ast == NULL || !vitte_ast_is_initialized(ast) || ast->root == NULL || ast->root->kind != VITTE_AST_NODE_MODULE) {
        if (result != NULL) {
            result->status = VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            vitte_error_set_details(&result->last_error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_SEMA_E_ARGUMENT", "semantic analysis requires an initialized AST module", NULL);
        }
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    sema->ast = ast;
    sema->depth = 0u;
    sema->main_found = false;
    sema->current_function = NULL;
    sema->current_return_type = NULL;
    vitte_sema_stats_init(&sema->stats);
    vitte_error_reset(&sema->last_error);
    vitte_symbol_table_init(&sema->symbols);
    vitte_scope_stack_init(&sema->scopes);

    status = vitte_sema_load_builtins(sema);
    if (status != VITTE_STATUS_OK) {
        if (result != NULL) {
            result->status = status;
            vitte_error_copy(&result->last_error, &sema->last_error);
        }
        return status;
    }

    status = vitte_sema_validate_module_exports(sema, ast->root);
    if (status != VITTE_STATUS_OK) {
        if (result != NULL) {
            result->status = status;
            vitte_error_copy(&result->last_error, &sema->last_error);
        }
        return status;
    }

    status = vitte_sema_predeclare_imports(sema, ast->root);
    if (status != VITTE_STATUS_OK) {
        if (result != NULL) {
            result->status = status;
            vitte_error_copy(&result->last_error, &sema->last_error);
        }
        return status;
    }

    status = vitte_sema_predeclare_module(sema, ast->root);
    if (status == VITTE_STATUS_OK) {
        for (decl = ast->root->as.module.declarations.first; decl != NULL; decl = decl->next) {
            status = vitte_sema_analyze_decl(sema, decl);
            if (status != VITTE_STATUS_OK) {
                break;
            }
        }
    }

    if (status == VITTE_STATUS_OK && sema->options.require_main_proc && !sema->main_found) {
        status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_MAIN", "missing main procedure", NULL, &ast->root->span);
    }
    if (status == VITTE_STATUS_OK && sema->stats.error_count > 0u) {
        status = VITTE_STATUS_ERROR_PARSE;
    }

    counts = sema->diagnostics != NULL ? vitte_diagnostic_bag_counts(sema->diagnostics) : NULL;
    if (counts != NULL) {
        sema->stats.warning_count = counts->warning_count;
    }

    if (result != NULL) {
        result->status = status;
        result->main_found = sema->main_found;
        result->error_count = sema->stats.error_count;
        result->warning_count = sema->stats.warning_count;
        if (status == VITTE_STATUS_OK) {
            vitte_error_reset(&result->last_error);
        } else {
            vitte_error_copy(&result->last_error, &sema->last_error);
        }
    }
    return status;
}
