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

static void print_err(const vitte_error* e) {
    if (!e) return;
    fprintf(stderr, "err: code=%d line=%u col=%u: %s\n",
            (int)e->code, (unsigned)e->line, (unsigned)e->col,
            e->message[0] ? e->message : "(no message)");
}

static void expect_desugar_ok(const char* title, const char* src) {
    vitte_ctx ctx;
    vitte_ctx_init(&ctx);

    vitte_ast* phrase = NULL;
    vitte_ast* core = NULL;
    vitte_error err;
    memset(&err, 0, sizeof(err));

    vitte_result r = vitte_parse_phrase(&ctx, src, strlen(src), &phrase, &err);
    if (r != VITTE_OK) {
        fprintf(stderr, "parse_phrase should succeed: %s\n", title);
        print_err(&err);
    }
    T_ASSERT(r == VITTE_OK);
    T_ASSERT(phrase != NULL);

    memset(&err, 0, sizeof(err));
    r = vitte_desugar_phrase(&ctx, phrase, &core, &err);
    if (r != VITTE_OK) {
        fprintf(stderr, "desugar should succeed: %s\n", title);
        print_err(&err);
    }
    T_ASSERT(r == VITTE_OK);
    T_ASSERT(core != NULL);

    if (core) vitte_ast_free(&ctx, core);
    if (phrase) vitte_ast_free(&ctx, phrase);
    vitte_ctx_free(&ctx);
}

static void expect_desugar_fail(const char* title, const char* src) {
    vitte_ctx ctx;
    vitte_ctx_init(&ctx);

    vitte_ast* phrase = NULL;
    vitte_ast* core = NULL;
    vitte_error err;
    memset(&err, 0, sizeof(err));

    vitte_result r = vitte_parse_phrase(&ctx, src, strlen(src), &phrase, &err);
    if (r == VITTE_OK) {
        memset(&err, 0, sizeof(err));
        r = vitte_desugar_phrase(&ctx, phrase, &core, &err);
        if (r == VITTE_OK) {
            fprintf(stderr, "desugar should fail: %s\n", title);
            g_fail = 1;
        } else {
            T_ASSERT(err.line > 0);
            T_ASSERT(err.col > 0);
        }
    } else {
        /* parse failing is acceptable for invalid phrase */
        T_ASSERT(err.line > 0);
        T_ASSERT(err.col > 0);
    }

    if (core) vitte_ast_free(&ctx, core);
    if (phrase) vitte_ast_free(&ctx, phrase);
    vitte_ctx_free(&ctx);
}

int main(void) {
    /* phrase surface: mod/use/type/fn/scn/prog with .end blocks */
    expect_desugar_ok("simple fn",
        "fn add(a: i32, b: i32) -> i32\n"
        "  ret a + b\n"
        ".end\n");

    expect_desugar_ok("when/loop sugar",
        "fn f() -> i32\n"
        "  let x: i32 = 0\n"
        "  loop\n"
        "    when x == 3\n"
        "      ret x\n"
        "    .end\n"
        "    set x = x + 1\n"
        "  .end\n"
        ".end\n");

    expect_desugar_fail("missing .end",
        "fn bad() -> i32\n"
        "  ret 1\n");

    if (g_fail) return 1;
    printf("OK: vitte phrase desugar tests\n");
    return 0;
}
