

// output.h - benchmark output/reporting for vitte/bench (C17)
//
// SPDX-License-Identifier: MIT

#ifndef VITTE_BENCH_OUTPUT_H
#define VITTE_BENCH_OUTPUT_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>  // int32_t, int64_t, uint64_t
#include <stdbool.h> // bool
#include <stdio.h>   // FILE

// -----------------------------------------------------------------------------
// Types
// -----------------------------------------------------------------------------

typedef enum bench_status
{
    BENCH_STATUS_OK = 0,
    BENCH_STATUS_FAILED = 1,
    BENCH_STATUS_SKIPPED = 2
} bench_status;

typedef struct bench_metric
{
    // Primary metric: nanoseconds per operation.
    double ns_per_op;

    // Extended stats over repeats (same unit as ns_per_op).
    double ns_per_op_median;
    double ns_per_op_p95;
    double ns_per_op_mad;
    double ns_per_op_iqr;
    double ns_per_op_ci95_low;
    double ns_per_op_ci95_high;

    // Optional throughput metrics.
    double bytes_per_sec;
    double items_per_sec;

    // Run context.
    int64_t iterations;
    double elapsed_ms;

    // Runner context (best-effort; 0 if unknown).
    int64_t iters_per_call;      // argument passed to benchmark fn
    int64_t calls_per_sample;    // number of fn calls per measured sample
    int64_t target_time_ms;      // calibration target for one sample

    // CPU/cycles telemetry (best-effort; 0 if unsupported).
    double cycles_per_sec_min;
    double cycles_per_sec_max;
    bool throttling_suspected;
} bench_metric;

typedef struct bench_result
{
    const char* name;
    bench_status status;
    bench_metric metric;

    // Optional measured per-repeat samples (NULL unless JSON output requested).
    const double* samples_ns_per_op;
    int32_t samples_count;

    // Optional failure reason (may be NULL).
    const char* error;
} bench_result;

typedef struct bench_report
{
    const bench_result* results;
    int32_t count;

    // Optional metadata.
    const char* schema; // output schema id (default: "vitte.bench.v1")
    const char* suite_name;
    uint64_t seed;
    int32_t threads;
    int32_t repeat;
    int32_t warmup;
    int64_t timestamp_ms;
    bool include_samples; // include per-repeat arrays in JSON

    // Runner info (for stable machine-readable output).
    int32_t cpu_pinned;      // 1 if pin requested+success, 0 otherwise
    int32_t cpu_index;       // requested CPU index, or -1 if not requested
    int64_t calibrate_ms;    // calibration target (0 disables)
    int64_t iters;           // requested iters per call (0 means auto)
} bench_report;

// -----------------------------------------------------------------------------
// API
// -----------------------------------------------------------------------------

// Print a compact human-readable table to `out`.
void bench_output_print_human(FILE* out, const bench_report* rep);

// Write JSON report to `path`. Returns true on success.
bool bench_output_write_json_path(const char* path, const bench_report* rep);

// Write CSV report to `path`. Returns true on success.
bool bench_output_write_csv_path(const char* path, const bench_report* rep);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_BENCH_OUTPUT_H
