#include "bench/bench.h"
#include <stdlib.h>

static int cmp_dbl(const void* a, const void* b) {
  const double da = *(const double*)a;
  const double db = *(const double*)b;
  return (da < db) ? -1 : (da > db);
}

bench_stats bench_compute_stats(const double* samples, int n) {
  bench_stats s = {0,0,0,0,0};
  if(n <= 0) return s;

  double* tmp = (double*)malloc((size_t)n * sizeof(double));
  if(!tmp) return s;

  s.min = samples[0];
  s.max = samples[0];

  double sum = 0.0;
  for(int i=0;i<n;i++) {
    tmp[i] = samples[i];
    if(samples[i] < s.min) s.min = samples[i];
    if(samples[i] > s.max) s.max = samples[i];
    sum += samples[i];
  }
  s.mean = sum / (double)n;

  qsort(tmp, (size_t)n, sizeof(double), cmp_dbl);

  int i50 = (int)((double)(n - 1) * 0.50);
  int i95 = (int)((double)(n - 1) * 0.95);
  s.p50 = tmp[i50];
  s.p95 = tmp[i95];

  free(tmp);
  return s;
}
