// bench_stats.h - statistics helpers for vitte/bench (C17)
//
// SPDX-License-Identifier: MIT

#ifndef VITTE_BENCH_STATS_H
#define VITTE_BENCH_STATS_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct bench_stats {
  double min;
  double max;
  double mean;
  double stddev;  // sample stddev (n-1), 0 if n<2
  double p50;
  double p90;
  double p95;
  double p99;
  double mad;     // median absolute deviation (about median)
  double iqr;     // interquartile range (p75 - p25)
} bench_stats;

// Compute stats over an array of doubles.
// Returns 1 on success (including n==0), 0 on error (NULL pointers with n>0).
int bench_stats_compute_f64(const double* samples, size_t n, bench_stats* out);

// Bootstrap CI over the median (p50) of samples.
// - out_low/high: percentiles of the bootstrap distribution (e.g. 0.025/0.975).
// Returns 1 on success (including n==0), 0 on error.
int bench_stats_bootstrap_ci_median_f64(const double* samples, size_t n,
                                       uint64_t seed,
                                       int iters,
                                       double alpha_low,
                                       double alpha_high,
                                       double* out_low,
                                       double* out_high);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_BENCH_STATS_H

