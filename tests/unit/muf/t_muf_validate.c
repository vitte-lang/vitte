/*
 * tests/unit/muf/t_muf_validate.c
 *
 * Unit tests: MUF validation (semantic checks).
 *
 * Assumed public API (C):
 *   - #include "muf/muf.h"
 *   - #include "muf/muf_validate.h"
 *   - muf_parse_string(...)
 *   - muf_validate_doc(ctx, doc, &out_report)
 *   - muf_validate_report_free(ctx, &report)
 *
 * Validation expectations tested here:
 *   - [package].name and [package].version required
 *   - dependency entries must be objects with exactly one of: {path, git}
 *   - version must be semver-ish "X.Y.Z" (basic)
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "muf/muf.h"
#include "muf/muf_validate.h"

static int g_fail = 0;

static void t_fail(const char* file, int line, const char* expr) {
    fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expr);
    g_fail = 1;
}

#define T_ASSERT(x) do { if (!(x)) t_fail(__FILE__, __LINE__, #x); } while (0)

static void parse_or_die(muf_ctx* ctx, const char* src, muf_doc** out_doc) {
    muf_error err;
    memset(&err, 0, sizeof(err));

    muf_result r = muf_parse_string(ctx, src, (size_t)strlen(src), out_doc, &err);
    if (r != MUF_OK) {
        fprintf(stderr, "parse failed unexpectedly: code=%d line=%u col=%u msg=%s\n",
                (int)err.code, (unsigned)err.line, (unsigned)err.col,
                err.message[0] ? err.message : "(no message)");
        g_fail = 1;
    }
    T_ASSERT(r == MUF_OK);
    T_ASSERT(*out_doc != NULL);
}

/* -----------------------------------------------------------------------------
 * Fixtures
 * -------------------------------------------------------------------------- */

static const char* k_valid =
    "[package]\n"
    "name = \"demo\"\n"
    "version = \"1.2.3\"\n"
    ".end\n"
    "\n"
    "[dependencies]\n"
    "core = { path = \"../core\" }\n"
    "std = { git = \"https://example.com/std\", rev = \"deadbeef\" }\n"
    ".end\n";

static const char* k_invalid_missing_name =
    "[package]\n"
    "version = \"1.2.3\"\n"
    ".end\n";

static const char* k_invalid_bad_version =
    "[package]\n"
    "name = \"demo\"\n"
    "version = \"1.2\"\n" /* not X.Y.Z */
    ".end\n";

static const char* k_invalid_dep_both =
    "[package]\n"
    "name = \"demo\"\n"
    "version = \"1.2.3\"\n"
    ".end\n"
    "\n"
    "[dependencies]\n"
    "bad = { path = \"../x\", git = \"https://example.com/x\" }\n"
    ".end\n";

/* -----------------------------------------------------------------------------
 * Tests
 * -------------------------------------------------------------------------- */

static void expect_valid(const char* title, const char* src) {
    muf_ctx ctx;
    muf_ctx_init(&ctx);

    muf_doc* doc = NULL;
    parse_or_die(&ctx, src, &doc);

    muf_validate_report rep;
    memset(&rep, 0, sizeof(rep));

    muf_result vr = muf_validate_doc(&ctx, doc, &rep);
    if (vr != MUF_OK) {
        fprintf(stderr, "validate should succeed: %s\n", title);
        if (rep.count) {
            for (uint32_t i = 0; i < rep.count; ++i) {
                const muf_diag* d = &rep.items[i];
                fprintf(stderr, "  diag[%u]: sev=%d line=%u col=%u: %s\n",
                        (unsigned)i, (int)d->severity,
                        (unsigned)d->line, (unsigned)d->col,
                        d->message[0] ? d->message : "(no message)");
            }
        }
    }

    T_ASSERT(vr == MUF_OK);
    T_ASSERT(rep.count == 0);

    muf_validate_report_free(&ctx, &rep);
    muf_doc_free(&ctx, doc);
    muf_ctx_free(&ctx);
}

static void expect_invalid(const char* title, const char* src) {
    muf_ctx ctx;
    muf_ctx_init(&ctx);

    muf_doc* doc = NULL;
    parse_or_die(&ctx, src, &doc);

    muf_validate_report rep;
    memset(&rep, 0, sizeof(rep));

    muf_result vr = muf_validate_doc(&ctx, doc, &rep);

    /* Validation should fail OR return OK with at least one diagnostic
     * depending on your design; accept both but enforce diagnostics exist.
     */
    if (vr == MUF_OK && rep.count == 0) {
        fprintf(stderr, "validate should report errors: %s\n", title);
        g_fail = 1;
    }

    T_ASSERT(rep.count > 0);

    /* Ensure at least one ERROR severity exists. */
    bool has_err = false;
    for (uint32_t i = 0; i < rep.count; ++i) {
        if (rep.items[i].severity == MUF_SEV_ERROR) { has_err = true; break; }
    }
    T_ASSERT(has_err);

    muf_validate_report_free(&ctx, &rep);
    muf_doc_free(&ctx, doc);
    muf_ctx_free(&ctx);
}

int main(void) {
    expect_valid("valid manifest", k_valid);

    expect_invalid("missing package.name", k_invalid_missing_name);
    expect_invalid("bad package.version", k_invalid_bad_version);
    expect_invalid("dependency with both path and git", k_invalid_dep_both);

    if (g_fail) return 1;
    printf("OK: muf validate tests\n");
    return 0;
}
