

// runner.c - benchmark runner for vitte/bench (C17)
//
// Responsibilities:
//   - Parse CLI options
//   - Enumerate benchmarks from the registry
//   - Execute selected benchmarks with warmup + repeats
//   - Produce human output and optional JSON/CSV reports
//
// NOTE about calling convention:
//   The registry stores `bench_fn_t fn` and a `void* ctx`.
//   This runner supports an overridable calling convention:
//     - If BENCH_FN_CALL(case_ptr, iters) is defined, it will be used.
//     - Otherwise, we assume: int (*)(void* ctx, int64_t iters)
//       returning 0 on success.
//
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "bench/options.h"
#include "bench/output.h"
#include "bench/registry.h"

// Provide platform time impl here to avoid link errors in small builds.
// If you already compile another TU with BENCH_PLATFORM_IMPLEMENTATION,
// define BENCH_PLATFORM_NO_IMPL for this file in your build.
#ifndef BENCH_PLATFORM_NO_IMPL
  #define BENCH_PLATFORM_IMPLEMENTATION
#endif
#include "bench/platform.h"

#if defined(__has_include)
  #if __has_include("bench/log.h")
    #include "bench/log.h"
  #endif
#endif

// -----------------------------------------------------------------------------
// Calling convention
// -----------------------------------------------------------------------------

#ifndef BENCH_FN_CALL
static int runner__call_case(const bench_case_t* c, int64_t iters)
{
    // Default convention: int (*)(void* ctx, int64_t iters)
    // If your project uses a different signature, define BENCH_FN_CALL.
    int (*fn)(void*, int64_t) = (int (*)(void*, int64_t))(void*)c->fn;
    return fn(c->ctx, iters);
}
#define BENCH_FN_CALL(case_ptr, iters) runner__call_case((case_ptr), (iters))
#endif

// Optional runner RC semantics (override if your benches use different codes).
#ifndef BENCH_RC_OK
#define BENCH_RC_OK 0
#endif
#ifndef BENCH_RC_SKIPPED
#define BENCH_RC_SKIPPED 2
#endif

// -----------------------------------------------------------------------------
// Utils
// -----------------------------------------------------------------------------

static bool runner__match_substr(const char* hay, const char* needle)
{
    if (!needle || !*needle) return true;
    if (!hay) return false;
    return strstr(hay, needle) != NULL;
}

static int64_t runner__epoch_ms_best_effort(void)
{
#if defined(bench_log_now_ms)
    int64_t ms = bench_log_now_ms();
    if (ms >= 0) return ms;
#endif
    // Fallback: use monotonic time as pseudo timestamp.
    return (int64_t)bench_time_now_ms();
}

static void runner__format_err(char* dst, size_t cap, const char* bench_id, int rc)
{
    if (!dst || cap == 0) return;
    if (!bench_id) bench_id = "";
    (void)snprintf(dst, cap, "%s returned rc=%d", bench_id, rc);
}

static double runner__ns_to_ms(uint64_t ns)
{
    return (double)ns / 1000000.0;
}

static double runner__ns_per_op(uint64_t ns, int64_t iters)
{
    if (iters <= 0) return 0.0;
    return (double)ns / (double)iters;
}

// Calibrate an iteration count so elapsed time meets min_time_ms (if > 0).
// Requires that BENCH_FN_CALL supports (ctx, iters).
static int runner__calibrate_iters(const bench_case_t* c, int64_t min_time_ms, int64_t* out_iters)
{
    if (out_iters) *out_iters = 1;
    if (!c) return -1;
    if (min_time_ms <= 0)
    {
        if (out_iters) *out_iters = 1;
        return 0;
    }

    const int64_t target_ns = min_time_ms * 1000000ll;
    int64_t iters = 1;

    // Hard cap to avoid runaway.
    const int64_t iters_cap = (int64_t)1e12;

    for (int step = 0; step < 60; ++step)
    {
        uint64_t t0 = bench_time_now_ns();
        int rc = BENCH_FN_CALL(c, iters);
        uint64_t t1 = bench_time_now_ns();

        if (rc == BENCH_RC_SKIPPED) return BENCH_RC_SKIPPED;
        if (rc != BENCH_RC_OK) return rc;

        uint64_t dt = (t1 >= t0) ? (t1 - t0) : 0;
        if ((int64_t)dt >= target_ns)
        {
            if (out_iters) *out_iters = iters;
            return 0;
        }

        // If dt is 0 (very fast), aggressively increase.
        if (dt == 0)
        {
            if (iters > iters_cap / 1024) { iters = iters_cap; }
            else iters *= 1024;
        }
        else
        {
            // Scale approximately to reach target.
            double scale = (double)target_ns / (double)dt;
            if (scale < 2.0) scale = 2.0;
            if (scale > 1024.0) scale = 1024.0;

            double next = (double)iters * scale;
            if (next > (double)iters_cap) next = (double)iters_cap;
            iters = (int64_t)next;
        }

        if (iters <= 0) iters = 1;
        if (iters >= iters_cap)
        {
            if (out_iters) *out_iters = iters_cap;
            return 0;
        }
    }

    if (out_iters) *out_iters = iters;
    return 0;
}

// -----------------------------------------------------------------------------
// Runner
// -----------------------------------------------------------------------------

typedef struct runner_sel
{
    const bench_options* opt;
    int32_t selected_count;
    int32_t total_count;
} runner_sel;

static int runner__count_selected_it(const bench_case_t* c, void* user)
{
    runner_sel* s = (runner_sel*)user;
    s->total_count++;

    const bench_options* opt = s->opt;
    const char* id = c && c->id ? c->id : "";

    if (opt->bench_name && *opt->bench_name)
    {
        if (strcmp(id, opt->bench_name) == 0)
            s->selected_count++;
        return 0;
    }

    if (opt->filter && *opt->filter)
    {
        if (runner__match_substr(id, opt->filter))
            s->selected_count++;
        return 0;
    }

    // no filter: select all
    s->selected_count++;
    return 0;
}

typedef struct runner_fill
{
    const bench_options* opt;
    bench_result* out;
    int32_t out_i;
} runner_fill;

static int runner__fill_selected_it(const bench_case_t* c, void* user)
{
    runner_fill* f = (runner_fill*)user;
    const bench_options* opt = f->opt;

    const char* id = c && c->id ? c->id : "";

    bool take = true;
    if (opt->bench_name && *opt->bench_name)
        take = (strcmp(id, opt->bench_name) == 0);
    else if (opt->filter && *opt->filter)
        take = runner__match_substr(id, opt->filter);

    if (!take) return 0;

    bench_result* r = &f->out[f->out_i++];
    memset(r, 0, sizeof(*r));
    r->name = c->id;
    r->status = BENCH_STATUS_OK;
    r->metric.ns_per_op = 0.0;
    r->metric.bytes_per_sec = 0.0;
    r->metric.items_per_sec = 0.0;
    r->metric.iterations = 0;
    r->metric.elapsed_ms = 0.0;
    r->error = NULL;

    return 0;
}

static void runner__list_benches(FILE* out)
{
    if (!out) out = stdout;

    int32_t n = bench_registry_count();
    (void)fprintf(out, "benchmarks (%d):\n", (int)n);
    for (int32_t i = 0; i < n; ++i)
    {
        const bench_case_t* c = bench_registry_get(i);
        if (!c) continue;
        (void)fprintf(out, "  %s\n", c->id ? c->id : "");
    }
}

static bench_status runner__run_one(const bench_case_t* c, const bench_options* opt, bench_metric* m, const char** out_err)
{
    if (out_err) *out_err = NULL;
    if (!c || !m) return BENCH_STATUS_FAILED;

    // warmup
    for (int32_t w = 0; w < opt->warmup; ++w)
    {
        int rc = BENCH_FN_CALL(c, 1);
        if (rc == BENCH_RC_SKIPPED)
        {
            if (out_err) *out_err = NULL;
            return BENCH_STATUS_SKIPPED;
        }
        if (rc != BENCH_RC_OK)
        {
            static char buf[256];
            runner__format_err(buf, sizeof(buf), c->id, rc);
            if (out_err) *out_err = buf;
            return BENCH_STATUS_FAILED;
        }
    }

    // calibrate iterations
    int64_t iters = 1;
    int calib_rc = runner__calibrate_iters(c, opt->min_time_ms, &iters);
    if (calib_rc == BENCH_RC_SKIPPED)
        return BENCH_STATUS_SKIPPED;
    if (calib_rc != 0)
    {
        static char buf[256];
        runner__format_err(buf, sizeof(buf), c->id, calib_rc);
        if (out_err) *out_err = buf;
        return BENCH_STATUS_FAILED;
    }

    // repeats (aggregate mean)
    uint64_t total_ns = 0;
    int64_t total_iters = 0;

    for (int32_t r = 0; r < opt->repeat; ++r)
    {
        uint64_t t0 = bench_time_now_ns();
        int rc = BENCH_FN_CALL(c, iters);
        uint64_t t1 = bench_time_now_ns();

        if (rc == BENCH_RC_SKIPPED)
            return BENCH_STATUS_SKIPPED;
        if (rc != BENCH_RC_OK)
        {
            static char buf[256];
            runner__format_err(buf, sizeof(buf), c->id, rc);
            if (out_err) *out_err = buf;
            return BENCH_STATUS_FAILED;
        }

        uint64_t dt = (t1 >= t0) ? (t1 - t0) : 0;
        total_ns += dt;
        total_iters += iters;
    }

    m->iterations = total_iters;
    m->elapsed_ms = runner__ns_to_ms(total_ns);
    m->ns_per_op = runner__ns_per_op(total_ns, total_iters);

    return BENCH_STATUS_OK;
}

// Public entry point intended to be called by bench_main.c.
// Returns process exit code.
int bench_runner_run(int argc, char** argv)
{
    bench_options opt;
    char err[256];

    bench_opt_result pr = bench_options_parse(&opt, argc, argv, err, sizeof(err));
    if (pr == BENCH_OPT_EXIT)
    {
        if (opt.show_help) bench_options_print_help(argv && argv[0] ? argv[0] : "bench", stdout);
        if (opt.show_version) bench_options_print_version(stdout);
        return 0;
    }
    if (pr == BENCH_OPT_ERR)
    {
        (void)fprintf(stderr, "error: %s\n", err[0] ? err : "invalid arguments");
        (void)fprintf(stderr, "hint: use --help\n");
        return 2;
    }

    if (opt.list)
    {
        runner__list_benches(stdout);
        return 0;
    }

    runner_sel sel;
    memset(&sel, 0, sizeof(sel));
    sel.opt = &opt;
    (void)bench_registry_foreach(runner__count_selected_it, &sel);

    if (sel.selected_count <= 0)
    {
        if (opt.bench_name && *opt.bench_name)
            (void)fprintf(stderr, "no benchmark named '%s'\n", opt.bench_name);
        else if (opt.filter && *opt.filter)
            (void)fprintf(stderr, "no benchmarks match filter '%s'\n", opt.filter);
        else
            (void)fprintf(stderr, "no benchmarks registered\n");
        return 1;
    }

    bench_result* results = (bench_result*)calloc((size_t)sel.selected_count, sizeof(*results));
    if (!results)
    {
        (void)fprintf(stderr, "out of memory\n");
        return 3;
    }

    runner_fill fill;
    memset(&fill, 0, sizeof(fill));
    fill.opt = &opt;
    fill.out = results;
    fill.out_i = 0;
    (void)bench_registry_foreach(runner__fill_selected_it, &fill);

    // Execute.
    int failures = 0;

    for (int32_t i = 0; i < sel.selected_count; ++i)
    {
        const char* name = results[i].name;
        int32_t idx = bench_registry_find(name);
        const bench_case_t* c = bench_registry_get(idx);

        if (!c)
        {
            results[i].status = BENCH_STATUS_FAILED;
            results[i].error = "registry lookup failed";
            failures++;
            if (opt.fail_fast) break;
            continue;
        }

        const char* perr = NULL;
        bench_metric m;
        memset(&m, 0, sizeof(m));

        bench_status st = runner__run_one(c, &opt, &m, &perr);
        results[i].status = st;
        results[i].metric = m;
        results[i].error = perr;

        if (st == BENCH_STATUS_FAILED)
        {
            failures++;
            if (opt.fail_fast) break;
        }
    }

    bench_report rep;
    memset(&rep, 0, sizeof(rep));
    rep.results = results;
    rep.count = sel.selected_count;
    rep.suite_name = "bench";
    rep.seed = opt.seed;
    rep.threads = opt.threads;
    rep.repeat = opt.repeat;
    rep.warmup = opt.warmup;
    rep.timestamp_ms = runner__epoch_ms_best_effort();

    if (!opt.quiet)
        bench_output_print_human(stdout, &rep);

    if (opt.out_json && *opt.out_json)
        (void)bench_output_write_json_path(opt.out_json, &rep);

    if (opt.out_csv && *opt.out_csv)
        (void)bench_output_write_csv_path(opt.out_csv, &rep);

    free(results);

    return (failures == 0) ? 0 : 1;
}

// Optional convenience main if you want to build runner.c standalone.
// Define BENCH_RUNNER_STANDALONE to enable.
#ifdef BENCH_RUNNER_STANDALONE
int main(int argc, char** argv)
{
    return bench_runner_run(argc, argv);
}
#endif