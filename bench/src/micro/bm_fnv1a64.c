#include "bench/bench.h"
#include "bench/common.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
  #include <malloc.h>
#endif

#if defined(__aarch64__) && defined(__ARM_NEON)
  #include <arm_neon.h>
  #define BM_FNV_HAS_NEON 1
#else
  #define BM_FNV_HAS_NEON 0
#endif

#if defined(VITTE_BENCH_USE_ASM_HASH)
  #include "vitte/asm_fastpaths.h"
  #define BM_FNV_HAS_ASM 1
#else
  #define BM_FNV_HAS_ASM 0
#endif

static volatile uint64_t g_sink = 0;

// -----------------------------------------------------------------------------
// Portable aligned allocation (for alignment micro-benches).
// -----------------------------------------------------------------------------

static void* bm_aligned_malloc(size_t alignment, size_t size) {
#if defined(_MSC_VER)
  return _aligned_malloc(size, alignment);
#elif defined(_ISOC11_SOURCE)
  // aligned_alloc requires size to be a multiple of alignment.
  size_t rounded = (size + (alignment - 1)) & ~(alignment - 1);
  return aligned_alloc(alignment, rounded);
#else
  void* p = NULL;
  if (posix_memalign(&p, alignment, size) != 0) return NULL;
  return p;
#endif
}

static void bm_aligned_free(void* p) {
#if defined(_MSC_VER)
  _aligned_free(p);
#else
  free(p);
#endif
}

// -----------------------------------------------------------------------------
// Deterministic data
// -----------------------------------------------------------------------------

static BENCH_INLINE uint64_t splitmix64(uint64_t* x) {
  uint64_t z = (*x += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

static void bm_fill_random(uint8_t* p, size_t n, uint64_t seed) {
  uint64_t s = seed;
  size_t i = 0;
  while (i + 8 <= n) {
    uint64_t v = splitmix64(&s);
    memcpy(p + i, &v, 8);
    i += 8;
  }
  if (i < n) {
    uint64_t v = splitmix64(&s);
    memcpy(p + i, &v, n - i);
  }
}

// -----------------------------------------------------------------------------
// FNV-1a 64 implementations
// -----------------------------------------------------------------------------

enum bm_fnv_impl {
  BM_FNV_IMPL_C_BYTE = 0,
  BM_FNV_IMPL_C_UNROLL8 = 1,
  BM_FNV_IMPL_NEON_LOAD16 = 2,
  BM_FNV_IMPL_ASM = 3,
};

typedef uint64_t (*bm_fnv_fn_t)(const uint8_t* data, size_t len);

static BENCH_INLINE uint64_t bm_fnv1a64_c_byte(const uint8_t* p, size_t n) {
  uint64_t h = UINT64_C(0xcbf29ce484222325);
  for (size_t i = 0; i < n; i++) {
    h ^= (uint64_t)p[i];
    h *= UINT64_C(0x00000100000001B3);
  }
  return h;
}

static BENCH_INLINE void bm_fnv1a64_process_u64(uint64_t* h, uint64_t w) {
  // Strict byte-serial semantics: process low byte first (little-endian load).
  // Unrolled to reduce loop overhead.
  *h ^= (uint64_t)(uint8_t)(w);       *h *= UINT64_C(0x00000100000001B3); w >>= 8;
  *h ^= (uint64_t)(uint8_t)(w);       *h *= UINT64_C(0x00000100000001B3); w >>= 8;
  *h ^= (uint64_t)(uint8_t)(w);       *h *= UINT64_C(0x00000100000001B3); w >>= 8;
  *h ^= (uint64_t)(uint8_t)(w);       *h *= UINT64_C(0x00000100000001B3); w >>= 8;
  *h ^= (uint64_t)(uint8_t)(w);       *h *= UINT64_C(0x00000100000001B3); w >>= 8;
  *h ^= (uint64_t)(uint8_t)(w);       *h *= UINT64_C(0x00000100000001B3); w >>= 8;
  *h ^= (uint64_t)(uint8_t)(w);       *h *= UINT64_C(0x00000100000001B3); w >>= 8;
  *h ^= (uint64_t)(uint8_t)(w);       *h *= UINT64_C(0x00000100000001B3);
}

static BENCH_INLINE uint64_t bm_fnv1a64_c_unroll8(const uint8_t* p, size_t n) {
  uint64_t h = UINT64_C(0xcbf29ce484222325);
  while (n >= 8) {
    uint64_t w;
    memcpy(&w, p, 8);
    bm_fnv1a64_process_u64(&h, w);
    p += 8;
    n -= 8;
  }
  while (n) {
    h ^= (uint64_t)(*p++);
    h *= UINT64_C(0x00000100000001B3);
    n--;
  }
  return h;
}

#if BM_FNV_HAS_NEON
static BENCH_INLINE uint64_t bm_fnv1a64_neon_load16(const uint8_t* p, size_t n) {
  uint64_t h = UINT64_C(0xcbf29ce484222325);
  while (n >= 16) {
    uint8x16_t v = vld1q_u8(p);
    uint64x2_t d = vreinterpretq_u64_u8(v);
    uint64_t w0 = vgetq_lane_u64(d, 0);
    uint64_t w1 = vgetq_lane_u64(d, 1);
    bm_fnv1a64_process_u64(&h, w0);
    bm_fnv1a64_process_u64(&h, w1);
    p += 16;
    n -= 16;
  }
  while (n >= 8) {
    uint64_t w;
    memcpy(&w, p, 8);
    bm_fnv1a64_process_u64(&h, w);
    p += 8;
    n -= 8;
  }
  while (n) {
    h ^= (uint64_t)(*p++);
    h *= UINT64_C(0x00000100000001B3);
    n--;
  }
  return h;
}
#endif

#if BM_FNV_HAS_ASM
static BENCH_INLINE uint64_t bm_fnv1a64_asm(const uint8_t* p, size_t n) {
  return vitte_fnv1a64(p, n);
}
#endif

static BENCH_INLINE bm_fnv_fn_t bm_fnv_impl_fn(enum bm_fnv_impl impl) {
  switch (impl) {
    case BM_FNV_IMPL_C_BYTE: return bm_fnv1a64_c_byte;
    case BM_FNV_IMPL_C_UNROLL8: return bm_fnv1a64_c_unroll8;
#if BM_FNV_HAS_NEON
    case BM_FNV_IMPL_NEON_LOAD16: return bm_fnv1a64_neon_load16;
#endif
#if BM_FNV_HAS_ASM
    case BM_FNV_IMPL_ASM: return bm_fnv1a64_asm;
#endif
    default: return bm_fnv1a64_c_byte;
  }
}

static const char* bm_fnv_impl_name(enum bm_fnv_impl impl) {
  switch (impl) {
    case BM_FNV_IMPL_C_BYTE: return "c.byte";
    case BM_FNV_IMPL_C_UNROLL8: return "c.unroll8";
    case BM_FNV_IMPL_NEON_LOAD16: return "neon.load16";
    case BM_FNV_IMPL_ASM: return "asm";
    default: return "c.byte";
  }
}

// -----------------------------------------------------------------------------
// State + cache eviction
// -----------------------------------------------------------------------------

typedef struct bm_fnv_state {
  uint8_t* buf;
  size_t buf_cap;

  uint8_t* evict;
  size_t evict_cap;

  int inited;
} bm_fnv_state_t;

static bm_fnv_state_t g_state = {0};

static void bm_fnv_init_once(void) {
  if (g_state.inited) return;
  g_state.inited = 1;

  const size_t max_n = 65536;
  g_state.buf_cap = max_n + 64;
  g_state.buf = (uint8_t*)bm_aligned_malloc(64, g_state.buf_cap);

  // Large enough to evict at least L1/L2 on most systems; "cold" here is best-effort.
  g_state.evict_cap = (size_t)8u * 1024u * 1024u;
  g_state.evict = (uint8_t*)bm_aligned_malloc(64, g_state.evict_cap);

  if (!g_state.buf || !g_state.evict) {
    // If allocation fails, keep pointers NULL; benches will error out.
    return;
  }

  bm_fill_random(g_state.buf, g_state.buf_cap, UINT64_C(0x123456789abcdef0));
  bm_fill_random(g_state.evict, g_state.evict_cap, UINT64_C(0x0fedcba987654321));
}

static void bm_fnv_shutdown(void) {
  bm_aligned_free(g_state.buf);
  bm_aligned_free(g_state.evict);
  g_state.buf = NULL;
  g_state.evict = NULL;
  g_state.buf_cap = 0;
  g_state.evict_cap = 0;
  g_state.inited = 0;
}

static BENCH_NOINLINE void bm_evict_cache(void) {
  // Stride by cache line to maximize eviction efficiency.
  const size_t step = (size_t)BENCH_CACHELINE_SIZE;
  uint64_t acc = (uint64_t)g_sink;
  for (size_t i = 0; i < g_state.evict_cap; i += step) {
    acc += g_state.evict[i];
  }
  g_sink = acc;
  DO_NOT_OPTIMIZE(acc);
}

// -----------------------------------------------------------------------------
// Bench entry
// -----------------------------------------------------------------------------

typedef struct bm_fnv_case {
  enum bm_fnv_impl impl;
  size_t size;
  uint32_t align_off; // 0..63
  uint32_t cold;      // 0/1
} bm_fnv_case_t;

static int bm_fnv_entry(void* user_ctx, int64_t iters) {
  if (iters <= 0) return 0;
  bm_fnv_init_once();
  static int shutdown_registered = 0;
  if (!shutdown_registered) {
    if (atexit(bm_fnv_shutdown) == 0) shutdown_registered = 1;
  }

  if (!g_state.buf || !g_state.evict) return -1;

  const bm_fnv_case_t* c = (const bm_fnv_case_t*)user_ctx;
  if (!c) return -1;
  if (c->size + c->align_off > g_state.buf_cap) return -1;

  const uint8_t* p = g_state.buf + c->align_off;
  const bm_fnv_fn_t fn = bm_fnv_impl_fn(c->impl);

  uint64_t acc = (uint64_t)g_sink;
  for (int64_t i = 0; i < iters; i++) {
    if (c->cold) bm_evict_cache();
    uint64_t h = fn(p, c->size);
    acc ^= h + (uint64_t)(i + 1) * UINT64_C(0x9E3779B97F4A7C15);
  }
  g_sink = acc;
  DO_NOT_OPTIMIZE(acc);
  return 0;
}

// -----------------------------------------------------------------------------
// Registration
// -----------------------------------------------------------------------------

static bm_fnv_case_t* bm_fnv_new_case(enum bm_fnv_impl impl, size_t size, uint32_t align_off, uint32_t cold) {
  bm_fnv_case_t* c = (bm_fnv_case_t*)calloc(1, sizeof(*c));
  if (!c) return NULL;
  c->impl = impl;
  c->size = size;
  c->align_off = align_off;
  c->cold = cold;
  return c;
}

static void bm_fnv_register_impl(enum bm_fnv_impl impl) {
#if !BM_FNV_HAS_NEON
  if (impl == BM_FNV_IMPL_NEON_LOAD16) return;
#endif
#if !BM_FNV_HAS_ASM
  if (impl == BM_FNV_IMPL_ASM) return;
#endif

  static const size_t sizes[] = {
    16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536
  };
  static const uint32_t aligns[] = { 0, 1 }; // aligned vs worst-common misalignment

  for (size_t si = 0; si < (sizeof(sizes) / sizeof(sizes[0])); si++) {
    for (size_t ai = 0; ai < (sizeof(aligns) / sizeof(aligns[0])); ai++) {
      for (uint32_t cold = 0; cold <= 1; cold++) {
        char id[128];
        const char* hotcold = cold ? "cold" : "hot";
        const char* in = bm_fnv_impl_name(impl);
        (void)snprintf(id, sizeof(id), "micro:fnv1a64.%s.%s.%zub.align%u",
                       in, hotcold, sizes[si], (unsigned)aligns[ai]);

        bm_fnv_case_t* c = bm_fnv_new_case(impl, sizes[si], aligns[ai], cold);
        if (!c) continue;
        bench_registry_add(id, BENCH_MICRO, bm_fnv_entry, c);
      }
    }
  }
}

void bench_register_micro_fnv1a64(void) {
  bm_fnv_register_impl(BM_FNV_IMPL_C_BYTE);
  bm_fnv_register_impl(BM_FNV_IMPL_C_UNROLL8);
  bm_fnv_register_impl(BM_FNV_IMPL_NEON_LOAD16);
  bm_fnv_register_impl(BM_FNV_IMPL_ASM);
}
