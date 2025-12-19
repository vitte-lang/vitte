#include "bench/bench.h"
#include <stdint.h>

/*
  bm_branch_prediction.c (micro)

  Goal:
  - Micro-benchmark for branch prediction behavior.
  - Tests predictable vs unpredictable branches.
  - Measures pipeline flush costs.

  Design:
  - Sequential if-patterns (predictable).
  - Random if-patterns (unpredictable).
  - Varying branch frequencies.
*/

static int bm_branch_predictable(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  uint32_t data[256];
  
  /* Initialize with pattern */
  for (int i = 0; i < 256; i++) {
    data[i] = i;
  }
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 256; i++) {
      if (i < 128) {  /* Highly predictable */
        sum += data[i];
      } else {
        sum -= data[i];
      }
    }
  }
  (void)sum;
  return 0;
}

static int bm_branch_unpredictable(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  uint32_t data[256];
  uint32_t pattern[256];
  
  /* Initialize with pseudo-random pattern */
  uint32_t seed = 0x12345678;
  for (int i = 0; i < 256; i++) {
    data[i] = i;
    seed = seed * 1103515245 + 12345;
    pattern[i] = seed & 1;  /* Random 0/1 */
  }
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 256; i++) {
      if (pattern[i]) {  /* Unpredictable */
        sum += data[i];
      } else {
        sum -= data[i];
      }
    }
  }
  (void)sum;
  return 0;
}

static int bm_branch_dense(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 256; i++) {
      if (i & 1) sum++;
      if (i & 2) sum++;
      if (i & 4) sum++;
      if (i & 8) sum++;
    }
  }
  (void)sum;
  return 0;
}

void bench_register_micro_branch(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:branch_predict", BENCH_MICRO, bm_branch_predictable, NULL);
  bench_registry_add("micro:branch_unpredict", BENCH_MICRO, bm_branch_unpredictable, NULL);
  bench_registry_add("micro:branch_dense", BENCH_MICRO, bm_branch_dense, NULL);
}
