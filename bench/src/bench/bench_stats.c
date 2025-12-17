

// bench_stats.c — statistics helpers for benchmarks (C17, max)
//
// Purpose:
//   Centralize stats computations used by the bench runner:
//     - min/max/mean/stddev
//     - percentiles (p50/p90/p95/p99)
//     - robust percentile interpolation
//     - helpers for ns/iter, ops/s
//
// Integration:
//   If you have a bench_stats.h, move the public declarations there.
//   This .c is self-contained and can be compiled as-is.
//
// Build:
//   cc -std=c17 -O2 -c bench_stats.c
//
// Optional test:
//   cc -std=c17 -O2 -DBENCH_STATS_TEST bench_stats.c -lm && ./a.out

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// ============================================================================
// Public types
// ============================================================================

typedef struct bench_stats {
  double min;
  double max;
  double mean;
  double stddev;  // sample stddev (n-1), 0 if n<2
  double p50;
  double p90;
  double p95;
  double p99;
} bench_stats;

// Optional: store raw + derived values.
typedef struct bench_sample {
  uint64_t elapsed_ns;
  uint64_t iters;
  uint64_t cycles;
  uint64_t checksum;

  // Derived
  double ns_per_iter;
  double ops_per_s;
} bench_sample;

// ============================================================================
// Public API (declare in a header if needed)
// ============================================================================

// Compute stats over an array of doubles.
// - samples: pointer to n doubles
// - n: count
// - out: computed stats
// Returns 1 on success (including n==0), 0 on error (NULL pointers).
int bench_stats_compute_f64(const double* samples, size_t n, bench_stats* out);

// Compute stats over u64 values (converted to double).
int bench_stats_compute_u64(const uint64_t* samples, size_t n, bench_stats* out);

// Build sample derived fields (ns/iter + ops/s) from elapsed_ns/iters.
void bench_sample_derive(bench_sample* s);

// Convenience: compute stats on samples[].ns_per_iter
int bench_stats_from_ns_per_iter(const bench_sample* samples, size_t n, bench_stats* out);

// Convenience: compute stats on samples[].ops_per_s
int bench_stats_from_ops_per_s(const bench_sample* samples, size_t n, bench_stats* out);

// Pretty print (single line) for human output.
void bench_stats_print(FILE* out, const char* label, const bench_stats* st);

// ============================================================================
// Internals
// ============================================================================

static int bench_cmp_f64(const void* a, const void* b) {
  const double da = *(const double*)a;
  const double db = *(const double*)b;

  // Put NaNs at end deterministically.
  const int a_nan = isnan(da) ? 1 : 0;
  const int b_nan = isnan(db) ? 1 : 0;
  if (a_nan != b_nan) return a_nan - b_nan;

  return (da < db) ? -1 : (da > db) ? 1 : 0;
}

// Percentile on sorted array, with linear interpolation.
static double bench_percentile_sorted(const double* v_sorted, size_t n, double p) {
  if (!v_sorted || n == 0) return 0.0;
  if (p <= 0.0) return v_sorted[0];
  if (p >= 1.0) return v_sorted[n - 1];

  const double x = p * (double)(n - 1);
  const size_t i = (size_t)x;
  const double f = x - (double)i;

  if (i + 1 >= n) return v_sorted[n - 1];
  return v_sorted[i] * (1.0 - f) + v_sorted[i + 1] * f;
}

static void bench_stats_zero(bench_stats* st) {
  if (!st) return;
  st->min = 0.0;
  st->max = 0.0;
  st->mean = 0.0;
  st->stddev = 0.0;
  st->p50 = 0.0;
  st->p90 = 0.0;
  st->p95 = 0.0;
  st->p99 = 0.0;
}

// Welford (stable) mean/variance.
static void bench_welford(const double* v, size_t n, double* out_mean, double* out_var_sample) {
  double mean = 0.0;
  double m2 = 0.0;
  size_t k = 0;

  for (size_t i = 0; i < n; ++i) {
    const double x = v[i];
    if (isnan(x) || isinf(x)) continue; // skip non-finite values

    ++k;
    const double delta = x - mean;
    mean += delta / (double)k;
    const double delta2 = x - mean;
    m2 += delta * delta2;
  }

  if (out_mean) *out_mean = (k ? mean : 0.0);
  if (out_var_sample) {
    if (k >= 2) *out_var_sample = m2 / (double)(k - 1);
    else *out_var_sample = 0.0;
  }
}

// ============================================================================
// Implementations
// ============================================================================

int bench_stats_compute_f64(const double* samples, size_t n, bench_stats* out) {
  if (!out) return 0;
  bench_stats_zero(out);
  if (!samples) {
    // Treat NULL samples as error only if n != 0.
    return (n == 0) ? 1 : 0;
  }
  if (n == 0) return 1;

  // Compute min/max over finite values.
  int have = 0;
  double mn = 0.0, mx = 0.0;
  for (size_t i = 0; i < n; ++i) {
    const double x = samples[i];
    if (isnan(x) || isinf(x)) continue;
    if (!have) {
      mn = mx = x;
      have = 1;
    } else {
      if (x < mn) mn = x;
      if (x > mx) mx = x;
    }
  }

  // If all values are non-finite, keep zeros.
  if (!have) return 1;

  // Mean + stddev.
  double mean = 0.0;
  double var = 0.0;
  bench_welford(samples, n, &mean, &var);

  out->min = mn;
  out->max = mx;
  out->mean = mean;
  out->stddev = sqrt(var);

  // Percentiles: need sorted finite copy.
  double* tmp = (double*)malloc(n * sizeof(double));
  if (!tmp) {
    // fallback: percentiles = mean
    out->p50 = mean;
    out->p90 = mean;
    out->p95 = mean;
    out->p99 = mean;
    return 1;
  }

  size_t m = 0;
  for (size_t i = 0; i < n; ++i) {
    const double x = samples[i];
    if (isnan(x) || isinf(x)) continue;
    tmp[m++] = x;
  }

  if (m == 0) {
    free(tmp);
    return 1;
  }

  qsort(tmp, m, sizeof(double), bench_cmp_f64);

  out->p50 = bench_percentile_sorted(tmp, m, 0.50);
  out->p90 = bench_percentile_sorted(tmp, m, 0.90);
  out->p95 = bench_percentile_sorted(tmp, m, 0.95);
  out->p99 = bench_percentile_sorted(tmp, m, 0.99);

  free(tmp);
  return 1;
}

int bench_stats_compute_u64(const uint64_t* samples, size_t n, bench_stats* out) {
  if (!out) return 0;
  bench_stats_zero(out);
  if (!samples) return (n == 0) ? 1 : 0;
  if (n == 0) return 1;

  // Convert to double buffer.
  double* tmp = (double*)malloc(n * sizeof(double));
  if (!tmp) return 0;

  for (size_t i = 0; i < n; ++i) tmp[i] = (double)samples[i];
  const int ok = bench_stats_compute_f64(tmp, n, out);
  free(tmp);
  return ok;
}

void bench_sample_derive(bench_sample* s) {
  if (!s) return;
  const double it = (double)(s->iters ? s->iters : 1);
  const double ns = (double)s->elapsed_ns;

  s->ns_per_iter = (s->iters ? (ns / it) : 0.0);
  s->ops_per_s = (s->elapsed_ns ? (1e9 * it / ns) : 0.0);
}

int bench_stats_from_ns_per_iter(const bench_sample* samples, size_t n, bench_stats* out) {
  if (!out) return 0;
  bench_stats_zero(out);
  if (!samples) return (n == 0) ? 1 : 0;
  if (n == 0) return 1;

  double* v = (double*)malloc(n * sizeof(double));
  if (!v) return 0;
  for (size_t i = 0; i < n; ++i) v[i] = samples[i].ns_per_iter;
  const int ok = bench_stats_compute_f64(v, n, out);
  free(v);
  return ok;
}

int bench_stats_from_ops_per_s(const bench_sample* samples, size_t n, bench_stats* out) {
  if (!out) return 0;
  bench_stats_zero(out);
  if (!samples) return (n == 0) ? 1 : 0;
  if (n == 0) return 1;

  double* v = (double*)malloc(n * sizeof(double));
  if (!v) return 0;
  for (size_t i = 0; i < n; ++i) v[i] = samples[i].ops_per_s;
  const int ok = bench_stats_compute_f64(v, n, out);
  free(v);
  return ok;
}

void bench_stats_print(FILE* out, const char* label, const bench_stats* st) {
  if (!out) out = stdout;
  if (!st) {
    fprintf(out, "%s: (null)\n", label ? label : "stats");
    return;
  }

  fprintf(out,
    "%s min=%.6f mean=%.6f p50=%.6f p90=%.6f p95=%.6f p99=%.6f max=%.6f sd=%.6f\n",
    label ? label : "stats",
    st->min,
    st->mean,
    st->p50,
    st->p90,
    st->p95,
    st->p99,
    st->max,
    st->stddev
  );
}

// ============================================================================
// Self-test
// ============================================================================

#if defined(BENCH_STATS_TEST)

static void test_basic(void) {
  const double v[] = { 1.0, 2.0, 3.0, 4.0, 100.0 };
  bench_stats st;
  bench_stats_compute_f64(v, sizeof(v)/sizeof(v[0]), &st);
  bench_stats_print(stdout, "basic", &st);
}

static void test_nan_inf(void) {
  const double v[] = { NAN, INFINITY, -INFINITY, 10.0, 20.0, 30.0 };
  bench_stats st;
  bench_stats_compute_f64(v, sizeof(v)/sizeof(v[0]), &st);
  bench_stats_print(stdout, "nan_inf", &st);
}

int main(void) {
  test_basic();
  test_nan_inf();
  return 0;
}

#endif