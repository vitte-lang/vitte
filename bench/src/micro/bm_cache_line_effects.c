#include "bench/bench.h"
#include <stdint.h>
#include <string.h>

/*
  bm_cache_line_effects.c (micro)

  Goal:
  - Micro-benchmark for cache line and alignment effects.
  - Tests access patterns within and across cache lines.
  - Measures false sharing and prefetching.

  Design:
  - Aligned access (single cache line).
  - Unaligned access (multiple cache lines).
  - False sharing simulation.
  - Sequential vs random within cache line.
*/

#define CACHE_LINE 64

typedef struct {
  uint32_t value;
  char padding[CACHE_LINE - sizeof(uint32_t)];
} CachePaddedInt;

static void bm_cache_aligned_access(void* ctx) {
  (void)ctx;
  
  CachePaddedInt data[256];
  for (int i = 0; i < 256; i++) {
    data[i].value = i;
  }
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 10000; iter++) {
    for (int i = 0; i < 256; i++) {
      sum += data[i].value;
    }
  }
  (void)sum;
}

static void bm_cache_unaligned_access(void* ctx) {
  (void)ctx;
  
  uint32_t data[512];
  for (int i = 0; i < 512; i++) {
    data[i] = i;
  }
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 10000; iter++) {
    /* Offset by unaligned amount */
    for (int i = 1; i < 512; i += 2) {
      sum += data[i];
    }
  }
  (void)sum;
}

static void bm_cache_line_dense(void* ctx) {
  (void)ctx;
  
  uint32_t data[256];
  for (int i = 0; i < 256; i++) {
    data[i] = i;
  }
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 100000; iter++) {
    /* Within single cache line (first 16 x uint32_t) */
    for (int i = 0; i < 16; i++) {
      sum += data[i];
    }
  }
  (void)sum;
}

static void bm_cache_line_sparse(void* ctx) {
  (void)ctx;
  
  uint32_t data[256];
  for (int i = 0; i < 256; i++) {
    data[i] = i;
  }
  
  uint32_t sum = 0;
  for (int iter = 0; iter < 100000; iter++) {
    /* Across many cache lines */
    for (int i = 0; i < 256; i += 16) {
      sum += data[i];
    }
  }
  (void)sum;
}

static void bm_cache_false_sharing(void* ctx) {
  (void)ctx;
  
  /* Two uint32_t in same cache line */
  uint32_t data[2];
  data[0] = 0;
  data[1] = 0;
  
  for (int iter = 0; iter < 100000; iter++) {
    data[0]++;
    data[1]++;
    data[0] += data[1];
  }
  (void)(data[0] + data[1]);
}

void bench_register_micro_cache_line(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:cacheline_aligned", BENCH_MICRO, bm_cache_aligned_access, NULL);
  bench_registry_add("micro:cacheline_unaligned", BENCH_MICRO, bm_cache_unaligned_access, NULL);
  bench_registry_add("micro:cacheline_dense", BENCH_MICRO, bm_cache_line_dense, NULL);
  bench_registry_add("micro:cacheline_sparse", BENCH_MICRO, bm_cache_line_sparse, NULL);
  bench_registry_add("micro:cacheline_false_share", BENCH_MICRO, bm_cache_false_sharing, NULL);
}
