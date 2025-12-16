// File: bench/src/bench/bench_stats.c
// Vitte benchmark statistics — "max" (C17/C23).
//
// Fournit :
//   - bench_samples_t: vecteur dynamique de samples (double)
//   - bench_stats_compute(): min/max/mean/stddev/variance + quantiles + MAD/IQR + outliers
//   - bench_samples_trimmed(): trimming (none/iqr/mad) -> nouveau bench_samples_t
//   - bench_stats_ops_per_s_from_ns_per_op(): conversion
//   - bench_stats_debug_print(): dump lisible
//
// Conventions :
//   - Les samples sont des double, typiquement ns/op.
//   - Tous les calculs ignorent les NaN/Inf (n_finite).
//   - Quantiles: interpolation linéaire sur série triée.
//   - Outliers IQR: fences Tukey (1.5 * IQR).
//   - Outliers MAD: robust z-score (0.6745 * (x - median) / MAD) > 3.5.
//
// Note sur la stabilité numérique :
//   - mean/stddev via Welford, sur finite only.
//
// Dépendances :
//   - libc + math.h

#include "bench/bench.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

// ======================================================================================
// Internal helpers
// ======================================================================================

static void* bench_xmalloc(size_t n) {
  void* p = malloc(n);
  if (!p) {
    fprintf(stderr, "bench_stats: OOM (%zu bytes)\n", n);
    exit(1);
  }
  return p;
}

static void* bench_xrealloc(void* p, size_t n) {
  void* q = realloc(p, n);
  if (!q) {
    fprintf(stderr, "bench_stats: OOM (%zu bytes)\n", n);
    exit(1);
  }
  return q;
}

static int bench_cmp_double(const void* a, const void* b) {
  double da = *(const double*)a;
  double db = *(const double*)b;
  return (da < db) ? -1 : (da > db) ? 1 : 0;
}

static bool bench_isfinite_d(double x) { return isfinite(x) != 0; }
static double bench_abs_d(double x) { return (x < 0.0) ? -x : x; }

// ======================================================================================
// bench_samples_t
// ======================================================================================

void bench_samples_init(bench_samples_t* s) {
  if (!s) return;
  s->v = NULL;
  s->len = 0;
  s->cap = 0;
}

void bench_samples_free(bench_samples_t* s) {
  if (!s) return;
  free(s->v);
  s->v = NULL;
  s->len = 0;
  s->cap = 0;
}

void bench_samples_clear(bench_samples_t* s) {
  if (!s) return;
  s->len = 0;
}

void bench_samples_reserve(bench_samples_t* s, size_t n) {
  if (!s) return;
  if (n <= s->cap) return;
  size_t cap = s->cap ? s->cap : 32;
  while (cap < n) cap *= 2;
  s->v = (double*)bench_xrealloc(s->v, cap * sizeof(double));
  s->cap = cap;
}

void bench_samples_push(bench_samples_t* s, double x) {
  if (!s) return;
  if (s->len == s->cap) {
    s->cap = (s->cap == 0) ? 32 : (s->cap * 2);
    s->v = (double*)bench_xrealloc(s->v, s->cap * sizeof(double));
  }
  s->v[s->len++] = x;
}

size_t bench_samples_len(const bench_samples_t* s) { return s ? s->len : 0; }
const double* bench_samples_data(const bench_samples_t* s) { return s ? s->v : NULL; }

// Create compact array of finite values; returns malloc'd pointer + out_n (caller frees)
static double* bench_compact_finite(const bench_samples_t* s, size_t* out_n) {
  if (out_n) *out_n = 0;
  if (!s || s->len == 0) return NULL;

  size_t n = 0;
  for (size_t i = 0; i < s->len; i++) if (bench_isfinite_d(s->v[i])) n++;

  if (out_n) *out_n = n;
  if (n == 0) return NULL;

  double* v = (double*)bench_xmalloc(n * sizeof(double));
  size_t j = 0;
  for (size_t i = 0; i < s->len; i++) {
    double x = s->v[i];
    if (bench_isfinite_d(x)) v[j++] = x;
  }
  return v;
}

// ======================================================================================
// Quantiles + robust measures
// ======================================================================================

static double bench_quantile_sorted(const double* v, size_t n, double q) {
  if (!v || n == 0) return NAN;
  if (q <= 0.0) return v[0];
  if (q >= 1.0) return v[n - 1];

  double pos = q * (double)(n - 1);
  size_t i = (size_t)pos;
  double frac = pos - (double)i;
  if (i + 1 < n) return v[i] * (1.0 - frac) + v[i + 1] * frac;
  return v[i];
}

static double bench_median_sorted(const double* v, size_t n) {
  return bench_quantile_sorted(v, n, 0.50);
}

static double bench_mad_from_sorted(const double* sorted, size_t n, double median) {
  if (!sorted || n == 0 || !bench_isfinite_d(median)) return NAN;

  double* dev = (double*)bench_xmalloc(n * sizeof(double));
  for (size_t i = 0; i < n; i++) dev[i] = bench_abs_d(sorted[i] - median);
  qsort(dev, n, sizeof(double), bench_cmp_double);
  double mad = bench_median_sorted(dev, n);
  free(dev);
  return mad;
}

// ======================================================================================
// bench_stats_compute
// ======================================================================================

static bench_stats_t bench_stats_empty(void) {
  bench_stats_t st;
  memset(&st, 0, sizeof(st));

  st.min = NAN;
  st.max = NAN;
  st.mean = NAN;
  st.stddev = NAN;
  st.variance = NAN;

  st.q50 = NAN;
  st.q90 = NAN;
  st.q95 = NAN;
  st.q99 = NAN;

  st.mad = NAN;
  st.iqr = NAN;
  st.q25 = NAN;
  st.q75 = NAN;

  st.n = 0;
  st.n_finite = 0;
  st.n_outliers_iqr = 0;
  st.n_outliers_mad = 0;
  return st;
}

bench_stats_t bench_stats_compute(const bench_samples_t* s) {
  bench_stats_t st = bench_stats_empty();
  if (!s || s->len == 0) return st;

  st.n = s->len;

  // Welford mean/var on finite
  double mean = 0.0;
  double m2 = 0.0;
  double mn = 0.0;
  double mx = 0.0;
  bool first = true;

  for (size_t i = 0; i < s->len; i++) {
    double x = s->v[i];
    if (!bench_isfinite_d(x)) continue;
    st.n_finite++;

    if (first) {
      first = false;
      mean = x;
      m2 = 0.0;
      mn = x;
      mx = x;
      continue;
    }

    if (x < mn) mn = x;
    if (x > mx) mx = x;

    double delta = x - mean;
    mean += delta / (double)st.n_finite;
    double delta2 = x - mean;
    m2 += delta * delta2;
  }

  if (st.n_finite == 0) return st;

  double var = (st.n_finite > 1) ? (m2 / (double)(st.n_finite - 1)) : 0.0;
  double sd = sqrt(var);

  st.min = mn;
  st.max = mx;
  st.mean = mean;
  st.variance = var;
  st.stddev = sd;

  // Quantiles + robust on finite compacted sorted array
  size_t nf = 0;
  double* v = bench_compact_finite(s, &nf);
  if (!v || nf == 0) return st;

  qsort(v, nf, sizeof(double), bench_cmp_double);

  st.q25 = bench_quantile_sorted(v, nf, 0.25);
  st.q50 = bench_quantile_sorted(v, nf, 0.50);
  st.q75 = bench_quantile_sorted(v, nf, 0.75);
  st.iqr = st.q75 - st.q25;

  st.q90 = bench_quantile_sorted(v, nf, 0.90);
  st.q95 = bench_quantile_sorted(v, nf, 0.95);
  st.q99 = bench_quantile_sorted(v, nf, 0.99);

  st.mad = bench_mad_from_sorted(v, nf, st.q50);

  // Outliers: IQR (Tukey fences)
  if (bench_isfinite_d(st.iqr) && st.iqr > 0.0) {
    double lo = st.q25 - 1.5 * st.iqr;
    double hi = st.q75 + 1.5 * st.iqr;
    size_t out = 0;
    for (size_t i = 0; i < nf; i++) {
      double x = v[i];
      if (x < lo || x > hi) out++;
    }
    st.n_outliers_iqr = out;
  } else {
    st.n_outliers_iqr = 0;
  }

  // Outliers: MAD robust z-score
  if (bench_isfinite_d(st.mad) && st.mad > 0.0 && bench_isfinite_d(st.q50)) {
    const double thresh = 3.5;
    size_t out = 0;
    for (size_t i = 0; i < nf; i++) {
      double x = v[i];
      double rz = 0.6745 * (x - st.q50) / st.mad;
      if (bench_abs_d(rz) > thresh) out++;
    }
    st.n_outliers_mad = out;
  } else {
    st.n_outliers_mad = 0;
  }

  free(v);
  return st;
}

// ======================================================================================
// Debug print
// ======================================================================================

void bench_stats_debug_print(const bench_stats_t* st, const char* unit) {
  if (!st) return;
  printf(
    "n=%zu (finite=%zu) min=%.9f max=%.9f mean=%.9f sd=%.9f "
    "median=%.9f p90=%.9f p95=%.9f p99=%.9f "
    "mad=%.9f iqr=%.9f q25=%.9f q75=%.9f out_iqr=%zu out_mad=%zu [%s]\n",
    st->n, st->n_finite,
    st->min, st->max, st->mean, st->stddev,
    st->q50, st->q90, st->q95, st->q99,
    st->mad, st->iqr, st->q25, st->q75,
    st->n_outliers_iqr, st->n_outliers_mad,
    unit ? unit : ""
  );
}

// ======================================================================================
// Trimming
// ======================================================================================

bench_samples_t bench_samples_trimmed(const bench_samples_t* s, bench_trim_mode_t mode) {
  bench_samples_t out;
  bench_samples_init(&out);

  if (!s || s->len == 0) return out;

  if (mode == BENCH_TRIM_NONE) {
    bench_samples_reserve(&out, s->len);
    for (size_t i = 0; i < s->len; i++) bench_samples_push(&out, s->v[i]);
    return out;
  }

  bench_stats_t st = bench_stats_compute(s);
  if (st.n_finite == 0) return out;

  if (mode == BENCH_TRIM_IQR) {
    if (!bench_isfinite_d(st.iqr) || st.iqr <= 0.0) {
      // if iqr degenerate, return copy
      bench_samples_reserve(&out, s->len);
      for (size_t i = 0; i < s->len; i++) bench_samples_push(&out, s->v[i]);
      return out;
    }
    double lo = st.q25 - 1.5 * st.iqr;
    double hi = st.q75 + 1.5 * st.iqr;

    for (size_t i = 0; i < s->len; i++) {
      double x = s->v[i];
      if (!bench_isfinite_d(x)) continue;
      if (x < lo || x > hi) continue;
      bench_samples_push(&out, x);
    }
    return out;
  }

  if (mode == BENCH_TRIM_MAD) {
    if (!bench_isfinite_d(st.mad) || st.mad <= 0.0 || !bench_isfinite_d(st.q50)) {
      bench_samples_reserve(&out, s->len);
      for (size_t i = 0; i < s->len; i++) bench_samples_push(&out, s->v[i]);
      return out;
    }
    const double thresh = 3.5;

    for (size_t i = 0; i < s->len; i++) {
      double x = s->v[i];
      if (!bench_isfinite_d(x)) continue;
      double rz = 0.6745 * (x - st.q50) / st.mad;
      if (bench_abs_d(rz) > thresh) continue;
      bench_samples_push(&out, x);
    }
    return out;
  }

  // fallback: copy
  bench_samples_reserve(&out, s->len);
  for (size_t i = 0; i < s->len; i++) bench_samples_push(&out, s->v[i]);
  return out;
}

// ======================================================================================
// Conversions
// ======================================================================================

double bench_stats_ops_per_s_from_ns_per_op(double ns_per_op) {
  if (!bench_isfinite_d(ns_per_op) || ns_per_op <= 0.0) return 0.0;
  return 1e9 / ns_per_op;
}
