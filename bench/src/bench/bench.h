#pragma once
#include <stdint.h>

typedef void (*bench_fn)(void* ctx);

typedef enum bench_kind {
  BENCH_MICRO = 0,
  BENCH_MACRO = 1,
} bench_kind;

typedef struct bench_case {
  const char* id;        /* e.g. "micro:add" */
  bench_kind kind;
  bench_fn fn;
  void* ctx;
} bench_case;

/* registry */
const bench_case* bench_registry_all(int* out_count);

/* time */
uint64_t bench_now_ns(void);

/* stats */
typedef struct bench_stats {
  double mean;
  double min;
  double max;
  double p50;
  double p95;
} bench_stats;

bench_stats bench_compute_stats(const double* samples, int n);

/* runner */
int bench_run(int argc, char** argv);
