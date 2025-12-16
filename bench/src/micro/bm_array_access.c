#include "bench/bench.h"
#include <stdint.h>
#include <string.h>

/*
  bm_array_access.c (micro)

  Goal:
  - Micro-benchmark for array access patterns.
  - Tests sequential, stride, and random access performance.
  - Measures cache behavior and memory bandwidth.

  Design:
  - Fixed-iteration loops over array patterns.
  - Sequential access (cache-friendly).
  - Strided access (cache-unfriendly).
  - Random access patterns.
*/

#define ARRAY_SIZE 4096

static uint32_t g_array[ARRAY_SIZE];
static uint32_t g_indices[ARRAY_SIZE];

static void init_benchmark_arrays(void) {
  static int initialized = 0;
  if (initialized) return;
  
  /* Initialize data array */
  for (int i = 0; i < ARRAY_SIZE; i++) {
    g_array[i] = (uint32_t)(i * 37 + 11);
  }
  
  /* Initialize pseudo-random indices */
  uint32_t seed = 12345;
  for (int i = 0; i < ARRAY_SIZE; i++) {
    seed = seed * 1103515245 + 12345;
    g_indices[i] = (seed / 65536) % ARRAY_SIZE;
  }
  
  initialized = 1;
}

static void bm_array_sequential(void* ctx) {
  (void)ctx;
  init_benchmark_arrays();
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 1000; iter++) {
    for (int i = 0; i < ARRAY_SIZE; i++) {
      sum += g_array[i];
    }
  }
  /* Use sum to prevent optimization */
  (void)sum;
}

static void bm_array_stride4(void* ctx) {
  (void)ctx;
  init_benchmark_arrays();
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 1000; iter++) {
    for (int i = 0; i < ARRAY_SIZE; i += 4) {
      sum += g_array[i];
    }
  }
  (void)sum;
}

static void bm_array_stride16(void* ctx) {
  (void)ctx;
  init_benchmark_arrays();
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 1000; iter++) {
    for (int i = 0; i < ARRAY_SIZE; i += 16) {
      sum += g_array[i];
    }
  }
  (void)sum;
}

static void bm_array_random(void* ctx) {
  (void)ctx;
  init_benchmark_arrays();
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 100; iter++) {
    for (int i = 0; i < ARRAY_SIZE; i++) {
      sum += g_array[g_indices[i]];
    }
  }
  (void)sum;
}

void bench_register_micro_array(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:array_seq", BENCH_MICRO, bm_array_sequential, NULL);
  bench_registry_add("micro:array_s4", BENCH_MICRO, bm_array_stride4, NULL);
  bench_registry_add("micro:array_s16", BENCH_MICRO, bm_array_stride16, NULL);
  bench_registry_add("micro:array_rand", BENCH_MICRO, bm_array_random, NULL);
}
