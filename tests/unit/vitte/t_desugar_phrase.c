/*
 * tests/unit/vitte/t_desugar_phrase.c
 *
 * Unit tests: Vitte "phrase" desugaring -> core AST.
 *
 * Assumed public API (C):
 *   - #include "vitte/lexer.h"
 *   - #include "vitte/parser_phrase.h"
 *   - #include "vitte/desugar_phrase.h"
 *   - vitte_ctx_init/free
 *   - vitte_parse_phrase(ctx, src, len, &phrase_ast, &err)
 *   - vitte_desugar_phrase(ctx, phrase_ast, &core_ast, &err)
 *   - vitte_ast_free(ctx, ast)
 *
 * If your names differ, keep the intent and adapt the calls.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "vitte/lexer.h"
#include "vitte/parser_phrase.h"
#include "vitte/desugar_phrase.h"

static int g_fail = 0;

static void t_fail(const char* file, int line, const char* msg) {
    fprintf(stderr, "FAIL %s:%d: %s\n", file, line, msg);
    g_fail = 1;
}
#define T_ASSERT(x) do { if (!(x)) t_fail(__FILE__, __LINE__, #x); } while (0)

static const vitte_ast* child_at(const vitte_ast* node, size_t index) {
    const vitte_ast* child = node ? node->first_child : NULL;
    while (child && index--) {
        child = child->next;
    }
    return child;
}

static const vitte_ast* child_of_kind(const vitte_ast* node, vitte_ast_kind kind, size_t occurrence) {
    const vitte_ast* child = node ? node->first_child : NULL;
    while (child) {
        if (child->kind == kind) {
            if (occurrence == 0) return child;
            occurrence--;
        }
        child = child->next;
    }
    return NULL;
}

static vitte_ast* child_of_kind_mut(vitte_ast* node, vitte_ast_kind kind, size_t occurrence) {
    vitte_ast* child = node ? node->first_child : NULL;
    while (child) {
        if (child->kind == kind) {
            if (occurrence == 0) return child;
            occurrence--;
        }
        child = child->next;
    }
    return NULL;
}

static void test_core_assignments(void) {
    const char* src =
        "fn worker(a: i32) -> i32\n"
        "  set acc = a\n"
        "  say \"start\"\n"
        "  do log acc\n"
        "  ret acc\n"
        ".end\n";

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);
    vitte_ast *phrase = NULL, *core = NULL;
    vitte_diag_bag diags;
    vitte_diag_bag_init(&diags);
    T_ASSERT(vitte_parse_phrase(&ctx, 0u, src, strlen(src), &phrase, &diags) == VITTE_OK);
    T_ASSERT(!vitte_diag_bag_has_errors(&diags));
    vitte_diag_bag_free(&diags);
    vitte_diag_bag_init(&diags);
    T_ASSERT(vitte_desugar_phrase(&ctx, phrase, &core, &diags) == VITTE_OK);
    T_ASSERT(!vitte_diag_bag_has_errors(&diags));

    const vitte_ast* fn = child_of_kind(core, VITTE_AST_FN_DECL, 0);
    T_ASSERT(fn != NULL);
    const vitte_ast* body = child_of_kind(fn, VITTE_AST_BLOCK, 0);
    T_ASSERT(body != NULL);

    const vitte_ast* stmt = child_at(body, 0);
    T_ASSERT(stmt && stmt->kind == VITTE_AST_CORE_STMT_ASSIGN);
    const vitte_ast* target = child_of_kind(stmt, VITTE_AST_EXPR_PATH, 0);
    T_ASSERT(target && target->text && strcmp(target->text, "acc") == 0);

    stmt = stmt->next;
    T_ASSERT(stmt && stmt->kind == VITTE_AST_CORE_STMT_EXPR);
    const vitte_ast* call = stmt->first_child;
    T_ASSERT(call && call->kind == VITTE_AST_EXPR_CALL);
    T_ASSERT(call->text && strcmp(call->text, "say") == 0);

    stmt = stmt->next;
    T_ASSERT(stmt && stmt->kind == VITTE_AST_CORE_STMT_EXPR);
    call = stmt->first_child;
    T_ASSERT(call && call->kind == VITTE_AST_EXPR_CALL);
    T_ASSERT(call->text && strcmp(call->text, "log") == 0);
    T_ASSERT(call->first_child && call->first_child->kind == VITTE_AST_EXPR_IDENT);

    stmt = stmt->next;
    T_ASSERT(stmt && stmt->kind == VITTE_AST_CORE_STMT_RETURN);

    vitte_ast_free(&ctx, core);
    vitte_ast_free(&ctx, phrase);
    vitte_diag_bag_free(&diags);
    vitte_ctx_free(&ctx);
}

static void test_when_loop_lowering(void) {
    const char* src =
        "fn accum() -> i32\n"
        "  set acc = 0\n"
        "  when acc == 0\n"
        "    ret 0\n"
        "  else\n"
        "    ret acc\n"
        "  .end\n"
        "  loop i from 0 to 3 step 1\n"
        "    set acc = acc + i\n"
        "  .end\n"
        "  ret acc\n"
        ".end\n";

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);
    vitte_ast *phrase = NULL, *core = NULL;
    vitte_diag_bag diags;
    vitte_diag_bag_init(&diags);
    T_ASSERT(vitte_parse_phrase(&ctx, 0u, src, strlen(src), &phrase, &diags) == VITTE_OK);
    T_ASSERT(!vitte_diag_bag_has_errors(&diags));
    vitte_diag_bag_free(&diags);
    vitte_diag_bag_init(&diags);
    T_ASSERT(vitte_desugar_phrase(&ctx, phrase, &core, &diags) == VITTE_OK);
    T_ASSERT(!vitte_diag_bag_has_errors(&diags));

    const vitte_ast* fn = child_of_kind(core, VITTE_AST_FN_DECL, 0);
    T_ASSERT(fn != NULL);
    const vitte_ast* block = child_of_kind(fn, VITTE_AST_BLOCK, 0);
    T_ASSERT(block != NULL);

    const vitte_ast* assign = child_at(block, 0);
    T_ASSERT(assign && assign->kind == VITTE_AST_CORE_STMT_ASSIGN);

    const vitte_ast* when_stmt = assign ? assign->next : NULL;
    T_ASSERT(when_stmt && when_stmt->kind == VITTE_AST_CORE_STMT_IF);
    const vitte_ast* first_branch = child_of_kind(when_stmt, VITTE_AST_BRANCH, 0);
    T_ASSERT(first_branch != NULL);
    const vitte_ast* cond = child_at(first_branch, 0);
    T_ASSERT(cond && cond->kind == VITTE_AST_EXPR_BINARY);
    const vitte_ast* when_block = child_of_kind(first_branch, VITTE_AST_BLOCK, 0);
    T_ASSERT(when_block && when_block->first_child && when_block->first_child->kind == VITTE_AST_CORE_STMT_RETURN);

    const vitte_ast* else_branch = child_of_kind(when_stmt, VITTE_AST_BRANCH, 1);
    T_ASSERT(else_branch && else_branch->literal.bool_value == 1);

    const vitte_ast* loop_stmt = when_stmt->next;
    T_ASSERT(loop_stmt && loop_stmt->kind == VITTE_AST_CORE_STMT_FOR);
    T_ASSERT(loop_stmt->text && strcmp(loop_stmt->text, "i") == 0);
    const vitte_ast* start_expr = child_at(loop_stmt, 0);
    const vitte_ast* end_expr = child_at(loop_stmt, 1);
    T_ASSERT(start_expr && start_expr->kind == VITTE_AST_EXPR_LITERAL);
    T_ASSERT(end_expr && end_expr->kind == VITTE_AST_EXPR_LITERAL);
    const vitte_ast* loop_block = child_of_kind(loop_stmt, VITTE_AST_BLOCK, 0);
    T_ASSERT(loop_block && loop_block->first_child && loop_block->first_child->kind == VITTE_AST_CORE_STMT_ASSIGN);

    const vitte_ast* ret_stmt = loop_stmt->next;
    T_ASSERT(ret_stmt && ret_stmt->kind == VITTE_AST_CORE_STMT_RETURN);

    vitte_ast_free(&ctx, core);
    vitte_ast_free(&ctx, phrase);
    vitte_diag_bag_free(&diags);
    vitte_ctx_free(&ctx);
}

static void test_module_and_use_passthrough(void) {
    const char* src =
        "mod demo.core\n"
        "use runtime/io as log\n"
        "fn noop() -> i32\n"
        "  ret 0\n"
        ".end\n";

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);
    vitte_ast *phrase = NULL, *core = NULL;
    vitte_diag_bag diags;
    vitte_diag_bag_init(&diags);
    T_ASSERT(vitte_parse_phrase(&ctx, 0u, src, strlen(src), &phrase, &diags) == VITTE_OK);
    T_ASSERT(!vitte_diag_bag_has_errors(&diags));
    vitte_diag_bag_free(&diags);
    vitte_diag_bag_init(&diags);
    T_ASSERT(vitte_desugar_phrase(&ctx, phrase, &core, &diags) == VITTE_OK);
    T_ASSERT(!vitte_diag_bag_has_errors(&diags));

    const vitte_ast* mod = child_of_kind(core, VITTE_AST_MODULE_DECL, 0);
    T_ASSERT(mod && mod->text && strcmp(mod->text, "demo.core") == 0);
    const vitte_ast* use = child_of_kind(core, VITTE_AST_USE_DECL, 0);
    T_ASSERT(use && use->text && strcmp(use->text, "runtime/io") == 0);
    T_ASSERT(use->aux_text && strcmp(use->aux_text, "log") == 0);

    vitte_ast_free(&ctx, core);
    vitte_ast_free(&ctx, phrase);
    vitte_diag_bag_free(&diags);
    vitte_ctx_free(&ctx);
}

static void test_chained_else_when_lowering(void) {
    const char* src =
        "fn classify(x: i32) -> i32\n"
        "  when x == 0\n"
        "    ret 0\n"
        "  else when x == 1\n"
        "    ret 1\n"
        "  else\n"
        "    ret 2\n"
        "  .end\n"
        "  ret x\n"
        ".end\n";

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);
    vitte_ast *phrase = NULL, *core = NULL;
    vitte_diag_bag diags;
    vitte_diag_bag_init(&diags);
    T_ASSERT(vitte_parse_phrase(&ctx, 0u, src, strlen(src), &phrase, &diags) == VITTE_OK);
    T_ASSERT(!vitte_diag_bag_has_errors(&diags));
    vitte_diag_bag_free(&diags);
    vitte_diag_bag_init(&diags);
    T_ASSERT(vitte_desugar_phrase(&ctx, phrase, &core, &diags) == VITTE_OK);
    T_ASSERT(!vitte_diag_bag_has_errors(&diags));

    const vitte_ast* fn = child_of_kind(core, VITTE_AST_FN_DECL, 0);
    T_ASSERT(fn != NULL);
    const vitte_ast* body = child_of_kind(fn, VITTE_AST_BLOCK, 0);
    T_ASSERT(body != NULL);
    const vitte_ast* when_stmt = child_of_kind(body, VITTE_AST_CORE_STMT_IF, 0);
    T_ASSERT(when_stmt != NULL);

    size_t branch_count = 0;
    const vitte_ast* branch = when_stmt->first_child;
    while (branch) {
        T_ASSERT(branch->kind == VITTE_AST_BRANCH);
        if (branch->literal.bool_value) {
            /* else branch should not have a condition */
            T_ASSERT(branch->first_child && branch->first_child->kind == VITTE_AST_BLOCK);
        } else {
            T_ASSERT(branch->first_child && branch->first_child->kind != VITTE_AST_BLOCK);
            const vitte_ast* branch_block = child_of_kind(branch, VITTE_AST_BLOCK, 0);
            T_ASSERT(branch_block && branch_block->first_child && branch_block->first_child->kind == VITTE_AST_CORE_STMT_RETURN);
        }
        branch_count++;
        branch = branch->next;
    }
    T_ASSERT(branch_count == 3);

    vitte_ast_free(&ctx, core);
    vitte_ast_free(&ctx, phrase);
    vitte_diag_bag_free(&diags);
    vitte_ctx_free(&ctx);
}

static void test_desugar_reports_corrupted_loop_and_set(void) {
    vitte_ctx ctx;
    vitte_ctx_init(&ctx);
    vitte_diag_bag diags;
    vitte_diag_bag_init(&diags);
    vitte_ast *phrase = NULL, *core = NULL;

    const char* loop_src =
        "fn looper() -> i32\n"
        "  loop i from 0 to 3\n"
        "    ret i\n"
        "  .end\n"
        "  ret 0\n"
        ".end\n";
    T_ASSERT(vitte_parse_phrase(&ctx, 0u, loop_src, strlen(loop_src), &phrase, &diags) == VITTE_OK);
    T_ASSERT(!vitte_diag_bag_has_errors(&diags));
    vitte_diag_bag_free(&diags);
    vitte_ast* fn = child_of_kind_mut(phrase, VITTE_AST_FN_DECL, 0);
    T_ASSERT(fn != NULL);
    vitte_ast* fn_block = child_of_kind_mut(fn, VITTE_AST_BLOCK, 0);
    T_ASSERT(fn_block != NULL);
    vitte_ast* loop_stmt = child_of_kind_mut(fn_block, VITTE_AST_PHR_STMT_LOOP, 0);
    T_ASSERT(loop_stmt != NULL);
    /* Remove the block child to simulate a malformed loop */
    vitte_ast* prev = NULL;
    vitte_ast* child = loop_stmt->first_child;
    while (child && child->next) {
        prev = child;
        child = child->next;
    }
    if (prev) {
        prev->next = NULL;
        loop_stmt->last_child = prev;
    } else {
        loop_stmt->first_child = loop_stmt->last_child = NULL;
    }
    if (child) {
        vitte_ast_free(&ctx, child);
    }

    vitte_diag_bag_init(&diags);
    T_ASSERT(vitte_desugar_phrase(&ctx, phrase, &core, &diags) == VITTE_ERR_DESUGAR);
    T_ASSERT(vitte_diag_bag_has_errors(&diags));
    T_ASSERT(diags.len > 0 && strcmp(diags.diags[0].code, "V0004") == 0);
    vitte_diag_bag_free(&diags);
    if (core) {
        vitte_ast_free(&ctx, core);
        core = NULL;
    }
    vitte_ast_free(&ctx, phrase);
    phrase = NULL;

    const char* set_src =
        "fn setter()\n"
        "  set value = 1\n"
        ".end\n";
    vitte_diag_bag_init(&diags);
    T_ASSERT(vitte_parse_phrase(&ctx, 0u, set_src, strlen(set_src), &phrase, &diags) == VITTE_OK);
    T_ASSERT(!vitte_diag_bag_has_errors(&diags));
    vitte_diag_bag_free(&diags);
    fn = child_of_kind_mut(phrase, VITTE_AST_FN_DECL, 0);
    T_ASSERT(fn != NULL);
    fn_block = child_of_kind_mut(fn, VITTE_AST_BLOCK, 0);
    T_ASSERT(fn_block != NULL);
    vitte_ast* set_stmt = child_of_kind_mut(fn_block, VITTE_AST_PHR_STMT_SET, 0);
    T_ASSERT(set_stmt != NULL);
    vitte_ast* target = set_stmt->first_child;
    if (target && target->next) {
        vitte_ast_free(&ctx, target->next);
        target->next = NULL;
    }

    vitte_diag_bag_init(&diags);
    T_ASSERT(vitte_desugar_phrase(&ctx, phrase, &core, &diags) == VITTE_ERR_DESUGAR);
    T_ASSERT(vitte_diag_bag_has_errors(&diags));
    T_ASSERT(diags.len > 0 && strcmp(diags.diags[0].code, "V0004") == 0);
    vitte_diag_bag_free(&diags);
    if (core) vitte_ast_free(&ctx, core);
    vitte_ast_free(&ctx, phrase);
    vitte_ctx_free(&ctx);
}

static void expect_desugar_fail(const char* title, const char* src) {
    vitte_ctx ctx;
    vitte_ctx_init(&ctx);

    vitte_ast* phrase = NULL;
    vitte_ast* core = NULL;
    vitte_diag_bag diags;
    vitte_diag_bag_init(&diags);

    vitte_result r = vitte_parse_phrase(&ctx, 0u, src, strlen(src), &phrase, &diags);
    if (r == VITTE_OK) {
        T_ASSERT(!vitte_diag_bag_has_errors(&diags));
        vitte_diag_bag_free(&diags);
        vitte_diag_bag_init(&diags);
        r = vitte_desugar_phrase(&ctx, phrase, &core, &diags);
        if (r == VITTE_OK) {
            fprintf(stderr, "desugar should fail: %s\n", title);
            g_fail = 1;
        } else {
            T_ASSERT(vitte_diag_bag_has_errors(&diags));
        }
    } else {
        /* parse failing is acceptable for invalid phrase */
        T_ASSERT(vitte_diag_bag_has_errors(&diags));
    }

    if (core) vitte_ast_free(&ctx, core);
    if (phrase) vitte_ast_free(&ctx, phrase);
    vitte_diag_bag_free(&diags);
    vitte_ctx_free(&ctx);
}

static void test_parse_error_unmatched_end(void) {
    const char* src =
        ".end\n"
        ".end\n";

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);

    vitte_ast* phrase = NULL;
    vitte_diag_bag diags;
    vitte_diag_bag_init(&diags);

    vitte_result r = vitte_parse_phrase(&ctx, 0u, src, strlen(src), &phrase, &diags);
    T_ASSERT(r == VITTE_ERR_PARSE);
    T_ASSERT(vitte_diag_bag_has_errors(&diags));
    T_ASSERT(diags.len > 0 && strcmp(diags.diags[0].code, "V0002") == 0);

    if (phrase) vitte_ast_free(&ctx, phrase);
    vitte_diag_bag_free(&diags);
    vitte_ctx_free(&ctx);
}

int main(void) {
    test_core_assignments();
    test_when_loop_lowering();
    test_module_and_use_passthrough();
    test_chained_else_when_lowering();

    test_parse_error_unmatched_end();

    expect_desugar_fail("missing .end",
        "fn bad() -> i32\n"
        "  ret 1\n");

    test_desugar_reports_corrupted_loop_and_set();

    if (g_fail) return 1;
    printf("OK: vitte phrase desugar tests\n");
    return 0;
}
