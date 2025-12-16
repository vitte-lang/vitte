#include "bench/bench.h"
#include <stdint.h>
#include <string.h>

/*
  bm_floating_point.c (micro)

  Goal:
  - Micro-benchmark for floating-point operations.
  - Tests FPU latency, throughput, and vectorization potential.
  - Measures scalar FP performance.

  Design:
  - Simple FP operations (add, mul, div).
  - FP chains (dependent operations).
  - Parallel FP (independent operations).
*/

static void bm_float_add(void* ctx) {
  (void)ctx;
  
  float sum = 0.0f;
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      sum += 0.1f;
    }
  }
  (void)sum;
}

static void bm_float_mul(void* ctx) {
  (void)ctx;
  
  float prod = 1.0f;
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 50; i++) {
      prod *= 1.001f;
    }
    if (prod > 1e10f) prod = 1.0f;  /* Prevent overflow */
  }
  (void)prod;
}

static void bm_float_div(void* ctx) {
  (void)ctx;
  
  float q = 1.0f;
  for (int iter = 0; iter < 10000; iter++) {
    for (int i = 0; i < 100; i++) {
      q = q / 1.001f;
      if (q < 1e-10f) q = 1.0f;
    }
  }
  (void)q;
}

static void bm_float_chain(void* ctx) {
  (void)ctx;
  
  float x = 1.0f;
  for (int iter = 0; iter < 100000; iter++) {
    x = x * 1.0001f + 0.0001f;
    x = x / 1.00001f - 0.00001f;
  }
  (void)x;
}

static void bm_float_parallel(void* ctx) {
  (void)ctx;
  
  float x1 = 1.0f, x2 = 2.0f, x3 = 3.0f, x4 = 4.0f;
  
  for (int iter = 0; iter < 100000; iter++) {
    x1 = x1 * 1.0001f + 0.1f;
    x2 = x2 * 1.0001f + 0.2f;
    x3 = x3 * 1.0001f + 0.3f;
    x4 = x4 * 1.0001f + 0.4f;
  }
  (void)(x1 + x2 + x3 + x4);
}

static void bm_float_array(void* ctx) {
  (void)ctx;
  
  float data[256];
  for (int i = 0; i < 256; i++) {
    data[i] = (float)i;
  }
  
  float sum = 0.0f;
  for (int iter = 0; iter < 10000; iter++) {
    for (int i = 0; i < 256; i++) {
      sum += data[i] * data[i];
    }
  }
  (void)sum;
}

void bench_register_micro_float(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:float_add", BENCH_MICRO, bm_float_add, NULL);
  bench_registry_add("micro:float_mul", BENCH_MICRO, bm_float_mul, NULL);
  bench_registry_add("micro:float_div", BENCH_MICRO, bm_float_div, NULL);
  bench_registry_add("micro:float_chain", BENCH_MICRO, bm_float_chain, NULL);
  bench_registry_add("micro:float_parallel", BENCH_MICRO, bm_float_parallel, NULL);
  bench_registry_add("micro:float_array", BENCH_MICRO, bm_float_array, NULL);
}
