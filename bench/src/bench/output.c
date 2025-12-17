

// output.c - benchmark results output for vitte/bench (C17)
//
// Provides:
//   - Human-readable console table
//   - JSON and CSV writers (no external deps)
//
// Integration philosophy:
//   - If project headers exist (bench/output.h etc.), they are included.
//   - Otherwise, this unit provides a small fallback ABI so it can compile.
//
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#if defined(__has_include)
  #if __has_include("bench/output.h")
    #include "bench/output.h"
  #elif __has_include("output.h")
    #include "output.h"
  #endif
#endif

#if defined(__has_include)
  #if __has_include("bench/log.h")
    #include "bench/log.h"
  #elif __has_include("log.h")
    #include "log.h"
  #endif
#endif

// -----------------------------------------------------------------------------
// Fallback public surface (only if output.h was not included)
// -----------------------------------------------------------------------------

#ifndef VITTE_BENCH_OUTPUT_H
#define VITTE_BENCH_OUTPUT_H

typedef enum bench_status
{
    BENCH_STATUS_OK = 0,
    BENCH_STATUS_FAILED = 1,
    BENCH_STATUS_SKIPPED = 2
} bench_status;

typedef struct bench_metric
{
    // Primary metric: ns/op (nanoseconds per operation)
    double ns_per_op;

    // Optional throughput metrics
    double bytes_per_sec;
    double items_per_sec;

    // Run context
    int64_t iterations;
    double elapsed_ms;
} bench_metric;

typedef struct bench_result
{
    const char* name;
    bench_status status;
    bench_metric metric;

    // Optional failure reason (may be NULL)
    const char* error;
} bench_result;

typedef struct bench_report
{
    const bench_result* results;
    int32_t count;

    // Optional metadata
    const char* suite_name;
    uint64_t seed;
    int32_t threads;
    int32_t repeat;
    int32_t warmup;
    int64_t timestamp_ms;
} bench_report;

// Print a compact table to `out`.
void bench_output_print_human(FILE* out, const bench_report* rep);

// Write JSON report to path. Returns true on success.
bool bench_output_write_json_path(const char* path, const bench_report* rep);

// Write CSV report to path. Returns true on success.
bool bench_output_write_csv_path(const char* path, const bench_report* rep);

#endif // VITTE_BENCH_OUTPUT_H

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

static const char* bo__status_str(bench_status st)
{
    switch (st)
    {
        case BENCH_STATUS_OK: return "ok";
        case BENCH_STATUS_FAILED: return "failed";
        case BENCH_STATUS_SKIPPED: return "skipped";
        default: return "unknown";
    }
}

static bool bo__is_finite(double x)
{
    // portable-ish finite check: NaN fails x==x; inf fails subtraction test.
    if (!(x == x)) return false;
    double y = x - x;
    return (y == 0.0);
}

static void bo__print_num(FILE* out, double v, const char* fallback)
{
    if (!out) return;
    if (!bo__is_finite(v))
    {
        (void)fputs(fallback ? fallback : "-", out);
        return;
    }
    (void)fprintf(out, "%.6f", v);
}

static void bo__json_escape_write(FILE* out, const char* s)
{
    if (!out) return;
    if (!s) s = "";

    for (const unsigned char* p = (const unsigned char*)s; *p; ++p)
    {
        unsigned char c = *p;
        switch (c)
        {
            case '"': (void)fputs("\\\"", out); break;
            case '\\': (void)fputs("\\\\", out); break;
            case '\b': (void)fputs("\\b", out); break;
            case '\f': (void)fputs("\\f", out); break;
            case '\n': (void)fputs("\\n", out); break;
            case '\r': (void)fputs("\\r", out); break;
            case '\t': (void)fputs("\\t", out); break;
            default:
                if (c < 0x20)
                {
                    (void)fprintf(out, "\\u%04x", (unsigned)c);
                }
                else
                {
                    (void)fputc((int)c, out);
                }
                break;
        }
    }
}

static void bo__csv_write_cell(FILE* out, const char* s)
{
    if (!out) return;
    if (!s) s = "";

    bool need_quote = false;
    for (const char* p = s; *p; ++p)
    {
        if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') { need_quote = true; break; }
    }

    if (!need_quote)
    {
        (void)fputs(s, out);
        return;
    }

    (void)fputc('"', out);
    for (const char* p = s; *p; ++p)
    {
        if (*p == '"') (void)fputc('"', out);
        (void)fputc(*p, out);
    }
    (void)fputc('"', out);
}

static int bo__max_i(int a, int b) { return (a > b) ? a : b; }

static int bo__name_width(const bench_report* rep, int minw, int maxw)
{
    int w = minw;
    if (!rep || !rep->results || rep->count <= 0) return w;
    for (int32_t i = 0; i < rep->count; ++i)
    {
        const char* n = rep->results[i].name;
        if (!n) n = "";
        int len = (int)strlen(n);
        w = bo__max_i(w, len);
        if (w >= maxw) return maxw;
    }
    if (w > maxw) w = maxw;
    return w;
}

static bool bo__write_json_stream(FILE* out, const bench_report* rep)
{
    if (!out || !rep) return false;

    (void)fputs("{\n", out);

    // metadata
    (void)fputs("  \"suite\": \"", out);
    bo__json_escape_write(out, rep->suite_name ? rep->suite_name : "bench");
    (void)fputs("\",\n", out);

    (void)fprintf(out, "  \"timestamp_ms\": %" PRId64 ",\n", (int64_t)rep->timestamp_ms);
    (void)fprintf(out, "  \"seed\": %" PRIu64 ",\n", (uint64_t)rep->seed);
    (void)fprintf(out, "  \"threads\": %d,\n", (int)rep->threads);
    (void)fprintf(out, "  \"repeat\": %d,\n", (int)rep->repeat);
    (void)fprintf(out, "  \"warmup\": %d,\n", (int)rep->warmup);

    // results
    (void)fputs("  \"results\": [\n", out);

    for (int32_t i = 0; i < rep->count; ++i)
    {
        const bench_result* r = &rep->results[i];
        if (!r) continue;

        (void)fputs("    {\n", out);

        (void)fputs("      \"name\": \"", out);
        bo__json_escape_write(out, r->name ? r->name : "");
        (void)fputs("\",\n", out);

        (void)fputs("      \"status\": \"", out);
        bo__json_escape_write(out, bo__status_str(r->status));
        (void)fputs("\",\n", out);

        (void)fputs("      \"iterations\": ", out);
        (void)fprintf(out, "%" PRId64 ",\n", (int64_t)r->metric.iterations);

        (void)fputs("      \"elapsed_ms\": ", out);
        bo__print_num(out, r->metric.elapsed_ms, "0");
        (void)fputs(",\n", out);

        (void)fputs("      \"ns_per_op\": ", out);
        bo__print_num(out, r->metric.ns_per_op, "0");
        (void)fputs(",\n", out);

        (void)fputs("      \"bytes_per_sec\": ", out);
        bo__print_num(out, r->metric.bytes_per_sec, "0");
        (void)fputs(",\n", out);

        (void)fputs("      \"items_per_sec\": ", out);
        bo__print_num(out, r->metric.items_per_sec, "0");
        (void)fputs(",\n", out);

        (void)fputs("      \"error\": ", out);
        if (r->error && *r->error)
        {
            (void)fputs("\"", out);
            bo__json_escape_write(out, r->error);
            (void)fputs("\"\n", out);
        }
        else
        {
            (void)fputs("null\n", out);
        }

        (void)fputs("    }", out);
        if (i + 1 < rep->count) (void)fputc(',', out);
        (void)fputc('\n', out);
    }

    (void)fputs("  ]\n", out);
    (void)fputs("}\n", out);

    return ferror(out) == 0;
}

static bool bo__write_csv_stream(FILE* out, const bench_report* rep)
{
    if (!out || !rep) return false;

    // header
    (void)fputs("name,status,iterations,elapsed_ms,ns_per_op,bytes_per_sec,items_per_sec,error\n", out);

    for (int32_t i = 0; i < rep->count; ++i)
    {
        const bench_result* r = &rep->results[i];
        if (!r) continue;

        bo__csv_write_cell(out, r->name ? r->name : "");
        (void)fputc(',', out);
        bo__csv_write_cell(out, bo__status_str(r->status));
        (void)fputc(',', out);
        (void)fprintf(out, "%" PRId64 ",", (int64_t)r->metric.iterations);

        // numbers
        if (bo__is_finite(r->metric.elapsed_ms)) (void)fprintf(out, "%.6f,", r->metric.elapsed_ms);
        else (void)fputs("0,", out);

        if (bo__is_finite(r->metric.ns_per_op)) (void)fprintf(out, "%.6f,", r->metric.ns_per_op);
        else (void)fputs("0,", out);

        if (bo__is_finite(r->metric.bytes_per_sec)) (void)fprintf(out, "%.6f,", r->metric.bytes_per_sec);
        else (void)fputs("0,", out);

        if (bo__is_finite(r->metric.items_per_sec)) (void)fprintf(out, "%.6f,", r->metric.items_per_sec);
        else (void)fputs("0,", out);

        bo__csv_write_cell(out, (r->error && *r->error) ? r->error : "");
        (void)fputc('\n', out);
    }

    return ferror(out) == 0;
}

static bool bo__write_file_atomic(const char* path, bool (*writer)(FILE*, const bench_report*), const bench_report* rep)
{
    if (!path || !*path || !writer || !rep) return false;

    // Best-effort atomic write: write to path.tmp then rename.
    // On Windows, rename behavior can differ; we still attempt.
    char tmp_path[1024];
    size_t n = strlen(path);
    if (n + 5 >= sizeof(tmp_path)) return false;
    (void)snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE* f = fopen(tmp_path, "wb");
    if (!f) return false;

    bool ok = writer(f, rep);
    (void)fflush(f);
    ok = ok && (ferror(f) == 0);
    (void)fclose(f);

    if (!ok)
    {
        (void)remove(tmp_path);
        return false;
    }

    // Replace destination
    (void)remove(path);
    if (rename(tmp_path, path) != 0)
    {
        // fallback: try to copy
        FILE* src = fopen(tmp_path, "rb");
        FILE* dst = fopen(path, "wb");
        if (!src || !dst)
        {
            if (src) fclose(src);
            if (dst) fclose(dst);
            (void)remove(tmp_path);
            return false;
        }
        char buf[64 * 1024];
        size_t rd;
        while ((rd = fread(buf, 1, sizeof(buf), src)) > 0)
        {
            if (fwrite(buf, 1, rd, dst) != rd) { ok = false; break; }
        }
        ok = ok && (ferror(src) == 0) && (ferror(dst) == 0);
        fclose(src);
        fclose(dst);
        (void)remove(tmp_path);
        return ok;
    }

    return true;
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void bench_output_print_human(FILE* out, const bench_report* rep)
{
    if (!out) out = stdout;
    if (!rep || !rep->results || rep->count <= 0)
    {
        (void)fprintf(out, "(no results)\n");
        return;
    }

    const int name_w = bo__name_width(rep, 8, 48);

    (void)fprintf(out, "suite: %s\n", rep->suite_name ? rep->suite_name : "bench");
    (void)fprintf(out, "count: %d  threads: %d  repeat: %d  warmup: %d  seed: %" PRIu64 "\n\n",
                  (int)rep->count, (int)rep->threads, (int)rep->repeat, (int)rep->warmup, (uint64_t)rep->seed);

    (void)fprintf(out, "%-*s  %-7s  %12s  %12s  %14s  %14s\n",
                  name_w, "name", "status", "iters", "ms", "ns/op", "bytes/s");
    for (int k = 0; k < name_w + 2 + 7 + 2 + 12 + 2 + 12 + 2 + 14 + 2 + 14; ++k) (void)fputc('-', out);
    (void)fputc('\n', out);

    int okc = 0, failc = 0, skipc = 0;

    for (int32_t i = 0; i < rep->count; ++i)
    {
        const bench_result* r = &rep->results[i];
        const char* name = (r->name ? r->name : "");
        const char* st = bo__status_str(r->status);

        if (r->status == BENCH_STATUS_OK) okc++;
        else if (r->status == BENCH_STATUS_FAILED) failc++;
        else if (r->status == BENCH_STATUS_SKIPPED) skipc++;

        (void)fprintf(out, "%-*.*s  %-7s  %12" PRId64 "  ",
                      name_w, name_w, name, st, (int64_t)r->metric.iterations);

        if (bo__is_finite(r->metric.elapsed_ms)) (void)fprintf(out, "%12.6f  ", r->metric.elapsed_ms);
        else (void)fprintf(out, "%12s  ", "-");

        if (bo__is_finite(r->metric.ns_per_op)) (void)fprintf(out, "%14.6f  ", r->metric.ns_per_op);
        else (void)fprintf(out, "%14s  ", "-");

        if (bo__is_finite(r->metric.bytes_per_sec) && r->metric.bytes_per_sec > 0.0)
            (void)fprintf(out, "%14.3f", r->metric.bytes_per_sec);
        else
            (void)fprintf(out, "%14s", "-");

        (void)fputc('\n', out);

        if (r->status == BENCH_STATUS_FAILED && r->error && *r->error)
        {
            (void)fprintf(out, "  -> %s\n", r->error);
        }
    }

    (void)fprintf(out, "\nsummary: ok=%d failed=%d skipped=%d\n", okc, failc, skipc);
}

bool bench_output_write_json_path(const char* path, const bench_report* rep)
{
    if (!path || !*path || !rep) return false;
    bool ok = bo__write_file_atomic(path, bo__write_json_stream, rep);

#if defined(BLOG_INFO)
    if (ok) BLOG_INFO("wrote json: %s", path);
    else BLOG_WARN("failed to write json: %s", path);
#endif

    return ok;
}

bool bench_output_write_csv_path(const char* path, const bench_report* rep)
{
    if (!path || !*path || !rep) return false;
    bool ok = bo__write_file_atomic(path, bo__write_csv_stream, rep);

#if defined(BLOG_INFO)
    if (ok) BLOG_INFO("wrote csv: %s", path);
    else BLOG_WARN("failed to write csv: %s", path);
#endif

    return ok;
}