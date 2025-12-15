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
 * Core types
 * ---------------------- */

typedef enum bench_kind {
  BENCH_MICRO = 0,
  BENCH_MACRO = 1,
} bench_kind;

typedef void (*bench_fn)(void* ctx);

typedef struct bench_case {
  const char* id;     /* e.g. "micro:hash" */
  bench_kind kind;    /* BENCH_MICRO or BENCH_MACRO */
  bench_fn fn;        /* benchmark body */
  void* ctx;          /* opaque context (often NULL) */
} bench_case;

typedef struct bench_stats {
  double mean;
  double p50;
  double p95;
  double min;
  double max;
} bench_stats;

/* ----------------------
 * Time
 * ---------------------- */

/* Monotonic nanosecond clock. */
uint64_t bench_now_ns(void);

/* ----------------------
 * Registry
 * ---------------------- */

/* Returns pointer to a static array of cases; count written to out_count (if non-NULL). */
const bench_case* bench_registry_all(int* out_count);

/* ----------------------
 * Stats
 * ---------------------- */

bench_stats bench_compute_stats(const double* samples, int n);

/* ----------------------
 * Runner
 * ---------------------- */

/* benchc main entry (implemented in bench_main.c). */
int bench_run(int argc, char** argv);

#ifdef __cplusplus
} /* extern "C" */
#endif
