#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <unistd.h>
#endif

// bench.c — core benchmark utilities (C17, max)
//
// This module provides small, reusable helpers used by benchmark suites:
//   - deterministic PRNG (splitmix64)
//   - fast hash/mix (for checksums / anti-DCE)
//   - black_box helpers to inhibit over-optimization
//   - alignment helpers
//   - memory pattern helpers
//   - lightweight formatting helpers
//   - OS page size query (best-effort)
//
// Notes:
//   - This file is intentionally standalone and does not depend on project headers.
//   - If you have an existing bench.h, you can move the public declarations there.

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

// -----------------------------------------------------------------------------
// Anti-DCE sink
// -----------------------------------------------------------------------------

static volatile uint64_t g_bench_sink_u64 = 0;
static volatile const void* g_bench_sink_ptr = NULL;

// -----------------------------------------------------------------------------
// Hash / mixing
// -----------------------------------------------------------------------------

// splitmix64 step (also good as a standalone mixer)
static inline uint64_t bench_splitmix64_step(uint64_t* state) {
  uint64_t x = (*state += UINT64_C(0x9E3779B97F4A7C15));
  x = (x ^ (x >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  x = (x ^ (x >> 27)) * UINT64_C(0x94D049BB133111EB);
  return x ^ (x >> 31);
}

uint64_t bench_hash_u64(uint64_t x) {
  // One-shot splitmix-like mix.
  x ^= x >> 30;
  x *= UINT64_C(0xBF58476D1CE4E5B9);
  x ^= x >> 27;
  x *= UINT64_C(0x94D049BB133111EB);
  x ^= x >> 31;
  return x;
}

uint64_t bench_hash_bytes(const void* data, size_t len, uint64_t seed) {
  // Lightweight byte hash (not crypto): mix chunks with splitmix.
  const uint8_t* p = (const uint8_t*)data;
  uint64_t h = bench_hash_u64(seed ^ (uint64_t)len);

  // 8-byte chunks
  while (len >= 8) {
    uint64_t x;
    memcpy(&x, p, sizeof(x));
    h ^= bench_hash_u64(x + UINT64_C(0x9E3779B97F4A7C15));
    h = (h << 27) | (h >> (64 - 27));
    h *= UINT64_C(0x3C79AC492BA7B653);
    p += 8;
    len -= 8;
  }

  // tail
  uint64_t tail = 0;
  for (size_t i = 0; i < len; ++i) {
    tail |= (uint64_t)p[i] << (8u * (unsigned)i);
  }
  h ^= bench_hash_u64(tail ^ UINT64_C(0x1C69B3F74AC4AE35));
  h ^= h >> 33;
  h *= UINT64_C(0xFF51AFD7ED558CCD);
  h ^= h >> 33;
  h *= UINT64_C(0xC4CEB9FE1A85EC53);
  h ^= h >> 33;
  return h;
}

// -----------------------------------------------------------------------------
// PRNG
// -----------------------------------------------------------------------------

typedef struct bench_rng {
  uint64_t state;
} bench_rng;

void bench_rng_seed(bench_rng* r, uint64_t seed) {
  if (!r) return;
  r->state = seed ? seed : UINT64_C(0x243F6A8885A308D3);
  // warm up
  (void)bench_splitmix64_step(&r->state);
  (void)bench_splitmix64_step(&r->state);
}

uint64_t bench_rng_next_u64(bench_rng* r) {
  if (!r) return 0;
  return bench_splitmix64_step(&r->state);
}

uint32_t bench_rng_next_u32(bench_rng* r) {
  return (uint32_t)(bench_rng_next_u64(r) >> 32);
}

// uniform in [0, bound) using rejection to avoid modulo bias.
uint64_t bench_rng_range_u64(bench_rng* r, uint64_t bound) {
  if (!r || bound == 0) return 0;

  // threshold = 2^64 % bound
  const uint64_t threshold = (uint64_t)(0 - bound) % bound;
  for (;;) {
    const uint64_t x = bench_rng_next_u64(r);
    if (x >= threshold) return x % bound;
  }
}

// -----------------------------------------------------------------------------
// Black box (optimization barrier)
// -----------------------------------------------------------------------------

void bench_black_box_u64(uint64_t x) {
  g_bench_sink_u64 ^= bench_hash_u64(x);
  atomic_signal_fence(memory_order_seq_cst);
}

void bench_black_box_ptr(const void* p) {
  g_bench_sink_ptr = p;
  atomic_signal_fence(memory_order_seq_cst);
}

uint64_t bench_sink_u64(void) {
  atomic_signal_fence(memory_order_seq_cst);
  return g_bench_sink_u64;
}

// -----------------------------------------------------------------------------
// Alignment
// -----------------------------------------------------------------------------

size_t bench_align_up_size(size_t x, size_t align) {
  if (align == 0) return x;
  const size_t m = align - 1;
  return (x + m) & ~m;
}

uintptr_t bench_align_up_ptr(uintptr_t x, size_t align) {
  if (align == 0) return x;
  const uintptr_t m = (uintptr_t)align - 1;
  return (x + m) & ~m;
}

// -----------------------------------------------------------------------------
// Memory helpers
// -----------------------------------------------------------------------------

void bench_memfill_pattern_u32(void* dst, size_t bytes, uint32_t seed) {
  if (!dst || bytes == 0) return;
  uint8_t* p = (uint8_t*)dst;

  // Fill 4-byte pattern.
  uint32_t x = seed ? seed : 0xA5A5A5A5u;
  while (bytes >= 4) {
    memcpy(p, &x, 4);
    x = (x * 1664525u) + 1013904223u;
    p += 4;
    bytes -= 4;
  }
  // Tail
  while (bytes--) {
    *p++ = (uint8_t)(x & 0xFFu);
    x = (x * 1103515245u) + 12345u;
  }
}

uint64_t bench_memchecksum(const void* data, size_t bytes, uint64_t seed) {
  return bench_hash_bytes(data, bytes, seed);
}

// -----------------------------------------------------------------------------
// OS queries (best-effort)
// -----------------------------------------------------------------------------

size_t bench_os_page_size(void) {
#if defined(_WIN32)
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return (size_t)si.dwPageSize;
#else
  #if defined(_SC_PAGESIZE)
    long v = sysconf(_SC_PAGESIZE);
    if (v > 0) return (size_t)v;
  #endif
  return (size_t)4096;
#endif
}

// Conservative default; real cache line size varies.
size_t bench_os_cacheline_size(void) {
#if defined(_WIN32)
  // Querying cache line size properly requires GetLogicalProcessorInformation;
  // keep 64 as safe default.
  return (size_t)64;
#else
  return (size_t)64;
#endif
}

// -----------------------------------------------------------------------------
// Formatting helpers
// -----------------------------------------------------------------------------

// Human formatting: ns -> {ns,us,ms,s}
// Writes into buf (NUL-terminated). Returns buf.
char* bench_format_ns(char* buf, size_t cap, uint64_t ns) {
  if (!buf || cap == 0) return buf;

  const double d = (double)ns;
  if (ns < 1000ULL) {
    (void)snprintf(buf, cap, "%lluns", (unsigned long long)ns);
  } else if (ns < 1000000ULL) {
    (void)snprintf(buf, cap, "%.3fus", d / 1e3);
  } else if (ns < 1000000000ULL) {
    (void)snprintf(buf, cap, "%.3fms", d / 1e6);
  } else {
    (void)snprintf(buf, cap, "%.3fs", d / 1e9);
  }
  return buf;
}

// Human formatting: rate (ops/s)
char* bench_format_rate(char* buf, size_t cap, double ops_per_s) {
  if (!buf || cap == 0) return buf;

  double v = ops_per_s;
  const char* unit = "ops/s";
  if (v >= 1e12) { v /= 1e12; unit = "Top/s"; }
  else if (v >= 1e9) { v /= 1e9; unit = "Gop/s"; }
  else if (v >= 1e6) { v /= 1e6; unit = "Mop/s"; }
  else if (v >= 1e3) { v /= 1e3; unit = "Kop/s"; }

  (void)snprintf(buf, cap, "%.3f%s", v, unit);
  return buf;
}

// -----------------------------------------------------------------------------
// Optional minimal self-test
// -----------------------------------------------------------------------------

#if defined(BENCH_CORE_TEST)

int main(void) {
  bench_rng r;
  bench_rng_seed(&r, 123);

  uint8_t buf[64];
  bench_memfill_pattern_u32(buf, sizeof(buf), 0xDEADBEEFu);
  const uint64_t h = bench_memchecksum(buf, sizeof(buf), 1);

  char t[32], rr[32];
  bench_format_ns(t, sizeof(t), 1234567);
  bench_format_rate(rr, sizeof(rr), 123456789.0);

  printf("rng=%llu hash=%llu page=%zu cacheline=%zu time=%s rate=%s\n",
         (unsigned long long)bench_rng_next_u64(&r),
         (unsigned long long)h,
         bench_os_page_size(),
         bench_os_cacheline_size(),
         t,
         rr);
  return 0;
}

#endif