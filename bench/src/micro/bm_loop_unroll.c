#include "bench/bench.h"
#include <string.h>

/*
  bm_loop_unroll.c (micro)

  Goal:
  - Micro-benchmark demonstrating loop unrolling impact.
  - Tests naive loops vs manually unrolled loops.
  - Measures instruction-level parallelism gains.

  Design:
  - Fixed iterations with varying unroll factors.
  - Compiler may or may not unroll automatically.
  - Shows ILP improvement potential.
*/

#define BENCH_ARRAY_SZ 1024

static int g_bench_data[BENCH_ARRAY_SZ];

static void init_bench_data(void) {
  static int done = 0;
  if (done) return;
  for (int i = 0; i < BENCH_ARRAY_SZ; i++) {
    g_bench_data[i] = (i * 7 + 3) % 256;
  }
  done = 1;
}

/* Naive loop: no unrolling */
static void bm_loop_naive(void* ctx) {
  (void)ctx;
  init_bench_data();
  
  int sum = 0;
  for (int iter = 0; iter < 10000; iter++) {
    for (int i = 0; i < BENCH_ARRAY_SZ; i++) {
      sum += g_bench_data[i];
    }
  }
  (void)sum;
}

/* Manual 2x unroll */
static void bm_loop_unroll2(void* ctx) {
  (void)ctx;
  init_bench_data();
  
  int sum = 0;
  for (int iter = 0; iter < 10000; iter++) {
    for (int i = 0; i < BENCH_ARRAY_SZ; i += 2) {
      sum += g_bench_data[i];
      sum += g_bench_data[i + 1];
    }
  }
  (void)sum;
}

/* Manual 4x unroll */
static void bm_loop_unroll4(void* ctx) {
  (void)ctx;
  init_bench_data();
  
  int sum = 0;
  for (int iter = 0; iter < 10000; iter++) {
    for (int i = 0; i < BENCH_ARRAY_SZ; i += 4) {
      sum += g_bench_data[i];
      sum += g_bench_data[i + 1];
      sum += g_bench_data[i + 2];
      sum += g_bench_data[i + 3];
    }
  }
  (void)sum;
}

/* Manual 8x unroll */
static void bm_loop_unroll8(void* ctx) {
  (void)ctx;
  init_bench_data();
  
  int sum = 0;
  for (int iter = 0; iter < 10000; iter++) {
    for (int i = 0; i < BENCH_ARRAY_SZ; i += 8) {
      sum += g_bench_data[i];
      sum += g_bench_data[i + 1];
      sum += g_bench_data[i + 2];
      sum += g_bench_data[i + 3];
      sum += g_bench_data[i + 4];
      sum += g_bench_data[i + 5];
      sum += g_bench_data[i + 6];
      sum += g_bench_data[i + 7];
    }
  }
  (void)sum;
}

void bench_register_micro_loop(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:loop_naive", BENCH_MICRO, bm_loop_naive, NULL);
  bench_registry_add("micro:loop_u2", BENCH_MICRO, bm_loop_unroll2, NULL);
  bench_registry_add("micro:loop_u4", BENCH_MICRO, bm_loop_unroll4, NULL);
  bench_registry_add("micro:loop_u8", BENCH_MICRO, bm_loop_unroll8, NULL);
}
