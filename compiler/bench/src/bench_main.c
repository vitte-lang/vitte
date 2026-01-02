// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// bench_main.c
// -----------------------------------------------------------------------------
// Minimal bench runner for vitte/compiler benchmarks.
//
// Responsibilities:
//   - Provide suite registry ABI (bench_register_suite)
//   - Initialize suites (constructor or fallback) via suites_init.c
//   - Run selected suites/cases with timing + statistics
//   - Print results as a readable table and/or export CSV
//
// Notes:
//   - Suites are expected to be compiled/linked into the final binary.
//   - If constructor-based auto-registration is disabled, call:
//       vitte_bench_suites_init() (provided by suites_init.c)
//     which will attempt fallback explicit registration.
// -----------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "suites/suites_init.h"

#if defined(__GNUC__) || defined(__clang__)
    #define VITTE_WEAK __attribute__((weak))
#else
    #define VITTE_WEAK
#endif

// -----------------------------------------------------------------------------
// Bench ABI (must match suites)
// -----------------------------------------------------------------------------

typedef void (*bench_fn)(uint64_t iters, void* user);

typedef struct bench_case
{
    const char* name;
    bench_fn fn;
    void* user;
} bench_case_t;

typedef struct bench_suite
{
    const char* name;
    const bench_case_t* cases;
    size_t case_count;
} bench_suite_t;

// -----------------------------------------------------------------------------
// Optional baseline explicit registration (weak)
// -----------------------------------------------------------------------------
VITTE_WEAK void vitte_bench_register_baseline_suite(void);
VITTE_WEAK void vitte_bench_baseline_teardown(void);

// CSV writer (implemented in bench_csv.c)

typedef struct vitte_bench_csv_meta
{
    const char* run_id;
    const char* started_at;
    const char* host;
    const char* os;
    const char* arch;
    const char* compiler;
    const char* flags;
} vitte_bench_csv_meta_t;

typedef struct vitte_bench_result_row
{
    const char* suite;
    const char* name;

    uint64_t iters;

    uint64_t ns_total;
    uint64_t ns_min;
    uint64_t ns_max;
    uint64_t ns_mean;
    uint64_t ns_p50;
    uint64_t ns_p90;
    uint64_t ns_p99;

    uint64_t bytes;
} vitte_bench_result_row_t;

bool vitte_bench_write_csv(const char* path,
                           const vitte_bench_csv_meta_t* meta,
                           const vitte_bench_result_row_t* rows,
                           size_t row_count);

// -----------------------------------------------------------------------------
// Timing
// -----------------------------------------------------------------------------

#if defined(_WIN32)
    #error "This bench runner currently targets POSIX-like platforms."
#endif

#if defined(__APPLE__)
    #include <mach/mach_time.h>
    static inline uint64_t ns_now(void)
    {
        static mach_timebase_info_data_t tb;
        if (tb.denom == 0)
            (void)mach_timebase_info(&tb);
        const uint64_t t = mach_absolute_time();
        return (t * (uint64_t)tb.numer) / (uint64_t)tb.denom;
    }
#else
    #include <time.h>
    static inline uint64_t ns_now(void)
    {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    }
#endif

// -----------------------------------------------------------------------------
// Registry
// -----------------------------------------------------------------------------

enum { kMaxSuites = 128 };
static const bench_suite_t* g_suites[kMaxSuites];
static size_t g_suite_count;

static const bench_suite_t* find_suite_by_name(const char* name)
{
    if (!name) return NULL;
    for (size_t i = 0; i < g_suite_count; ++i)
    {
        const bench_suite_t* s = g_suites[i];
        if (s && s->name && strcmp(s->name, name) == 0)
            return s;
    }
    return NULL;
}

static bool suite_ptr_exists(const bench_suite_t* suite)
{
    for (size_t i = 0; i < g_suite_count; ++i)
        if (g_suites[i] == suite)
            return true;
    return false;
}

void bench_register_suite(const bench_suite_t* suite)
{
    if (!suite || !suite->name || !suite->cases || suite->case_count == 0)
        return;

    if (suite_ptr_exists(suite))
        return;

    // avoid duplicates by name (if both constructor + fallback attempted)
    if (find_suite_by_name(suite->name))
        return;

    if (g_suite_count < kMaxSuites)
        g_suites[g_suite_count++] = suite;
}

// Optional runner helper for suites_init.c to detect whether constructors worked.
const bench_suite_t* bench_get_suites(size_t* out_count)
{
    if (out_count) *out_count = g_suite_count;
    return (g_suite_count ? g_suites[0] : NULL);
}

// -----------------------------------------------------------------------------
// Filters
// -----------------------------------------------------------------------------

static bool str_contains(const char* hay, const char* needle)
{
    if (!needle || !*needle) return true;
    if (!hay) return false;
    return strstr(hay, needle) != NULL;
}

// -----------------------------------------------------------------------------
// Stats
// -----------------------------------------------------------------------------

typedef struct stats
{
    uint64_t ns_total;
    uint64_t ns_min;
    uint64_t ns_max;
    uint64_t ns_mean;
    uint64_t ns_p50;
    uint64_t ns_p90;
    uint64_t ns_p99;
} stats_t;

static int cmp_u64(const void* a, const void* b)
{
    const uint64_t x = *(const uint64_t*)a;
    const uint64_t y = *(const uint64_t*)b;
    return (x < y) ? -1 : (x > y) ? 1 : 0;
}

static uint64_t quantile_sorted_u64(const uint64_t* v, size_t n, double q)
{
    if (!n) return 0;
    if (q <= 0.0) return v[0];
    if (q >= 1.0) return v[n - 1];

    const double pos = q * (double)(n - 1);
    const size_t i = (size_t)pos;
    const double frac = pos - (double)i;
    if (i + 1 >= n) return v[n - 1];

    const double a = (double)v[i];
    const double b = (double)v[i + 1];
    const double x = a + (b - a) * frac;
    return (uint64_t)(x + 0.5);
}

static stats_t compute_stats_ns_per_iter(uint64_t iters, const uint64_t* sample_ns, size_t sample_count)
{
    stats_t st;
    memset(&st, 0, sizeof(st));
    if (!iters || !sample_count) return st;

    // Convert to ns/iter and sort
    uint64_t* v = (uint64_t*)malloc(sizeof(uint64_t) * sample_count);
    if (!v) abort();

    uint64_t total_ns = 0;
    uint64_t min_pi = UINT64_MAX;
    uint64_t max_pi = 0;

    for (size_t i = 0; i < sample_count; ++i)
    {
        total_ns += sample_ns[i];
        const uint64_t per_iter = sample_ns[i] / iters;
        v[i] = per_iter;
        if (per_iter < min_pi) min_pi = per_iter;
        if (per_iter > max_pi) max_pi = per_iter;
    }

    qsort(v, sample_count, sizeof(uint64_t), cmp_u64);

    // mean ns/iter
    uint64_t sum_pi = 0;
    for (size_t i = 0; i < sample_count; ++i) sum_pi += v[i];

    st.ns_total = total_ns;
    st.ns_min = min_pi;
    st.ns_max = max_pi;
    st.ns_mean = sum_pi / (uint64_t)sample_count;
    st.ns_p50 = quantile_sorted_u64(v, sample_count, 0.50);
    st.ns_p90 = quantile_sorted_u64(v, sample_count, 0.90);
    st.ns_p99 = quantile_sorted_u64(v, sample_count, 0.99);

    free(v);
    return st;
}

// -----------------------------------------------------------------------------
// Runner
// -----------------------------------------------------------------------------

typedef struct run_cfg
{
    const char* suite_filter;
    const char* case_filter;

    uint64_t iters;
    uint32_t samples;
    uint32_t warmup;

    bool list_only;

    bool print_table;
    const char* csv_path; // NULL => no CSV
} run_cfg_t;

static void print_usage(const char* exe)
{
    printf("Usage: %s [options]\n", exe);
    printf("\n");
    printf("Options:\n");
    printf("  --list                 List suites and cases\n");
    printf("  --suite <substr>        Filter suite name by substring\n");
    printf("  --case <substr>         Filter case name by substring\n");
    printf("  --iters <n>             Iterations per sample (default: 100)\n");
    printf("  --samples <n>           Samples per case (default: 10)\n");
    printf("  --warmup <n>            Warmup runs per case (default: 1)\n");
    printf("  --csv <path|- >         Write CSV to file (or '-' for stdout)\n");
    printf("  --no-table              Disable table output\n");
    printf("  -h, --help              Show help\n");
}

static bool parse_u64(const char* s, uint64_t* out)
{
    if (!s || !*s) return false;
    char* end = NULL;
    unsigned long long v = strtoull(s, &end, 10);
    if (!end || *end != '\0') return false;
    *out = (uint64_t)v;
    return true;
}

static bool parse_u32(const char* s, uint32_t* out)
{
    uint64_t v = 0;
    if (!parse_u64(s, &v)) return false;
    if (v > 0xFFFFFFFFu) return false;
    *out = (uint32_t)v;
    return true;
}

static run_cfg_t parse_args(int argc, char** argv)
{
    run_cfg_t cfg;
    memset(&cfg, 0, sizeof(cfg));

    cfg.iters = 100;
    cfg.samples = 10;
    cfg.warmup = 1;
    cfg.print_table = true;

    for (int i = 1; i < argc; ++i)
    {
        const char* a = argv[i];

        if (strcmp(a, "--list") == 0)
        {
            cfg.list_only = true;
        }
        else if (strcmp(a, "--suite") == 0 && i + 1 < argc)
        {
            cfg.suite_filter = argv[++i];
        }
        else if (strcmp(a, "--case") == 0 && i + 1 < argc)
        {
            cfg.case_filter = argv[++i];
        }
        else if (strcmp(a, "--iters") == 0 && i + 1 < argc)
        {
            (void)parse_u64(argv[++i], &cfg.iters);
            if (cfg.iters == 0) cfg.iters = 1;
        }
        else if (strcmp(a, "--samples") == 0 && i + 1 < argc)
        {
            (void)parse_u32(argv[++i], &cfg.samples);
            if (cfg.samples == 0) cfg.samples = 1;
        }
        else if (strcmp(a, "--warmup") == 0 && i + 1 < argc)
        {
            (void)parse_u32(argv[++i], &cfg.warmup);
        }
        else if (strcmp(a, "--csv") == 0 && i + 1 < argc)
        {
            cfg.csv_path = argv[++i];
        }
        else if (strcmp(a, "--no-table") == 0)
        {
            cfg.print_table = false;
        }
        else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0)
        {
            print_usage(argv[0]);
            exit(0);
        }
        else
        {
            printf("Unknown argument: %s\n\n", a);
            print_usage(argv[0]);
            exit(2);
        }
    }

    return cfg;
}

static void list_suites(const run_cfg_t* cfg)
{
    for (size_t si = 0; si < g_suite_count; ++si)
    {
        const bench_suite_t* s = g_suites[si];
        if (!s) continue;
        if (!str_contains(s->name, cfg->suite_filter)) continue;

        printf("%s\n", s->name);
        for (size_t ci = 0; ci < s->case_count; ++ci)
        {
            const bench_case_t* c = &s->cases[ci];
            if (!str_contains(c->name, cfg->case_filter)) continue;
            printf("  - %s\n", c->name);
        }
    }
}

static uint64_t run_one_sample(const bench_case_t* c, uint64_t iters)
{
    const uint64_t t0 = ns_now();
    c->fn(iters, c->user);
    const uint64_t t1 = ns_now();
    return (t1 - t0);
}

static void print_table_header(void)
{
    printf("%-12s  %-28s  %10s  %12s  %12s  %12s  %12s  %12s  %12s\n",
           "suite",
           "case",
           "iters",
           "mean(ns/i)",
           "min(ns/i)",
           "p50(ns/i)",
           "p90(ns/i)",
           "p99(ns/i)",
           "max(ns/i)");
}

static void print_table_row(const char* suite, const char* name, uint64_t iters, const stats_t* st)
{
    printf("%-12s  %-28.28s  %10" PRIu64 "  %12" PRIu64 "  %12" PRIu64 "  %12" PRIu64 "  %12" PRIu64 "  %12" PRIu64 "  %12" PRIu64 "\n",
           suite ? suite : "",
           name ? name : "",
           iters,
           st->ns_mean,
           st->ns_min,
           st->ns_p50,
           st->ns_p90,
           st->ns_p99,
           st->ns_max);
}

static size_t count_selected_cases(const run_cfg_t* cfg)
{
    size_t n = 0;
    for (size_t si = 0; si < g_suite_count; ++si)
    {
        const bench_suite_t* s = g_suites[si];
        if (!s) continue;
        if (!str_contains(s->name, cfg->suite_filter)) continue;

        for (size_t ci = 0; ci < s->case_count; ++ci)
        {
            const bench_case_t* c = &s->cases[ci];
            if (!str_contains(c->name, cfg->case_filter)) continue;
            n++;
        }
    }
    return n;
}

static void run_benchmarks(const run_cfg_t* cfg)
{
    const size_t total_rows = count_selected_cases(cfg);
    vitte_bench_result_row_t* rows = NULL;
    size_t row_count = 0;

    if (cfg->csv_path)
    {
        rows = (vitte_bench_result_row_t*)calloc(total_rows ? total_rows : 1, sizeof(*rows));
        if (!rows) abort();
    }

    if (cfg->print_table)
        print_table_header();

    uint64_t* samples = (uint64_t*)malloc(sizeof(uint64_t) * (size_t)cfg->samples);
    if (!samples) abort();

    for (size_t si = 0; si < g_suite_count; ++si)
    {
        const bench_suite_t* s = g_suites[si];
        if (!s) continue;
        if (!str_contains(s->name, cfg->suite_filter)) continue;

        for (size_t ci = 0; ci < s->case_count; ++ci)
        {
            const bench_case_t* c = &s->cases[ci];
            if (!str_contains(c->name, cfg->case_filter)) continue;

            // warmup
            for (uint32_t w = 0; w < cfg->warmup; ++w)
                (void)run_one_sample(c, cfg->iters);

            // samples
            for (uint32_t k = 0; k < cfg->samples; ++k)
                samples[k] = run_one_sample(c, cfg->iters);

            stats_t st = compute_stats_ns_per_iter(cfg->iters, samples, (size_t)cfg->samples);

            if (cfg->print_table)
                print_table_row(s->name, c->name, cfg->iters, &st);

            if (rows)
            {
                vitte_bench_result_row_t r;
                memset(&r, 0, sizeof(r));
                r.suite = s->name;
                r.name = c->name;
                r.iters = cfg->iters;
                r.ns_total = st.ns_total;
                r.ns_min = st.ns_min;
                r.ns_max = st.ns_max;
                r.ns_mean = st.ns_mean;
                r.ns_p50 = st.ns_p50;
                r.ns_p90 = st.ns_p90;
                r.ns_p99 = st.ns_p99;
                r.bytes = 0;
                rows[row_count++] = r;
            }
        }
    }

    free(samples);

    if (cfg->csv_path)
    {
        vitte_bench_csv_meta_t meta;
        memset(&meta, 0, sizeof(meta));
        // Keep metadata minimal here; runner/CI may populate externally.
        meta.run_id = NULL;
        meta.started_at = NULL;
        meta.host = NULL;
        meta.os = NULL;
        meta.arch = NULL;
        meta.compiler = NULL;
        meta.flags = NULL;

        if (!vitte_bench_write_csv(cfg->csv_path, &meta, rows, row_count))
            printf("CSV write failed: %s\n", cfg->csv_path);
    }

    free(rows);
}

// -----------------------------------------------------------------------------
// Entry
// -----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    const run_cfg_t cfg = parse_args(argc, argv);

    // Ensure suites are registered.
    vitte_bench_suites_init();

    // Ensure baseline exists even when constructors are disabled.
    if (vitte_bench_register_baseline_suite && !find_suite_by_name("baseline"))
        vitte_bench_register_baseline_suite();

    if (g_suite_count == 0)
    {
        printf("No benchmark suites registered.\n");
        return 1;
    }

    if (cfg.list_only)
    {
        list_suites(&cfg);
        return 0;
    }

    run_benchmarks(&cfg);

    // teardown
    vitte_bench_suites_teardown();
    if (vitte_bench_baseline_teardown) vitte_bench_baseline_teardown();

    return 0;
}
