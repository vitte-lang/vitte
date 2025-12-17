#include "vitte/desugar_phrase.h"
#include <stdlib.h>
#include <string.h>

static void set_err(vitte_error* err, vitte_error_code code, uint32_t line, uint32_t col, const char* msg) {
    if (!err) return;
    err->code = code;
    err->line = line;
    err->col = col;
    if (msg) {
        strncpy(err->message, msg, sizeof(err->message) - 1);
        err->message[sizeof(err->message) - 1] = '\0';
    }
}

static void ast_add_child(vitte_ast* parent, vitte_ast* child) {
    if (!parent || !child) return;
    if (!parent->first_child) {
        parent->first_child = child;
        parent->last_child = child;
    } else {
        parent->last_child->next = child;
        parent->last_child = child;
    }
}

static char* dup_cstr(const char* in) {
    if (!in) return NULL;
    size_t len = strlen(in);
    char* copy = (char*)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, in, len);
    copy[len] = '\0';
    return copy;
}

static vitte_ast* ast_clone_header(const vitte_ast* node, vitte_ast_kind kind) {
    vitte_ast* out = (vitte_ast*)calloc(1, sizeof(vitte_ast));
    if (!out) return NULL;
    if (node) {
        out->span = node->span;
        out->literal = node->literal;
        out->binary_op = node->binary_op;
        out->unary_op = node->unary_op;
        if (node->text) {
            out->text = dup_cstr(node->text);
            if (!out->text) {
                free(out);
                return NULL;
            }
        }
        if (node->aux_text) {
            out->aux_text = dup_cstr(node->aux_text);
            if (!out->aux_text) {
                free(out->text);
                free(out);
                return NULL;
            }
        }
    }
    out->kind = kind;
    return out;
}

static vitte_ast* clone_subtree(const vitte_ast* node) {
    if (!node) return NULL;
    vitte_ast* copy = ast_clone_header(node, node->kind);
    if (!copy) return NULL;
    const vitte_ast* child = node->first_child;
    while (child) {
        vitte_ast* child_copy = clone_subtree(child);
        if (!child_copy) {
            vitte_ast_free(NULL, copy);
            return NULL;
        }
        ast_add_child(copy, child_copy);
        child = child->next;
    }
    return copy;
}

static vitte_ast* desugar_node(const vitte_ast* node, vitte_error* err);
static vitte_ast* desugar_block(const vitte_ast* block, vitte_error* err);
static vitte_ast* desugar_stmt(const vitte_ast* stmt, vitte_error* err);

static vitte_ast* build_call(const char* callee,
                             const vitte_ast* arg_list,
                             vitte_error* err) {
    (void)err;
    vitte_ast* call = (vitte_ast*)calloc(1, sizeof(vitte_ast));
    if (!call) return NULL;
    call->kind = VITTE_AST_EXPR_CALL;
    if (callee) {
        call->text = dup_cstr(callee);
        if (!call->text) {
            vitte_ast_free(NULL, call);
            return NULL;
        }
    }
    const vitte_ast* it = arg_list;
    while (it) {
        const vitte_ast* expr_node = (it->kind == VITTE_AST_EXPR_ARG) ? it->first_child : it;
        vitte_ast* cloned = clone_subtree(expr_node);
        if (!cloned) {
            vitte_ast_free(NULL, call);
            return NULL;
        }
        ast_add_child(call, cloned);
        it = it->next;
    }
    return call;
}

static vitte_ast* desugar_block(const vitte_ast* block, vitte_error* err) {
    vitte_ast* out = ast_clone_header(block, VITTE_AST_BLOCK);
    if (!out) return NULL;
    out->first_child = out->last_child = NULL;
    const vitte_ast* stmt = block->first_child;
    while (stmt) {
        vitte_ast* lowered = desugar_stmt(stmt, err);
        if (!lowered) {
            vitte_ast_free(NULL, out);
            return NULL;
        }
        ast_add_child(out, lowered);
        stmt = stmt->next;
    }
    return out;
}

static vitte_ast* desugar_set_stmt(const vitte_ast* stmt, vitte_error* err) {
    const vitte_ast* target = stmt->first_child;
    const vitte_ast* value = target ? target->next : NULL;
    if (!target || !value) {
        set_err(err, VITTE_ERRC_SYNTAX, stmt->span.start.line, stmt->span.start.col, "set requires target and value");
        return NULL;
    }
    vitte_ast* node = ast_clone_header(stmt, VITTE_AST_CORE_STMT_ASSIGN);
    if (!node) return NULL;
    node->first_child = node->last_child = NULL;
    vitte_ast* target_copy = clone_subtree(target);
    vitte_ast* value_copy = clone_subtree(value);
    if (!target_copy || !value_copy) {
        vitte_ast_free(NULL, target_copy);
        vitte_ast_free(NULL, value_copy);
        vitte_ast_free(NULL, node);
        return NULL;
    }
    ast_add_child(node, target_copy);
    ast_add_child(node, value_copy);
    return node;
}

static vitte_ast* desugar_say_stmt(const vitte_ast* stmt, vitte_error* err) {
    const vitte_ast* expr = stmt->first_child;
    if (!expr) {
        set_err(err, VITTE_ERRC_SYNTAX, stmt->span.start.line, stmt->span.start.col, "say requires expression");
        return NULL;
    }
    vitte_ast* call = build_call("say", expr, err);
    if (!call) return NULL;
    vitte_ast* node = ast_clone_header(stmt, VITTE_AST_CORE_STMT_EXPR);
    if (!node) {
        vitte_ast_free(NULL, call);
        return NULL;
    }
    ast_add_child(node, call);
    return node;
}

static vitte_ast* desugar_do_stmt(const vitte_ast* stmt, vitte_error* err) {
    if (!stmt->text) {
        set_err(err, VITTE_ERRC_SYNTAX, stmt->span.start.line, stmt->span.start.col, "do requires callee");
        return NULL;
    }
    vitte_ast* call = build_call(stmt->text, stmt->first_child, err);
    if (!call) return NULL;
    vitte_ast* node = ast_clone_header(stmt, VITTE_AST_CORE_STMT_EXPR);
    if (!node) {
        vitte_ast_free(NULL, call);
        return NULL;
    }
    ast_add_child(node, call);
    return node;
}

static vitte_ast* desugar_ret_stmt(const vitte_ast* stmt) {
    vitte_ast* node = ast_clone_header(stmt, VITTE_AST_CORE_STMT_RETURN);
    if (!node) return NULL;
    const vitte_ast* expr = stmt->first_child;
    if (expr) {
        vitte_ast* copy = clone_subtree(expr);
        if (!copy) {
            vitte_ast_free(NULL, node);
            return NULL;
        }
        ast_add_child(node, copy);
    }
    return node;
}

static vitte_ast* desugar_when_stmt(const vitte_ast* stmt, vitte_error* err) {
    vitte_ast* node = ast_clone_header(stmt, VITTE_AST_CORE_STMT_IF);
    if (!node) return NULL;
    const vitte_ast* branch = stmt->first_child;
    while (branch) {
        vitte_ast* branch_copy = ast_clone_header(branch, VITTE_AST_BRANCH);
        if (!branch_copy) {
            vitte_ast_free(NULL, node);
            return NULL;
        }
        branch_copy->first_child = branch_copy->last_child = NULL;
        const vitte_ast* cond = branch->literal.bool_value ? NULL : branch->first_child;
        const vitte_ast* block = branch->literal.bool_value ? branch->first_child
                                                            : (cond ? cond->next : NULL);
        if (!branch->literal.bool_value && !cond) {
            vitte_ast_free(NULL, branch_copy);
            vitte_ast_free(NULL, node);
            set_err(err, VITTE_ERRC_SYNTAX, branch->span.start.line, branch->span.start.col, "invalid when branch");
            return NULL;
        }
        if (cond) {
            vitte_ast* cond_copy = clone_subtree(cond);
            if (!cond_copy) {
                vitte_ast_free(NULL, branch_copy);
                vitte_ast_free(NULL, node);
                return NULL;
            }
            ast_add_child(branch_copy, cond_copy);
        }
        if (!block) {
            vitte_ast_free(NULL, branch_copy);
            vitte_ast_free(NULL, node);
            set_err(err, VITTE_ERRC_SYNTAX, branch->span.start.line, branch->span.start.col, "when branch missing block");
            return NULL;
        }
        vitte_ast* block_copy = desugar_block(block, err);
        if (!block_copy) {
            vitte_ast_free(NULL, branch_copy);
            vitte_ast_free(NULL, node);
            return NULL;
        }
        ast_add_child(branch_copy, block_copy);
        ast_add_child(node, branch_copy);
        branch = branch->next;
    }
    return node;
}

static vitte_ast* desugar_loop_stmt(const vitte_ast* stmt, vitte_error* err) {
    const vitte_ast* start_expr = stmt->first_child;
    const vitte_ast* end_expr = start_expr ? start_expr->next : NULL;
    const vitte_ast* next_expr = end_expr ? end_expr->next : NULL;
    const vitte_ast* step_expr = NULL;
    const vitte_ast* block_node = NULL;
    if (next_expr && next_expr->kind != VITTE_AST_BLOCK) {
        step_expr = next_expr;
        block_node = step_expr->next;
    } else {
        block_node = next_expr;
    }
    if (!start_expr || !end_expr || !block_node) {
        set_err(err, VITTE_ERRC_SYNTAX, stmt->span.start.line, stmt->span.start.col, "loop missing components");
        return NULL;
    }
    vitte_ast* node = ast_clone_header(stmt, VITTE_AST_CORE_STMT_FOR);
    if (!node) return NULL;
    node->first_child = node->last_child = NULL;
    vitte_ast* start_copy = clone_subtree(start_expr);
    vitte_ast* end_copy = clone_subtree(end_expr);
    if (!start_copy || !end_copy) {
        vitte_ast_free(NULL, start_copy);
        vitte_ast_free(NULL, end_copy);
        vitte_ast_free(NULL, node);
        return NULL;
    }
    ast_add_child(node, start_copy);
    ast_add_child(node, end_copy);
    if (step_expr) {
        vitte_ast* step_copy = clone_subtree(step_expr);
        if (!step_copy) {
            vitte_ast_free(NULL, node);
            return NULL;
        }
        ast_add_child(node, step_copy);
    }
    vitte_ast* body_copy = desugar_block(block_node, err);
    if (!body_copy) {
        vitte_ast_free(NULL, node);
        return NULL;
    }
    ast_add_child(node, body_copy);
    return node;
}

static vitte_ast* desugar_stmt(const vitte_ast* stmt, vitte_error* err) {
    switch (stmt->kind) {
        case VITTE_AST_PHR_STMT_SET:
            return desugar_set_stmt(stmt, err);
        case VITTE_AST_PHR_STMT_SAY:
            return desugar_say_stmt(stmt, err);
        case VITTE_AST_PHR_STMT_DO:
            return desugar_do_stmt(stmt, err);
        case VITTE_AST_PHR_STMT_RET:
            return desugar_ret_stmt(stmt);
        case VITTE_AST_PHR_STMT_WHEN:
            return desugar_when_stmt(stmt, err);
        case VITTE_AST_PHR_STMT_LOOP:
            return desugar_loop_stmt(stmt, err);
        default:
            return clone_subtree(stmt);
    }
}

static vitte_ast* desugar_node(const vitte_ast* node, vitte_error* err) {
    if (!node) return NULL;
    switch (node->kind) {
        case VITTE_AST_BLOCK:
            return desugar_block(node, err);
        case VITTE_AST_PHR_STMT_SET:
        case VITTE_AST_PHR_STMT_SAY:
        case VITTE_AST_PHR_STMT_DO:
        case VITTE_AST_PHR_STMT_RET:
        case VITTE_AST_PHR_STMT_WHEN:
        case VITTE_AST_PHR_STMT_LOOP:
            return desugar_stmt(node, err);
        default: {
            vitte_ast* copy = ast_clone_header(node, node->kind);
            if (!copy) return NULL;
            copy->first_child = copy->last_child = NULL;
            const vitte_ast* child = node->first_child;
            while (child) {
                vitte_ast* child_copy = desugar_node(child, err);
                if (!child_copy) {
                    vitte_ast_free(NULL, copy);
                    return NULL;
                }
                ast_add_child(copy, child_copy);
                child = child->next;
            }
            return copy;
        }
    }
}

vitte_result vitte_desugar_phrase(vitte_ctx* ctx,
                                  vitte_ast* phrase_ast,
                                  vitte_ast** out_core_ast,
                                  vitte_error* err) {
    (void)ctx;
    if (!phrase_ast || !out_core_ast) {
        set_err(err, VITTE_ERRC_SYNTAX, 0, 0, "invalid arguments");
        return VITTE_ERR_DESUGAR;
    }
    if (phrase_ast->kind != VITTE_AST_PHR_UNIT) {
        set_err(err, VITTE_ERRC_SYNTAX, phrase_ast->span.start.line, phrase_ast->span.start.col, "expected phrase AST");
        return VITTE_ERR_DESUGAR;
    }
    vitte_ast* core = ast_clone_header(phrase_ast, VITTE_AST_CORE_UNIT);
    if (!core) return VITTE_ERR_INTERNAL;
    core->first_child = core->last_child = NULL;
    const vitte_ast* child = phrase_ast->first_child;
    while (child) {
        vitte_ast* lowered = desugar_node(child, err);
        if (!lowered) {
            vitte_ast_free(ctx, core);
            return VITTE_ERR_DESUGAR;
        }
        ast_add_child(core, lowered);
        child = child->next;
    }
    *out_core_ast = core;
    return VITTE_OK;
}
