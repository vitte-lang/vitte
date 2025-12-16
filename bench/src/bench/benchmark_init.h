#pragma once

/*
  benchmark_init.h

  Public initialization API for all benchmarks.
*/

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize and register all micro and macro benchmarks. */
void bench_init_all_benchmarks(void);

#ifdef __cplusplus
}
#endif
