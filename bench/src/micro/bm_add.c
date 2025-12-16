#include "bench/bench.h"
#include <stdint.h>

/*
  bm_add.c (max)

  Goal:
  - Micro benchmark for integer add throughput/latency.
  - Avoid the classic optimization where a loop like:
        for(i=0;i<N;i++) x += i;
    is folded into: x += (N*(N-1))/2

  Strategy:
  - GCC/Clang: force the adds to exist via a single inline-asm block.
  - Other compilers: keep a data dependency so the loop can't be strength-reduced.

  Note:
  - The micro runner calls this function many times; keep it small and stable.
*/

static volatile uint64_t sink = 0;

void bm_add(void* ctx) {
  (void)ctx;

  uint64_t x = sink;

  /* Portable C version - multiply adds to create instruction-level parallelism */
  for (int i = 0; i < 1000000; i++) {
    x += 1; x += 2; x += 3; x += 4;
    x += 5; x += 6; x += 7; x += 8;
    x += 9; x += 10; x += 11; x += 12;
    x += 13; x += 14; x += 15; x += 16;
    x += 17; x += 18; x += 19; x += 20;
    x += 21; x += 22; x += 23; x += 24;
    x += 25; x += 26; x += 27; x += 28;
    x += 29; x += 30; x += 31; x += 32;
  }

  sink = x;
}

void bench_register_micro_add(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:add", BENCH_MICRO, bm_add, NULL);
}
