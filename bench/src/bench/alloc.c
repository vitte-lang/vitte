

// alloc.c — allocation benchmarks (C17)
//
// Goal: provide robust allocation micro-benchmarks in pure C17, usable both:
//   1) standalone (compile this file with -DVITTE_BENCH_STANDALONE)
//   2) embedded into your bench runner (call vitte_bm_alloc_list_cases())
//
// Notes:
// - This file avoids any dependency on the rest of the project by default.
// - Optional integration hooks are exposed as simple data (name + function pointer).
// - The benchmark loops return a checksum to prevent dead-code elimination.
//
// Build example (standalone):
//   cc -std=c17 -O3 -DVITTE_BENCH_STANDALONE alloc.c -o bm_alloc
//   ./bm_alloc --case malloc_free --iters 500000 --size 64
//
// Embedded usage:
//   size_t n = 0; const vitte_bm_case* cases = vitte_bm_alloc_list_cases(&n);
//   runner_register_many("alloc", cases, n);

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <time.h>
  #include <unistd.h>
  #if defined(__APPLE__)
    #include <mach/mach_time.h>
  #endif
#endif

// =====================================================================================
// Small utilities
// =====================================================================================

static inline uint64_t vitte_rotl64(uint64_t x, unsigned r) {
  return (x << (r & 63u)) | (x >> ((64u - r) & 63u));
}

// xorshift64* — tiny PRNG for stable benchmarks.
static inline uint64_t vitte_rng_next(uint64_t* s) {
  uint64_t x = *s;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  *s = x;
  return x * UINT64_C(2685821657736338717);
}

static inline uint64_t vitte_hash_mix_u64(uint64_t x) {
  x ^= x >> 33;
  x *= UINT64_C(0xff51afd7ed558ccd);
  x ^= x >> 33;
  x *= UINT64_C(0xc4ceb9fe1a85ec53);
  x ^= x >> 33;
  return x;
}

static inline void vitte_touch_memory(uint8_t* p, size_t n, uint64_t* checksum) {
  // Touch a few cache lines deterministically to model “real” alloc usage.
  // For very small blocks, this degenerates to a few bytes.
  if (!p || n == 0) return;

  const size_t step = 64;
  const size_t last = n - 1;

  p[0] ^= (uint8_t)(*checksum);
  p[last] ^= (uint8_t)(*checksum >> 8);

  for (size_t i = step; i < n; i += step) {
    p[i] ^= (uint8_t)(i);
  }

  // Fold some bytes back into checksum.
  uint64_t acc = 0;
  acc ^= p[0];
  acc ^= (uint64_t)p[last] << 8;
  if (n > 8) {
    acc ^= (uint64_t)p[step < n ? step : 0] << 16;
  }
  *checksum = vitte_hash_mix_u64(*checksum ^ acc);
}

// =====================================================================================
// Timing (ns)
// =====================================================================================

static uint64_t vitte_time_now_ns(void) {
#if defined(_WIN32)
  static LARGE_INTEGER freq;
  static int init = 0;
  if (!init) {
    QueryPerformanceFrequency(&freq);
    init = 1;
  }
  LARGE_INTEGER c;
  QueryPerformanceCounter(&c);
  // Convert to ns carefully (avoid overflow by splitting).
  const uint64_t ticks = (uint64_t)c.QuadPart;
  const uint64_t f = (uint64_t)freq.QuadPart;
  const uint64_t sec = ticks / f;
  const uint64_t rem = ticks - sec * f;
  return sec * UINT64_C(1000000000) + (rem * UINT64_C(1000000000) / f);
#elif defined(__APPLE__)
  static mach_timebase_info_data_t tb;
  static int init = 0;
  if (!init) {
    (void)mach_timebase_info(&tb);
    init = 1;
  }
  const uint64_t t = mach_absolute_time();
  // t * numer/denom => ns
  return (t * (uint64_t)tb.numer) / (uint64_t)tb.denom;
#else
  struct timespec ts;
  // CLOCK_MONOTONIC is standard enough on POSIX.
  (void)clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
#endif
}

// =====================================================================================
// Aligned allocation helpers (C17)
// =====================================================================================

static void* vitte_aligned_alloc_wrap(size_t alignment, size_t size) {
  // alignment must be power-of-two and multiple of sizeof(void*) for aligned_alloc.
  if (alignment < sizeof(void*)) alignment = sizeof(void*);

  // Round size up to multiple of alignment for aligned_alloc.
  size_t rounded = size;
  if (rounded == 0) rounded = alignment;
  const size_t rem = rounded % alignment;
  if (rem) {
    const size_t add = alignment - rem;
    if (rounded > SIZE_MAX - add) return NULL;
    rounded += add;
  }

#if defined(_WIN32)
  return _aligned_malloc(rounded, alignment);
#else
  // Prefer C11 aligned_alloc when available.
  // Many compilers expose it in C17 mode.
  void* p = NULL;
  #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    p = aligned_alloc(alignment, rounded);
    if (p) return p;
  #endif

  // POSIX fallback.
  if (posix_memalign(&p, alignment, rounded) != 0) return NULL;
  return p;
#endif
}

static void vitte_aligned_free_wrap(void* p) {
#if defined(_WIN32)
  _aligned_free(p);
#else
  free(p);
#endif
}

// =====================================================================================
// Benchmark case interface (project-agnostic)
// =====================================================================================

typedef struct vitte_bm_result {
  uint64_t elapsed_ns;
  uint64_t checksum;
  uint64_t iters;
  size_t   size;
} vitte_bm_result;

typedef vitte_bm_result (*vitte_bm_fn)(uint64_t iters, size_t size, uint64_t seed);

typedef struct vitte_bm_case {
  const char* name;
  vitte_bm_fn fn;
} vitte_bm_case;

// =====================================================================================
// Allocation benchmark implementations
// =====================================================================================

static vitte_bm_result bm_malloc_free(uint64_t iters, size_t size, uint64_t seed) {
  uint64_t checksum = vitte_hash_mix_u64(seed ^ UINT64_C(0xA110C0DE)); // "ALLOC0DE"
  const uint64_t t0 = vitte_time_now_ns();

  for (uint64_t i = 0; i < iters; ++i) {
    void* p = malloc(size ? size : 1);
    if (!p) {
      // If the allocator fails, fold the failure into checksum and stop.
      checksum ^= UINT64_C(0xDEAD) ^ i;
      break;
    }
    vitte_touch_memory((uint8_t*)p, size, &checksum);
    checksum ^= vitte_hash_mix_u64((uintptr_t)p);
    free(p);
  }

  const uint64_t t1 = vitte_time_now_ns();
  vitte_bm_result r;
  r.elapsed_ns = (t1 >= t0) ? (t1 - t0) : 0;
  r.checksum = checksum;
  r.iters = iters;
  r.size = size;
  return r;
}

static vitte_bm_result bm_calloc_free(uint64_t iters, size_t size, uint64_t seed) {
  uint64_t checksum = vitte_hash_mix_u64(seed ^ UINT64_C(0xCA110C)); // "CALL0C"
  const uint64_t t0 = vitte_time_now_ns();

  for (uint64_t i = 0; i < iters; ++i) {
    // calloc(1, size) ensures zeroing cost is included.
    void* p = calloc(1, size ? size : 1);
    if (!p) {
      checksum ^= UINT64_C(0xBADC0DE) ^ i;
      break;
    }

    // Verify at least a couple of bytes are zero (cheap), fold into checksum.
    uint8_t* b = (uint8_t*)p;
    if (size) {
      checksum ^= (uint64_t)b[0];
      checksum ^= (uint64_t)b[size - 1] << 8;
    }

    vitte_touch_memory((uint8_t*)p, size, &checksum);
    checksum ^= vitte_hash_mix_u64((uintptr_t)p);
    free(p);
  }

  const uint64_t t1 = vitte_time_now_ns();
  vitte_bm_result r;
  r.elapsed_ns = (t1 >= t0) ? (t1 - t0) : 0;
  r.checksum = checksum;
  r.iters = iters;
  r.size = size;
  return r;
}

static vitte_bm_result bm_realloc_grow_shrink(uint64_t iters, size_t size, uint64_t seed) {
  // Pattern: allocate small, grow to size, shrink, free.
  // This captures realloc metadata behavior.
  uint64_t checksum = vitte_hash_mix_u64(seed ^ UINT64_C(0xDEA110C)); // "DEA110C"
  const size_t small = (size >= 16) ? 16 : (size ? size : 1);
  const uint64_t t0 = vitte_time_now_ns();

  for (uint64_t i = 0; i < iters; ++i) {
    uint8_t* p = (uint8_t*)malloc(small);
    if (!p) { checksum ^= UINT64_C(0xF00D) ^ i; break; }

    vitte_touch_memory(p, small, &checksum);

    uint8_t* q = (uint8_t*)realloc(p, size ? size : 1);
    if (!q) {
      // realloc failure leaves original p valid, but we don't know; free p.
      free(p);
      checksum ^= UINT64_C(0x0BADF00D) ^ i;
      break;
    }

    vitte_touch_memory(q, size, &checksum);

    // Shrink back.
    uint8_t* r2 = (uint8_t*)realloc(q, small);
    if (!r2) {
      // If shrink fails, q is still valid.
      free(q);
      checksum ^= UINT64_C(0x5A71B9) ^ i; // "SHRINK"
      break;
    }

    vitte_touch_memory(r2, small, &checksum);
    checksum ^= vitte_hash_mix_u64((uintptr_t)r2);
    free(r2);
  }

  const uint64_t t1 = vitte_time_now_ns();
  vitte_bm_result r;
  r.elapsed_ns = (t1 >= t0) ? (t1 - t0) : 0;
  r.checksum = checksum;
  r.iters = iters;
  r.size = size;
  return r;
}

static vitte_bm_result bm_aligned_alloc_free(uint64_t iters, size_t size, uint64_t seed) {
  // alignment is varied a bit to stress different allocator paths.
  uint64_t checksum = vitte_hash_mix_u64(seed ^ UINT64_C(0xA11E09)); // "ALIGN"
  const uint64_t t0 = vitte_time_now_ns();

  for (uint64_t i = 0; i < iters; ++i) {
    const uint64_t r = vitte_rng_next(&checksum);
    const size_t alignment = (size_t)(1u << (4u + (unsigned)(r & 4u))); // 16..256

    void* p = vitte_aligned_alloc_wrap(alignment, size ? size : 1);
    if (!p) {
      checksum ^= UINT64_C(0xA11FA11) ^ i; // "ALLFAIL"
      break;
    }

    vitte_touch_memory((uint8_t*)p, size, &checksum);
    checksum ^= vitte_hash_mix_u64((uintptr_t)p ^ (uint64_t)alignment);
    vitte_aligned_free_wrap(p);
  }

  const uint64_t t1 = vitte_time_now_ns();
  vitte_bm_result r;
  r.elapsed_ns = (t1 >= t0) ? (t1 - t0) : 0;
  r.checksum = checksum;
  r.iters = iters;
  r.size = size;
  return r;
}

// --- A simple arena allocator benchmark ---

typedef struct vitte_arena {
  uint8_t* base;
  size_t cap;
  size_t off;
} vitte_arena;

static int vitte_arena_init(vitte_arena* a, size_t cap) {
  if (!a) return 0;
  a->base = (uint8_t*)malloc(cap ? cap : 1);
  if (!a->base) return 0;
  a->cap = cap;
  a->off = 0;
  return 1;
}

static void vitte_arena_reset(vitte_arena* a) {
  if (!a) return;
  a->off = 0;
}

static void vitte_arena_free(vitte_arena* a) {
  if (!a) return;
  free(a->base);
  a->base = NULL;
  a->cap = 0;
  a->off = 0;
}

static void* vitte_arena_alloc(vitte_arena* a, size_t n, size_t align) {
  if (!a || !a->base) return NULL;
  if (align < sizeof(void*)) align = sizeof(void*);
  // align must be power-of-two.
  const size_t mask = align - 1;
  if ((align & mask) != 0) return NULL;

  size_t off = a->off;
  const size_t aligned = (off + mask) & ~mask;
  if (aligned > a->cap) return NULL;
  if (n > a->cap - aligned) return NULL;

  a->off = aligned + n;
  return a->base + aligned;
}

static vitte_bm_result bm_arena_bump(uint64_t iters, size_t size, uint64_t seed) {
  // Allocate iters blocks of `size` from a pre-allocated arena, reset periodically.
  // This models a fast allocator path (useful baseline vs malloc).
  uint64_t checksum = vitte_hash_mix_u64(seed ^ UINT64_C(0xA2E4A));

  // Cap is sized so we reset multiple times.
  const size_t align = 16;
  const size_t cap = (size ? size : 1) * 1024u + 4096u;
  vitte_arena arena;
  if (!vitte_arena_init(&arena, cap)) {
    vitte_bm_result r = {0, checksum ^ UINT64_C(0xAF5A11), iters, size}; // "ARFAIL"
    return r;
  }

  const uint64_t t0 = vitte_time_now_ns();

  for (uint64_t i = 0; i < iters; ++i) {
    void* p = vitte_arena_alloc(&arena, size ? size : 1, align);
    if (!p) {
      // reset and retry
      vitte_arena_reset(&arena);
      p = vitte_arena_alloc(&arena, size ? size : 1, align);
      if (!p) {
        checksum ^= UINT64_C(0xAF1AF11) ^ i; // "ARNAFAIL"
        break;
      }
    }

    vitte_touch_memory((uint8_t*)p, size, &checksum);
    checksum ^= vitte_hash_mix_u64((uintptr_t)p);
  }

  const uint64_t t1 = vitte_time_now_ns();
  vitte_arena_free(&arena);

  vitte_bm_result r;
  r.elapsed_ns = (t1 >= t0) ? (t1 - t0) : 0;
  r.checksum = checksum;
  r.iters = iters;
  r.size = size;
  return r;
}

// --- A simple fixed-size pool benchmark (malloc once, reuse) ---

typedef struct vitte_pool {
  uint8_t* mem;
  uint32_t* next;
  uint32_t head;
  uint32_t count;
  uint32_t stride;
} vitte_pool;

static int vitte_pool_init(vitte_pool* p, uint32_t count, uint32_t stride) {
  if (!p || count == 0 || stride == 0) return 0;
  p->mem = (uint8_t*)malloc((size_t)count * (size_t)stride);
  p->next = (uint32_t*)malloc((size_t)count * sizeof(uint32_t));
  if (!p->mem || !p->next) {
    free(p->mem);
    free(p->next);
    memset(p, 0, sizeof(*p));
    return 0;
  }
  p->count = count;
  p->stride = stride;
  for (uint32_t i = 0; i + 1u < count; ++i) p->next[i] = i + 1u;
  p->next[count - 1u] = UINT32_MAX;
  p->head = 0;
  return 1;
}

static void vitte_pool_free(vitte_pool* p) {
  if (!p) return;
  free(p->mem);
  free(p->next);
  memset(p, 0, sizeof(*p));
}

static void* vitte_pool_alloc(vitte_pool* p) {
  if (!p || p->head == UINT32_MAX) return NULL;
  const uint32_t idx = p->head;
  p->head = p->next[idx];
  return p->mem + (size_t)idx * (size_t)p->stride;
}

static void vitte_pool_free_one(vitte_pool* p, void* ptr) {
  if (!p || !ptr) return;
  const uintptr_t base = (uintptr_t)p->mem;
  const uintptr_t x = (uintptr_t)ptr;
  if (x < base) return;
  const uintptr_t off = x - base;
  const uint32_t idx = (uint32_t)(off / p->stride);
  if (idx >= p->count) return;
  p->next[idx] = p->head;
  p->head = idx;
}

static vitte_bm_result bm_pool_reuse(uint64_t iters, size_t size, uint64_t seed) {
  // Allocate/free from a fixed-size pool: models allocator fast-path w/ reuse.
  uint64_t checksum = vitte_hash_mix_u64(seed ^ UINT64_C(0x5001)); // "POOL"
  const uint32_t stride = (uint32_t)((size < 8 ? 8 : size) + 8);
  vitte_pool pool;
  if (!vitte_pool_init(&pool, 4096u, stride)) {
    vitte_bm_result r = {0, checksum ^ UINT64_C(0x5001FA11), iters, size}; // "POOLFAIL"
    return r;
  }

  const uint64_t t0 = vitte_time_now_ns();

  for (uint64_t i = 0; i < iters; ++i) {
    void* p = vitte_pool_alloc(&pool);
    if (!p) {
      checksum ^= UINT64_C(0x5001E9F7) ^ i; // "POOLEMPTY"
      break;
    }
    vitte_touch_memory((uint8_t*)p, size, &checksum);
    checksum ^= vitte_hash_mix_u64((uintptr_t)p);
    vitte_pool_free_one(&pool, p);
  }

  const uint64_t t1 = vitte_time_now_ns();
  vitte_pool_free(&pool);

  vitte_bm_result r;
  r.elapsed_ns = (t1 >= t0) ? (t1 - t0) : 0;
  r.checksum = checksum;
  r.iters = iters;
  r.size = size;
  return r;
}

// =====================================================================================
// Case registry
// =====================================================================================

static const vitte_bm_case g_alloc_cases[] = {
  { "malloc_free",        bm_malloc_free },
  { "calloc_free",        bm_calloc_free },
  { "realloc_grow_shrink",bm_realloc_grow_shrink },
  { "aligned_alloc_free", bm_aligned_alloc_free },
  { "arena_bump",         bm_arena_bump },
  { "pool_reuse",         bm_pool_reuse },
};

const vitte_bm_case* vitte_bm_alloc_list_cases(size_t* out_count) {
  if (out_count) *out_count = (sizeof(g_alloc_cases) / sizeof(g_alloc_cases[0]));
  return g_alloc_cases;
}

// =====================================================================================
// Standalone runner
// =====================================================================================

#if defined(VITTE_BENCH_STANDALONE)

static void vitte_print_usage(const char* argv0) {
  fprintf(stderr,
    "Usage: %s --case <name> [--iters N] [--size BYTES] [--seed S] [--json]\\n"
    "Cases:\\n"
    "  malloc_free | calloc_free | realloc_grow_shrink | aligned_alloc_free | arena_bump | pool_reuse\\n"
    "Examples:\\n"
    "  %s --case malloc_free --iters 500000 --size 64\\n"
    "  %s --case pool_reuse --iters 2000000 --size 128 --json\\n",
    argv0, argv0, argv0);
}

static const vitte_bm_case* vitte_find_case(const char* name) {
  size_t n = 0;
  const vitte_bm_case* cases = vitte_bm_alloc_list_cases(&n);
  for (size_t i = 0; i < n; ++i) {
    if (strcmp(cases[i].name, name) == 0) return &cases[i];
  }
  return NULL;
}

static int vitte_parse_u64(const char* s, uint64_t* out) {
  if (!s || !*s) return 0;
  errno = 0;
  char* end = NULL;
  unsigned long long v = strtoull(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0') return 0;
  if (out) *out = (uint64_t)v;
  return 1;
}

static int vitte_parse_size(const char* s, size_t* out) {
  if (!s || !*s) return 0;
  errno = 0;
  char* end = NULL;
  unsigned long long v = strtoull(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0') return 0;
  if (out) *out = (size_t)v;
  return 1;
}

int main(int argc, char** argv) {
  const char* case_name = "malloc_free";
  uint64_t iters = 500000;
  size_t size = 64;
  uint64_t seed = UINT64_C(0x123456789ABCDEF0);
  int json = 0;

  for (int i = 1; i < argc; ++i) {
    const char* a = argv[i];
    if (strcmp(a, "--help") == 0 || strcmp(a, "-h") == 0) {
      vitte_print_usage(argv[0]);
      return 0;
    } else if (strcmp(a, "--json") == 0) {
      json = 1;
    } else if (strcmp(a, "--case") == 0 && i + 1 < argc) {
      case_name = argv[++i];
    } else if (strcmp(a, "--iters") == 0 && i + 1 < argc) {
      if (!vitte_parse_u64(argv[++i], &iters)) {
        fprintf(stderr, "Invalid --iters value\\n");
        return 2;
      }
    } else if (strcmp(a, "--size") == 0 && i + 1 < argc) {
      if (!vitte_parse_size(argv[++i], &size)) {
        fprintf(stderr, "Invalid --size value\\n");
        return 2;
      }
    } else if (strcmp(a, "--seed") == 0 && i + 1 < argc) {
      if (!vitte_parse_u64(argv[++i], &seed)) {
        fprintf(stderr, "Invalid --seed value\\n");
        return 2;
      }
    } else {
      fprintf(stderr, "Unknown arg: %s\\n", a);
      vitte_print_usage(argv[0]);
      return 2;
    }
  }

  const vitte_bm_case* c = vitte_find_case(case_name);
  if (!c) {
    fprintf(stderr, "Unknown case: %s\\n", case_name);
    vitte_print_usage(argv[0]);
    return 2;
  }

  vitte_bm_result r = c->fn(iters, size, seed);

  // Derived metrics
  const double ns = (double)r.elapsed_ns;
  const double it = (double)r.iters;
  const double ns_per_iter = (it > 0.0) ? (ns / it) : 0.0;
  const double ops_per_s = (ns > 0.0) ? (1e9 * it / ns) : 0.0;

  if (json) {
    // Minimal JSON, stable keys.
    printf("{\n");
    printf("  \"bench\": \"alloc\",\n");
    printf("  \"case\": \"%s\",\n", c->name);
    printf("  \"iters\": %llu,\n", (unsigned long long)r.iters);
    printf("  \"size\": %llu,\n", (unsigned long long)r.size);
    printf("  \"elapsed_ns\": %llu,\n", (unsigned long long)r.elapsed_ns);
    printf("  \"ns_per_iter\": %.3f,\n", ns_per_iter);
    printf("  \"ops_per_s\": %.3f,\n", ops_per_s);
    printf("  \"checksum\": %llu\n", (unsigned long long)r.checksum);
    printf("}\n");
  } else {
    printf("alloc/%s iters=%llu size=%zu elapsed=%lluns (%.3f ns/iter, %.3f ops/s) checksum=%llu\n",
      c->name,
      (unsigned long long)r.iters,
      r.size,
      (unsigned long long)r.elapsed_ns,
      ns_per_iter,
      ops_per_s,
      (unsigned long long)r.checksum);
  }

  return 0;
}

#endif // VITTE_BENCH_STANDALONE
