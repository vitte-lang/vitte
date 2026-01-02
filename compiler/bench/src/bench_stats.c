\
/*
  bench_stats.c - percentile + basic stats
*/
#include "bench/bench.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int cmp_dbl(const void *a, const void *b) {
  double A = *(const double *)a;
  double B = *(const double *)b;
  return (A < B) ? -1 : (A > B) ? 1 : 0;
}

static double pct(const double *xs, size_t n, double p) {
  if (!n) return 0.0;
  if (p <= 0.0) return xs[0];
  if (p >= 1.0) return xs[n - 1];
  double idx = (double)(n - 1) * p;
  size_t i = (size_t)idx;
  size_t j = (i + 1 < n) ? i + 1 : i;
  double frac = idx - (double)i;
  return xs[i] * (1.0 - frac) + xs[j] * frac;
}

void bench_compute_result(bench_result *out,
                          const char *suite,
                          const char *name,
                          uint32_t iters,
                          const bench_sample *samples,
                          size_t sample_count) {
  memset(out, 0, sizeof(*out));
  out->suite = suite;
  out->name = name;
  out->iters = iters;

  if (!samples || sample_count == 0) return;

  double total = 0.0;
  double minv = samples[0].seconds;
  double maxv = samples[0].seconds;

  double *xs = (double *)malloc(sizeof(double) * sample_count);
  if (!xs) abort();

  for (size_t i = 0; i < sample_count; ++i) {
    double v = samples[i].seconds;
    xs[i] = v;
    total += v;
    if (v < minv) minv = v;
    if (v > maxv) maxv = v;
  }

  qsort(xs, sample_count, sizeof(double), cmp_dbl);

  double mean = total / (double)sample_count;

  double var = 0.0;
  for (size_t i = 0; i < sample_count; ++i) {
    double d = xs[i] - mean;
    var += d * d;
  }
  var /= (double)sample_count;

  out->total_s = total;
  out->mean_s = mean;
  out->min_s = minv;
  out->max_s = maxv;
  out->stddev_s = sqrt(var);

  out->p50_s = pct(xs, sample_count, 0.50);
  out->p90_s = pct(xs, sample_count, 0.90);
  out->p99_s = pct(xs, sample_count, 0.99);

  free(xs);
}
