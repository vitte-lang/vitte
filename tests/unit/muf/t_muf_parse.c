/*
 * tests/unit/muf/t_muf_parse.c
 *
 * Unit tests: MUF parsing (muffin manifest).
 *
 * Assumed public API (C):
 *   - #include "muf/muf.h"
 *   - muf_ctx_init / muf_ctx_free
 *   - muf_parse_string(ctx, text, len, &out_doc, &out_err)
 *   - muf_doc_free(ctx, doc)
 *
 * If your actual API names differ, keep the test logic and adapt the calls.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* ---- expected MUF public headers ---- */
#include "muf/muf.h"

#ifndef MUF_TEST_MAX_ERR
#define MUF_TEST_MAX_ERR 512u
#endif

/* -----------------------------------------------------------------------------
 * Minimal test harness
 * -------------------------------------------------------------------------- */

static int g_fail = 0;

static void t_fail(const char* file, int line, const char* expr) {
    fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expr);
    g_fail = 1;
}

#define T_ASSERT(x) do { if (!(x)) t_fail(__FILE__, __LINE__, #x); } while (0)
#define T_ASSERT_MSG(x, msg) do { if (!(x)) { t_fail(__FILE__, __LINE__, msg); } } while (0)

static void print_err(const muf_error* e) {
    if (!e) return;
    fprintf(stderr, "muf_error: code=%d line=%u col=%u: %s\n",
            (int)e->code, (unsigned)e->line, (unsigned)e->col,
            e->message[0] ? e->message : "(no message)");
}

/* -----------------------------------------------------------------------------
 * Fixtures
 * -------------------------------------------------------------------------- */

static const char* k_ok_minimal =
    "# minimal MUF\n"
    "[package]\n"
    "name = \"vitte\"\n"
    "version = \"0.1.0\"\n"
    ".end\n";

static const char* k_ok_deps =
    "[package]\n"
    "name = \"demo\"\n"
    "version = \"1.2.3\"\n"
    ".end\n"
    "\n"
    "[dependencies]\n"
    "vitte_std = { git = \"https://example.com/vitte/std\", rev = \"deadbeef\" }\n"
    "json = { path = \"../json\" }\n"
    ".end\n";

static const char* k_bad_unterminated_string =
    "[package]\n"
    "name = \"demo\n"          /* missing quote */
    "version = \"1.0.0\"\n"
    ".end\n";

static const char* k_bad_missing_end =
    "[package]\n"
    "name = \"demo\"\n"
    "version = \"1.0.0\"\n";   /* missing .end */

/* -----------------------------------------------------------------------------
 * Tests
 * -------------------------------------------------------------------------- */

static void test_parse_ok(const char* title, const char* src) {
    muf_ctx ctx;
    muf_ctx_init(&ctx);

    muf_doc* doc = NULL;
    muf_error err;
    memset(&err, 0, sizeof(err));

    muf_result r = muf_parse_string(&ctx, src, (size_t)strlen(src), &doc, &err);
    if (r != MUF_OK) {
        fprintf(stderr, "parse should succeed: %s\n", title);
        print_err(&err);
    }
    T_ASSERT(r == MUF_OK);
    T_ASSERT(doc != NULL);

    if (doc) muf_doc_free(&ctx, doc);
    muf_ctx_free(&ctx);
}

static void test_parse_fail(const char* title, const char* src) {
    muf_ctx ctx;
    muf_ctx_init(&ctx);

    muf_doc* doc = NULL;
    muf_error err;
    memset(&err, 0, sizeof(err));

    muf_result r = muf_parse_string(&ctx, src, (size_t)strlen(src), &doc, &err);

    if (r == MUF_OK) {
        fprintf(stderr, "parse should fail: %s\n", title);
    } else {
        /* Ensure we got some location info when failing. */
        T_ASSERT(err.line > 0);
        T_ASSERT(err.col > 0);
    }

    T_ASSERT(r != MUF_OK);
    T_ASSERT(doc == NULL);

    muf_ctx_free(&ctx);
}

int main(void) {
    test_parse_ok("minimal", k_ok_minimal);
    test_parse_ok("deps", k_ok_deps);

    test_parse_fail("unterminated string", k_bad_unterminated_string);
    test_parse_fail("missing .end", k_bad_missing_end);

    if (g_fail) return 1;
    printf("OK: muf parse tests\n");
    return 0;
}
