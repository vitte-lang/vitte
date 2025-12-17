#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vitte/lexer.h"

static int g_fail = 0;

static void assert_true(int cond, const char* msg) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", msg);
        g_fail = 1;
    }
}

static void test_token_sequence(void) {
    const char* src =
        "mod foo.bar/baz\n"
        "use foo/bar as bar\n"
        "fn add(a: i32, b: i32) -> i32\n"
        "  set acc = a + b\n"
        "  say \"sum\"\n"
        "  do log acc\n"
        "  when acc == 0\n"
        "    ret acc\n"
        "  .end\n"
        ".end\n";

    vitte_ctx ctx;
    vitte_ctx_init(&ctx);

    vitte_token* toks = NULL;
    size_t count = 0;
    vitte_error err = {0};
    vitte_result r = vitte_lex_all(&ctx, src, strlen(src), &toks, &count, &err);
    assert_true(r == VITTE_OK, "token sequence lex");
    assert_true(toks != NULL && count > 0, "token buffer");

    const vitte_token_kind expected[] = {
        VITTE_TOK_KW_MOD, VITTE_TOK_IDENT, VITTE_TOK_DOT, VITTE_TOK_IDENT, VITTE_TOK_SLASH, VITTE_TOK_IDENT, VITTE_TOK_NEWLINE,
        VITTE_TOK_KW_USE, VITTE_TOK_IDENT, VITTE_TOK_SLASH, VITTE_TOK_IDENT, VITTE_TOK_KW_AS, VITTE_TOK_IDENT, VITTE_TOK_NEWLINE,
        VITTE_TOK_KW_FN, VITTE_TOK_IDENT, VITTE_TOK_LPAREN, VITTE_TOK_IDENT, VITTE_TOK_COLON, VITTE_TOK_IDENT
    };
    size_t expected_len = sizeof(expected) / sizeof(expected[0]);
    assert_true(count >= expected_len, "enough tokens");
    for (size_t i = 0; i < expected_len; ++i) {
        if (toks[i].kind != expected[i]) {
            fprintf(stderr, "unexpected token[%zu]: got %d expected %d\n", i, (int)toks[i].kind, (int)expected[i]);
            g_fail = 1;
            break;
        }
    }

    size_t dotend = 0;
    size_t arrows = 0;
    for (size_t i = 0; i < count; ++i) {
        if (toks[i].kind == VITTE_TOK_DOTEND) dotend++;
        if (toks[i].kind == VITTE_TOK_ARROW) arrows++;
    }
    assert_true(dotend == 2, ".end occurrences");
    assert_true(arrows == 1, "arrow token");

    free(toks);
    vitte_ctx_free(&ctx);
}

static void test_literals(void) {
    const char* src = "say \"hello\"\nset val = 42\n";
    vitte_ctx ctx;
    vitte_ctx_init(&ctx);
    vitte_token* toks = NULL;
    size_t count = 0;
    vitte_error err = {0};
    vitte_result r = vitte_lex_all(&ctx, src, strlen(src), &toks, &count, &err);
    assert_true(r == VITTE_OK, "literal lex ok");
    assert_true(count >= 6, "literal tokens count");
    size_t str_idx = (size_t)-1;
    size_t int_idx = (size_t)-1;
    for (size_t i = 0; i < count; ++i) {
        if (toks[i].kind == VITTE_TOK_STRING) str_idx = i;
        if (toks[i].kind == VITTE_TOK_INT) int_idx = i;
    }
    assert_true(str_idx != (size_t)-1, "found string literal");
    assert_true(int_idx != (size_t)-1, "found int literal");
    assert_true(toks[str_idx].len == 7, "string literal length includes quotes");
    assert_true(int_idx > str_idx, "int literal present");
    free(toks);
    vitte_ctx_free(&ctx);
}

int main(void) {
    test_token_sequence();
    test_literals();
    if (g_fail) return 1;
    printf("OK: vitte lexer\n");
    return 0;
}
