

// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// bench_csv.c
// -----------------------------------------------------------------------------
// CSV export helpers for the compiler bench harness.
//
// This module is intentionally self-contained and C17-friendly.
// It implements RFC4180-style escaping (quotes doubled, fields quoted when
// needed) and provides a stable minimal API for the bench runner.
//
// The bench runner can feed an array of result rows and metadata.
// If the harness uses different structures, it can still call the low-level
// writer helpers (vitte_csv_*).
// -----------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Low-level CSV writer
// -----------------------------------------------------------------------------

typedef struct vitte_csv
{
    FILE* f;
    char sep; // usually ','
    bool at_bol;
} vitte_csv_t;

static void vitte_csv_init(vitte_csv_t* w, FILE* f)
{
    w->f = f;
    w->sep = ',';
    w->at_bol = true;
}

static bool vitte_csv_needs_quotes(const char* s)
{
    if (!s || !*s) return false;
    // Quote if contains separator, quote, CR/LF, or leading/trailing spaces.
    const unsigned char* p = (const unsigned char*)s;

    if (*p == ' ' || *p == '\t') return true;

    unsigned char last = 0;
    for (; *p; ++p)
    {
        const unsigned char c = *p;
        last = c;
        if (c == ',' || c == '"' || c == '\n' || c == '\r')
            return true;
    }

    if (last == ' ' || last == '\t') return true;
    return false;
}

static void vitte_csv_put_sep(vitte_csv_t* w)
{
    if (!w->at_bol)
        fputc(w->sep, w->f);
    w->at_bol = false;
}

static void vitte_csv_put_eol(vitte_csv_t* w)
{
    fputc('\n', w->f);
    w->at_bol = true;
}

static void vitte_csv_put_str(vitte_csv_t* w, const char* s)
{
    vitte_csv_put_sep(w);

    if (!s) return;

    if (!vitte_csv_needs_quotes(s))
    {
        fputs(s, w->f);
        return;
    }

    fputc('"', w->f);
    for (const char* p = s; *p; ++p)
    {
        if (*p == '"')
            fputc('"', w->f); // escape by doubling
        fputc(*p, w->f);
    }
    fputc('"', w->f);
}

static void vitte_csv_put_u64(vitte_csv_t* w, uint64_t v)
{
    vitte_csv_put_sep(w);
    // max 20 digits
    char buf[32];
    char* p = buf + sizeof(buf);
    *--p = '\0';
    if (v == 0)
    {
        *--p = '0';
    }
    else
    {
        while (v)
        {
            const uint64_t q = v / 10u;
            const uint64_t r = v - q * 10u;
            *--p = (char)('0' + (int)r);
            v = q;
        }
    }
    fputs(p, w->f);
}

static void vitte_csv_put_i64(vitte_csv_t* w, int64_t v)
{
    vitte_csv_put_sep(w);
    if (v < 0)
    {
        fputc('-', w->f);
        // avoid UB on INT64_MIN
        uint64_t uv = (uint64_t)(-(v + 1)) + 1u;
        vitte_csv_put_u64(&(vitte_csv_t){ .f = w->f, .sep = w->sep, .at_bol = false }, uv);
        return;
    }
    vitte_csv_put_u64(&(vitte_csv_t){ .f = w->f, .sep = w->sep, .at_bol = false }, (uint64_t)v);
}

// -----------------------------------------------------------------------------
// High-level bench CSV API
// -----------------------------------------------------------------------------

typedef struct vitte_bench_csv_meta
{
    // Optional metadata (may be NULL)
    const char* run_id;      // e.g. git sha, CI build id
    const char* started_at;  // ISO8601 timestamp
    const char* host;        // hostname
    const char* os;          // e.g. macOS/Linux/Windows
    const char* arch;        // e.g. arm64/x86_64
    const char* compiler;    // e.g. clang-18
    const char* flags;       // e.g. -O3 -march=native
} vitte_bench_csv_meta_t;

typedef struct vitte_bench_result_row
{
    const char* suite;
    const char* name;

    uint64_t iters;

    // Time statistics in nanoseconds.
    // Provide what you have; zero values will still be written.
    uint64_t ns_total;
    uint64_t ns_min;
    uint64_t ns_max;
    uint64_t ns_mean;
    uint64_t ns_p50;
    uint64_t ns_p90;
    uint64_t ns_p99;

    // Optional work/size counters.
    uint64_t bytes;
} vitte_bench_result_row_t;

static void vitte_bench_csv_write_header(vitte_csv_t* w)
{
    // metadata columns (repeat per row for simplicity)
    vitte_csv_put_str(w, "run_id");
    vitte_csv_put_str(w, "started_at");
    vitte_csv_put_str(w, "host");
    vitte_csv_put_str(w, "os");
    vitte_csv_put_str(w, "arch");
    vitte_csv_put_str(w, "compiler");
    vitte_csv_put_str(w, "flags");

    // result columns
    vitte_csv_put_str(w, "suite");
    vitte_csv_put_str(w, "case");
    vitte_csv_put_str(w, "iters");
    vitte_csv_put_str(w, "ns_total");
    vitte_csv_put_str(w, "ns_min");
    vitte_csv_put_str(w, "ns_max");
    vitte_csv_put_str(w, "ns_mean");
    vitte_csv_put_str(w, "ns_p50");
    vitte_csv_put_str(w, "ns_p90");
    vitte_csv_put_str(w, "ns_p99");
    vitte_csv_put_str(w, "bytes");

    vitte_csv_put_eol(w);
}

static void vitte_bench_csv_write_row(vitte_csv_t* w, const vitte_bench_csv_meta_t* m, const vitte_bench_result_row_t* r)
{
    vitte_csv_put_str(w, m ? m->run_id : NULL);
    vitte_csv_put_str(w, m ? m->started_at : NULL);
    vitte_csv_put_str(w, m ? m->host : NULL);
    vitte_csv_put_str(w, m ? m->os : NULL);
    vitte_csv_put_str(w, m ? m->arch : NULL);
    vitte_csv_put_str(w, m ? m->compiler : NULL);
    vitte_csv_put_str(w, m ? m->flags : NULL);

    vitte_csv_put_str(w, r ? r->suite : NULL);
    vitte_csv_put_str(w, r ? r->name : NULL);

    vitte_csv_put_u64(w, r ? r->iters : 0);
    vitte_csv_put_u64(w, r ? r->ns_total : 0);
    vitte_csv_put_u64(w, r ? r->ns_min : 0);
    vitte_csv_put_u64(w, r ? r->ns_max : 0);
    vitte_csv_put_u64(w, r ? r->ns_mean : 0);
    vitte_csv_put_u64(w, r ? r->ns_p50 : 0);
    vitte_csv_put_u64(w, r ? r->ns_p90 : 0);
    vitte_csv_put_u64(w, r ? r->ns_p99 : 0);
    vitte_csv_put_u64(w, r ? r->bytes : 0);

    vitte_csv_put_eol(w);
}

// Public API: write all rows to CSV.
//
// - If `path` is NULL or "-", writes to stdout.
// - Returns true on success.
bool vitte_bench_write_csv(const char* path,
                           const vitte_bench_csv_meta_t* meta,
                           const vitte_bench_result_row_t* rows,
                           size_t row_count)
{
    FILE* f = NULL;
    bool close_file = false;

    if (!path || (path[0] == '-' && path[1] == '\0'))
    {
        f = stdout;
        close_file = false;
    }
    else
    {
        f = fopen(path, "wb");
        if (!f) return false;
        close_file = true;
    }

    vitte_csv_t w;
    vitte_csv_init(&w, f);
    vitte_bench_csv_write_header(&w);

    for (size_t i = 0; i < row_count; ++i)
        vitte_bench_csv_write_row(&w, meta, &rows[i]);

    const int err = ferror(f);

    if (close_file)
        fclose(f);

    return err == 0;
}

// -----------------------------------------------------------------------------
// Optional: minimal self-check (compile-time only)
// -----------------------------------------------------------------------------

#ifdef VITTE_BENCH_CSV_SELFTEST
static void vitte_bench_csv_selftest(void)
{
    vitte_bench_csv_meta_t m = { "run123", "2026-01-01T00:00:00Z", "host", "os", "arch", "cc", "-O3" };
    vitte_bench_result_row_t r = { "suite", "case,needs\"quotes", 10, 100, 9, 11, 10, 10, 11, 11, 1234 };
    (void)vitte_bench_write_csv("-", &m, &r, 1);
}
#endif