#include "bench/bench.h"
#include <stdlib.h>
#include <math.h>

/*
  bench_stats.c (max / robust)

  Improvements vs baseline:
  - Ignores non-finite samples (NaN/Inf) for min/max/mean/quantiles.
  - Uses Kahan summation for a more stable mean.
  - Quantiles computed with linear interpolation on sorted finite samples.

  API remains:
    bench_stats bench_compute_stats(const double* samples, int n);
*/

static int cmp_dbl_nan_last(const void* a, const void* b) {
  const double da = *(const double*)a;
  const double db = *(const double*)b;

  const int a_nan = isnan(da);
  const int b_nan = isnan(db);
  if(a_nan && b_nan) return 0;
  if(a_nan) return 1;
  if(b_nan) return -1;

  return (da < db) ? -1 : (da > db) ? 1 : 0;
}

static double quantile_lerp(const double* sorted, int n, double q) {
  if(n <= 0) return 0.0;
  if(n == 1) return sorted[0];

  if(q <= 0.0) return sorted[0];
  if(q >= 1.0) return sorted[n - 1];

  const double pos = q * (double)(n - 1);
  const int idx = (int)floor(pos);
  const double frac = pos - (double)idx;

  const double a = sorted[idx];
  const double b = sorted[idx + 1];
  return a + (b - a) * frac;
}

bench_stats_t bench_compute_stats(const double* samples, int n) {
  bench_stats_t s = (bench_stats_t){0,0,0,0,0,0,0};
  if(!samples || n <= 0) return s;

  double* tmp = (double*)malloc((size_t)n * sizeof(double));
  if(!tmp) return s;

  /* Filter finite samples */
  int m = 0;
  for(int i=0;i<n;i++) {
    const double v = samples[i];
    if(isfinite(v)) tmp[m++] = v;
  }

  if(m <= 0) {
    free(tmp);
    return s;
  }

  /* Kahan summation for mean */
  double sum = 0.0;
  double c = 0.0;
  for(int i=0;i<m;i++) {
    const double y = tmp[i] - c;
    const double t = sum + y;
    c = (t - sum) - y;
    sum = t;
  }
  s.mean = sum / (double)m;

  /* Sort for min/max/quantiles */
  qsort(tmp, (size_t)m, sizeof(double), cmp_dbl_nan_last);

  s.min = tmp[0];
  s.max = tmp[m - 1];
  s.median = quantile_lerp(tmp, m, 0.50);
  s.p95 = quantile_lerp(tmp, m, 0.95);
  s.p99 = quantile_lerp(tmp, m, 0.99);

  /* Compute standard deviation */
  double sum_sq_diff = 0.0;
  for(int i = 0; i < m; i++) {
    const double diff = tmp[i] - s.mean;
    sum_sq_diff += diff * diff;
  }
  s.stddev = sqrt(sum_sq_diff / (double)m);

  free(tmp);
  return s;
}
