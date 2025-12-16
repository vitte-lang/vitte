#pragma once

/*
  bench.h (max)

  Minimal, stable benchmarking API used by benchc.

  Design goals:
  - Simple registry of cases (micro/macro)
  - Monotonic clock in ns
  - Robust stats (mean, p50, p95, min, max)

  Conventions:
  - Case ids are strings: "micro:<name>" or "macro:<name>"
  - Case functions take a single opaque ctx pointer.
*/

#include <stddef.h>
#include <stdint.h>
#include "bench/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------
 * Platform helpers
 * ---------------------- */

#if defined(_MSC_VER)
  #define BENCH_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
  #define BENCH_INLINE __attribute__((always_inline)) inline
#else
  #define BENCH_INLINE inline
#endif

/* ----------------------
 * Time
 * ---------------------- */

/* Monotonic nanosecond clock. */
uint64_t bench_now_ns(void);

/* ----------------------
 * Registry
 * ---------------------- */

/* Returns pointer to a static array of cases; count written to out_count (if non-NULL). */
const bench_case_t* bench_registry_all(int* out_count);

/* ----------------------
 * Stats
 * ---------------------- */

bench_stats_t bench_compute_stats(const double* samples, int n);

/* ----------------------
 * Runner
 * ---------------------- */

/* benchc main entry (implemented in bench_main.c). */
int bench_run(int argc, char** argv);

#ifdef __cplusplus
} /* extern "C" */
#endif
