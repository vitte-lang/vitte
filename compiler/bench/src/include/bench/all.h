/*
  bench/all.h

  Umbrella include for the Vitte benchmark harness.

  Design
  - C17-first, safe for inclusion from C++ translation units.
  - Minimal stable surface; "detail" headers are implementation-oriented.
  - Deterministic: utilities avoid locale, wall-clock, and non-deterministic IO.

  Typical usage
    #include "bench/all.h"

  Notes
  - `bench/detail/*` headers are considered private to the bench subsystem.
  - This umbrella is intended for the bench runtime itself and internal tools.
*/

#pragma once
#ifndef VITTE_BENCH_ALL_H
#define VITTE_BENCH_ALL_H

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Standard includes                                                           */
/* -------------------------------------------------------------------------- */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* -------------------------------------------------------------------------- */
/* Core building blocks                                                        */
/* -------------------------------------------------------------------------- */

#include "bench/detail/compat.h"
#include "bench/detail/format.h"
#include "bench/detail/sort.h"

/* -------------------------------------------------------------------------- */
/* Versioning                                                                  */
/* -------------------------------------------------------------------------- */

/* Public bench API version (umbrella surface). */
#define VITTE_BENCH_API_VERSION 1

/* Compatibility layer version (bench/detail/compat.h). */
#define VITTE_BENCH_COMPAT_VERSION VITTE_BENCH_COMPAT_API_VERSION

/* -------------------------------------------------------------------------- */
/* Optional headers (best-effort)                                              */
/* -------------------------------------------------------------------------- */

/*
  If the repository later grows stable public headers under `bench/`, this
  umbrella can include them opportunistically without breaking older snapshots.

  Example future headers:
    - bench/runner.h
    - bench/report.h
    - bench/stats.h
*/

#if defined(__has_include)
  #if __has_include("bench/runner.h")
    #include "bench/runner.h"
  #endif
  #if __has_include("bench/report.h")
    #include "bench/report.h"
  #endif
  #if __has_include("bench/stats.h")
    #include "bench/stats.h"
  #endif
#endif

/* -------------------------------------------------------------------------- */
/* Status codes                                                                */
/* -------------------------------------------------------------------------- */

typedef enum vitte_bench_status {
    VITTE_BENCH_OK = 0,
    VITTE_BENCH_ERR_INVALID = 1,
    VITTE_BENCH_ERR_IO = 2,
    VITTE_BENCH_ERR_OOM = 3,
    VITTE_BENCH_ERR_UNSUPPORTED = 4
} vitte_bench_status;

/* -------------------------------------------------------------------------- */
/* Common types                                                                */
/* -------------------------------------------------------------------------- */

typedef struct vitte_bench_span_u64 {
    uint64_t* data;
    size_t    len;
} vitte_bench_span_u64;

typedef struct vitte_bench_span_f64 {
    double* data;
    size_t  len;
} vitte_bench_span_f64;

/*
  Canonical summary statistics for a sample set.

  Contract
  - `min` and `max` are sample extrema.
  - `mean` is arithmetic mean.
  - percentiles use nearest-rank over sorted samples.
*/
typedef struct vitte_bench_summary_u64 {
    uint64_t min;
    uint64_t p50;
    uint64_t p90;
    uint64_t p95;
    uint64_t p99;
    uint64_t max;
    double   mean;
} vitte_bench_summary_u64;

typedef struct vitte_bench_summary_f64 {
    double min;
    double p50;
    double p90;
    double p95;
    double p99;
    double max;
    double mean;
} vitte_bench_summary_f64;

/* -------------------------------------------------------------------------- */
/* Convenience wrappers                                                        */
/* -------------------------------------------------------------------------- */

/*
  Snapshot a timestamp and cycles counter.

  This is useful to capture the overhead or calibration cost.
*/
static inline uint64_t vitte_bench_now_ns(void)
{
    return vitte_bench_time_now_ns();
}

static inline uint64_t vitte_bench_cycles(void)
{
    return vitte_bench_cycles_now();
}

/* -------------------------------------------------------------------------- */
/* Summary computation (in-place sort)                                         */
/* -------------------------------------------------------------------------- */

/*
  Compute summary stats for u64 samples.

  - Sorts `samples` in-place.
  - Requires n > 0.
  - Returns 0 on success.
*/
static inline int vitte_bench_summarize_u64(uint64_t* samples, size_t n, vitte_bench_summary_u64* out)
{
    if (!samples || !out || n == 0) {
        return (int)VITTE_BENCH_ERR_INVALID;
    }

    vitte_sort_u64(samples, n);

    out->min = samples[0];
    out->max = samples[n - 1];

    if (vitte_mean_u64(samples, n, &out->mean) != 0) {
        out->mean = 0.0;
        return (int)VITTE_BENCH_ERR_INVALID;
    }

    /* Percentiles (nearest-rank). */
    if (vitte_percentile_sorted_u64(samples, n, 50.0, &out->p50) != 0) return (int)VITTE_BENCH_ERR_INVALID;
    if (vitte_percentile_sorted_u64(samples, n, 90.0, &out->p90) != 0) return (int)VITTE_BENCH_ERR_INVALID;
    if (vitte_percentile_sorted_u64(samples, n, 95.0, &out->p95) != 0) return (int)VITTE_BENCH_ERR_INVALID;
    if (vitte_percentile_sorted_u64(samples, n, 99.0, &out->p99) != 0) return (int)VITTE_BENCH_ERR_INVALID;

    return 0;
}

/*
  Compute summary stats for double samples.

  - Sorts `samples` in-place.
  - Deterministic NaN handling: NaNs sort last.
  - Requires n > 0.
*/
static inline int vitte_bench_summarize_f64(double* samples, size_t n, vitte_bench_summary_f64* out)
{
    if (!samples || !out || n == 0) {
        return (int)VITTE_BENCH_ERR_INVALID;
    }

    vitte_sort_f64(samples, n);

    out->min = samples[0];
    out->max = samples[n - 1];

    if (vitte_mean_f64(samples, n, &out->mean) != 0) {
        out->mean = 0.0;
        return (int)VITTE_BENCH_ERR_INVALID;
    }

    if (vitte_percentile_sorted_f64(samples, n, 50.0, &out->p50) != 0) return (int)VITTE_BENCH_ERR_INVALID;
    if (vitte_percentile_sorted_f64(samples, n, 90.0, &out->p90) != 0) return (int)VITTE_BENCH_ERR_INVALID;
    if (vitte_percentile_sorted_f64(samples, n, 95.0, &out->p95) != 0) return (int)VITTE_BENCH_ERR_INVALID;
    if (vitte_percentile_sorted_f64(samples, n, 99.0, &out->p99) != 0) return (int)VITTE_BENCH_ERR_INVALID;

    return 0;
}

/* -------------------------------------------------------------------------- */
/* Formatting helpers                                                          */
/* -------------------------------------------------------------------------- */

/*
  Format a u64 duration (nanoseconds) as a short human-readable string.

  - Writes into `buf` as NUL-terminated.
  - Returns the number of chars that would have been written (snprintf style).

  Suggested output forms:
    - "123ns"
    - "12.3us"
    - "1.23ms"
    - "0.12s"

  Implementation note:
  - Implemented as a header-only helper to avoid new object files.
*/
static inline size_t vitte_bench_format_duration_ns(char* buf, size_t cap, uint64_t ns)
{
    /* Simple scaling without locale; keep deterministic rounding. */
    if (!buf || cap == 0) {
        return 0;
    }

    if (ns < 1000ULL) {
        return vitte_bench_snprintf(buf, cap, "%llu%s", (unsigned long long)ns, "ns");
    }

    if (ns < 1000ULL * 1000ULL) {
        double us = (double)ns / 1000.0;
        return vitte_bench_snprintf(buf, cap, "%.3g%s", us, "us");
    }

    if (ns < 1000ULL * 1000ULL * 1000ULL) {
        double ms = (double)ns / 1000000.0;
        return vitte_bench_snprintf(buf, cap, "%.3g%s", ms, "ms");
    }

    {
        double s = (double)ns / 1000000000.0;
        return vitte_bench_snprintf(buf, cap, "%.3g%s", s, "s");
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_ALL_H */
