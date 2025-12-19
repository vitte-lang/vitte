#include "bench/bench.h"
#include <stdint.h>

/*
  bm_conditional_move.c (micro)

  Goal:
  - Micro-benchmark for conditional move (cmov) vs branch.
  - Tests branch vs branchless code patterns.
  - Measures predicated execution benefits.

  Design:
  - Traditional if-else patterns.
  - Branchless conditional move patterns.
  - Ternary operator patterns.
*/

static uint32_t conditional_branch(uint32_t a, uint32_t b, int cond) {
  if (cond) {
    return a + b;
  } else {
    return a - b;
  }
}

static uint32_t conditional_move(uint32_t a, uint32_t b, int cond) {
  /* Compiler may optimize this to cmov */
  return cond ? (a + b) : (a - b);
}

static uint32_t conditional_bitwise(uint32_t a, uint32_t b, int cond) {
  /* Branchless using bitwise operations */
  uint32_t mask = (cond ? (uint32_t)-1 : 0);
  return (a + b) ^ ((a + b) ^ (a - b)) & ~mask;
}

static int bm_cmov_branch(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  uint32_t conds[] = { 1, 0, 1, 0, 1, 0, 1, 0 };
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 8; i++) {
      sum += conditional_branch(i + 100, i + 200, conds[i]);
    }
  }
  (void)sum;
  return 0;
}

static int bm_cmov_ternary(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  uint32_t conds[] = { 1, 0, 1, 0, 1, 0, 1, 0 };
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 8; i++) {
      sum += conditional_move(i + 100, i + 200, conds[i]);
    }
  }
  (void)sum;
  return 0;
}

static int bm_cmov_bitwise(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  uint32_t conds[] = { 1, 0, 1, 0, 1, 0, 1, 0 };
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 8; i++) {
      sum += conditional_bitwise(i + 100, i + 200, conds[i]);
    }
  }
  (void)sum;
  return 0;
}

static int bm_cmov_random(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  uint32_t seed = 0xDEADBEEF;
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      seed = seed * 1103515245 + 12345;
      int cond = seed & 1;
      sum += conditional_move(i + 100, i + 200, cond);
    }
  }
  (void)sum;
  return 0;
}

void bench_register_micro_cmov(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:cmov_branch", BENCH_MICRO, bm_cmov_branch, NULL);
  bench_registry_add("micro:cmov_ternary", BENCH_MICRO, bm_cmov_ternary, NULL);
  bench_registry_add("micro:cmov_bitwise", BENCH_MICRO, bm_cmov_bitwise, NULL);
  bench_registry_add("micro:cmov_random", BENCH_MICRO, bm_cmov_random, NULL);
}
