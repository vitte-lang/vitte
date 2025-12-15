// vitte/src/asm/src/vitte/asm_verify.c
//
// ASM fastpaths verification / self-tests
//
// Goal:
//   - Provide lightweight correctness checks comparing variants
//   - Intended for: unit tests, CI, debug builds, fuzz harness hooks
//
// Design:
//   - No dynamic allocation
//   - No IO by default (return error codes)
//   - Deterministic pseudo-random generator
//
// API idea:
//   - vitte_asm_verify_all(iterations) -> 0 on success, non-zero on failure
//
// Notes:
//   - memcpy semantics forbid overlap: tests avoid overlap.
//   - memmove is not tested here.
//   - UTF-8 validator: compares stub vs itself (placeholder for future SIMD).
//
// Build:
//   - Compile into test binary or into runtime behind a feature flag.
//

#include <stddef.h>
#include <stdint.h>

#include "vitte/src/asm/include/vitte/asm.h"
#include "vitte/src/asm/include/vitte/asm_verify.h"
#include "vitte/src/asm/include/vitte/cpu.h"

// -----------------------------------------------------------------------------
// Error model
// -----------------------------------------------------------------------------

typedef enum vitte_verify_err_t {
  VITTE_VERIFY_OK = 0,
  VITTE_VERIFY_E_MEMCPY_MISMATCH = 1,
  VITTE_VERIFY_E_MEMSET_MISMATCH = 2,
  VITTE_VERIFY_E_FNV_MISMATCH = 3,
  VITTE_VERIFY_E_UTF8_MISMATCH = 4,
  VITTE_VERIFY_E_INVAL = 10
} vitte_verify_err_t;

// -----------------------------------------------------------------------------
// Deterministic RNG (xorshift64*)
// -----------------------------------------------------------------------------

static uint64_t vitte_rng_u64(uint64_t* state) {
  // xorshift64*
  uint64_t x = *state;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  *state = x;
  return x * 2685821657736338717ULL;
}

static uint32_t vitte_rng_u32(uint64_t* state) {
  return (uint32_t)(vitte_rng_u64(state) >> 32);
}

static uint8_t vitte_rng_u8(uint64_t* state) {
  return (uint8_t)(vitte_rng_u64(state) >> 56);
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static void vitte_fill(uint8_t* buf, size_t n, uint64_t* rng) {
  for (size_t i = 0; i < n; i++) buf[i] = vitte_rng_u8(rng);
}

static int vitte_memeq(const uint8_t* a, const uint8_t* b, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (a[i] != b[i]) return 0;
  }
  return 1;
}

static uint64_t vitte_fnv1a64_ref(const uint8_t* data, size_t len) {
  uint64_t h = 14695981039346656037ULL;
  const uint64_t prime = 1099511628211ULL;
  for (size_t i = 0; i < len; i++) {
    h ^= (uint64_t)data[i];
    h *= prime;
  }
  return h;
}

// Simple strict UTF-8 reference (C scalar), same rules as stub.
static int vitte_utf8_validate_ref(const uint8_t* s, size_t n) {
  size_t i = 0;
  while (i < n) {
    uint8_t b0 = s[i++];

    if (b0 < 0x80) continue;

    if (b0 < 0xC2) return 0;

    if (b0 < 0xE0) {
      if (i >= n) return 0;
      uint8_t b1 = s[i++];
      if ((b1 & 0xC0) != 0x80) return 0;
      continue;
    }

    if (b0 < 0xF0) {
      if (i + 1 >= n) return 0;
      uint8_t b1 = s[i++];
      uint8_t b2 = s[i++];
      if ((b1 & 0xC0) != 0x80) return 0;
      if ((b2 & 0xC0) != 0x80) return 0;
      if (b0 == 0xE0 && b1 < 0xA0) return 0;
      if (b0 == 0xED && b1 > 0x9F) return 0;
      continue;
    }

    if (b0 < 0xF5) {
      if (i + 2 >= n) return 0;
      uint8_t b1 = s[i++];
      uint8_t b2 = s[i++];
      uint8_t b3 = s[i++];
      if ((b1 & 0xC0) != 0x80) return 0;
      if ((b2 & 0xC0) != 0x80) return 0;
      if ((b3 & 0xC0) != 0x80) return 0;
      if (b0 == 0xF0 && b1 < 0x90) return 0;
      if (b0 == 0xF4 && b1 > 0x8F) return 0;
      continue;
    }

    return 0;
  }
  return 1;
}

// -----------------------------------------------------------------------------
// Variant selectors (arch-based, no dispatch table here)
// -----------------------------------------------------------------------------

typedef void* (*memcpy_fn)(void*, const void*, size_t);
typedef void* (*memset_fn)(void*, int, size_t);
typedef int   (*utf8_fn)(const uint8_t*, size_t);

static memcpy_fn pick_memcpy_baseline(void) {
#if defined(__aarch64__) || defined(_M_ARM64)
  return vitte_memcpy_baseline;
#elif defined(__x86_64__) || defined(_M_X64)
  // prefer SSE2 as baseline for x86_64
  return vitte_memcpy_sse2;
#else
  return vitte_memcpy_baseline;
#endif
}

static memset_fn pick_memset_baseline(void) {
#if defined(__aarch64__) || defined(_M_ARM64)
  return vitte_memset_baseline;
#elif defined(__x86_64__) || defined(_M_X64)
  return vitte_memset_sse2;
#else
  return vitte_memset_baseline;
#endif
}

static memcpy_fn pick_memcpy_fast(void) {
#if defined(__aarch64__) || defined(_M_ARM64)
  return vitte_memcpy_neon;
#elif defined(__x86_64__) || defined(_M_X64)
  // Only pick AVX2 if your CPU detection says ok; here: assume caller gated tests.
  return vitte_memcpy_avx2;
#else
  return pick_memcpy_baseline();
#endif
}

static memset_fn pick_memset_fast(void) {
#if defined(__aarch64__) || defined(_M_ARM64)
  return vitte_memset_neon;
#elif defined(__x86_64__) || defined(_M_X64)
  return vitte_memset_avx2;
#else
  return pick_memset_baseline();
#endif
}

static utf8_fn pick_utf8_any(void) {
  return vitte_utf8_validate_stub;
}

// -----------------------------------------------------------------------------
// Core verification
// -----------------------------------------------------------------------------

static vitte_verify_err_t verify_memcpy(uint64_t* rng, size_t iters) {
  // Avoid overlap: use separate src/dst buffers.
  // Provide random misalignments too.
  enum { BUFSZ = 4096 };
  static uint8_t src[BUFSZ + 64];
  static uint8_t d0[BUFSZ + 64];
  static uint8_t d1[BUFSZ + 64];

  memcpy_fn base = pick_memcpy_baseline();
  memcpy_fn fast = pick_memcpy_fast();

  for (size_t k = 0; k < iters; k++) {
    size_t n = (size_t)(vitte_rng_u32(rng) % (BUFSZ + 1));

    size_t so = (size_t)(vitte_rng_u32(rng) % 32);
    size_t o0 = (size_t)(vitte_rng_u32(rng) % 32);
    size_t o1 = (size_t)(vitte_rng_u32(rng) % 32);

    vitte_fill(src, BUFSZ + 64, rng);
    vitte_fill(d0, BUFSZ + 64, rng);
    vitte_fill(d1, BUFSZ + 64, rng);

    uint8_t* srcp = src + so;
    uint8_t* d0p  = d0 + o0;
    uint8_t* d1p  = d1 + o1;

    // Copy same input to both outputs
    base(d0p, srcp, n);
    fast(d1p, srcp, n);

    if (!vitte_memeq(d0p, d1p, n)) return VITTE_VERIFY_E_MEMCPY_MISMATCH;
  }
  return VITTE_VERIFY_OK;
}

static vitte_verify_err_t verify_memset(uint64_t* rng, size_t iters) {
  enum { BUFSZ = 4096 };
  static uint8_t d0[BUFSZ + 64];
  static uint8_t d1[BUFSZ + 64];

  memset_fn base = pick_memset_baseline();
  memset_fn fast = pick_memset_fast();

  for (size_t k = 0; k < iters; k++) {
    size_t n = (size_t)(vitte_rng_u32(rng) % (BUFSZ + 1));
    int c = (int)(vitte_rng_u8(rng));

    size_t o0 = (size_t)(vitte_rng_u32(rng) % 32);
    size_t o1 = (size_t)(vitte_rng_u32(rng) % 32);

    vitte_fill(d0, BUFSZ + 64, rng);
    vitte_fill(d1, BUFSZ + 64, rng);

    uint8_t* p0 = d0 + o0;
    uint8_t* p1 = d1 + o1;

    base(p0, c, n);
    fast(p1, c, n);

    if (!vitte_memeq(p0, p1, n)) return VITTE_VERIFY_E_MEMSET_MISMATCH;
  }
  return VITTE_VERIFY_OK;
}

static vitte_verify_err_t verify_fnv(uint64_t* rng, size_t iters) {
  enum { BUFSZ = 4096 };
  static uint8_t buf[BUFSZ];

  for (size_t k = 0; k < iters; k++) {
    size_t n = (size_t)(vitte_rng_u32(rng) % (BUFSZ + 1));
    vitte_fill(buf, BUFSZ, rng);

    uint64_t a = vitte_fnv1a64(buf, n);
    uint64_t b = vitte_fnv1a64_ref(buf, n);
    if (a != b) return VITTE_VERIFY_E_FNV_MISMATCH;
  }
  return VITTE_VERIFY_OK;
}

static vitte_verify_err_t verify_utf8(uint64_t* rng, size_t iters) {
  // Generate biased random bytes; compare stub vs reference.
  enum { BUFSZ = 1024 };
  static uint8_t buf[BUFSZ];

  utf8_fn fn = pick_utf8_any();

  for (size_t k = 0; k < iters; k++) {
    size_t n = (size_t)(vitte_rng_u32(rng) % (BUFSZ + 1));

    // Bias toward ASCII but inject random high bytes too.
    for (size_t i = 0; i < n; i++) {
      uint8_t r = vitte_rng_u8(rng);
      if ((r & 0xF0) == 0) buf[i] = (uint8_t)(r & 0x7F);
      else buf[i] = r;
    }

    int a = fn(buf, n);
    int b = vitte_utf8_validate_ref(buf, n);
    if (a != b) return VITTE_VERIFY_E_UTF8_MISMATCH;
  }
  return VITTE_VERIFY_OK;
}

// -----------------------------------------------------------------------------
// Public entry points
// -----------------------------------------------------------------------------

int vitte_asm_verify_all(size_t iterations) {
  if (iterations == 0) return (int)VITTE_VERIFY_E_INVAL;

  uint64_t rng = 0x9e3779b97f4a7c15ULL; // fixed seed

  vitte_verify_err_t e;

  e = verify_memcpy(&rng, iterations);
  if (e) return (int)e;

  e = verify_memset(&rng, iterations);
  if (e) return (int)e;

  e = verify_fnv(&rng, iterations);
  if (e) return (int)e;

  e = verify_utf8(&rng, iterations);
  if (e) return (int)e;

  return 0;
}
