#include "bench/bench.h"
#include <stdint.h>

/*
  bm_recursion.c (micro)

  Goal:
  - Micro-benchmark for recursive function calls.
  - Tests call overhead, stack management, function return.
  - Measures shallow vs deep recursion costs.

  Design:
  - Fibonacci (exponential for shallow depth).
  - Factorial (linear for deeper depth).
  - Ackermann (explosive for moderate inputs).
*/

static uint64_t fib(int n) {
  if (n <= 1) return n;
  return fib(n - 1) + fib(n - 2);
}

static uint64_t factorial(int n) {
  if (n <= 1) return 1;
  return n * factorial(n - 1);
}

static uint32_t ackermann(uint32_t m, uint32_t n) {
  if (m == 0) return n + 1;
  if (n == 0) return ackermann(m - 1, 1);
  return ackermann(m - 1, ackermann(m, n - 1));
}

static int bm_recursion_fib(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint64_t result = 0;
  for (int iter = 0; iter < 100; iter++) {
    result += fib(20);  /* ~10k calls per iter */
  }
  (void)result;
  return 0;
}

static int bm_recursion_factorial(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint64_t result = 0;
  for (int iter = 0; iter < 10000; iter++) {
    result += factorial(15);
  }
  (void)result;
  return 0;
}

static int bm_recursion_ackermann(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  uint32_t result = 0;
  for (int iter = 0; iter < 1000; iter++) {
    result += ackermann(3, 3);  /* ~61k calls */
  }
  (void)result;
  return 0;
}

void bench_register_micro_recursion(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:recursion_fib", BENCH_MICRO, bm_recursion_fib, NULL);
  bench_registry_add("micro:recursion_fact", BENCH_MICRO, bm_recursion_factorial, NULL);
  bench_registry_add("micro:recursion_ack", BENCH_MICRO, bm_recursion_ackermann, NULL);
}
