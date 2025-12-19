#include "bench/bench.h"
#include <stdint.h>

/*
  bm_division.c (micro)

  Goal:
  - Micro-benchmark for expensive integer operations.
  - Tests division, modulo, and their combinations.
  - Measures latency of multi-cycle operations.

  Design:
  - Simple division by constant.
  - Division by variable.
  - Modulo operations.
  - Division chains.
*/

static int bm_div_constant(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  uint32_t x = 0x12345678;
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      sum += x / 10;
    }
  }
  (void)sum;
  return 0;
}

static int bm_div_variable(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  uint32_t divisors[] = { 3, 5, 7, 11, 13, 17, 19, 23 };
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      uint32_t x = 0x12345678 + i;
      sum += x / divisors[i & 7];
    }
  }
  (void)sum;
  return 0;
}

static int bm_modulo(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  uint32_t x = 0x12345678;
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      sum += x % 97;
    }
  }
  (void)sum;
  return 0;
}

static int bm_divmod(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  uint32_t x = 0x12345678;
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      uint32_t q = x / 97;
      uint32_t r = x % 97;
      sum += q + r;
    }
  }
  (void)sum;
  return 0;
}

static int bm_div_chain(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t x = 0xFFFFFFFF;
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 50; i++) {
      x = (x / 2) + (x % 2);
    }
  }
  (void)x;
  return 0;
}

void bench_register_micro_division(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:div_const", BENCH_MICRO, bm_div_constant, NULL);
  bench_registry_add("micro:div_var", BENCH_MICRO, bm_div_variable, NULL);
  bench_registry_add("micro:modulo", BENCH_MICRO, bm_modulo, NULL);
  bench_registry_add("micro:divmod", BENCH_MICRO, bm_divmod, NULL);
  bench_registry_add("micro:div_chain", BENCH_MICRO, bm_div_chain, NULL);
}
