#pragma once

/*
  types.h

  Benchmark types and data structures.
*/

#ifndef VITTE_BENCH_TYPES_H
#define VITTE_BENCH_TYPES_H

#include <stdint.h>
#include <stddef.h>

/* Benchmark case kinds */
typedef enum {
  BENCH_MICRO,   /* Fast, nanosecond-scale */
  BENCH_MACRO,   /* Larger workloads, millisecond-scale */
} bench_kind_t;

/* Benchmark function signature */
typedef void (*bench_fn_t)(void* ctx);

/* Single benchmark case */
typedef struct {
  const char* id;      /* Case identifier (e.g., "micro:hash") */
  bench_kind_t kind;   /* BENCH_MICRO or BENCH_MACRO */
  bench_fn_t fn;       /* Benchmark function */
  void* ctx;           /* Optional context */
} bench_case_t;

/* Benchmark statistics */
typedef struct {
  double mean;    /* Mean of samples */
  double median;  /* Median (p50) */
  double p95;     /* 95th percentile */
  double p99;     /* 99th percentile */
  double min;     /* Minimum */
  double max;     /* Maximum */
  double stddev;  /* Standard deviation */
} bench_stats_t;

/* Benchmark result */
typedef struct {
  const bench_case_t* case_info;
  bench_stats_t stats;
  uint64_t iterations;
  uint64_t samples;
  double duration_ns;
  double throughput;  /* ops/sec or similar */
} bench_result_t;

#endif /* VITTE_BENCH_TYPES_H */
