

// output.c - benchmark results output for vitte/bench (C17)
//
// Provides:
//   - Human-readable console table
//   - JSON and CSV writers (no external deps)
//
// Integration philosophy:
//   - Built as part of vitte/bench; depends on the bench headers/ABI.
//
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <locale.h>

#if defined(VITTE_ENABLE_RUST_API) && VITTE_ENABLE_RUST_API
  #include "vitte_rust_api.h"
#endif

#include "bench/output.h"

#include "bench/log.h"

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

    const char* schema = (rep->schema && *rep->schema) ? rep->schema : "vitte.bench.v1";
    (void)fputs("  \"schema\": \"", out);
    bo__json_escape_write(out, schema);
    (void)fputs("\",\n", out);

    // metadata
    (void)fputs("  \"suite\": \"", out);
    bo__json_escape_write(out, rep->suite_name ? rep->suite_name : "bench");
    (void)fputs("\",\n", out);

    (void)fprintf(out, "  \"timestamp_ms\": %" PRId64 ",\n", (int64_t)rep->timestamp_ms);
    (void)fprintf(out, "  \"seed\": %" PRIu64 ",\n", (uint64_t)rep->seed);
    (void)fprintf(out, "  \"threads\": %d,\n", (int)rep->threads);
    (void)fprintf(out, "  \"repeat\": %d,\n", (int)rep->repeat);
    (void)fprintf(out, "  \"warmup\": %d,\n", (int)rep->warmup);
    (void)fprintf(out, "  \"iters\": %" PRId64 ",\n", (int64_t)rep->iters);
    (void)fprintf(out, "  \"calibrate_ms\": %" PRId64 ",\n", (int64_t)rep->calibrate_ms);
    (void)fprintf(out, "  \"cpu\": {\"requested\": %d, \"pinned\": %d},\n",
                  (int)rep->cpu_index, (int)rep->cpu_pinned);

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

        (void)fputs("      \"ns_per_op_stats\": {\n", out);
        (void)fputs("        \"median\": ", out); bo__print_num(out, r->metric.ns_per_op_median, "0"); (void)fputs(",\n", out);
        (void)fputs("        \"p95\": ", out); bo__print_num(out, r->metric.ns_per_op_p95, "0"); (void)fputs(",\n", out);
        (void)fputs("        \"mad\": ", out); bo__print_num(out, r->metric.ns_per_op_mad, "0"); (void)fputs(",\n", out);
        (void)fputs("        \"iqr\": ", out); bo__print_num(out, r->metric.ns_per_op_iqr, "0"); (void)fputs(",\n", out);
        (void)fputs("        \"ci95_low\": ", out); bo__print_num(out, r->metric.ns_per_op_ci95_low, "0"); (void)fputs(",\n", out);
        (void)fputs("        \"ci95_high\": ", out); bo__print_num(out, r->metric.ns_per_op_ci95_high, "0"); (void)fputs("\n", out);
        (void)fputs("      },\n", out);

        (void)fputs("      \"runner\": {", out);
        (void)fprintf(out, "\"iters_per_call\": %" PRId64 ", ", (int64_t)r->metric.iters_per_call);
        (void)fprintf(out, "\"calls_per_sample\": %" PRId64 ", ", (int64_t)r->metric.calls_per_sample);
        (void)fprintf(out, "\"target_time_ms\": %" PRId64 "},\n", (int64_t)r->metric.target_time_ms);

        (void)fputs("      \"cpu_telemetry\": {", out);
        (void)fputs("\"cycles_per_sec_min\": ", out); bo__print_num(out, r->metric.cycles_per_sec_min, "0"); (void)fputs(", ", out);
        (void)fputs("\"cycles_per_sec_max\": ", out); bo__print_num(out, r->metric.cycles_per_sec_max, "0"); (void)fputs(", ", out);
        (void)fprintf(out, "\"throttling_suspected\": %s},\n",
                      r->metric.throttling_suspected ? "true" : "false");

        if (rep->include_samples)
        {
            (void)fputs("      \"samples_ns_per_op\": [", out);
            for (int32_t k = 0; k < r->samples_count; ++k)
            {
                if (k) (void)fputs(", ", out);
                bo__print_num(out, r->samples_ns_per_op ? r->samples_ns_per_op[k] : 0.0, "0");
            }
            (void)fputs("],\n", out);
        }

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

#if defined(VITTE_ENABLE_RUST_API) && VITTE_ENABLE_RUST_API
static int32_t bo__rust_write_file(void* ctx, const uint8_t* bytes, size_t len)
{
    if (!ctx) return (int32_t)VITTE_ERR_NULL_POINTER;
    FILE* f = (FILE*)ctx;
    if (!bytes && len != 0) return (int32_t)VITTE_ERR_NULL_POINTER;
    if (len == 0) return 0;
    const size_t wr = fwrite(bytes, 1, len, f);
    if (wr != len || ferror(f)) return (int32_t)VITTE_ERR_IO;
    return 0;
}

static int32_t bo__idx_check(const bench_report* rep, int32_t idx)
{
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    if (idx < 0 || idx >= rep->count) return (int32_t)VITTE_ERR_OUT_OF_RANGE;
    if (!rep->results && rep->count != 0) return (int32_t)VITTE_ERR_NULL_POINTER;
    return 0;
}

static int32_t bo__get_schema(void* ctx, vitte_str_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = vitte_str_from_cstr(rep->schema ? rep->schema : "");
    return 0;
}

static int32_t bo__get_suite(void* ctx, vitte_str_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = vitte_str_from_cstr(rep->suite_name ? rep->suite_name : "");
    return 0;
}

static int32_t bo__get_timestamp_ms(void* ctx, int64_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = rep->timestamp_ms;
    return 0;
}

static int32_t bo__get_seed(void* ctx, uint64_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = rep->seed;
    return 0;
}

static int32_t bo__get_threads(void* ctx, int32_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = rep->threads;
    return 0;
}

static int32_t bo__get_repeat(void* ctx, int32_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = rep->repeat;
    return 0;
}

static int32_t bo__get_warmup(void* ctx, int32_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = rep->warmup;
    return 0;
}

static int32_t bo__get_iters(void* ctx, int64_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = rep->iters;
    return 0;
}

static int32_t bo__get_calibrate_ms(void* ctx, int64_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = rep->calibrate_ms;
    return 0;
}

static int32_t bo__get_cpu_index(void* ctx, int32_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = rep->cpu_index;
    return 0;
}

static int32_t bo__get_cpu_pinned(void* ctx, int32_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = rep->cpu_pinned;
    return 0;
}

static int32_t bo__get_include_samples(void* ctx, int32_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = rep->include_samples ? 1 : 0;
    return 0;
}

static int32_t bo__get_results_count(void* ctx, int32_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    if (!rep) return (int32_t)VITTE_ERR_NULL_POINTER;
    *out = rep->count;
    return 0;
}

static int32_t bo__get_result_name(void* ctx, int32_t idx, vitte_str_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    const int32_t rc = bo__idx_check(rep, idx);
    if (rc != 0) return rc;
    const bench_result* r = &rep->results[idx];
    *out = vitte_str_from_cstr(r->name ? r->name : "");
    return 0;
}

static int32_t bo__get_result_status(void* ctx, int32_t idx, int32_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    const int32_t rc = bo__idx_check(rep, idx);
    if (rc != 0) return rc;
    const bench_result* r = &rep->results[idx];
    *out = (int32_t)r->status;
    return 0;
}

static int32_t bo__get_result_error(void* ctx, int32_t idx, vitte_str_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    const int32_t rc = bo__idx_check(rep, idx);
    if (rc != 0) return rc;
    const bench_result* r = &rep->results[idx];
    *out = vitte_str_from_cstr(r->error ? r->error : "");
    return 0;
}

#define BO__METRIC_GETTER_F64(name, field) \
static int32_t name(void* ctx, int32_t idx, double* out) \
{ \
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER; \
    const bench_report* rep = (const bench_report*)ctx; \
    const int32_t rc = bo__idx_check(rep, idx); \
    if (rc != 0) return rc; \
    const bench_result* r = &rep->results[idx]; \
    *out = r->metric.field; \
    return 0; \
}

#define BO__METRIC_GETTER_I64(name, field) \
static int32_t name(void* ctx, int32_t idx, int64_t* out) \
{ \
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER; \
    const bench_report* rep = (const bench_report*)ctx; \
    const int32_t rc = bo__idx_check(rep, idx); \
    if (rc != 0) return rc; \
    const bench_result* r = &rep->results[idx]; \
    *out = r->metric.field; \
    return 0; \
}

BO__METRIC_GETTER_F64(bo__m_ns_per_op, ns_per_op)
BO__METRIC_GETTER_F64(bo__m_ns_per_op_median, ns_per_op_median)
BO__METRIC_GETTER_F64(bo__m_ns_per_op_p95, ns_per_op_p95)
BO__METRIC_GETTER_F64(bo__m_ns_per_op_mad, ns_per_op_mad)
BO__METRIC_GETTER_F64(bo__m_ns_per_op_iqr, ns_per_op_iqr)
BO__METRIC_GETTER_F64(bo__m_ns_per_op_ci95_low, ns_per_op_ci95_low)
BO__METRIC_GETTER_F64(bo__m_ns_per_op_ci95_high, ns_per_op_ci95_high)
BO__METRIC_GETTER_F64(bo__m_bytes_per_sec, bytes_per_sec)
BO__METRIC_GETTER_F64(bo__m_items_per_sec, items_per_sec)
BO__METRIC_GETTER_I64(bo__m_iterations, iterations)
BO__METRIC_GETTER_F64(bo__m_elapsed_ms, elapsed_ms)
BO__METRIC_GETTER_I64(bo__m_iters_per_call, iters_per_call)
BO__METRIC_GETTER_I64(bo__m_calls_per_sample, calls_per_sample)
BO__METRIC_GETTER_I64(bo__m_target_time_ms, target_time_ms)
BO__METRIC_GETTER_F64(bo__m_cycles_per_sec_min, cycles_per_sec_min)
BO__METRIC_GETTER_F64(bo__m_cycles_per_sec_max, cycles_per_sec_max)

static int32_t bo__m_throttling_suspected(void* ctx, int32_t idx, int32_t* out)
{
    if (!out) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    const int32_t rc = bo__idx_check(rep, idx);
    if (rc != 0) return rc;
    const bench_result* r = &rep->results[idx];
    *out = r->metric.throttling_suspected ? 1 : 0;
    return 0;
}

static int32_t bo__get_samples(void* ctx, int32_t idx, const double** out_ptr, int32_t* out_count)
{
    if (!out_ptr || !out_count) return (int32_t)VITTE_ERR_NULL_POINTER;
    const bench_report* rep = (const bench_report*)ctx;
    const int32_t rc = bo__idx_check(rep, idx);
    if (rc != 0) return rc;
    const bench_result* r = &rep->results[idx];
    *out_ptr = r->samples_ns_per_op;
    *out_count = r->samples_count;
    return 0;
}

static const vitte_bench_report_vtable_t BO__VT = {
    .get_schema = bo__get_schema,
    .get_suite = bo__get_suite,
    .get_timestamp_ms = bo__get_timestamp_ms,
    .get_seed = bo__get_seed,
    .get_threads = bo__get_threads,
    .get_repeat = bo__get_repeat,
    .get_warmup = bo__get_warmup,
    .get_iters = bo__get_iters,
    .get_calibrate_ms = bo__get_calibrate_ms,
    .get_cpu_index = bo__get_cpu_index,
    .get_cpu_pinned = bo__get_cpu_pinned,
    .get_include_samples = bo__get_include_samples,

    .get_results_count = bo__get_results_count,
    .get_result_name = bo__get_result_name,
    .get_result_status = bo__get_result_status,
    .get_result_error = bo__get_result_error,

    .get_metric_ns_per_op = bo__m_ns_per_op,
    .get_metric_ns_per_op_median = bo__m_ns_per_op_median,
    .get_metric_ns_per_op_p95 = bo__m_ns_per_op_p95,
    .get_metric_ns_per_op_mad = bo__m_ns_per_op_mad,
    .get_metric_ns_per_op_iqr = bo__m_ns_per_op_iqr,
    .get_metric_ns_per_op_ci95_low = bo__m_ns_per_op_ci95_low,
    .get_metric_ns_per_op_ci95_high = bo__m_ns_per_op_ci95_high,
    .get_metric_bytes_per_sec = bo__m_bytes_per_sec,
    .get_metric_items_per_sec = bo__m_items_per_sec,
    .get_metric_iterations = bo__m_iterations,
    .get_metric_elapsed_ms = bo__m_elapsed_ms,
    .get_metric_iters_per_call = bo__m_iters_per_call,
    .get_metric_calls_per_sample = bo__m_calls_per_sample,
    .get_metric_target_time_ms = bo__m_target_time_ms,
    .get_metric_cycles_per_sec_min = bo__m_cycles_per_sec_min,
    .get_metric_cycles_per_sec_max = bo__m_cycles_per_sec_max,
    .get_metric_throttling_suspected = bo__m_throttling_suspected,

    .get_samples = bo__get_samples,
};


static inline vitte_err_code_t bo__rust_api_validate(void)
{
    if (vitte_rust_api_abi_version() != VITTE_RUST_API_ABI_VERSION) {
        return VITTE_ERR_ABI_VERSION_UNSUPPORTED;
    }
    return VITTE_ERR_OK;
}

static vitte_err_code_t bo__write_json_stream_rust_status(FILE* out, const bench_report* rep)
{
    if (!out || !rep) return VITTE_ERR_INVALID_ARGUMENT;
    const vitte_err_code_t validation = bo__rust_api_validate();
    if (validation != VITTE_ERR_OK) return validation;
    const vitte_bench_report_view_t view = { .ctx = (void*)rep, .vt = &BO__VT };
    const vitte_writer_t w = { .ctx = (void*)out, .write = bo__rust_write_file, .max_bytes = 0 };
    const vitte_status_t st = vitte_bench_report_write_json(view, w);
    if (st.code != (int32_t)VITTE_ERR_OK) return (vitte_err_code_t)st.code;
    if (ferror(out) != 0) return VITTE_ERR_IO;
    return VITTE_ERR_OK;
}

static bool bo__write_json_stream_rust(FILE* out, const bench_report* rep)
{
    return bo__write_json_stream_rust_status(out, rep) == VITTE_ERR_OK;
}

static vitte_err_code_t bo__write_csv_stream_rust_status(FILE* out, const bench_report* rep)
{
    if (!out || !rep) return VITTE_ERR_INVALID_ARGUMENT;
    const vitte_err_code_t validation = bo__rust_api_validate();
    if (validation != VITTE_ERR_OK) return validation;
    const vitte_bench_report_view_t view = { .ctx = (void*)rep, .vt = &BO__VT };
    const vitte_writer_t w = { .ctx = (void*)out, .write = bo__rust_write_file, .max_bytes = 0 };
    const vitte_status_t st = vitte_bench_report_write_csv(view, w);
    if (st.code != (int32_t)VITTE_ERR_OK) return (vitte_err_code_t)st.code;
    if (ferror(out) != 0) return VITTE_ERR_IO;
    return VITTE_ERR_OK;
}

static bool bo__write_csv_stream_rust(FILE* out, const bench_report* rep)
{
    return bo__write_csv_stream_rust_status(out, rep) == VITTE_ERR_OK;
}
#endif

static bool bo__write_csv_stream(FILE* out, const bench_report* rep)
{
    if (!out || !rep) return false;

    // header
    (void)fputs(
        "schema,name,status,iterations,elapsed_ms,ns_per_op,ns_per_op_median,ns_per_op_p95,ns_per_op_mad,ns_per_op_iqr,ns_per_op_ci95_low,ns_per_op_ci95_high,"
        "bytes_per_sec,items_per_sec,"
        "iters_per_call,calls_per_sample,target_time_ms,"
        "cycles_per_sec_min,cycles_per_sec_max,throttling_suspected,"
        "error\n",
        out);

    for (int32_t i = 0; i < rep->count; ++i)
    {
        const bench_result* r = &rep->results[i];
        if (!r) continue;

        const char* schema = (rep->schema && *rep->schema) ? rep->schema : "vitte.bench.v1";
        bo__csv_write_cell(out, schema);
        (void)fputc(',', out);
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

        if (bo__is_finite(r->metric.ns_per_op_median)) (void)fprintf(out, "%.6f,", r->metric.ns_per_op_median);
        else (void)fputs("0,", out);

        if (bo__is_finite(r->metric.ns_per_op_p95)) (void)fprintf(out, "%.6f,", r->metric.ns_per_op_p95);
        else (void)fputs("0,", out);

        if (bo__is_finite(r->metric.ns_per_op_mad)) (void)fprintf(out, "%.6f,", r->metric.ns_per_op_mad);
        else (void)fputs("0,", out);

        if (bo__is_finite(r->metric.ns_per_op_iqr)) (void)fprintf(out, "%.6f,", r->metric.ns_per_op_iqr);
        else (void)fputs("0,", out);

        if (bo__is_finite(r->metric.ns_per_op_ci95_low)) (void)fprintf(out, "%.6f,", r->metric.ns_per_op_ci95_low);
        else (void)fputs("0,", out);

        if (bo__is_finite(r->metric.ns_per_op_ci95_high)) (void)fprintf(out, "%.6f,", r->metric.ns_per_op_ci95_high);
        else (void)fputs("0,", out);

        if (bo__is_finite(r->metric.bytes_per_sec)) (void)fprintf(out, "%.6f,", r->metric.bytes_per_sec);
        else (void)fputs("0,", out);

        if (bo__is_finite(r->metric.items_per_sec)) (void)fprintf(out, "%.6f,", r->metric.items_per_sec);
        else (void)fputs("0,", out);

        (void)fprintf(out, "%" PRId64 ",", (int64_t)r->metric.iters_per_call);
        (void)fprintf(out, "%" PRId64 ",", (int64_t)r->metric.calls_per_sample);
        (void)fprintf(out, "%" PRId64 ",", (int64_t)r->metric.target_time_ms);

        if (bo__is_finite(r->metric.cycles_per_sec_min)) (void)fprintf(out, "%.6f,", r->metric.cycles_per_sec_min);
        else (void)fputs("0,", out);

        if (bo__is_finite(r->metric.cycles_per_sec_max)) (void)fprintf(out, "%.6f,", r->metric.cycles_per_sec_max);
        else (void)fputs("0,", out);

        (void)fputs(r->metric.throttling_suspected ? "true," : "false,", out);

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
    const char* prev = setlocale(LC_NUMERIC, NULL);
    char prev_copy[64];
    prev_copy[0] = '\0';
    if (prev) (void)snprintf(prev_copy, sizeof(prev_copy), "%s", prev);
    (void)setlocale(LC_NUMERIC, "C");
    bool ok = false;
    if (strcmp(path, "-") == 0)
    {
#if defined(VITTE_ENABLE_RUST_API) && VITTE_ENABLE_RUST_API
        ok = bo__write_json_stream_rust(stdout, rep);
        if (!ok) ok = bo__write_json_stream(stdout, rep);
#else
        ok = bo__write_json_stream(stdout, rep);
#endif
    }
    else
    {
#if defined(VITTE_ENABLE_RUST_API) && VITTE_ENABLE_RUST_API
        ok = bo__write_file_atomic(path, bo__write_json_stream_rust, rep);
        if (!ok) ok = bo__write_file_atomic(path, bo__write_json_stream, rep);
#else
        ok = bo__write_file_atomic(path, bo__write_json_stream, rep);
#endif
    }
    if (prev_copy[0]) (void)setlocale(LC_NUMERIC, prev_copy);

#if defined(BLOG_INFO)
    if (ok) BLOG_INFO("wrote json: %s", path);
    else BLOG_WARN("failed to write json: %s", path);
#endif

    return ok;
}

bool bench_output_write_csv_path(const char* path, const bench_report* rep)
{
    if (!path || !*path || !rep) return false;
    const char* prev = setlocale(LC_NUMERIC, NULL);
    char prev_copy[64];
    prev_copy[0] = '\0';
    if (prev) (void)snprintf(prev_copy, sizeof(prev_copy), "%s", prev);
    (void)setlocale(LC_NUMERIC, "C");
    bool ok = false;
    if (strcmp(path, "-") == 0)
    {
#if defined(VITTE_ENABLE_RUST_API) && VITTE_ENABLE_RUST_API
        ok = bo__write_csv_stream_rust(stdout, rep);
        if (!ok) ok = bo__write_csv_stream(stdout, rep);
#else
        ok = bo__write_csv_stream(stdout, rep);
#endif
    }
    else
    {
#if defined(VITTE_ENABLE_RUST_API) && VITTE_ENABLE_RUST_API
        ok = bo__write_file_atomic(path, bo__write_csv_stream_rust, rep);
        if (!ok) ok = bo__write_file_atomic(path, bo__write_csv_stream, rep);
#else
        ok = bo__write_file_atomic(path, bo__write_csv_stream, rep);
#endif
    }
    if (prev_copy[0]) (void)setlocale(LC_NUMERIC, prev_copy);

#if defined(BLOG_INFO)
    if (ok) BLOG_INFO("wrote csv: %s", path);
    else BLOG_WARN("failed to write csv: %s", path);
#endif

    return ok;
}
