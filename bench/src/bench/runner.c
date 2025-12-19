

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
#include "bench/benchmark_init.h"
#include "bench/bench_stats.h"
#include "bench/bench_time.h"

void bench_register_builtin_suites(void);

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

static uint64_t runner__hash_str_u64(const char* s)
{
    // FNV-1a 64-bit
    uint64_t h = UINT64_C(1469598103934665603);
    if (!s) return h;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p)
    {
        h ^= (uint64_t)(*p);
        h *= UINT64_C(1099511628211);
    }
    return h;
}

typedef struct runner_batch
{
    uint64_t elapsed_ns;
    uint64_t cycles;
    int64_t work; // iters_per_call * calls
} runner_batch;

static int runner__run_batch(const bench_case_t* c, int64_t iters_per_call, int64_t calls, runner_batch* out)
{
    if (out) memset(out, 0, sizeof(*out));
    if (!c) return -1;
    if (iters_per_call <= 0) iters_per_call = 1;
    if (calls <= 0) calls = 1;

    const uint64_t t0 = bench_time_now_ns();
    const uint64_t cy0 = bench_time_cycles_now();

    for (int64_t i = 0; i < calls; ++i)
    {
        int rc = BENCH_FN_CALL(c, iters_per_call);
        if (rc == BENCH_RC_SKIPPED) return BENCH_RC_SKIPPED;
        if (rc != BENCH_RC_OK) return rc;
    }

    const uint64_t cy1 = bench_time_cycles_now();
    const uint64_t t1 = bench_time_now_ns();

    if (out)
    {
        out->elapsed_ns = (t1 >= t0) ? (t1 - t0) : 0;
        out->cycles = (cy1 >= cy0) ? (cy1 - cy0) : 0;
        out->work = iters_per_call * calls;
    }
    return 0;
}

// Calibrate a (iters_per_call, calls_per_sample) pair so elapsed time meets target_ms (if > 0).
// Strategy:
//   - If opt_itters > 0: keep iters_per_call fixed and scale calls_per_sample.
//   - If opt_itters == 0: probe whether the benchmark scales with iters; if not, scale calls.
static int runner__calibrate(const bench_case_t* c,
                             int64_t opt_iters,
                             int64_t target_ms,
                             int64_t* out_iters_per_call,
                             int64_t* out_calls_per_sample)
{
    if (out_iters_per_call) *out_iters_per_call = (opt_iters > 0) ? opt_iters : 1;
    if (out_calls_per_sample) *out_calls_per_sample = 1;
    if (!c) return -1;

    if (target_ms <= 0)
    {
        if (out_iters_per_call) *out_iters_per_call = (opt_iters > 0) ? opt_iters : 1;
        if (out_calls_per_sample) *out_calls_per_sample = 1;
        return 0;
    }

    const int64_t target_ns = target_ms * 1000000ll;
    const int64_t cap = (int64_t)1e12;

    int64_t iters_per_call = (opt_iters > 0) ? opt_iters : 1;
    int64_t calls = 1;

    // Probe scaling: does doubling iters increase time meaningfully?
    bool scales_with_iters = false;
    if (opt_iters == 0)
    {
        runner_batch a, b;
        int rc1 = runner__run_batch(c, 1, 1, &a);
        if (rc1 == BENCH_RC_SKIPPED) return BENCH_RC_SKIPPED;
        if (rc1 != 0) return rc1;
        int rc2 = runner__run_batch(c, 2, 1, &b);
        if (rc2 == BENCH_RC_SKIPPED) return BENCH_RC_SKIPPED;
        if (rc2 != 0) return rc2;

        if (a.elapsed_ns > 0 && b.elapsed_ns > a.elapsed_ns)
        {
            double ratio = (double)b.elapsed_ns / (double)a.elapsed_ns;
            scales_with_iters = (ratio > 1.5);
        }
    }

    for (int step = 0; step < 40; ++step)
    {
        runner_batch m;
        int rc = runner__run_batch(c, iters_per_call, calls, &m);
        if (rc == BENCH_RC_SKIPPED) return BENCH_RC_SKIPPED;
        if (rc != 0) return rc;

        int64_t dt = (int64_t)m.elapsed_ns;
        if (dt >= target_ns)
            break;

        if (dt <= 0)
        {
            // Too fast: quickly grow calls.
            if (calls > cap / 1024) calls = cap;
            else calls *= 1024;
            continue;
        }

        double scale = (double)target_ns / (double)dt;
        if (scale < 2.0) scale = 2.0;
        if (scale > 1024.0) scale = 1024.0;

        if (opt_iters == 0 && scales_with_iters)
        {
            double next = (double)iters_per_call * scale;
            if (next > (double)cap) next = (double)cap;
            iters_per_call = (int64_t)next;
        }
        else
        {
            double next = (double)calls * scale;
            if (next > (double)cap) next = (double)cap;
            calls = (int64_t)next;
        }

        if (iters_per_call <= 0) iters_per_call = 1;
        if (calls <= 0) calls = 1;
        if (iters_per_call >= cap) iters_per_call = cap;
        if (calls >= cap) calls = cap;
    }

    if (out_iters_per_call) *out_iters_per_call = iters_per_call;
    if (out_calls_per_sample) *out_calls_per_sample = calls;
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

static bench_status runner__run_one(const bench_case_t* c,
                                    const bench_options* opt,
                                    bench_metric* m,
                                    const char** out_err,
                                    double** out_samples_ns_per_op,
                                    int32_t* out_samples_count)
{
    if (out_err) *out_err = NULL;
    if (out_samples_ns_per_op) *out_samples_ns_per_op = NULL;
    if (out_samples_count) *out_samples_count = 0;
    if (!c || !m) return BENCH_STATUS_FAILED;

    // calibrate
    int64_t iters_per_call = 1;
    int64_t calls_per_sample = 1;
    int calib_rc = runner__calibrate(c, opt->iters, opt->calibrate_ms, &iters_per_call, &calls_per_sample);
    if (calib_rc == BENCH_RC_SKIPPED)
        return BENCH_STATUS_SKIPPED;
    if (calib_rc != 0)
    {
        static char buf[256];
        runner__format_err(buf, sizeof(buf), c->id, calib_rc);
        if (out_err) *out_err = buf;
        return BENCH_STATUS_FAILED;
    }

    // warmup (best-effort; not counted)
    for (int32_t w = 0; w < opt->warmup; ++w)
    {
        runner_batch wb;
        int rc = runner__run_batch(c, iters_per_call, calls_per_sample, &wb);
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

    // repeats (collect samples)
    const int32_t reps = (opt->repeat > 0) ? opt->repeat : 1;
    double* samples = (double*)calloc((size_t)reps, sizeof(double));
    double* cycles_per_sec = (double*)calloc((size_t)reps, sizeof(double));
    if (!samples || !cycles_per_sec)
    {
        free(samples);
        free(cycles_per_sec);
        if (out_err) *out_err = "out of memory";
        return BENCH_STATUS_FAILED;
    }

    uint64_t total_ns = 0;
    int64_t total_work = 0;
    double cyc_min = 0.0, cyc_max = 0.0;
    int cyc_have = 0;

    for (int32_t r = 0; r < reps; ++r)
    {
        runner_batch b;
        int rc = runner__run_batch(c, iters_per_call, calls_per_sample, &b);
        if (rc == BENCH_RC_SKIPPED)
        {
            free(samples);
            free(cycles_per_sec);
            return BENCH_STATUS_SKIPPED;
        }
        if (rc != BENCH_RC_OK)
        {
            static char buf[256];
            runner__format_err(buf, sizeof(buf), c->id, rc);
            if (out_err) *out_err = buf;
            free(samples);
            free(cycles_per_sec);
            return BENCH_STATUS_FAILED;
        }

        const uint64_t dt = b.elapsed_ns;
        total_ns += dt;
        total_work += b.work;

        samples[r] = runner__ns_per_op(dt, b.work);

        if (dt > 0 && b.cycles > 0)
        {
            const double cps = (double)b.cycles * 1e9 / (double)dt;
            cycles_per_sec[r] = cps;
            if (!cyc_have) { cyc_min = cyc_max = cps; cyc_have = 1; }
            else { if (cps < cyc_min) cyc_min = cps; if (cps > cyc_max) cyc_max = cps; }
        }
    }

    bench_stats st;
    (void)bench_stats_compute_f64(samples, (size_t)reps, &st);

    double ci_low = st.p50, ci_high = st.p50;
    (void)bench_stats_bootstrap_ci_median_f64(samples, (size_t)reps,
                                              opt->seed ^ runner__hash_str_u64(c->id),
                                              500, 0.025, 0.975, &ci_low, &ci_high);

    m->iterations = total_work;
    m->elapsed_ms = runner__ns_to_ms(total_ns);
    m->ns_per_op = runner__ns_per_op(total_ns, total_work);

    m->ns_per_op_median = st.p50;
    m->ns_per_op_p95 = st.p95;
    m->ns_per_op_mad = st.mad;
    m->ns_per_op_iqr = st.iqr;
    m->ns_per_op_ci95_low = ci_low;
    m->ns_per_op_ci95_high = ci_high;

    m->iters_per_call = iters_per_call;
    m->calls_per_sample = calls_per_sample;
    m->target_time_ms = opt->calibrate_ms;

    m->cycles_per_sec_min = cyc_have ? cyc_min : 0.0;
    m->cycles_per_sec_max = cyc_have ? cyc_max : 0.0;
    m->throttling_suspected = (cyc_have && cyc_min > 0.0) ? ((cyc_max / cyc_min) > 1.10) : false;

    if (out_samples_ns_per_op && opt->include_samples && (opt->out_json && *opt->out_json))
    {
        *out_samples_ns_per_op = samples;
        if (out_samples_count) *out_samples_count = reps;
    }
    else
    {
        free(samples);
    }
    free(cycles_per_sec);

    return BENCH_STATUS_OK;
}

// Public entry point intended to be called by bench_main.c.
// Returns process exit code.
int bench_runner_run(int argc, char** argv)
{
    bench_register_builtin_suites();

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

    const bool want_human =
        (!opt.quiet) &&
        (
            (opt.format && strcmp(opt.format, "human") == 0) ||
            (
                (!opt.format || strcmp(opt.format, "auto") == 0) &&
                !(opt.out_json && *opt.out_json) &&
                !(opt.out_csv && *opt.out_csv)
            )
        );

    benchmark_runtime_init();
    int cpu_pinned = 0;
    if (opt.cpu >= 0)
    {
        cpu_pinned = benchmark_pin_to_single_cpu(opt.cpu) ? 1 : 0;
        if (!cpu_pinned && !opt.quiet)
            (void)fprintf(stderr, "warn: failed to pin to CPU %d (unsupported or permission denied)\n", (int)opt.cpu);
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
    const int64_t t0_ms = (int64_t)bench_time_now_ms();
    static const char* k_time_budget_exceeded = "time budget exceeded";

    for (int32_t i = 0; i < sel.selected_count; ++i)
    {
        if (opt.time_budget_ms > 0)
        {
            const int64_t now_ms = (int64_t)bench_time_now_ms();
            if ((now_ms - t0_ms) >= opt.time_budget_ms)
            {
                for (int32_t k = i; k < sel.selected_count; ++k)
                {
                    results[k].status = BENCH_STATUS_SKIPPED;
                    results[k].error = k_time_budget_exceeded;
                }
                break;
            }
        }

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
        double* samples_ns = NULL;
        int32_t samples_count = 0;

        bench_status st = runner__run_one(c, &opt, &m, &perr, &samples_ns, &samples_count);
        results[i].status = st;
        results[i].metric = m;
        results[i].samples_ns_per_op = samples_ns;
        results[i].samples_count = samples_count;
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
    rep.schema = opt.output_version;
    rep.suite_name = "bench";
    rep.seed = opt.seed;
    rep.threads = opt.threads;
    rep.repeat = opt.repeat;
    rep.warmup = opt.warmup;
    rep.timestamp_ms = runner__epoch_ms_best_effort();
    rep.include_samples = opt.include_samples;
    rep.cpu_pinned = cpu_pinned;
    rep.cpu_index = opt.cpu;
    rep.calibrate_ms = opt.calibrate_ms;
    rep.iters = opt.iters;

    if (want_human)
        bench_output_print_human(stdout, &rep);

    if (opt.out_json && *opt.out_json)
        (void)bench_output_write_json_path(opt.out_json, &rep);

    if (opt.out_csv && *opt.out_csv)
        (void)bench_output_write_csv_path(opt.out_csv, &rep);

    for (int32_t i = 0; i < sel.selected_count; ++i)
        free((void*)results[i].samples_ns_per_op);

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
