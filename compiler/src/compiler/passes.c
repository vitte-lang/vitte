// SPDX-License-Identifier: MIT
// passes.c
//
// Compiler pass pipeline (max):
//  - Defines a generic pass interface with timing, diagnostics hooks, and tracing.
//  - Provides a default "front-end" pipeline for Vitte:
//      lex -> parse -> (optional) desugar phrase -> HIR -> IR -> validate
//  - Keeps the actual heavy lifting in their respective modules.
//
// This module is intentionally conservative: it does not assume specific
// internal representations beyond what the public headers expose.
// Where your project differs, adapt the glue points (see TODO markers).

#include "passes.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "hir_build.h"
#include "ir_build.h"

//------------------------------------------------------------------------------
// Portable timing
//------------------------------------------------------------------------------

static uint64_t steel_now_ns_(void)
{
#if defined(_WIN32)
    // Fallback to clock() (coarse). If you have QueryPerformanceCounter, use it.
    return (uint64_t)clock() * (1000000000ull / (uint64_t)CLOCKS_PER_SEC);
#else
    struct timespec ts;
    #if defined(CLOCK_MONOTONIC)
    clock_gettime(CLOCK_MONOTONIC, &ts);
    #else
    clock_gettime(CLOCK_REALTIME, &ts);
    #endif
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#endif
}

static double steel_ns_to_ms_(uint64_t ns)
{
    return (double)ns / 1000000.0;
}

//------------------------------------------------------------------------------
// Diagnostics hook (pluggable)
//------------------------------------------------------------------------------

static void steel_diag_default_(const steel_pass_ctx* ctx,
                               steel_diag_sev sev,
                               steel_span span,
                               const char* msg)
{
    (void)ctx;
    const char* ssev = "info";
    if (sev == STEEL_DIAG_WARN) ssev = "warn";
    if (sev == STEEL_DIAG_ERROR) ssev = "error";

    fprintf(stderr, "[%s] file=%u span=[%u..%u] line=%u col=%u: %s\n",
            ssev,
            span.file_id,
            span.start,
            span.end,
            span.line,
            span.col,
            msg ? msg : "(null)");
}

//------------------------------------------------------------------------------
// Pass runner
//------------------------------------------------------------------------------

typedef struct steel_pass_run
{
    const char* name;
    steel_pass_fn fn;
    double ms;
    bool ok;
} steel_pass_run;

static bool steel_run_pass_(steel_pass_run* pr, steel_pass_ctx* ctx)
{
    uint64_t t0 = steel_now_ns_();
    bool ok = pr->fn(ctx);
    uint64_t t1 = steel_now_ns_();

    pr->ok = ok;
    pr->ms = steel_ns_to_ms_(t1 - t0);

    if (ctx->trace)
    {
        fprintf(stderr, "[pass] %-18s : %s (%.3f ms)\n", pr->name, ok ? "OK" : "FAIL", pr->ms);
    }

    return ok;
}

//------------------------------------------------------------------------------
// Default passes
//------------------------------------------------------------------------------

static bool pass_lex_smoke_(steel_pass_ctx* ctx)
{
    // This pass is optional: it just ensures lexing doesn't immediately fail.
    // We do not store tokens here; parser will re-lex (or use same lexer) depending on your design.

    steel_lexer lx;
    steel_lexer_init(&lx, ctx->src, ctx->src_len, ctx->file_id);

    for (int i = 0; i < 3; i++)
    {
        steel_token t = steel_lexer_next(&lx);
        if (t.kind == STEEL_TOK_ERROR)
        {
            if (ctx->diag)
                ctx->diag(ctx, STEEL_DIAG_ERROR, t.span, t.text ? t.text : "lexer error");
            return false;
        }
        if (t.kind == STEEL_TOK_EOF) break;
    }

    return true;
}

static bool pass_parse_(steel_pass_ctx* ctx)
{
    if (!ctx->ast)
        return false;

    steel_parser p;
    steel_parser_init(&p, ctx->ast, ctx->src, ctx->src_len, ctx->file_id);

    ctx->ast_root = steel_parser_parse_file(&p);

    if (steel_parser_had_error(&p))
    {
        if (ctx->diag)
        {
            // best-effort span: use current token span
            steel_token t = steel_lexer_peek(&p.lx); // requires parser struct visibility in header; if not, omit.
            ctx->diag(ctx, STEEL_DIAG_ERROR, t.span, steel_parser_last_error(&p));
        }
        return false;
    }

    if (!ctx->ast_root)
    {
        if (ctx->diag)
        {
            steel_span sp = {0};
            sp.file_id = ctx->file_id;
            ctx->diag(ctx, STEEL_DIAG_ERROR, sp, "parser returned null root");
        }
        return false;
    }

    return true;
}

static bool pass_desugar_phrase_(steel_pass_ctx* ctx)
{
    // Optional hook: if your project has a phrase desugar pass, call it here.
    // In many setups, phrase is already parsed as statements and needs no rewrite.
    //
    // TODO: integrate real desugar:
    //    bool steel_desugar_phrase(steel_ast* ast, steel_ast_id root, steel_diag_fn diag, void* u);

    (void)ctx;
    return true;
}

static bool pass_hir_(steel_pass_ctx* ctx)
{
    if (!ctx->ast || !ctx->ast_root)
        return false;

    if (!ctx->hir_out)
        return false;

    steel_hir_build hb;
    steel_hir_build_init(&hb);

    bool ok = steel_hir_build_from_ast(&hb, ctx->ast, ctx->ast_root, ctx->hir_out);

    steel_hir_build_dispose(&hb);

    if (!ok && ctx->diag)
    {
        steel_span sp = {0};
        sp.file_id = ctx->file_id;
        ctx->diag(ctx, STEEL_DIAG_ERROR, sp, "HIR build failed");
    }

    return ok;
}

static bool pass_ir_(steel_pass_ctx* ctx)
{
    if (!ctx->hir_out || !ctx->ir_out)
        return false;

    steel_ir_build ib;
    steel_ir_build_init(&ib);

    bool ok = steel_ir_build_from_hir(&ib, ctx->hir_out, ctx->ir_out);

    steel_ir_build_dispose(&ib);

    if (!ok && ctx->diag)
    {
        steel_span sp = {0};
        sp.file_id = ctx->file_id;
        ctx->diag(ctx, STEEL_DIAG_ERROR, sp, "IR build failed");
    }

    return ok;
}

static bool pass_validate_ir_(steel_pass_ctx* ctx)
{
    if (!ctx->ir_out)
        return false;

    bool ok = steel_ir_validate(ctx->ir_out);
    if (!ok && ctx->diag)
    {
        steel_span sp = {0};
        sp.file_id = ctx->file_id;
        ctx->diag(ctx, STEEL_DIAG_ERROR, sp, "IR validation failed");
    }

    return ok;
}

//------------------------------------------------------------------------------
// Public pipeline
//------------------------------------------------------------------------------

void steel_pass_ctx_init(steel_pass_ctx* ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->diag = steel_diag_default_;
}

bool steel_run_default_pipeline(steel_pass_ctx* ctx)
{
    if (!ctx || !ctx->src)
        return false;

    steel_pass_run passes[] = {
        {"lex_smoke", pass_lex_smoke_ , 0.0, false},
        {"parse",     pass_parse_     , 0.0, false},
        {"desugar",   pass_desugar_phrase_, 0.0, false},
        {"hir",       pass_hir_       , 0.0, false},
        {"ir",        pass_ir_        , 0.0, false},
        {"ir_validate", pass_validate_ir_, 0.0, false},
    };

    const size_t n = sizeof(passes) / sizeof(passes[0]);

    for (size_t i = 0; i < n; i++)
    {
        if (!steel_run_pass_(&passes[i], ctx))
        {
            if (ctx->trace)
            {
                fprintf(stderr, "[pipeline] failed at %s\n", passes[i].name);
            }
            return false;
        }
    }

    if (ctx->trace)
    {
        fprintf(stderr, "[pipeline] OK\n");
        for (size_t i = 0; i < n; i++)
            fprintf(stderr, "  - %-18s %.3f ms\n", passes[i].name, passes[i].ms);
    }

    return true;
}
