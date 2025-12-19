#include "bench/bench.h"
#include <stdint.h>
#include <string.h>

/*
  bm_bitops.c (micro)

  Goal:
  - Micro-benchmark for bitwise operations.
  - Tests AND, OR, XOR, shifts, bit counting.
  - Measures instruction-level parallelism and CPU latency.

  Design:
  - Simple bitwise loops on arrays.
  - Bit manipulation patterns.
  - Population count (Hamming weight).
*/

static uint32_t popcount_naive(uint32_t x) {
  uint32_t count = 0;
  while (x) {
    count += x & 1;
    x >>= 1;
  }
  return count;
}

static uint32_t popcount_brian_kernighan(uint32_t x) {
  uint32_t count = 0;
  while (x) {
    x &= x - 1;
    count++;
  }
  return count;
}

static int bm_bitops_and_or(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t a = 0xAAAAAAAA;
  uint32_t b = 0x55555555;
  uint32_t result = 0;
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 1000; i++) {
      result |= (a & b);
      result &= (a | b);
      a ^= b;
    }
  }
  (void)result;
  return 0;
}

static int bm_bitops_shift(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t x = 0x12345678;
  uint32_t result = 0;
  
  for (int iter = 0; iter < 100000; iter++) {
    for (int i = 0; i < 100; i++) {
      result += (x << 1) | (x >> 31);
      x = (x >> 4) | ((x & 0xF) << 28);
    }
  }
  (void)result;
  return 0;
}

static int bm_bitops_popcount_naive(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t values[] = {
    0xAAAAAAAA, 0x55555555, 0x11111111, 0xFFFFFFFF,
    0x12345678, 0x87654321, 0xDEADBEEF, 0xCAFEBABE,
  };
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 10000; iter++) {
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
      sum += popcount_naive(values[i]);
    }
  }
  (void)sum;
  return 0;
}

static int bm_bitops_popcount_bk(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t values[] = {
    0xAAAAAAAA, 0x55555555, 0x11111111, 0xFFFFFFFF,
    0x12345678, 0x87654321, 0xDEADBEEF, 0xCAFEBABE,
  };
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 10000; iter++) {
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
      sum += popcount_brian_kernighan(values[i]);
    }
  }
  (void)sum;
  return 0;
}

void bench_register_micro_bitops(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:bitops_and_or", BENCH_MICRO, bm_bitops_and_or, NULL);
  bench_registry_add("micro:bitops_shift", BENCH_MICRO, bm_bitops_shift, NULL);
  bench_registry_add("micro:popcount_naive", BENCH_MICRO, bm_bitops_popcount_naive, NULL);
  bench_registry_add("micro:popcount_bk", BENCH_MICRO, bm_bitops_popcount_bk, NULL);
}
