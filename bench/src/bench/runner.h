#pragma once

/*
  runner.h

  Benchmark runner interface and orchestration.
*/

#ifndef VITTE_BENCH_RUNNER_H
#define VITTE_BENCH_RUNNER_H

#include "types.h"
#include <stdint.h>

/* Runner configuration */
typedef struct {
  uint32_t iterations;      /* Micro: iterations per sample */
  uint32_t samples;         /* Number of samples to collect */
  double duration_seconds;  /* Macro: duration per sample */
  uint32_t warmup_count;    /* Warmup iterations before sampling */
  uint32_t timecheck_freq;  /* Macro: check elapsed time every N iterations */
  const char* filter;       /* Filter benchmarks by substring */
  int run_all;              /* Run all registered benchmarks */
  const char* csv_output;   /* Optional CSV output file */
} bench_runner_config_t;

/* Default configuration */
#define BENCH_RUNNER_CONFIG_DEFAULT \
  { \
    .iterations = 1000000, \
    .samples = 7, \
    .duration_seconds = 2.0, \
    .warmup_count = 1000, \
    .timecheck_freq = 256, \
    .filter = NULL, \
    .run_all = 0, \
    .csv_output = NULL, \
  }

/* Run a single benchmark case */
bench_result_t bench_run_case(
    const bench_case_t* case_info,
    const bench_runner_config_t* config);

/* Run multiple benchmark cases */
void bench_run_all(
    const bench_result_t* results,
    int result_count,
    const bench_runner_config_t* config);

#endif /* VITTE_BENCH_RUNNER_H */
