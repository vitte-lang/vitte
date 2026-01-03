

// SPDX-License-Identifier: MIT
// diag.c
//
// Diagnostics core (max).
//
// Provides:
//  - Diagnostic record type (sev + code + span + message)
//  - A "bag" collector storing all diagnostics
//  - Optional sink callback for streaming diagnostics (stderr by default)
//  - Formatting helpers (emit/emitf)
//
// This module is intentionally usable even if `diag.h` is not ready.
// If you already have `diag.h`, include it and remove/disable the fallback block.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "../common/vec.h" // steel_vec
#include "../compiler/lexer.h" // steel_span

#include "codes.h" // steel_diag_code_name/message

//------------------------------------------------------------------------------
// Optional header integration
//------------------------------------------------------------------------------

#if defined(__has_include)
  #if __has_include("diag.h")
    #include "diag.h"
    #define STEEL_HAS_DIAG_H 1
  #endif
#endif

#ifndef STEEL_HAS_DIAG_H

typedef enum steel_diag_sev
{
    STEEL_DIAG_INFO = 0,
    STEEL_DIAG_WARN,
    STEEL_DIAG_ERROR,
} steel_diag_sev;

// `steel_diag_code` is expected from codes.h; provide fallback if needed.
#ifndef STEEL_DIAG_CODE_T
typedef int steel_diag_code;
#endif

typedef struct steel_diag
{
    steel_diag_sev sev;
    steel_diag_code code;
    steel_span span;
    const char* msg; // owned by bag
} steel_diag;

typedef void (*steel_diag_sink_fn)(void* user, const steel_diag* d);

typedef struct steel_diag_bag
{
    steel_vec diags; // steel_diag

    // counts
    uint32_t info_count;
    uint32_t warn_count;
    uint32_t error_count;

    // sink
    steel_diag_sink_fn sink;
    void* sink_user;

    bool ok;

} steel_diag_bag;

void steel_diag_bag_init(steel_diag_bag* b, steel_diag_sink_fn sink, void* sink_user);
void steel_diag_bag_dispose(steel_diag_bag* b);

void steel_diag_emit(steel_diag_bag* b, steel_diag_sev sev, steel_diag_code code, steel_span span, const char* msg);
void steel_diag_emitf(steel_diag_bag* b, steel_diag_sev sev, steel_diag_code code, steel_span span, const char* fmt, ...);

size_t steel_diag_count(const steel_diag_bag* b);
const steel_diag* steel_diag_at(const steel_diag_bag* b, size_t idx);
bool steel_diag_has_errors(const steel_diag_bag* b);

#endif // !STEEL_HAS_DIAG_H

//------------------------------------------------------------------------------
// Default sink
//------------------------------------------------------------------------------

static const char* sev_name_(steel_diag_sev sev)
{
    switch (sev)
    {
        case STEEL_DIAG_INFO: return "info";
        case STEEL_DIAG_WARN: return "warn";
        case STEEL_DIAG_ERROR: return "error";
        default: return "?";
    }
}

static void steel_diag_sink_stderr_(void* user, const steel_diag* d)
{
    (void)user;
    if (!d) return;

    const char* cname = steel_diag_code_name(d->code);

    // Keep format stable for tests/CI.
    fprintf(stderr,
            "[%s][%d:%s] file=%u span=[%u..%u] line=%u col=%u: %s\n",
            sev_name_(d->sev),
            (int)d->code,
            cname ? cname : "unknown",
            d->span.file_id,
            d->span.start,
            d->span.end,
            d->span.line,
            d->span.col,
            d->msg ? d->msg : "");
}

//------------------------------------------------------------------------------
// Utilities
//------------------------------------------------------------------------------

static char* steel_strdup_(const char* s)
{
    if (!s) s = "";
    size_t n = strlen(s);
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}

static char* steel_vformat_(const char* fmt, va_list ap)
{
    if (!fmt) fmt = "";

    va_list ap2;
    va_copy(ap2, ap);

    int need = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);

    if (need < 0)
        return steel_strdup_(fmt);

    size_t cap = (size_t)need + 1;
    char* buf = (char*)malloc(cap);
    if (!buf)
        return NULL;

    vsnprintf(buf, cap, fmt, ap);
    return buf;
}

static void steel_diag_bag_count_(steel_diag_bag* b, steel_diag_sev sev)
{
    if (!b) return;
    if (sev == STEEL_DIAG_INFO) b->info_count++;
    if (sev == STEEL_DIAG_WARN) b->warn_count++;
    if (sev == STEEL_DIAG_ERROR) b->error_count++;
    if (sev == STEEL_DIAG_ERROR) b->ok = false;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void steel_diag_bag_init(steel_diag_bag* b, steel_diag_sink_fn sink, void* sink_user)
{
    if (!b) return;
    memset(b, 0, sizeof(*b));

    steel_vec_init(&b->diags, sizeof(steel_diag));

    b->sink = sink ? sink : steel_diag_sink_stderr_;
    b->sink_user = sink_user;

    b->ok = true;
}

void steel_diag_bag_dispose(steel_diag_bag* b)
{
    if (!b) return;

    // Free owned messages
    size_t n = steel_vec_len(&b->diags);
    for (size_t i = 0; i < n; i++)
    {
        steel_diag* d = (steel_diag*)steel_vec_at(&b->diags, i);
        if (d && d->msg)
            free((void*)d->msg);
        if (d) d->msg = NULL;
    }

    steel_vec_free(&b->diags);

    memset(b, 0, sizeof(*b));
}

void steel_diag_emit(steel_diag_bag* b, steel_diag_sev sev, steel_diag_code code, steel_span span, const char* msg)
{
    if (!b)
        return;

    // Create record
    steel_diag d;
    memset(&d, 0, sizeof(d));
    d.sev = sev;
    d.code = code;
    d.span = span;

    // If msg is NULL, use default from codes table (if any)
    if (!msg || msg[0] == 0)
    {
        const char* def = steel_diag_code_message(code);
        if (def && def[0] != 0)
            msg = def;
        else
            msg = "";
    }

    d.msg = steel_strdup_(msg);
    if (!d.msg)
    {
        // Keep going: record a minimal OOM diag.
        d.msg = "<oom>";
        d.code = 2; // STEEL_D0002_OUT_OF_MEMORY (stable numeric)
        d.sev = STEEL_DIAG_ERROR;
    }

    steel_vec_push(&b->diags, &d);
    steel_diag_bag_count_(b, d.sev);

    if (b->sink)
        b->sink(b->sink_user, &d);
}

void steel_diag_emitf(steel_diag_bag* b, steel_diag_sev sev, steel_diag_code code, steel_span span, const char* fmt, ...)
{
    if (!b)
        return;

    va_list ap;
    va_start(ap, fmt);
    char* msg = steel_vformat_(fmt, ap);
    va_end(ap);

    if (!msg)
    {
        // OOM on formatting, fallback.
        steel_diag_emit(b, STEEL_DIAG_ERROR, 2 /* oom */, span, "out of memory");
        return;
    }

    steel_diag_emit(b, sev, code, span, msg);

    // steel_diag_emit copied the string; free temp.
    free(msg);
}

size_t steel_diag_count(const steel_diag_bag* b)
{
    if (!b) return 0;
    return steel_vec_len(&b->diags);
}

const steel_diag* steel_diag_at(const steel_diag_bag* b, size_t idx)
{
    if (!b) return NULL;
    if (idx >= steel_vec_len(&b->diags)) return NULL;
    return (const steel_diag*)steel_vec_cat(&b->diags, idx);
}

bool steel_diag_has_errors(const steel_diag_bag* b)
{
    if (!b) return false;
    return b->error_count != 0;
}
