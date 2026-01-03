

// SPDX-License-Identifier: MIT
// bench_runner.c - benchmark runner (C17)
//
// Minimal bench executable entrypoint wiring:
//  - bench_registry (discovery / selection)
//  - bench_report   (stats + output)
//
// This file is intentionally self-contained (no internal project headers). If you
// already have a bench public header, move the shared types/prototypes there and
// include it from bench_registry.c / bench_report.c / bench_runner.c.

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <time.h>
#endif

//------------------------------------------------------------------------------
// Shared types (must match bench_registry.c / bench_report.c)
//------------------------------------------------------------------------------

typedef struct bench_ctx {
    // Runner-provided per-run context.
    uint64_t iterations;   // logical iterations executed in the benchmark call
    uint64_t bytes;        // bytes processed (optional)
    uint64_t sample_index; // which sample is being executed

    // Optional scratch for benchmarks.
    void*    user;
} bench_ctx;

typedef void (*bench_fn)(bench_ctx* ctx);

typedef struct bench_entry {
    char*    name;   // owned by registry
    char*    group;  // owned by registry (optional)
    bench_fn fn;
} bench_entry;

typedef struct bench_registry {
    bench_entry* entries;
    size_t       len;
    size_t       cap;
} bench_registry;

typedef enum bench_report_format {
    BENCH_REPORT_HUMAN = 0,
    BENCH_REPORT_JSON  = 1,
} bench_report_format;

typedef struct bench_report_config {
    bench_report_format format;       // default: HUMAN
    bool                show_header;  // default: true
    bool                sort_by_mean; // default: false (sort by name)
} bench_report_config;

typedef struct bench_case_stats {
    uint64_t n;
    uint64_t min_ns;
    uint64_t max_ns;
    double   mean_ns;
    double   stdev_ns;
    uint64_t p50_ns;
    uint64_t p95_ns;
} bench_case_stats;

typedef struct bench_case_result {
    char* name;
    char* group;

    uint64_t iterations;
    uint64_t bytes;

    bench_case_stats stats;
} bench_case_result;

typedef struct bench_report {
    bench_report_config cfg;
    bench_case_result*  cases;
    size_t              len;
    size_t              cap;
} bench_report;

//------------------------------------------------------------------------------
// External API (implemented by bench_registry.c / bench_report.c)
//------------------------------------------------------------------------------

void bench_registry_init(bench_registry* r);
void bench_registry_destroy(bench_registry* r);
int  bench_registry_register_builtins(bench_registry* r);
void bench_registry_sort_by_name(bench_registry* r);
const bench_entry* bench_registry_entries(const bench_registry* r, size_t* out_count);
const bench_entry* bench_registry_find(const bench_registry* r, const char* name);
bench_registry* bench_registry_global(void);
void bench_registry_global_shutdown(void);

void bench_report_init(bench_report* r, const bench_report_config* cfg);
void bench_report_destroy(bench_report* r);
int  bench_report_add_case(bench_report* r,
                           const char* name,
                           const char* group,
                           uint64_t iterations,
                           uint64_t bytes,
                           const uint64_t* samples_ns,
                           size_t samples_count);
void bench_report_sort(bench_report* r);
const bench_case_result* bench_report_cases(const bench_report* r, size_t* out_count);
void bench_report_print(FILE* out, bench_report* r);
bench_report* bench_report_global(void);
void bench_report_global_shutdown(void);

//------------------------------------------------------------------------------
// Time
//------------------------------------------------------------------------------

static uint64_t bench_now_ns(void) {
#if defined(_WIN32)
    static LARGE_INTEGER freq;
    static bool inited = false;
    if (!inited) {
        QueryPerformanceFrequency(&freq);
        inited = true;
    }
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    // Convert to ns with 128-bit intermediate
    uint64_t ticks = (uint64_t)c.QuadPart;
    uint64_t f     = (uint64_t)freq.QuadPart;
    return (uint64_t)(((__int128)ticks * 1000000000) / (__int128)f);
#else
    struct timespec ts;
  #if defined(CLOCK_MONOTONIC_RAW)
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
  #else
    clock_gettime(CLOCK_MONOTONIC, &ts);
  #endif
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#endif
}

static uint64_t bench_elapsed_ns(uint64_t start_ns, uint64_t end_ns) {
    return (end_ns >= start_ns) ? (end_ns - start_ns) : 0;
}

//------------------------------------------------------------------------------
// CLI
//------------------------------------------------------------------------------

typedef struct bench_cli {
    const char* filter_substr; // substring filter (optional)
    const char* exact_name;    // run only this exact benchmark (optional)
    bool        list_only;
    bool        json;
    bool        sort_mean;
    bool        no_header;

    size_t      warmup;
    size_t      samples;

    uint64_t    iterations; // iterations per sample (runner-defined)
} bench_cli;

static void bench_usage(FILE* out, const char* argv0) {
    fprintf(out,
        "Usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  --list                 List available benchmarks\n"
        "  --filter <substr>      Run only benchmarks whose name contains <substr>\n"
        "  --name <exact>         Run only the benchmark named <exact>\n"
        "  --warmup <N>           Warmup runs per benchmark (default: 5)\n"
        "  --samples <N>          Measured samples per benchmark (default: 30)\n"
        "  --iters <N>            Iterations per sample (default: 1)\n"
        "  --json                 JSON output\n"
        "  --sort-mean            Sort output by mean time (ascending)\n"
        "  --no-header            Do not print table header (human format)\n"
        "  -h, --help             Show this help\n",
        argv0);
}

static bool bench_streq(const char* a, const char* b) {
    return a && b && strcmp(a, b) == 0;
}

static bool bench_has_substr(const char* s, const char* sub) {
    if (!sub || !*sub) return true;
    if (!s) return false;
    return strstr(s, sub) != NULL;
}

static int bench_parse_u64(const char* s, uint64_t* out) {
    if (!s || !*s || !out) return -EINVAL;
    char* end = NULL;
    errno = 0;
    unsigned long long v = strtoull(s, &end, 10);
    if (errno != 0 || end == s || (end && *end != '\0')) return -EINVAL;
    *out = (uint64_t)v;
    return 0;
}

static int bench_parse_size(const char* s, size_t* out) {
    uint64_t v = 0;
    int rc = bench_parse_u64(s, &v);
    if (rc != 0) return rc;
    *out = (size_t)v;
    return 0;
}

static int bench_cli_parse(bench_cli* cli, int argc, char** argv) {
    memset(cli, 0, sizeof(*cli));
    cli->warmup = 5;
    cli->samples = 30;
    cli->iterations = 1;

    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (bench_streq(a, "-h") || bench_streq(a, "--help")) {
            return 1; // help
        } else if (bench_streq(a, "--list")) {
            cli->list_only = true;
        } else if (bench_streq(a, "--json")) {
            cli->json = true;
        } else if (bench_streq(a, "--sort-mean")) {
            cli->sort_mean = true;
        } else if (bench_streq(a, "--no-header")) {
            cli->no_header = true;
        } else if (bench_streq(a, "--filter")) {
            if (i + 1 >= argc) return -EINVAL;
            cli->filter_substr = argv[++i];
        } else if (bench_streq(a, "--name")) {
            if (i + 1 >= argc) return -EINVAL;
            cli->exact_name = argv[++i];
        } else if (bench_streq(a, "--warmup")) {
            if (i + 1 >= argc) return -EINVAL;
            int rc = bench_parse_size(argv[++i], &cli->warmup);
            if (rc != 0) return rc;
        } else if (bench_streq(a, "--samples")) {
            if (i + 1 >= argc) return -EINVAL;
            int rc = bench_parse_size(argv[++i], &cli->samples);
            if (rc != 0) return rc;
        } else if (bench_streq(a, "--iters")) {
            if (i + 1 >= argc) return -EINVAL;
            int rc = bench_parse_u64(argv[++i], &cli->iterations);
            if (rc != 0) return rc;
        } else {
            return -EINVAL;
        }
    }

    if (cli->samples == 0) return -EINVAL;
    if (cli->iterations == 0) cli->iterations = 1;
    return 0;
}

//------------------------------------------------------------------------------
// Runner
//------------------------------------------------------------------------------

static int bench_list(const bench_registry* reg, const bench_cli* cli) {
    size_t n = 0;
    const bench_entry* es = bench_registry_entries(reg, &n);
    for (size_t i = 0; i < n; ++i) {
        const bench_entry* e = &es[i];
        const char* name = e->name ? e->name : "";
        if (cli->exact_name && !bench_streq(name, cli->exact_name)) continue;
        if (!bench_has_substr(name, cli->filter_substr)) continue;
        if (e->group && *e->group) {
            printf("%s\t[%s]\n", name, e->group);
        } else {
            printf("%s\n", name);
        }
    }
    return 0;
}

static int bench_run_one(const bench_entry* e, const bench_cli* cli, bench_report* rep) {
    if (!e || !e->fn || !e->name) return -EINVAL;

    // Warmup
    bench_ctx ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.iterations = cli->iterations;

    for (size_t i = 0; i < cli->warmup; ++i) {
        ctx.sample_index = i;
        ctx.bytes = 0;
        e->fn(&ctx);
    }

    // Samples
    uint64_t* samples = (uint64_t*)malloc(cli->samples * sizeof(uint64_t));
    if (!samples) return -ENOMEM;

    // Record bytes observed on the *last* measured sample (best-effort).
    uint64_t last_bytes = 0;

    for (size_t i = 0; i < cli->samples; ++i) {
        ctx.sample_index = i;
        ctx.bytes = 0;

        uint64_t t0 = bench_now_ns();
        e->fn(&ctx);
        uint64_t t1 = bench_now_ns();

        samples[i] = bench_elapsed_ns(t0, t1);
        last_bytes = ctx.bytes;
    }

    int rc = bench_report_add_case(rep,
                                  e->name,
                                  e->group,
                                  ctx.iterations,
                                  last_bytes,
                                  samples,
                                  cli->samples);

    free(samples);
    return rc;
}

static int bench_run_all(const bench_registry* reg, const bench_cli* cli, bench_report* rep) {
    size_t n = 0;
    const bench_entry* es = bench_registry_entries(reg, &n);
    int rc = 0;

    for (size_t i = 0; i < n; ++i) {
        const bench_entry* e = &es[i];
        const char* name = e->name ? e->name : "";
        if (cli->exact_name && !bench_streq(name, cli->exact_name)) continue;
        if (!bench_has_substr(name, cli->filter_substr)) continue;

        rc = bench_run_one(e, cli, rep);
        if (rc != 0) {
            fprintf(stderr, "bench: '%s' failed (rc=%d)\n", name, rc);
            return rc;
        }
    }

    return 0;
}

//------------------------------------------------------------------------------
// Entrypoint
//------------------------------------------------------------------------------

int main(int argc, char** argv) {
    bench_cli cli;
    int prc = bench_cli_parse(&cli, argc, argv);
    if (prc == 1) {
        bench_usage(stdout, argv[0]);
        return 0;
    }
    if (prc != 0) {
        bench_usage(stderr, argv[0]);
        return 2;
    }

    bench_registry* reg = bench_registry_global();

    if (cli.list_only) {
        int rc = bench_list(reg, &cli);
        bench_registry_global_shutdown();
        return (rc == 0) ? 0 : 3;
    }

    bench_report_config cfg;
    cfg.format = cli.json ? BENCH_REPORT_JSON : BENCH_REPORT_HUMAN;
    cfg.show_header = !cli.no_header;
    cfg.sort_by_mean = cli.sort_mean;

    bench_report rep;
    bench_report_init(&rep, &cfg);

    int rc = bench_run_all(reg, &cli, &rep);
    if (rc == 0) {
        bench_report_print(stdout, &rep);
    }

    bench_report_destroy(&rep);
    bench_registry_global_shutdown();

    return (rc == 0) ? 0 : 4;
}