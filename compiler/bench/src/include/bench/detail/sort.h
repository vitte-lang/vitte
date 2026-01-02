/*
  bench/detail/sort.h

  Sorting helpers for the Vitte benchmark harness.

  Motivation
  - Bench harnesses frequently need to:
    - sort samples to compute percentiles
    - select medians / quantiles
    - compute trimmed means
  - Using qsort is fine, but this header provides:
    - a small stable API
    - specialized fast paths for u64/double
    - deterministic behavior (no locale, no FP exceptions)

  Pairing
    - bench/detail/sort.c implements these functions.
    - bench/detail/compat.h provides portability helpers.

  Notes
  - All sort functions are in-place.
  - For `double`, NaN handling is deterministic:
    - NaNs are ordered last.
    - -0.0 and +0.0 are considered equal.
*/

#pragma once
#ifndef VITTE_BENCH_DETAIL_SORT_H
#define VITTE_BENCH_DETAIL_SORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/detail/compat.h"

/* -------------------------------------------------------------------------- */
/* Comparators                                                                 */
/* -------------------------------------------------------------------------- */

/*
  Compare two u64 values.
  Returns: -1, 0, +1.
*/
int vitte_cmp_u64(uint64_t a, uint64_t b);

/*
  Compare two doubles.
  Deterministic ordering:
    - NaNs sort last
    - -0.0 and +0.0 are equal
*/
int vitte_cmp_f64(double a, double b);

/* -------------------------------------------------------------------------- */
/* Sorting                                                                     */
/* -------------------------------------------------------------------------- */

/*
  Sort an array of u64 ascending.
*/
void vitte_sort_u64(uint64_t* a, size_t n);

/*
  Sort an array of double ascending (NaNs last).
*/
void vitte_sort_f64(double* a, size_t n);

/*
  Stable sort for records using an index indirection.

  - `keys` is the key array (u64) of length n.
  - `idx` is a permutation array length n.
  - On return, `idx` contains indices [0..n) in order of ascending key.
  - Ties are stable (preserve input order).

  Complexity: O(n log n)
*/
void vitte_sort_index_u64_stable(const uint64_t* keys, size_t n, uint32_t* idx);

/* -------------------------------------------------------------------------- */
/* Selection (order statistics)                                                */
/* -------------------------------------------------------------------------- */

/*
  Select the k-th smallest element (0-based) from a u64 array.
  The array is modified (partitioned).

  Returns the selected value.
*/
uint64_t vitte_select_k_u64(uint64_t* a, size_t n, size_t k);

/*
  Select the k-th smallest element (0-based) from a double array.
  The array is modified (partitioned).

  Deterministic ordering: NaNs are treated as +infinity (last).
*/
double vitte_select_k_f64(double* a, size_t n, size_t k);

/* -------------------------------------------------------------------------- */
/* Percentiles                                                                 */
/* -------------------------------------------------------------------------- */

/*
  Compute a percentile from an already-sorted array.

  Percentile definition:
    - Uses the nearest-rank method.
    - p is in [0.0, 100.0].

  Returns:
    - 0 on success
    - non-zero on error (invalid args)
*/
int vitte_percentile_sorted_u64(const uint64_t* sorted, size_t n, double p, uint64_t* out);

int vitte_percentile_sorted_f64(const double* sorted, size_t n, double p, double* out);

/* -------------------------------------------------------------------------- */
/* Aggregates                                                                  */
/* -------------------------------------------------------------------------- */

/*
  Compute mean of u64 samples as double.
  Returns 0 on success.
*/
int vitte_mean_u64(const uint64_t* a, size_t n, double* out_mean);

/*
  Compute mean of double samples.
*/
int vitte_mean_f64(const double* a, size_t n, double* out_mean);

/*
  Compute a trimmed mean of sorted samples.

  - `trim_low` and `trim_high` specify how many samples to drop from each end.
  - Requires sorted input.

  Returns 0 on success.
*/
int vitte_trimmed_mean_sorted_u64(const uint64_t* sorted, size_t n, size_t trim_low, size_t trim_high, double* out_mean);

int vitte_trimmed_mean_sorted_f64(const double* sorted, size_t n, size_t trim_low, size_t trim_high, double* out_mean);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_DETAIL_SORT_H */
