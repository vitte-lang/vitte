

// timing.h - timing/stat helpers for vitte/bench (C17)
//
// SPDX-License-Identifier: MIT

#ifndef VITTE_BENCH_TIMING_H
#define VITTE_BENCH_TIMING_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

// -----------------------------------------------------------------------------
// Timer
// -----------------------------------------------------------------------------

typedef struct bench_timer
{
    uint64_t t0_ns;
} bench_timer;

// Start timer.
void bench_timer_start(bench_timer* t);

// Elapsed time since start in nanoseconds.
uint64_t bench_timer_elapsed_ns(const bench_timer* t);

// Conversions.
double bench_ns_to_ms(uint64_t ns);
double bench_ns_to_s(uint64_t ns);

// -----------------------------------------------------------------------------
// Stats
// -----------------------------------------------------------------------------

typedef struct bench_stats
{
    int32_t count;

    // Basic stats
    double min;
    double max;
    double mean;
    double stddev;

    // Quantiles
    double median;
    double p90;
    double p99;
} bench_stats;

// Compute stats over `samples[count]`.
bench_stats bench_stats_compute(const double* samples, int32_t count);

// -----------------------------------------------------------------------------
// Formatting
// -----------------------------------------------------------------------------

// Format a duration expressed in nanoseconds into a compact string (ns/us/ms/s).
// Writes into `out` (capacity `cap`) and returns `out`.
char* bench_format_duration(double ns, char* out, size_t cap);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_BENCH_TIMING_H