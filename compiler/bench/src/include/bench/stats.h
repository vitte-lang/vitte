

/*
  bench/stats.h

  Statistics primitives for the Vitte benchmark harness.

  Responsibilities
  - Online / offline aggregation over timing samples.
  - Deterministic percentile computation.
  - Robust summary statistics (mean, median, stdev, MAD, min/max).
  - Simple histogram support for reporting.

  Design
  - Allocation is explicit via vitte_arena.
  - Algorithms are deterministic (stable sorting, consistent tie-breaking).
  - Not intended for high-precision scientific computing; tuned for benchmarks.

  Units
  - This module is unit-agnostic; callers usually use nanoseconds for time.

  Notes
  - Percentiles use the "nearest-rank" method by default for determinism.
  - Welford's algorithm is used for online mean/variance.
*/

#pragma once
#ifndef VITTE_BENCH_STATS_H
#define VITTE_BENCH_STATS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/arena.h"
#include "bench/diag.h"
#include "bench/compiler.h"

/* -------------------------------------------------------------------------- */
/* Online accumulator (Welford)                                                */
/* -------------------------------------------------------------------------- */

typedef struct vitte_stats_online {
    uint64_t n;
    double mean;
    double m2;   /* sum of squares of differences from the current mean */
    double min;
    double max;
} vitte_stats_online;

static inline void vitte_stats_online_reset(vitte_stats_online* s)
{
    if (!s) return;
    s->n = 0;
    s->mean = 0.0;
    s->m2 = 0.0;
    s->min = 0.0;
    s->max = 0.0;
}

static inline void vitte_stats_online_push(vitte_stats_online* s, double x)
{
    if (!s) return;

    if (s->n == 0) {
        s->n = 1;
        s->mean = x;
        s->m2 = 0.0;
        s->min = x;
        s->max = x;
        return;
    }

    s->n++;

    if (x < s->min) s->min = x;
    if (x > s->max) s->max = x;

    const double delta = x - s->mean;
    s->mean += delta / (double)s->n;
    const double delta2 = x - s->mean;
    s->m2 += delta * delta2;
}

static inline double vitte_stats_online_variance(const vitte_stats_online* s)
{
    if (!s || s->n < 2) return 0.0;
    return s->m2 / (double)(s->n - 1);
}

static inline double vitte_stats_online_stdev(const vitte_stats_online* s)
{
    const double v = vitte_stats_online_variance(s);
    /* avoid pulling libm in headers if not already linked; do a small Newton */
    if (v <= 0.0) return 0.0;

    double x = v;
    /* 8 iterations is enough for double here */
    for (int i = 0; i < 8; ++i) {
        x = 0.5 * (x + v / x);
    }
    return x;
}

/* -------------------------------------------------------------------------- */
/* Offline summary (sorted samples)                                            */
/* -------------------------------------------------------------------------- */

typedef struct vitte_stats_summary {
    uint64_t n;

    double mean;
    double median;
    double stdev;

    double min;
    double max;

    /* Robust */
    double mad;      /* median absolute deviation */
    double iqr;      /* interquartile range (p75 - p25) */

    /* Common percentiles */
    double p01;
    double p05;
    double p10;
    double p25;
    double p50;
    double p75;
    double p90;
    double p95;
    double p99;
} vitte_stats_summary;

/* Percentile method selection (deterministic). */
typedef enum vitte_stats_pct_method {
    VITTE_STATS_PCT_NEAREST_RANK = 0,
    VITTE_STATS_PCT_LINEAR = 1
} vitte_stats_pct_method;

/* Compute percentile from a sorted array (count > 0). */
double vitte_stats_percentile_sorted(
    const double* sorted,
    size_t count,
    double pct,
    vitte_stats_pct_method method
);

/* Stable sort samples in-place (deterministic). */
bool vitte_stats_sort(double* samples, size_t count);

/* Compute full summary from samples. If `copy_and_sort` true, will copy into arena and sort. */
bool vitte_stats_summarize(
    vitte_arena* arena,
    const double* samples,
    size_t count,
    bool copy_and_sort,
    vitte_stats_summary* out,
    vitte_diag* d
);

/* -------------------------------------------------------------------------- */
/* Histogram                                                                    */
/* -------------------------------------------------------------------------- */

typedef struct vitte_stats_hist {
    /* bins are [edge[i], edge[i+1]) for i < bins-1; last bin is [edge[bins-1], +inf) */
    double* edges;     /* count = bins (arena-backed) */
    uint64_t* counts;  /* count = bins (arena-backed) */
    size_t bins;
} vitte_stats_hist;

/* Build histogram from sorted samples (deterministic). */
bool vitte_stats_hist_build(
    vitte_arena* arena,
    const double* sorted,
    size_t count,
    size_t bins,
    vitte_stats_hist* out,
    vitte_diag* d
);

/* -------------------------------------------------------------------------- */
/* Implementation                                                               */
/* -------------------------------------------------------------------------- */

static inline void vitte_stats_summary_reset(vitte_stats_summary* s)
{
    if (!s) return;
    s->n = 0;
    s->mean = s->median = s->stdev = 0.0;
    s->min = s->max = 0.0;
    s->mad = s->iqr = 0.0;
    s->p01 = s->p05 = s->p10 = 0.0;
    s->p25 = s->p50 = s->p75 = 0.0;
    s->p90 = s->p95 = s->p99 = 0.0;
}

/* A stable, deterministic merge-sort for doubles (in-place, arena scratch). */
static inline bool vitte_stats__merge_sort(vitte_arena* arena, double* a, size_t n)
{
    if (n < 2) return true;
    if (!arena || !a) return false;

    double* tmp = (double*)vitte_arena_alloc(arena, n * sizeof(double), 8);
    if (!tmp) return false;

    for (size_t width = 1; width < n; width *= 2) {
        for (size_t i = 0; i < n; i += 2 * width) {
            size_t left = i;
            size_t mid = (i + width < n) ? (i + width) : n;
            size_t right = (i + 2 * width < n) ? (i + 2 * width) : n;

            size_t p = left;
            size_t l = left;
            size_t r = mid;

            while (l < mid && r < right) {
                const double lv = a[l];
                const double rv = a[r];
                if (lv <= rv) tmp[p++] = lv, l++; /* stable */
                else tmp[p++] = rv, r++;
            }
            while (l < mid) tmp[p++] = a[l++];
            while (r < right) tmp[p++] = a[r++];

            for (size_t k = left; k < right; ++k) a[k] = tmp[k];
        }
    }

    return true;
}

bool vitte_stats_sort(double* samples, size_t count)
{
    if (!samples) return (count == 0);

    /* Use a small stack arena for scratch in the common case.
       If your arena is required, call vitte_stats_summarize(copy_and_sort=true).
    */
    vitte_arena tmp;
    vitte_arena_init(&tmp, NULL, 0);

    bool ok = vitte_stats__merge_sort(&tmp, samples, count);

    vitte_arena_destroy(&tmp);
    return ok;
}

static inline double vitte_stats__abs(double x) { return (x < 0.0) ? -x : x; }

static inline double vitte_stats_percentile_sorted_nearest_rank(const double* s, size_t n, double pct)
{
    if (!s || n == 0) return 0.0;
    if (pct <= 0.0) return s[0];
    if (pct >= 100.0) return s[n - 1];

    /* nearest-rank: rank = ceil(p/100 * n) */
    const double r = (pct / 100.0) * (double)n;
    size_t idx = (size_t)r;
    if (r > (double)idx) {
        /* ceil */
        idx += 0;
    } else {
        /* exact integer -> that rank */
        idx = (idx == 0) ? 0 : (idx - 1);
        return s[idx];
    }

    /* if r was not integer, ceil is idx, but ranks are 1-based */
    size_t rank = (size_t)(r + 0.999999999999); /* avoid ceil() */
    if (rank < 1) rank = 1;
    if (rank > n) rank = n;
    return s[rank - 1];
}

static inline double vitte_stats_percentile_sorted_linear(const double* s, size_t n, double pct)
{
    if (!s || n == 0) return 0.0;
    if (pct <= 0.0) return s[0];
    if (pct >= 100.0) return s[n - 1];

    const double pos = (pct / 100.0) * (double)(n - 1);
    size_t i0 = (size_t)pos;
    size_t i1 = (i0 + 1 < n) ? (i0 + 1) : i0;
    const double t = pos - (double)i0;
    return s[i0] + (s[i1] - s[i0]) * t;
}

double vitte_stats_percentile_sorted(
    const double* sorted,
    size_t count,
    double pct,
    vitte_stats_pct_method method
)
{
    if (method == VITTE_STATS_PCT_LINEAR) {
        return vitte_stats_percentile_sorted_linear(sorted, count, pct);
    }
    return vitte_stats_percentile_sorted_nearest_rank(sorted, count, pct);
}

bool vitte_stats_summarize(
    vitte_arena* arena,
    const double* samples,
    size_t count,
    bool copy_and_sort,
    vitte_stats_summary* out,
    vitte_diag* d
)
{
    if (!out) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_stats_summarize: out is NULL");
        return false;
    }

    vitte_stats_summary_reset(out);

    if (!samples || count == 0) {
        return true;
    }

    if (!arena) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_stats_summarize: arena required");
        return false;
    }

    /* online mean/stdev + min/max */
    vitte_stats_online on;
    vitte_stats_online_reset(&on);
    for (size_t i = 0; i < count; ++i) {
        vitte_stats_online_push(&on, samples[i]);
    }

    out->n = on.n;
    out->mean = on.mean;
    out->stdev = vitte_stats_online_stdev(&on);
    out->min = on.min;
    out->max = on.max;

    /* sorted view */
    const double* s = samples;
    double* tmp = NULL;

    if (copy_and_sort) {
        tmp = (double*)vitte_arena_alloc(arena, count * sizeof(double), 8);
        if (!tmp) {
            VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_stats_summarize: oom");
            return false;
        }
        for (size_t i = 0; i < count; ++i) tmp[i] = samples[i];
        if (!vitte_stats__merge_sort(arena, tmp, count)) {
            VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INTERNAL, VITTE_DIAG_F_PERM, "vitte_stats_summarize: sort failed");
            return false;
        }
        s = tmp;
    }

    const vitte_stats_pct_method m = VITTE_STATS_PCT_NEAREST_RANK;

    out->p01 = vitte_stats_percentile_sorted(s, count, 1.0, m);
    out->p05 = vitte_stats_percentile_sorted(s, count, 5.0, m);
    out->p10 = vitte_stats_percentile_sorted(s, count, 10.0, m);
    out->p25 = vitte_stats_percentile_sorted(s, count, 25.0, m);
    out->p50 = vitte_stats_percentile_sorted(s, count, 50.0, m);
    out->p75 = vitte_stats_percentile_sorted(s, count, 75.0, m);
    out->p90 = vitte_stats_percentile_sorted(s, count, 90.0, m);
    out->p95 = vitte_stats_percentile_sorted(s, count, 95.0, m);
    out->p99 = vitte_stats_percentile_sorted(s, count, 99.0, m);

    out->median = out->p50;
    out->iqr = out->p75 - out->p25;

    /* MAD: median(|x - median|) */
    if (copy_and_sort) {
        double* dev = (double*)vitte_arena_alloc(arena, count * sizeof(double), 8);
        if (!dev) {
            VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_stats_summarize: oom(mad)");
            return false;
        }
        for (size_t i = 0; i < count; ++i) dev[i] = vitte_stats__abs(s[i] - out->median);
        if (!vitte_stats__merge_sort(arena, dev, count)) {
            VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INTERNAL, VITTE_DIAG_F_PERM, "vitte_stats_summarize: sort failed(mad)");
            return false;
        }
        out->mad = vitte_stats_percentile_sorted(dev, count, 50.0, m);
    }

    return true;
}

bool vitte_stats_hist_build(
    vitte_arena* arena,
    const double* sorted,
    size_t count,
    size_t bins,
    vitte_stats_hist* out,
    vitte_diag* d
)
{
    if (!out) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_stats_hist_build: out is NULL");
        return false;
    }

    out->edges = NULL;
    out->counts = NULL;
    out->bins = 0;

    if (!sorted || count == 0 || bins == 0) {
        return true;
    }

    if (!arena) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_stats_hist_build: arena required");
        return false;
    }

    double mn = sorted[0];
    double mx = sorted[count - 1];

    if (mx < mn) {
        double t = mn; mn = mx; mx = t;
    }

    /* if all equal, single bin */
    if (mx == mn) {
        bins = 1;
    }

    double* edges = (double*)vitte_arena_alloc(arena, bins * sizeof(double), 8);
    uint64_t* counts = (uint64_t*)vitte_arena_alloc(arena, bins * sizeof(uint64_t), 8);
    if (!edges || !counts) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_stats_hist_build: oom");
        return false;
    }

    for (size_t i = 0; i < bins; ++i) counts[i] = 0;

    if (bins == 1) {
        edges[0] = mn;
        counts[0] = (uint64_t)count;
        out->edges = edges;
        out->counts = counts;
        out->bins = bins;
        return true;
    }

    const double span = mx - mn;
    const double step = span / (double)(bins - 1);

    for (size_t i = 0; i < bins; ++i) {
        edges[i] = mn + step * (double)i;
    }

    /* assign samples to bins deterministically */
    size_t idx = 0;
    for (; idx < count; ++idx) {
        const double x = sorted[idx];

        size_t bi = 0;
        if (x <= edges[0]) {
            bi = 0;
        } else if (x >= edges[bins - 1]) {
            bi = bins - 1;
        } else {
            /* locate interval: linear scan is OK for small bins, deterministic */
            for (size_t j = 0; j + 1 < bins; ++j) {
                if (x >= edges[j] && x < edges[j + 1]) {
                    bi = j;
                    break;
                }
            }
        }
        counts[bi]++;
    }

    out->edges = edges;
    out->counts = counts;
    out->bins = bins;
    return true;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_STATS_H */