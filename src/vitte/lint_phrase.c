#include "lint_phrase.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    VITTE_BIND_PARAM = 0,
    VITTE_BIND_LOCAL = 1,
    VITTE_BIND_LOOP = 2
} vitte_bind_kind;

typedef struct {
    const char* name; /* points into AST-owned storage */
    vitte_span decl_span;
    vitte_bind_kind kind;
    int read;
} vitte_binding;

typedef struct vitte_scope {
    struct vitte_scope* parent;
    vitte_binding* bindings;
    uint32_t len;
    uint32_t cap;
} vitte_scope;

static void emit_warning(vitte_diag_bag* diags, const char* code, vitte_span sp, const char* msg, const char* help) {
    vitte_diag* d = vitte_diag_bag_push(diags, VITTE_SEV_WARNING, code, sp, msg ? msg : "");
    if (d && help && help[0]) vitte_diag_set_help(d, help);
}

static int scope_grow(vitte_scope* s, uint32_t need) {
    if (!s) return 0;
    if (need <= s->cap) return 1;
    uint32_t new_cap = s->cap ? s->cap * 2u : 16u;
    while (new_cap < need) new_cap *= 2u;
    void* nb = realloc(s->bindings, (size_t)new_cap * sizeof(*s->bindings));
    if (!nb) return 0;
    s->bindings = (vitte_binding*)nb;
    s->cap = new_cap;
    return 1;
}

static vitte_scope* scope_new(vitte_scope* parent) {
    vitte_scope* s = (vitte_scope*)calloc(1, sizeof(*s));
    if (!s) return NULL;
    s->parent = parent;
    return s;
}

static void scope_free(vitte_scope* s) {
    if (!s) return;
    free(s->bindings);
    free(s);
}

static int is_ignored_name(const char* name) {
    return name && name[0] == '_';
}

static int str_eq_nul(const char* a, const char* b, size_t b_len) {
    if (!a || !b) return 0;
    size_t a_len = strlen(a);
    if (a_len != b_len) return 0;
    return memcmp(a, b, b_len) == 0;
}

static const char* base_ident(const char* s, size_t* out_len) {
    if (!s) return NULL;
    const char* dot = strchr(s, '.');
    if (dot) {
        if (out_len) *out_len = (size_t)(dot - s);
        return s;
    }
    if (out_len) *out_len = strlen(s);
    return s;
}

static vitte_binding* scope_find_current(vitte_scope* s, const char* name, size_t name_len) {
    if (!s || !name) return NULL;
    for (uint32_t i = 0; i < s->len; i++) {
        if (str_eq_nul(s->bindings[i].name, name, name_len)) return &s->bindings[i];
    }
    return NULL;
}

static vitte_binding* scope_find_any(vitte_scope* s, const char* name, size_t name_len) {
    while (s) {
        vitte_binding* b = scope_find_current(s, name, name_len);
        if (b) return b;
        s = s->parent;
    }
    return NULL;
}

static int scope_add_binding(vitte_scope* s, const char* name, vitte_span sp, vitte_bind_kind kind) {
    if (!s || !name) return 0;
    if (!scope_grow(s, s->len + 1u)) return 0;
    vitte_binding* b = &s->bindings[s->len++];
    memset(b, 0, sizeof(*b));
    b->name = name;
    b->decl_span = sp;
    b->kind = kind;
    b->read = 0;
    return 1;
}

static void mark_read(vitte_scope* s, const char* name, size_t name_len) {
    vitte_binding* b = scope_find_any(s, name, name_len);
    if (b) b->read = 1;
}

static void emit_unused_for_scope(vitte_scope* s, vitte_diag_bag* diags) {
    if (!s || !diags) return;
    for (uint32_t i = 0; i < s->len; i++) {
        vitte_binding* b = &s->bindings[i];
        if (b->read) continue;
        if (is_ignored_name(b->name)) continue;
        char msg[256];
        snprintf(msg, sizeof(msg), "unused binding `%s`", b->name ? b->name : "");
        emit_warning(diags, "V1001", b->decl_span, msg, "Remove it, or use it (read it) to silence this warning.");
    }
}

static void lint_node(const vitte_ast* node, vitte_scope* scope, vitte_diag_bag* diags);

static void lint_expr(const vitte_ast* node, vitte_scope* scope, vitte_diag_bag* diags) {
    (void)diags;
    if (!node) return;
    switch (node->kind) {
        case VITTE_AST_EXPR_IDENT: {
            size_t n = 0;
            const char* name = base_ident(node->text, &n);
            if (name && n) mark_read(scope, name, n);
            break;
        }
        case VITTE_AST_EXPR_PATH: {
            size_t n = 0;
            const char* name = base_ident(node->text, &n);
            if (name && n) mark_read(scope, name, n);
            break;
        }
        default:
            break;
    }
    const vitte_ast* child = node->first_child;
    while (child) {
        lint_expr(child, scope, diags);
        child = child->next;
    }
}

static void lint_block(const vitte_ast* block, vitte_scope* parent, vitte_diag_bag* diags) {
    vitte_scope* s = scope_new(parent);
    if (!s) return;

    int terminated = 0;
    vitte_span terminator_span = vitte_span_make(0u, 0u, 0u);
    const vitte_ast* stmt = block ? block->first_child : NULL;
    while (stmt) {
        if (terminated) {
            vitte_diag* d = vitte_diag_bag_push(diags, VITTE_SEV_WARNING, "V1003", stmt->span, "unreachable statement");
            if (d) {
                (void)vitte_diag_add_label(d, VITTE_DIAG_LABEL_SECONDARY, terminator_span, "control-flow ends here");
                vitte_diag_set_help(d, "Remove the unreachable code, or restructure control-flow so it can execute.");
            }
        }
        lint_node(stmt, s, diags);
        if (stmt->kind == VITTE_AST_PHR_STMT_RET) {
            terminated = 1;
            terminator_span = stmt->span;
        }
        stmt = stmt->next;
    }

    emit_unused_for_scope(s, diags);
    scope_free(s);
}

static void lint_set_stmt(const vitte_ast* stmt, vitte_scope* scope, vitte_diag_bag* diags) {
    if (!stmt || !scope) return;
    const vitte_ast* target = stmt->first_child;
    const vitte_ast* value = target ? target->next : NULL;

    if (value) lint_expr(value, scope, diags);

    if (!target || !target->text) return;
    size_t name_len = 0;
    const char* base = base_ident(target->text, &name_len);
    if (!base || name_len == 0) return;

    /* Treat dotted paths as "member" writes, not local bindings. */
    if (strchr(target->text, '.')) return;

    /* Re-assignment to an existing binding is not a "declaration". */
    if (scope_find_current(scope, base, name_len)) return;

    if (!scope_add_binding(scope, base, target->span, VITTE_BIND_LOCAL)) return;

    vitte_binding* outer = scope_find_any(scope->parent, base, name_len);
    if (outer && !is_ignored_name(base)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "binding `%.*s` shadows an outer binding", (int)name_len, base);
        vitte_diag* d = vitte_diag_bag_push(diags, VITTE_SEV_WARNING, "V1002", target->span, msg);
        if (d) {
            (void)vitte_diag_add_label(d, VITTE_DIAG_LABEL_SECONDARY, outer->decl_span, "outer binding here");
            vitte_diag_set_help(d, "Rename the inner binding to avoid confusion, or use the outer binding directly.");
        }
    }
}

static void lint_loop_stmt(const vitte_ast* stmt, vitte_scope* scope, vitte_diag_bag* diags) {
    if (!stmt || !scope) return;
    const vitte_ast* start_expr = stmt->first_child;
    const vitte_ast* end_expr = start_expr ? start_expr->next : NULL;
    const vitte_ast* next_expr = end_expr ? end_expr->next : NULL;

    const vitte_ast* step_expr = NULL;
    const vitte_ast* body = NULL;
    if (next_expr && next_expr->kind != VITTE_AST_BLOCK) {
        step_expr = next_expr;
        body = step_expr->next;
    } else {
        body = next_expr;
    }

    if (start_expr) lint_expr(start_expr, scope, diags);
    if (end_expr) lint_expr(end_expr, scope, diags);
    if (step_expr) lint_expr(step_expr, scope, diags);

    vitte_scope* loop_scope = scope_new(scope);
    if (!loop_scope) return;
    if (stmt->text) {
        size_t name_len = 0;
        const char* base = base_ident(stmt->text, &name_len);
        if (base && name_len && !scope_add_binding(loop_scope, stmt->text, stmt->span, VITTE_BIND_LOOP)) {
            scope_free(loop_scope);
            return;
        }
    }

    if (body && body->kind == VITTE_AST_BLOCK) {
        lint_block(body, loop_scope, diags);
    }
    emit_unused_for_scope(loop_scope, diags);
    scope_free(loop_scope);
}

static void lint_when_stmt(const vitte_ast* stmt, vitte_scope* scope, vitte_diag_bag* diags) {
    const vitte_ast* branch = stmt ? stmt->first_child : NULL;
    while (branch) {
        const vitte_ast* cond_or_block = branch->first_child;
        if (branch->literal.bool_value) {
            if (cond_or_block && cond_or_block->kind == VITTE_AST_BLOCK) {
                lint_block(cond_or_block, scope, diags);
            }
        } else {
            const vitte_ast* cond = cond_or_block;
            const vitte_ast* block = cond ? cond->next : NULL;
            if (cond) lint_expr(cond, scope, diags);
            if (block && block->kind == VITTE_AST_BLOCK) lint_block(block, scope, diags);
        }
        branch = branch->next;
    }
}

static void lint_fn_decl(const vitte_ast* fn, vitte_scope* parent, vitte_diag_bag* diags) {
    vitte_scope* fn_scope = scope_new(parent);
    if (!fn_scope) return;

    const vitte_ast* child = fn ? fn->first_child : NULL;
    const vitte_ast* body = NULL;
    while (child) {
        if (child->kind == VITTE_AST_PARAM && child->text) {
            (void)scope_add_binding(fn_scope, child->text, child->span, VITTE_BIND_PARAM);
        } else if (child->kind == VITTE_AST_BLOCK) {
            body = child;
        }
        child = child->next;
    }

    if (body) lint_block(body, fn_scope, diags);
    emit_unused_for_scope(fn_scope, diags);
    scope_free(fn_scope);
}

static void lint_node(const vitte_ast* node, vitte_scope* scope, vitte_diag_bag* diags) {
    if (!node) return;

    switch (node->kind) {
        case VITTE_AST_FN_DECL:
            lint_fn_decl(node, scope, diags);
            return;
        case VITTE_AST_SCENARIO_DECL:
        case VITTE_AST_PROGRAM_DECL: {
            const vitte_ast* body = node->first_child;
            while (body && body->next) body = body->next;
            if (body && body->kind == VITTE_AST_BLOCK) lint_block(body, scope, diags);
            return;
        }
        case VITTE_AST_BLOCK:
            lint_block(node, scope, diags);
            return;
        case VITTE_AST_PHR_STMT_SET:
            lint_set_stmt(node, scope, diags);
            return;
        case VITTE_AST_PHR_STMT_DO:
            if (node->text) {
                size_t name_len = 0;
                const char* base = base_ident(node->text, &name_len);
                if (base && name_len) mark_read(scope, base, name_len);
            }
            break;
        case VITTE_AST_PHR_STMT_SAY:
        case VITTE_AST_PHR_STMT_RET:
            break;
        case VITTE_AST_PHR_STMT_LOOP:
            lint_loop_stmt(node, scope, diags);
            return;
        case VITTE_AST_PHR_STMT_WHEN:
            lint_when_stmt(node, scope, diags);
            return;
        default:
            break;
    }

    if (node->kind == VITTE_AST_PHR_STMT_SAY || node->kind == VITTE_AST_PHR_STMT_RET) {
        const vitte_ast* child = node->first_child;
        if (child) lint_expr(child, scope, diags);
        return;
    }

    /* Default recursive walk: treat unknown nodes as containers. */
    const vitte_ast* child = node->first_child;
    while (child) {
        lint_node(child, scope, diags);
        child = child->next;
    }
}

void vitte_lint_phrase(const vitte_ast* phrase_ast, vitte_diag_bag* diags) {
    if (!phrase_ast || !diags) return;
    if (phrase_ast->kind != VITTE_AST_PHR_UNIT) return;

    vitte_scope root = {0};
    const vitte_ast* child = phrase_ast->first_child;
    while (child) {
        lint_node(child, &root, diags);
        child = child->next;
    }

    free(root.bindings);
}
