#include "bench/bench.h"
#include <string.h>
#include <stdint.h>

static uint8_t srcbuf[4096];
static uint8_t dstbuf[4096];

void bm_memcpy(void* ctx) {
  (void)ctx;
  memcpy(dstbuf, srcbuf, sizeof(srcbuf));
}
