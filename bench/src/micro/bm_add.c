#include "bench/bench.h"
#include <stdint.h>

static volatile uint64_t sink = 0;

void bm_add(void* ctx) {
  (void)ctx;
  uint64_t x = sink;
  for(int i=0;i<32;i++) x += (uint64_t)i;
  sink = x;
}
