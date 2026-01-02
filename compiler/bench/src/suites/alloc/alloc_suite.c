

/* suite: alloc

  Benchmarks for allocation patterns.

  Notes
  - `bench_alloc()` is a fast arena allocator owned by the harness; all memory is
    released between iterations via `bench_free_all()`.
  - Some cases benchmark system `malloc/free` to provide a baseline for
    allocator behavior (tcache, fragmentation, etc.).

  The intent is to provide allocation-heavy micro workloads that stress:
  - small fixed-size allocs
  - mixed-size allocs
  - alignment handling
  - pointer chasing (cache behavior)
  - malloc/free churn
*/

#include "bench/bench.h"
#include "../suites/suite_common.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* Helpers                                                                     */
/* -------------------------------------------------------------------------- */

static inline uint64_t bench_u64_rotl(uint64_t x, unsigned r) {
  return (x << (r & 63u)) | (x >> ((64u - (r & 63u)) & 63u));
}

static inline uint64_t bench_u64_mix(uint64_t x) {
  /* splitmix64-style finalizer */
  x ^= x >> 30;
  x *= 0xbf58476d1ce4e5b9ULL;
  x ^= x >> 27;
  x *= 0x94d049bb133111ebULL;
  x ^= x >> 31;
  return x;
}

static inline size_t bench_align_up(size_t x, size_t a) {
  if (a == 0) return x;
  size_t m = a - 1;
  return (x + m) & ~m;
}

static inline void *bench_alloc_aligned(bench_ctx *ctx, size_t n, size_t align) {
  /* align must be power-of-two for the cheap path */
  if (align < 2) return bench_alloc(ctx, n);

  size_t total = n + (align - 1);
  unsigned char *raw = (unsigned char *)bench_alloc(ctx, total);
  uintptr_t p = (uintptr_t)raw;
  uintptr_t a = (uintptr_t)align;
  uintptr_t aligned = (p + (a - 1)) & ~(a - 1);
  return (void *)aligned;
}

static inline void bench_touch_bytes(unsigned char *p, size_t n, uint32_t seed) {
  /* touch by cacheline */
  uint64_t s = bench_u64_mix(((uint64_t)seed << 1) | 1ULL);
  for (size_t i = 0; i < n; i += 64) {
    s = bench_u64_rotl(s, 17) ^ (uint64_t)i;
    p[i] = (unsigned char)(s & 0xFFu);
  }
  /* always touch last byte */
  if (n) p[n - 1] ^= (unsigned char)(seed & 0xFFu);
}

static inline uint64_t bench_checksum_bytes(const unsigned char *p, size_t n) {
  uint64_t acc = 0x9e3779b97f4a7c15ULL;
  for (size_t i = 0; i < n; i += 97) {
    acc ^= (uint64_t)p[i] + 0x9e3779b97f4a7c15ULL + (acc << 6) + (acc >> 2);
  }
  if (n) {
    acc ^= (uint64_t)p[n - 1] + 0x9e3779b97f4a7c15ULL + (acc << 6) + (acc >> 2);
  }
  return acc;
}

static void *xmalloc(size_t n) {
  if (n == 0) n = 1;
  void *p = malloc(n);
  if (!p) abort();
  return p;
}

/* Fisher–Yates shuffle */
static void shuffle_u32(uint32_t *a, size_t n, uint32_t seed) {
  uint32_t s = seed ? seed : 0x12345678u;
  for (size_t i = n; i > 1; --i) {
    /* xorshift */
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    size_t j = (size_t)(s % (uint32_t)i);
    uint32_t t = a[i - 1];
    a[i - 1] = a[j];
    a[j] = t;
  }
}

/* -------------------------------------------------------------------------- */
/* Arena alloc cases                                                           */
/* -------------------------------------------------------------------------- */

BENCH_CASE("alloc", "arena_bump_16b_64k", bm_alloc_arena_bump_16b_64k) {
  /* Allocate 64k small blocks of 16 bytes (~1 MiB) */
  const size_t blocks = 64u * 1024u;
  const size_t sz = 16u;

  uint64_t sink = 0;
  uint32_t seed = bench_rand_u32(ctx);

  for (size_t i = 0; i < blocks; ++i) {
    unsigned char *p = (unsigned char *)bench_alloc(ctx, sz);
    bench_touch_bytes(p, sz, seed + (uint32_t)i);
    sink ^= (uint64_t)p[0] + (uint64_t)p[sz - 1] + (uint64_t)i;
  }

  /* prevent optimization */
  volatile uint64_t v = sink;
  (void)v;
}

BENCH_CASE("alloc", "arena_bump_mixed_small", bm_alloc_arena_bump_mixed_small) {
  /* Mixed small sizes to stress size-class like behavior (even though arena). */
  static const uint16_t sizes[] = { 8, 16, 24, 32, 40, 48, 64, 96, 128, 192, 256 };
  const size_t count = 20000u;

  uint64_t acc = 0;
  uint32_t seed = bench_rand_u32(ctx);

  for (size_t i = 0; i < count; ++i) {
    uint32_t r = bench_rand_u32(ctx) ^ (seed + (uint32_t)i * 2654435761u);
    size_t sz = (size_t)sizes[r % (uint32_t)(sizeof(sizes) / sizeof(sizes[0]))];
    unsigned char *p = (unsigned char *)bench_alloc(ctx, sz);
    bench_touch_bytes(p, sz, r);
    acc ^= bench_checksum_bytes(p, sz);
  }

  volatile uint64_t v = acc;
  (void)v;
}

BENCH_CASE("alloc", "arena_bump_aligned_64", bm_alloc_arena_bump_aligned_64) {
  /* Allocate 8k blocks aligned to 64 bytes (cacheline). */
  const size_t blocks = 8192u;
  const size_t sz = 96u;
  const size_t align = 64u;

  uint64_t acc = 0;
  uint32_t seed = bench_rand_u32(ctx);

  for (size_t i = 0; i < blocks; ++i) {
    unsigned char *p = (unsigned char *)bench_alloc_aligned(ctx, sz, align);
    bench_touch_bytes(p, sz, seed + (uint32_t)i);
    acc += (uint64_t)((uintptr_t)p & (align - 1)); /* should be 0 */
    acc ^= (uint64_t)p[0] + ((uint64_t)p[sz - 1] << 8);
  }

  volatile uint64_t v = acc;
  (void)v;
}

typedef struct node {
  struct node *next;
  uint64_t payload;
} node;

BENCH_CASE("alloc", "arena_pointer_chase", bm_alloc_arena_pointer_chase) {
  /* Build a randomized linked list then chase it. */
  const size_t n = 20000u;

  node **nodes = (node **)bench_alloc(ctx, n * sizeof(node *));
  for (size_t i = 0; i < n; ++i) {
    nodes[i] = (node *)bench_alloc_aligned(ctx, sizeof(node), 16u);
    nodes[i]->next = NULL;
    nodes[i]->payload = bench_u64_mix((uint64_t)i);
  }

  /* shuffle indices */
  uint32_t *idx = (uint32_t *)bench_alloc(ctx, n * sizeof(uint32_t));
  for (size_t i = 0; i < n; ++i) idx[i] = (uint32_t)i;
  shuffle_u32(idx, n, bench_rand_u32(ctx));

  for (size_t i = 1; i < n; ++i) {
    nodes[idx[i - 1]]->next = nodes[idx[i]];
  }
  nodes[idx[n - 1]]->next = nodes[idx[0]]; /* cycle */

  node *cur = nodes[idx[0]];
  uint64_t acc = 0;
  for (size_t it = 0; it < n * 4u; ++it) {
    acc ^= cur->payload;
    cur = cur->next;
  }

  volatile uint64_t v = acc;
  (void)v;
}

BENCH_CASE("alloc", "arena_bulk_memcpy_4k", bm_alloc_arena_bulk_memcpy_4k) {
  /* Allocate many 4 KiB blocks and memcpy between them. */
  const size_t blocks = 2048u;
  const size_t sz = 4096u;

  unsigned char **arr = (unsigned char **)bench_alloc(ctx, blocks * sizeof(unsigned char *));
  uint32_t seed = bench_rand_u32(ctx);

  for (size_t i = 0; i < blocks; ++i) {
    arr[i] = (unsigned char *)bench_alloc_aligned(ctx, sz, 64u);
    bench_touch_bytes(arr[i], sz, seed + (uint32_t)i);
  }

  uint64_t acc = 0;
  for (size_t i = 1; i < blocks; ++i) {
    memcpy(arr[i], arr[i - 1], sz);
    acc ^= bench_checksum_bytes(arr[i], sz);
  }

  volatile uint64_t v = acc;
  (void)v;
}

/* -------------------------------------------------------------------------- */
/* libc malloc/free cases                                                      */
/* -------------------------------------------------------------------------- */

BENCH_CASE("alloc", "malloc_free_16b_64k", bm_alloc_malloc_free_16b_64k) {
  /* Allocate and free 64k 16-byte blocks. */
  const size_t blocks = 64u * 1024u;
  const size_t sz = 16u;

  void **ptrs = (void **)xmalloc(blocks * sizeof(void *));

  uint64_t acc = 0;
  for (size_t i = 0; i < blocks; ++i) {
    unsigned char *p = (unsigned char *)xmalloc(sz);
    p[0] = (unsigned char)i;
    p[sz - 1] = (unsigned char)(i >> 8);
    ptrs[i] = p;
    acc ^= (uint64_t)p[0] + (uint64_t)p[sz - 1];
  }

  /* free in-order */
  for (size_t i = 0; i < blocks; ++i) free(ptrs[i]);
  free(ptrs);

  volatile uint64_t v = acc;
  (void)v;
}

BENCH_CASE("alloc", "malloc_free_mixed_shuffle", bm_alloc_malloc_free_mixed_shuffle) {
  /* Allocate mixed sizes, then free in randomized order to stress allocator. */
  static const uint16_t sizes[] = { 8, 16, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024 };
  const size_t n = 20000u;

  void **ptrs = (void **)xmalloc(n * sizeof(void *));
  uint32_t *order = (uint32_t *)xmalloc(n * sizeof(uint32_t));

  uint32_t seed = 0xC001D00Du ^ (uint32_t)(uintptr_t)ptrs;
  for (size_t i = 0; i < n; ++i) {
    uint32_t r = (uint32_t)(i * 747796405u + 2891336453u) ^ seed;
    size_t sz = (size_t)sizes[r % (uint32_t)(sizeof(sizes) / sizeof(sizes[0]))];
    unsigned char *p = (unsigned char *)xmalloc(sz);
    memset(p, (int)(r & 0xFFu), sz);
    ptrs[i] = p;
    order[i] = (uint32_t)i;
  }

  shuffle_u32(order, n, seed ^ 0x9E3779B9u);

  uint64_t acc = 0;
  for (size_t k = 0; k < n; ++k) {
    uint32_t i = order[k];
    unsigned char *p = (unsigned char *)ptrs[i];
    acc ^= (uint64_t)p[0] + ((uint64_t)p[(sizes[0] ? 0 : 0)] << 1);
    free(p);
  }

  free(order);
  free(ptrs);

  volatile uint64_t v = acc;
  (void)v;
}

BENCH_CASE("alloc", "malloc_reuse_pool_4k", bm_alloc_malloc_reuse_pool_4k) {
  /* Reuse a pool of 4 KiB blocks, alternating allocate/free patterns. */
  const size_t pool = 4096u;
  const size_t sz = 4096u;

  void **ptrs = (void **)xmalloc(pool * sizeof(void *));

  /* phase 1: allocate */
  for (size_t i = 0; i < pool; ++i) {
    unsigned char *p = (unsigned char *)xmalloc(sz);
    p[0] = (unsigned char)i;
    p[sz - 1] = (unsigned char)(i >> 8);
    ptrs[i] = p;
  }

  /* phase 2: free evens */
  for (size_t i = 0; i < pool; i += 2) {
    free(ptrs[i]);
    ptrs[i] = NULL;
  }

  /* phase 3: allocate evens again */
  for (size_t i = 0; i < pool; i += 2) {
    unsigned char *p = (unsigned char *)xmalloc(sz);
    memset(p, (int)(i & 0xFFu), sz);
    ptrs[i] = p;
  }

  /* phase 4: checksum */
  uint64_t acc = 0;
  for (size_t i = 0; i < pool; ++i) {
    unsigned char *p = (unsigned char *)ptrs[i];
    acc ^= bench_checksum_bytes(p, sz);
  }

  /* phase 5: free all */
  for (size_t i = 0; i < pool; ++i) free(ptrs[i]);
  free(ptrs);

  volatile uint64_t v = acc;
  (void)v;
}

/* -------------------------------------------------------------------------- */
/* Suite init                                                                   */
/* -------------------------------------------------------------------------- */

void bench_suite_alloc_init(void) {
  /* arena */
  BENCH_REG(bm_alloc_arena_bump_16b_64k);
  BENCH_REG(bm_alloc_arena_bump_mixed_small);
  BENCH_REG(bm_alloc_arena_bump_aligned_64);
  BENCH_REG(bm_alloc_arena_pointer_chase);
  BENCH_REG(bm_alloc_arena_bulk_memcpy_4k);

  /* libc */
  BENCH_REG(bm_alloc_malloc_free_16b_64k);
  BENCH_REG(bm_alloc_malloc_free_mixed_shuffle);
  BENCH_REG(bm_alloc_malloc_reuse_pool_4k);
}