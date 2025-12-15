#include "bench/bench.h"
#include <stdint.h>
#include <string.h>

static uint64_t fnv1a(const uint8_t* p, size_t n) {
  uint64_t h = 1469598103934665603ull;
  for(size_t i=0;i<n;i++) { h ^= p[i]; h *= 1099511628211ull; }
  return h;
}

static volatile uint64_t sink = 0;

void bm_hash(void* ctx) {
  (void)ctx;
  const char* s = "the quick brown fox jumps over the lazy dog";
  sink ^= fnv1a((const uint8_t*)s, strlen(s));
}
