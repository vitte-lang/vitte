#include "bench/bench.h"
#include <stdint.h>

/*
  bm_function_call.c (micro)

  Goal:
  - Micro-benchmark for function call overhead.
  - Tests direct call, inline potential, call chains.
  - Measures call/return latency.

  Design:
  - Direct small function calls.
  - Nested function calls.
  - Tail-recursive patterns.
*/

static inline uint32_t add_inline(uint32_t a, uint32_t b) {
  return a + b;
}

static uint32_t add_noinline(uint32_t a, uint32_t b) {
  return a + b;
}

static uint32_t leaf_call(uint32_t x) {
  return x * 2 + 1;
}

static uint32_t mid_call(uint32_t x) {
  return leaf_call(x) + leaf_call(x + 1);
}

static uint32_t deep_call(uint32_t x) {
  return mid_call(x) + mid_call(x + 1);
}

static int bm_call_inline(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      sum = add_inline(sum, i);
    }
  }
  (void)sum;
  return 0;
}

static int bm_call_direct(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      sum = add_noinline(sum, i);
    }
  }
  (void)sum;
  return 0;
}

static int bm_call_leaf(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      sum = leaf_call(i);
    }
  }
  (void)sum;
  return 0;
}

static int bm_call_chain(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 10000; iter++) {
    for (int i = 0; i < 100; i++) {
      sum = mid_call(i);
    }
  }
  (void)sum;
  return 0;
}

static int bm_call_deep(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 1000; iter++) {
    for (int i = 0; i < 100; i++) {
      sum = deep_call(i);
    }
  }
  (void)sum;
  return 0;
}

void bench_register_micro_calls(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:call_inline", BENCH_MICRO, bm_call_inline, NULL);
  bench_registry_add("micro:call_direct", BENCH_MICRO, bm_call_direct, NULL);
  bench_registry_add("micro:call_leaf", BENCH_MICRO, bm_call_leaf, NULL);
  bench_registry_add("micro:call_chain", BENCH_MICRO, bm_call_chain, NULL);
  bench_registry_add("micro:call_deep", BENCH_MICRO, bm_call_deep, NULL);
}
