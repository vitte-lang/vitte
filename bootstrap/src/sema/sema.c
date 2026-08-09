#include "sema.h"

#include <string.h>

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

static const vitte_ast_decl_t *vitte_sema_find_module_decl(
    const vitte_ast_module_t *module,
    const char *name
) {
    const vitte_ast_node_t *decl;

    if (module == NULL || name == NULL) {
        return NULL;
    }
    for (decl = module->as.module.declarations.first; decl != NULL; decl = decl->next) {
        if ((decl->kind == VITTE_AST_NODE_PROC_DECL && decl->as.proc_decl.name != NULL &&
                strcmp(decl->as.proc_decl.name, name) == 0) ||
            (decl->kind == VITTE_AST_NODE_CONST_DECL && decl->as.const_decl.name != NULL &&
                strcmp(decl->as.const_decl.name, name) == 0)) {
            return decl;
        }
    }
    return NULL;
}

static bool vitte_sema_decl_is_exported(const vitte_ast_decl_t *decl) {
    if (decl == NULL) {
        return false;
    }
    if (decl->kind == VITTE_AST_NODE_PROC_DECL) {
        return decl->as.proc_decl.exported;
    }
    if (decl->kind == VITTE_AST_NODE_CONST_DECL) {
        return decl->as.const_decl.exported;
    }
    return false;
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
    vitte_status_t status;

    if (sema == NULL || visible_name == NULL || decl == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    existing_symbol = vitte_scope_lookup_current(&sema->scopes, visible_name);
    if (existing_symbol != NULL) {
        if (existing_symbol->declaration == decl) {
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
        type = decl->as.proc_decl.return_type != NULL ?
            vitte_sema_resolve_type_ref(sema, decl->as.proc_decl.return_type) :
            vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_VOID);
        status = vitte_symbol_define_proc(
            &sema->symbols,
            visible_name,
            type,
            decl->as.proc_decl.parameters.count,
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
            const vitte_ast_node_t *decl;

            for (decl = imported_module->as.module.declarations.first; decl != NULL; decl = decl->next) {
                const char *decl_name = decl->kind == VITTE_AST_NODE_PROC_DECL ? decl->as.proc_decl.name :
                    decl->kind == VITTE_AST_NODE_CONST_DECL ? decl->as.const_decl.name : NULL;
                char *qualified_name;

                if (decl_name == NULL || !vitte_sema_decl_is_exported(decl)) {
                    continue;
                }
                qualified_name = vitte_sema_join_qualified_name(sema, prefix, decl_name);
                if (qualified_name == NULL) {
                    return vitte_sema_fail(
                        sema,
                        VITTE_STATUS_ERROR_OUT_OF_MEMORY,
                        "VITTE_SEMA_E_IMPORT",
                        "failed to allocate imported namespace symbol",
                        prefix,
                        &import_decl->span
                    );
                }
                if (vitte_sema_define_imported_decl(sema, qualified_name, decl, &import_decl->span) != VITTE_STATUS_OK) {
                    return sema->last_error.status;
                }
            }
        } else if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_GLOB) {
            const vitte_ast_node_t *decl;

            for (decl = imported_module->as.module.declarations.first; decl != NULL; decl = decl->next) {
                const char *decl_name = decl->kind == VITTE_AST_NODE_PROC_DECL ? decl->as.proc_decl.name :
                    decl->kind == VITTE_AST_NODE_CONST_DECL ? decl->as.const_decl.name : NULL;

                if (decl_name == NULL || !vitte_sema_decl_is_exported(decl)) {
                    continue;
                }
                if (vitte_sema_define_imported_decl(sema, decl_name, decl, &import_decl->span) != VITTE_STATUS_OK) {
                    return sema->last_error.status;
                }
            }
        } else {
            const vitte_ast_decl_t *decl = vitte_sema_find_module_decl(imported_module, leaf_name);
            const char *visible_name = import_decl->as.import_decl.alias != NULL ?
                import_decl->as.import_decl.alias :
                leaf_name;

            if (decl == NULL) {
                return vitte_sema_fail(
                    sema,
                    VITTE_STATUS_ERROR_PARSE,
                    "VITTE_SEMA_E_IMPORT",
                    "imported symbol was not found in module",
                    path,
                    &import_decl->span
                );
            }
            if (!vitte_sema_decl_is_exported(decl)) {
                return vitte_sema_fail(
                    sema,
                    VITTE_STATUS_ERROR_PARSE,
                    "VITTE_SEMA_E_IMPORT_PRIVATE",
                    "imported symbol is not exported by module",
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
    if (type == NULL) {
        (void)vitte_sema_fail(
            sema,
            VITTE_STATUS_ERROR_PARSE,
            "VITTE_SEMA_E_TYPE",
            "unknown type name",
            type_ref != NULL && type_ref->kind == VITTE_AST_NODE_TYPE_NAME ? type_ref->as.type_name.name : NULL,
            type_ref != NULL ? &type_ref->span : NULL
        );
        return vitte_sema_error_type(sema);
    }
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

    if (symbol == NULL && segment != NULL && segment != name) {
        symbol = vitte_scope_lookup(&sema->scopes, segment);
    }
    if (symbol == NULL) {
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

static const vitte_type_t *vitte_sema_analyze_expr(
    vitte_sema_t *sema,
    const vitte_ast_expr_t *expr
);

static const vitte_type_t *vitte_sema_analyze_call(
    vitte_sema_t *sema,
    const vitte_ast_expr_t *expr
) {
    const vitte_ast_node_t *argument;
    size_t arity = 0u;
    const vitte_symbol_t *callee_symbol = NULL;
    const vitte_type_t *callee_type;

    if (sema == NULL || expr == NULL) {
        return vitte_sema_error_type(sema);
    }

    sema->stats.expr_count++;
    if (expr->as.call_expr.callee != NULL && expr->as.call_expr.callee->kind == VITTE_AST_NODE_IDENTIFIER) {
        callee_symbol = vitte_sema_lookup_symbol(
            sema,
            expr->as.call_expr.callee->as.identifier.name,
            &expr->as.call_expr.callee->span
        );
        callee_type = callee_symbol != NULL ? callee_symbol->type : vitte_sema_error_type(sema);
    } else {
        callee_type = vitte_sema_analyze_expr(sema, expr->as.call_expr.callee);
    }

    for (argument = expr->as.call_expr.arguments.first; argument != NULL; argument = argument->next) {
        (void)vitte_sema_analyze_expr(sema, argument);
        arity++;
    }

    if (vitte_type_is_error(callee_type)) {
        return vitte_sema_error_type(sema);
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
            const vitte_type_t *argument_type = vitte_sema_analyze_expr(sema, expr->as.call_expr.arguments.first);
            if (parameter_type != NULL &&
                !vitte_type_is_error(argument_type) &&
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
    } else if (arity != callee_type->arity && !callee_type->variadic) {
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
        case VITTE_AST_NODE_IDENTIFIER: {
            const vitte_symbol_t *symbol = vitte_sema_lookup_symbol(sema, expr->as.identifier.name, &expr->span);
            sema->stats.expr_count++;
            vitte_sema_leave(sema);
            return symbol != NULL ? symbol->type : vitte_sema_error_type(sema);
        }
        case VITTE_AST_NODE_CALL_EXPR: {
            const vitte_type_t *result = vitte_sema_analyze_call(sema, expr);
            vitte_sema_leave(sema);
            return result;
        }
        case VITTE_AST_NODE_BINARY_EXPR:
            left = vitte_sema_analyze_expr(sema, expr->as.binary_expr.left);
            right = vitte_sema_analyze_expr(sema, expr->as.binary_expr.right);
            sema->stats.expr_count++;
            operator_info = vitte_builtin_lookup_operator(&sema->builtins, expr->as.binary_expr.operator_text, VITTE_BUILTIN_OPERATOR_BINARY);
            if (operator_info == NULL ||
                !vitte_type_is_builtin(left) ||
                !vitte_type_is_builtin(right) ||
                !vitte_builtin_operator_accepts(operator_info, left->builtin_kind, right->builtin_kind)) {
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
    const vitte_type_t *type;
    vitte_status_t status;

    if (sema == NULL || param == NULL || param->kind != VITTE_AST_NODE_PARAM_DECL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!sema->options.allow_shadowing && vitte_scope_lookup_current(&sema->scopes, param->as.param_decl.name) != NULL) {
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
    status = vitte_symbol_define(
        &sema->symbols,
        VITTE_SYMBOL_KIND_PARAM,
        param->as.param_decl.name,
        type,
        param,
        param->as.param_decl.mutable_value,
        &symbol
    );
    if (status == VITTE_STATUS_OK) {
        status = vitte_scope_define(&sema->scopes, param->as.param_decl.name, symbol);
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
                if (!vitte_type_is_error(value_type) &&
                    !vitte_type_is_assignable(type, value_type)) {
                    status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_ASSIGN", "initializer type mismatch", stmt->as.let_stmt.name, &stmt->as.let_stmt.value->span);
                    break;
                }
            }
            status = vitte_sema_define_local(sema, stmt->as.let_stmt.name, type, stmt, true);
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
        const vitte_symbol_t *symbol = NULL;
        const vitte_type_t *type = NULL;
        vitte_status_t status;

        if (decl->kind == VITTE_AST_NODE_PROC_DECL) {
            type = decl->as.proc_decl.return_type != NULL ?
                vitte_sema_resolve_type_ref(sema, decl->as.proc_decl.return_type) :
                vitte_type_builtin(&sema->types, VITTE_BUILTIN_TYPE_VOID);
            status = vitte_symbol_define_proc(
                &sema->symbols,
                decl->as.proc_decl.name,
                type,
                decl->as.proc_decl.parameters.count,
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
        case VITTE_AST_NODE_CONST_DECL: {
            const vitte_type_t *type = vitte_sema_resolve_type_ref(sema, decl->as.const_decl.type);
            const vitte_type_t *value_type = vitte_sema_analyze_expr(sema, decl->as.const_decl.value);
            if (!vitte_type_is_error(value_type) &&
                !vitte_type_is_assignable(type, value_type)) {
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
