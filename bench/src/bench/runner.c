#include "bench/runner.h"
#include "bench/types.h"
#include "bench/bench.h"
#include <stdlib.h>
#include <string.h>

/*
  runner.c

  Benchmark runner orchestration.
*/

bench_result_t bench_run_case(
    const bench_case_t* case_info,
    const bench_runner_config_t* config) {
  bench_result_t result = {0};
  result.case_info = case_info;
  result.samples = config->samples;

  /* Allocate sample buffer */
  double* samples = (double*)malloc(config->samples * sizeof(double));
  if (!samples) {
    return result;
  }

  /* Warmup */
  for (uint32_t i = 0; i < config->warmup_count; i++) {
    case_info->fn(case_info->ctx);
  }

  /* Run samples */
  for (uint32_t s = 0; s < config->samples; s++) {
    uint64_t start = bench_now_ns();
    uint32_t iterations = 0;

    if (case_info->kind == BENCH_MICRO) {
      /* Micro: run fixed iterations */
      for (uint32_t i = 0; i < config->iterations; i++) {
        case_info->fn(case_info->ctx);
      }
      iterations = config->iterations;
    } else {
      /* Macro: run for target duration */
      double target_ns = config->duration_seconds * 1e9;
      while (iterations < config->timecheck_freq) {
        case_info->fn(case_info->ctx);
        iterations++;
      }

      uint64_t elapsed;
      while ((elapsed = bench_now_ns() - start) < target_ns) {
        for (uint32_t i = 0; i < config->timecheck_freq; i++) {
          case_info->fn(case_info->ctx);
        }
        iterations += config->timecheck_freq;
      }
    }

    uint64_t end = bench_now_ns();
    double elapsed_ns = (double)(end - start);
    double time_per_op = elapsed_ns / (double)iterations;
    samples[s] = time_per_op;
  }

  /* Compute statistics */
  result.stats = bench_compute_stats(samples, config->samples);
  result.iterations = config->iterations;
  result.duration_ns = result.stats.mean * config->iterations;

  free(samples);
  return result;
}
