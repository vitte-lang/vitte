

// options.c - CLI options parsing for vitte/bench (C17)
//
// Goals:
//   - No getopt dependency; portable across Windows/macOS/Linux.
//   - Deterministic defaults suitable for benchmarking.
//   - Clear, CI-friendly error reporting.
//
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

// If an external options.h exists, prefer it.
#if defined(__has_include)
  #if __has_include("bench/options.h")
    #include "bench/options.h"
  #elif __has_include("options.h")
    #include "options.h"
  #endif
#endif

// Optional logger (best-effort).
#if defined(__has_include)
  #if __has_include("bench/log.h")
    #include "bench/log.h"
  #elif __has_include("log.h")
    #include "log.h"
  #endif
#endif

// -----------------------------------------------------------------------------
// Fallback public surface (only if options.h was not included)
// -----------------------------------------------------------------------------

#ifndef VITTE_BENCH_OPTIONS_H
#define VITTE_BENCH_OPTIONS_H

typedef struct bench_options
{
    const char* program;

    const char* format; // "auto" (default), "human", "json", "csv"

    // Selection
    const char* filter;     // substring or glob-like (runner-defined)
    const char* bench_name; // run a single named bench (optional)
    bool list;              // list available benches

    // Execution
    int64_t iters;          // iterations passed to benchmark fn
    int32_t repeat;         // run each benchmark N times
    int32_t warmup;         // warmup iterations per benchmark
    int32_t threads;        // runner-defined; 1 = single-thread
    int64_t calibrate_ms;   // calibration target per measured sample; 0 = disabled
    int64_t time_budget_ms; // global wall-clock budget; 0 = disabled
    uint64_t seed;          // RNG seed (if used)
    bool fail_fast;         // stop on first failure
    int32_t cpu;            // pin to CPU index; -1 disables

    // Output
    const char* out_json;   // path to json results
    const char* out_csv;    // path to csv results
    const char* output_version; // schema id
    bool include_samples;       // include samples in JSON

    // Logging/UI
    bool quiet;
    bool verbose;
    bool color;             // request color output

    // Meta
    bool show_help;
    bool show_version;
} bench_options;

typedef enum bench_opt_result
{
    BENCH_OPT_OK = 0,
    BENCH_OPT_EXIT = 1,
    BENCH_OPT_ERR = -1
} bench_opt_result;

bench_options bench_options_default(void);
bench_opt_result bench_options_parse(bench_options* opt, int argc, char** argv, char* err, size_t err_cap);
void bench_options_print_help(const char* prog, FILE* out);
void bench_options_print_version(FILE* out);

#endif // VITTE_BENCH_OPTIONS_H

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static void bo__set_err(char* err, size_t cap, const char* msg)
{
    if (!err || cap == 0) return;
    if (!msg) msg = "error";
    (void)snprintf(err, cap, "%s", msg);
}

static void bo__set_errf1(char* err, size_t cap, const char* fmt, const char* a)
{
    if (!err || cap == 0) return;
    if (!fmt) { bo__set_err(err, cap, "error"); return; }
    (void)snprintf(err, cap, fmt, a ? a : "");
}

static void bo__set_errf2(char* err, size_t cap, const char* fmt, const char* a, const char* b)
{
    if (!err || cap == 0) return;
    if (!fmt) { bo__set_err(err, cap, "error"); return; }
    (void)snprintf(err, cap, fmt, a ? a : "", b ? b : "");
}

static bool bo__streq(const char* a, const char* b)
{
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

static bool bo__starts_with(const char* s, const char* pfx)
{
    if (!s || !pfx) return false;
    size_t n = strlen(pfx);
    return strncmp(s, pfx, n) == 0;
}

static const char* bo__basename(const char* path)
{
    if (!path) return "bench";
    const char* b = path;
    for (const char* p = path; *p; ++p)
    {
        if (*p == '/' || *p == '\\') b = p + 1;
    }
    return b;
}

static bool bo__parse_i32(const char* s, int32_t* out)
{
    if (out) *out = 0;
    if (!s || !*s) return false;
    char* endp = NULL;
    long v = strtol(s, &endp, 10);
    if (!endp || *endp != '\0') return false;
    if (v < INT32_MIN || v > INT32_MAX) return false;
    if (out) *out = (int32_t)v;
    return true;
}

static bool bo__parse_i64(const char* s, int64_t* out)
{
    if (out) *out = 0;
    if (!s || !*s) return false;
    char* endp = NULL;
    long long v = strtoll(s, &endp, 10);
    if (!endp || *endp != '\0') return false;
    if (out) *out = (int64_t)v;
    return true;
}

static bool bo__parse_u64(const char* s, uint64_t* out)
{
    if (out) *out = 0;
    if (!s || !*s) return false;
    char* endp = NULL;
    unsigned long long v = strtoull(s, &endp, 0); // base 0: accepts 0x...
    if (!endp || *endp != '\0') return false;
    if (out) *out = (uint64_t)v;
    return true;
}

static bool bo__take_value(int* i, int argc, char** argv, const char* arg, const char* opt_name, const char** out_val)
{
    // Supports:
    //   --opt=value
    //   --opt value
    //   -o value
    // Does NOT support clustered short flags like -abc.

    if (out_val) *out_val = NULL;
    if (!i || !argv || !arg || !opt_name) return false;

    const char* eq = strchr(arg, '=');
    if (eq)
    {
        // --opt=value
        if (out_val) *out_val = eq + 1;
        return true;
    }

    // --opt value
    if (*i + 1 >= argc) return false;
    (*i)++;
    if (out_val) *out_val = argv[*i];
    return true;
}

static void bo__normalize_logging(bench_options* opt)
{
    // If both quiet and verbose are set, quiet wins.
    if (!opt) return;
    if (opt->quiet) opt->verbose = false;

#if defined(BENCH_LOG_TRACE) && defined(BLOG_INFO)
    // If log.h is present and initialized elsewhere, we can set level.
    // Keep this best-effort and non-fatal.
    if (opt->quiet)
        bench_log_set_level(BENCH_LOG_ERROR);
    else if (opt->verbose)
        bench_log_set_level(BENCH_LOG_DEBUG);
#endif
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

bench_options bench_options_default(void)
{
    bench_options o;
    memset(&o, 0, sizeof(o));

    o.program = "bench";

    o.format = "auto";

    o.filter = NULL;
    o.bench_name = NULL;
    o.list = false;

    o.iters = 0;
    o.repeat = 1;
    o.warmup = 0;
    o.threads = 1;
    o.calibrate_ms = 300;
    o.time_budget_ms = 0;
    o.seed = 0;
    o.fail_fast = false;
    o.cpu = -1;

    o.out_json = NULL;
    o.out_csv = NULL;
    o.output_version = "vitte.bench.v1";
    o.include_samples = false;

    o.quiet = false;
    o.verbose = false;
    o.color = true;

    o.show_help = false;
    o.show_version = false;

    return o;
}

bench_opt_result bench_options_parse(bench_options* opt, int argc, char** argv, char* err, size_t err_cap)
{
    if (err && err_cap) err[0] = '\0';

    if (!opt)
    {
        bo__set_err(err, err_cap, "options: null output struct");
        return BENCH_OPT_ERR;
    }

    *opt = bench_options_default();

    if (argc > 0 && argv && argv[0])
        opt->program = bo__basename(argv[0]);

    for (int i = 1; i < argc; ++i)
    {
        const char* a = argv[i];
        if (!a || !*a) continue;

        // Positional: treat as bench_name if provided and not an option.
        if (a[0] != '-')
        {
            if (!opt->bench_name)
            {
                opt->bench_name = a;
                continue;
            }
            bo__set_errf2(err, err_cap, "unexpected argument '%s' (already have bench '%s')", a, opt->bench_name);
            return BENCH_OPT_ERR;
        }

        // Common help/version
        if (bo__streq(a, "-h") || bo__streq(a, "--help"))
        {
            opt->show_help = true;
            return BENCH_OPT_EXIT;
        }
        if (bo__streq(a, "--version") || bo__streq(a, "-V"))
        {
            opt->show_version = true;
            return BENCH_OPT_EXIT;
        }

        if (bo__streq(a, "--list") || bo__streq(a, "-l"))
        {
            opt->list = true;
            continue;
        }

        if (bo__streq(a, "--fail-fast"))
        {
            opt->fail_fast = true;
            continue;
        }

        if (bo__streq(a, "--quiet") || bo__streq(a, "-q"))
        {
            opt->quiet = true;
            continue;
        }

        if (bo__streq(a, "--verbose") || bo__streq(a, "-v"))
        {
            opt->verbose = true;
            continue;
        }

        if (bo__streq(a, "--color"))
        {
            opt->color = true;
            continue;
        }

        if (bo__streq(a, "--no-color"))
        {
            opt->color = false;
            continue;
        }

        // value options
        if (bo__starts_with(a, "--filter"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--filter", &v) || !v || !*v)
            {
                bo__set_err(err, err_cap, "--filter requires a non-empty value");
                return BENCH_OPT_ERR;
            }
            opt->filter = v;
            continue;
        }

        if (bo__starts_with(a, "--bench") || bo__streq(a, "-b"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--bench", &v) || !v || !*v)
            {
                bo__set_err(err, err_cap, "--bench requires a non-empty value");
                return BENCH_OPT_ERR;
            }
            opt->bench_name = v;
            continue;
        }

        if (bo__starts_with(a, "--repeat") || bo__streq(a, "-r"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--repeat", &v))
            {
                bo__set_err(err, err_cap, "--repeat requires a value");
                return BENCH_OPT_ERR;
            }
            int32_t n = 0;
            if (!bo__parse_i32(v, &n) || n <= 0)
            {
                bo__set_errf1(err, err_cap, "invalid --repeat '%s' (expected integer > 0)", v);
                return BENCH_OPT_ERR;
            }
            opt->repeat = n;
            continue;
        }

        if (bo__starts_with(a, "--warmup"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--warmup", &v))
            {
                bo__set_err(err, err_cap, "--warmup requires a value");
                return BENCH_OPT_ERR;
            }
            int32_t n = 0;
            if (!bo__parse_i32(v, &n) || n < 0)
            {
                bo__set_errf1(err, err_cap, "invalid --warmup '%s' (expected integer >= 0)", v);
                return BENCH_OPT_ERR;
            }
            opt->warmup = n;
            continue;
        }

        if (bo__starts_with(a, "--threads") || bo__streq(a, "-j"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--threads", &v))
            {
                bo__set_err(err, err_cap, "--threads requires a value");
                return BENCH_OPT_ERR;
            }
            int32_t n = 0;
            if (!bo__parse_i32(v, &n) || n <= 0)
            {
                bo__set_errf1(err, err_cap, "invalid --threads '%s' (expected integer > 0)", v);
                return BENCH_OPT_ERR;
            }
            opt->threads = n;
            continue;
        }

        if (bo__starts_with(a, "--iters"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--iters", &v))
            {
                bo__set_err(err, err_cap, "--iters requires a value");
                return BENCH_OPT_ERR;
            }
            int64_t it = 0;
            if (!bo__parse_i64(v, &it) || it < 0)
            {
                bo__set_errf1(err, err_cap, "invalid --iters '%s' (expected integer >= 0)", v);
                return BENCH_OPT_ERR;
            }
            opt->iters = it;
            continue;
        }

        if (bo__starts_with(a, "--min-time-ms") || bo__starts_with(a, "--calibrate-ms"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--calibrate-ms", &v))
            {
                bo__set_err(err, err_cap, "--calibrate-ms requires a value");
                return BENCH_OPT_ERR;
            }
            int64_t ms = 0;
            if (!bo__parse_i64(v, &ms) || ms < 0)
            {
                bo__set_errf1(err, err_cap, "invalid --calibrate-ms '%s' (expected integer >= 0)", v);
                return BENCH_OPT_ERR;
            }
            opt->calibrate_ms = ms;
            continue;
        }

        if (bo__starts_with(a, "--time-budget-ms"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--time-budget-ms", &v))
            {
                bo__set_err(err, err_cap, "--time-budget-ms requires a value");
                return BENCH_OPT_ERR;
            }
            int64_t ms = 0;
            if (!bo__parse_i64(v, &ms) || ms < 0)
            {
                bo__set_errf1(err, err_cap, "invalid --time-budget-ms '%s' (expected integer >= 0)", v);
                return BENCH_OPT_ERR;
            }
            opt->time_budget_ms = ms;
            continue;
        }

        if (bo__starts_with(a, "--cpu"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--cpu", &v))
            {
                bo__set_err(err, err_cap, "--cpu requires a value");
                return BENCH_OPT_ERR;
            }
            int32_t cpu = -1;
            if (!bo__parse_i32(v, &cpu))
            {
                bo__set_errf1(err, err_cap, "invalid --cpu '%s' (expected integer)", v);
                return BENCH_OPT_ERR;
            }
            opt->cpu = cpu;
            continue;
        }

        if (bo__starts_with(a, "--seed"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--seed", &v))
            {
                bo__set_err(err, err_cap, "--seed requires a value");
                return BENCH_OPT_ERR;
            }
            uint64_t seed = 0;
            if (!bo__parse_u64(v, &seed))
            {
                bo__set_errf1(err, err_cap, "invalid --seed '%s' (expected integer, supports 0x..)", v);
                return BENCH_OPT_ERR;
            }
            opt->seed = seed;
            continue;
        }

        if (bo__starts_with(a, "--json"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--json", &v) || !v || !*v)
            {
                bo__set_err(err, err_cap, "--json requires a non-empty path");
                return BENCH_OPT_ERR;
            }
            opt->out_json = v;
            continue;
        }

        if (bo__starts_with(a, "--csv"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--csv", &v) || !v || !*v)
            {
                bo__set_err(err, err_cap, "--csv requires a non-empty path");
                return BENCH_OPT_ERR;
            }
            opt->out_csv = v;
            continue;
        }

        if (bo__starts_with(a, "--format"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--format", &v) || !v || !*v)
            {
                bo__set_err(err, err_cap, "--format requires a value (auto|human|json|csv)");
                return BENCH_OPT_ERR;
            }
            if (!bo__streq(v, "auto") && !bo__streq(v, "human") && !bo__streq(v, "json") && !bo__streq(v, "csv"))
            {
                bo__set_errf1(err, err_cap, "invalid --format '%s' (expected auto|human|json|csv)", v);
                return BENCH_OPT_ERR;
            }
            opt->format = v;
            continue;
        }

        if (bo__streq(a, "--samples"))
        {
            opt->include_samples = true;
            continue;
        }

        if (bo__streq(a, "--no-samples"))
        {
            opt->include_samples = false;
            continue;
        }

        if (bo__starts_with(a, "--output-version"))
        {
            const char* v = NULL;
            if (!bo__take_value(&i, argc, argv, a, "--output-version", &v) || !v || !*v)
            {
                bo__set_err(err, err_cap, "--output-version requires a value");
                return BENCH_OPT_ERR;
            }
            if (!bo__streq(v, "vitte.bench.v1"))
            {
                bo__set_errf1(err, err_cap, "unsupported --output-version '%s' (expected vitte.bench.v1)", v);
                return BENCH_OPT_ERR;
            }
            opt->output_version = v;
            continue;
        }

        // Unknown option
        bo__set_errf1(err, err_cap, "unknown option '%s'", a);
        return BENCH_OPT_ERR;
    }

    // --format convenience: default JSON/CSV destination to stdout.
    if (opt->format && bo__streq(opt->format, "json") && (!opt->out_json || !*opt->out_json))
        opt->out_json = "-";
    if (opt->format && bo__streq(opt->format, "csv") && (!opt->out_csv || !*opt->out_csv))
        opt->out_csv = "-";

    bo__normalize_logging(opt);
    return BENCH_OPT_OK;
}

void bench_options_print_help(const char* prog, FILE* out)
{
    if (!out) out = stderr;
    const char* p = bo__basename(prog);

    (void)fprintf(out,
        "Usage: %s [options] [bench_name]\n"
        "\n"
        "Selection:\n"
        "  -l, --list                 List available benchmarks\n"
        "  -b, --bench <name>         Run a single benchmark by exact name\n"
        "      --filter <pattern>     Filter benchmarks (runner-defined matching)\n"
        "\n"
        "Execution:\n"
        "      --iters <N>            Iterations passed to benchmark (0 = auto, default: 0)\n"
        "  -r, --repeat <N>           Run each benchmark N times (default: 1)\n"
        "      --warmup <N>           Warmup iterations per benchmark (default: 0)\n"
        "  -j, --threads <N>          Threads/concurrency hint (default: 1)\n"
        "      --calibrate-ms <MS>    Calibration target per measured sample (default: 300; 0 disables)\n"
        "      --min-time-ms <MS>     Alias for --calibrate-ms\n"
        "      --time-budget-ms <MS>  Global wall-clock budget for the whole run (0 disables)\n"
        "      --seed <U64>           RNG seed (supports 0x.., default: 0)\n"
        "      --fail-fast            Stop on first failure\n"
        "      --cpu <index>          Pin current thread to CPU (best-effort, default: -1)\n"
        "\n"
        "Output:\n"
        "      --format <f>           auto|human|json|csv (default: auto)\n"
        "      --json <path>          Write results as JSON\n"
        "      --csv <path>           Write results as CSV\n"
        "      --output-version <id>  Output schema id (default: vitte.bench.v1)\n"
        "      --samples              Include per-repeat samples in JSON (opt-in)\n"
        "      --no-samples           Disable per-repeat samples in JSON (default)\n"
        "\n"
        "Logging/UI:\n"
        "  -q, --quiet                Reduce output (errors only)\n"
        "  -v, --verbose              Increase output\n"
        "      --color                Force color output (default)\n"
        "      --no-color             Disable color output\n"
        "\n"
        "Meta:\n"
        "  -h, --help                 Show this help\n"
        "  -V, --version              Show version\n",
        p);
}

void bench_options_print_version(FILE* out)
{
    if (!out) out = stdout;

#if defined(VITTE_VERSION_STRING)
    (void)fprintf(out, "%s\n", VITTE_VERSION_STRING);
#elif defined(VITTE_VERSION)
    (void)fprintf(out, "%s\n", VITTE_VERSION);
#elif defined(BENCH_VERSION)
    (void)fprintf(out, "%s\n", BENCH_VERSION);
#else
    (void)fprintf(out, "dev\n");
#endif
}
