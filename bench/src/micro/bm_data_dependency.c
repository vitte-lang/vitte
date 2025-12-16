#include "bench/bench.h"
#include <stdint.h>

/*
  bm_data_dependency.c (micro)

  Goal:
  - Micro-benchmark for instruction-level parallelism vs data dependencies.
  - Tests dependent chains vs independent instructions.
  - Measures ILP and latency hiding.

  Design:
  - Chain of dependent operations (low ILP).
  - Parallel independent operations (high ILP).
  - Mixed patterns.
*/

static void bm_dep_chain_int(void* ctx) {
  (void)ctx;
  
  uint32_t x = 0x12345678;
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      x = (x * 1103515245 + 12345) & 0xFFFFFFFF;
    }
  }
  (void)x;
}

static void bm_dep_chain_add(void* ctx) {
  (void)ctx;
  
  uint32_t x = 1;
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      x = (x + 1) * 2 - 1;
    }
  }
  (void)x;
}

static void bm_dep_parallel(void* ctx) {
  (void)ctx;
  
  uint32_t x1 = 1, x2 = 2, x3 = 3, x4 = 4;
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      x1 = x1 * 2 + 1;
      x2 = x2 * 2 + 1;
      x3 = x3 * 2 + 1;
      x4 = x4 * 2 + 1;
    }
  }
  (void)(x1 + x2 + x3 + x4);
}

static void bm_dep_mixed(void* ctx) {
  (void)ctx;
  
  uint32_t x = 1;
  uint32_t y1 = 1, y2 = 2, y3 = 3;
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      x = (x * 3 + 7) & 0x7FFFFFFF;  /* Dependent chain */
      y1 += i;                       /* Independent */
      y2 *= 2;
      y3 ^= 0xAAAAAAAA;
    }
  }
  (void)(x + y1 + y2 + y3);
}

static void bm_dep_load_chain(void* ctx) {
  (void)ctx;
  
  uint32_t data[256];
  for (int i = 0; i < 256; i++) {
    data[i] = i * 37 + 11;
  }
  
  uint32_t idx = 0;
  for (int iter = 0; iter < 10000; iter++) {
    for (int i = 0; i < 100; i++) {
      idx = (data[idx & 0xFF] * 37 + 11) & 0xFF;
    }
  }
  (void)idx;
}

void bench_register_micro_dependency(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:dep_chain_int", BENCH_MICRO, bm_dep_chain_int, NULL);
  bench_registry_add("micro:dep_chain_add", BENCH_MICRO, bm_dep_chain_add, NULL);
  bench_registry_add("micro:dep_parallel", BENCH_MICRO, bm_dep_parallel, NULL);
  bench_registry_add("micro:dep_mixed", BENCH_MICRO, bm_dep_mixed, NULL);
  bench_registry_add("micro:dep_load_chain", BENCH_MICRO, bm_dep_load_chain, NULL);
}
