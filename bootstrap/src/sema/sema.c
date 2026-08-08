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

static const vitte_symbol_t *vitte_sema_lookup_symbol(
    vitte_sema_t *sema,
    const char *name,
    const vitte_ast_span_t *span
) {
    const vitte_symbol_t *symbol = vitte_scope_lookup(&sema->scopes, name);
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
        callee_symbol = vitte_sema_lookup_symbol(sema, expr->as.call_expr.callee->as.identifier.name, &expr->as.call_expr.callee->span);
    }
    callee_type = vitte_sema_analyze_expr(sema, expr->as.call_expr.callee);

    for (argument = expr->as.call_expr.arguments.first; argument != NULL; argument = argument->next) {
        (void)vitte_sema_analyze_expr(sema, argument);
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
            const vitte_type_t *argument_type = vitte_sema_analyze_expr(sema, expr->as.call_expr.arguments.first);
            if (parameter_type != NULL && !vitte_type_is_assignable(parameter_type, argument_type)) {
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
                if (!vitte_type_is_assignable(sema->current_return_type, type)) {
                    status = vitte_sema_fail(sema, VITTE_STATUS_ERROR_PARSE, "VITTE_SEMA_E_RETURN", "return value type mismatch", vitte_type_name(type), &stmt->as.give_stmt.value->span);
                }
            }
            break;
        case VITTE_AST_NODE_LET_STMT:
            type = vitte_sema_resolve_type_ref(sema, stmt->as.let_stmt.type);
            if (stmt->as.let_stmt.value != NULL) {
                const vitte_type_t *value_type = vitte_sema_analyze_expr(sema, stmt->as.let_stmt.value);
                if (!vitte_type_is_assignable(type, value_type)) {
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
                0u,
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
            if (!vitte_type_is_assignable(type, value_type)) {
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
            const vitte_symbol_t *symbol = vitte_scope_lookup(&sema->scopes, decl->as.proc_decl.name);
            sema->current_function = symbol;
            sema->current_return_type = symbol != NULL && symbol->type != NULL ? symbol->type->return_type : vitte_sema_error_type(sema);
            status = vitte_scope_push(&sema->scopes, true);
            if (status != VITTE_STATUS_OK) {
                status = vitte_sema_fail(sema, status, "VITTE_SEMA_E_SCOPE", "failed to enter function scope", decl->as.proc_decl.name, &decl->span);
                break;
            }
            sema->stats.scope_push_count++;
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
