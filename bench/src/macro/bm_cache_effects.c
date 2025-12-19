#include "bench/bench.h"
#include <string.h>
#include <stdlib.h>

/*
  bm_cache_effects.c (macro)

  Goal:
  - Macro benchmark demonstrating cache effects.
  - Tests working set size, cache line behavior.
  - Measures NUMA and memory hierarchy impacts.

  Design:
  - Vary working set sizes from L1 to L3 to RAM.
  - Linear access vs random within working set.
  - False sharing simulation.
*/

#define L1_SIZE (32 * 1024)      /* ~32KB */
#define L2_SIZE (256 * 1024)     /* ~256KB */
#define L3_SIZE (8 * 1024 * 1024) /* ~8MB */

static char g_large_buffer[L3_SIZE];

static void init_cache_buffer(void) {
  static int initialized = 0;
  if (initialized) return;
  
  for (size_t i = 0; i < sizeof(g_large_buffer); i++) {
    g_large_buffer[i] = (char)((i * 37) % 256);
  }
  initialized = 1;
}

/* Working set fits in L1 */
static int bm_cache_l1(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  init_cache_buffer();
  
  char local[L1_SIZE / 2];
  memcpy(local, g_large_buffer, sizeof(local));
  
  uint64_t sum = 0;
  for (int iter = 0; iter < 1000; iter++) {
    for (int i = 0; i < (int)sizeof(local); i++) {
      sum += local[i];
    }
  }
  (void)sum;
  return 0;
}

/* Working set fits in L2 */
static int bm_cache_l2(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  init_cache_buffer();
  
  char* buf = (char*)malloc(L2_SIZE / 4);
  if (!buf) return -1;
  memcpy(buf, g_large_buffer, L2_SIZE / 4);
  
  uint64_t sum = 0;
  for (int iter = 0; iter < 100; iter++) {
    for (int i = 0; i < (int)(L2_SIZE / 4); i++) {
      sum += buf[i];
    }
  }
  (void)sum;
  free(buf);
  return 0;
}

/* Working set fits in L3 */
static int bm_cache_l3(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  init_cache_buffer();
  
  uint64_t sum = 0;
  for (int iter = 0; iter < 10; iter++) {
    for (int i = 0; i < (int)(L3_SIZE / 2); i += 64) {
      sum += g_large_buffer[i];
    }
  }
  (void)sum;
  return 0;
}

/* Cache line stride (false sharing) */
static int bm_cache_line_stride(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  init_cache_buffer();
  
  uint64_t sum = 0;
  /* 64-byte cache line stride */
  for (int iter = 0; iter < 1000; iter++) {
    for (int i = 0; i < (int)L3_SIZE; i += 64) {
      sum += g_large_buffer[i];
      sum += g_large_buffer[i + 32];  /* Same cache line */
    }
  }
  (void)sum;
  return 0;
}

/* Random access in working set */
static int bm_cache_random_small(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  init_cache_buffer();
  
  /* Random access within L2 */
  char* buf = (char*)malloc(L2_SIZE);
  if (!buf) return -1;
  memcpy(buf, g_large_buffer, L2_SIZE);
  
  uint32_t seed = 0x12345678;
  uint64_t sum = 0;
  for (int iter = 0; iter < 100; iter++) {
    for (int access = 0; access < 10000; access++) {
      seed = seed * 1103515245 + 12345;
      int idx = (seed % L2_SIZE);
      sum += buf[idx];
    }
  }
  (void)sum;
  free(buf);
  return 0;
}

void bench_register_macro_cache(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("macro:cache_l1", BENCH_MACRO, bm_cache_l1, NULL);
  bench_registry_add("macro:cache_l2", BENCH_MACRO, bm_cache_l2, NULL);
  bench_registry_add("macro:cache_l3", BENCH_MACRO, bm_cache_l3, NULL);
  bench_registry_add("macro:cache_line", BENCH_MACRO, bm_cache_line_stride, NULL);
  bench_registry_add("macro:cache_random", BENCH_MACRO, bm_cache_random_small, NULL);
}
