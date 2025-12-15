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

#if defined(__GNUC__) || defined(__clang__)
  /* 32 real add instructions, in-register, single asm block (low overhead). */
  __asm__ volatile(
    "add $1,  %0\n\t" "add $2,  %0\n\t" "add $3,  %0\n\t" "add $4,  %0\n\t"
    "add $5,  %0\n\t" "add $6,  %0\n\t" "add $7,  %0\n\t" "add $8,  %0\n\t"

    "add $9,  %0\n\t" "add $10, %0\n\t" "add $11, %0\n\t" "add $12, %0\n\t"
    "add $13, %0\n\t" "add $14, %0\n\t" "add $15, %0\n\t" "add $16, %0\n\t"

    "add $17, %0\n\t" "add $18, %0\n\t" "add $19, %0\n\t" "add $20, %0\n\t"
    "add $21, %0\n\t" "add $22, %0\n\t" "add $23, %0\n\t" "add $24, %0\n\t"

    "add $25, %0\n\t" "add $26, %0\n\t" "add $27, %0\n\t" "add $28, %0\n\t"
    "add $29, %0\n\t" "add $30, %0\n\t" "add $31, %0\n\t" "add $32, %0\n\t"
    : "+r"(x)
    :
    : "cc");
#else
  /* Portable fallback: keep a data dependency (x feeds the increment). */
  for(int i=0;i<32;i++) {
    /* (x & 7) makes the increment depend on x => prevents folding into one add. */
    x += (uint64_t)(i + 1) + (x & 7u);
  }
#endif

  sink = x;
}
