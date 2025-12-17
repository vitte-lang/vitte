

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

    // Optional throughput metrics.
    double bytes_per_sec;
    double items_per_sec;

    // Run context.
    int64_t iterations;
    double elapsed_ms;
} bench_metric;

typedef struct bench_result
{
    const char* name;
    bench_status status;
    bench_metric metric;

    // Optional failure reason (may be NULL).
    const char* error;
} bench_result;

typedef struct bench_report
{
    const bench_result* results;
    int32_t count;

    // Optional metadata.
    const char* suite_name;
    uint64_t seed;
    int32_t threads;
    int32_t repeat;
    int32_t warmup;
    int64_t timestamp_ms;
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