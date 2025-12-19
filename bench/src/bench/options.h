// options.h - CLI options for vitte/bench (C17)
//
// SPDX-License-Identifier: MIT

#ifndef VITTE_BENCH_OPTIONS_H
#define VITTE_BENCH_OPTIONS_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>  // size_t
#include <stdint.h>  // int32_t, int64_t, uint64_t
#include <stdbool.h> // bool
#include <stdio.h>   // FILE

// -----------------------------------------------------------------------------
// Public types
// -----------------------------------------------------------------------------

typedef struct bench_options
{
    const char* program;

    // Output format (controls human table vs machine output to stdout).
    // "auto" means:
    //   - print human table if no machine-readable output requested
    //   - suppress human table if --json/--csv is used
    const char* format; // "auto" (default), "human", "json", "csv"

    // Selection
    const char* filter;     // substring or glob-like (runner-defined matching)
    const char* bench_name; // run a single named bench (optional)
    bool list;              // list available benches

    // Execution
    int64_t iters;          // iterations per call passed to benchmark fn (0 = auto/best-effort)
    int32_t repeat;         // run each benchmark N times
    int32_t warmup;         // warmup iterations per benchmark
    int32_t threads;        // concurrency hint; 1 = single-thread
    int64_t calibrate_ms;   // calibration target per measured sample; 0 = disabled
    int64_t time_budget_ms; // global wall-clock budget for the whole run; 0 = disabled
    uint64_t seed;          // RNG seed (supports 0x..)
    bool fail_fast;         // stop on first failure
    int32_t cpu;            // pin to CPU index; -1 disables

    // Output
    const char* out_json;   // path to json results
    const char* out_csv;    // path to csv results
    const char* output_version; // schema id, ex: "vitte.bench.v1"
    bool include_samples;       // include per-repeat samples in JSON (default: false)

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

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

// Returns deterministic defaults suitable for benchmarks.
bench_options bench_options_default(void);

// Parse argv into `opt`.
// Returns:
//   - BENCH_OPT_OK   : parsed successfully
//   - BENCH_OPT_EXIT : user asked for help/version (opt flags set)
//   - BENCH_OPT_ERR  : invalid arguments; error string written to `err`
bench_opt_result bench_options_parse(bench_options* opt, int argc, char** argv, char* err, size_t err_cap);

// Print usage/help text.
void bench_options_print_help(const char* prog, FILE* out);

// Print version string (depends on build macros; may be "dev").
void bench_options_print_version(FILE* out);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_BENCH_OPTIONS_H
