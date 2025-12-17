// C:\Users\vince\Documents\GitHub\vitte\fuzz\include\fuzz\fuzz_util.h
// fuzz_util.h — small utilities for fuzz harnesses (C17)
//
// Provides:
//  - stable hashing (FNV-1a), cheap PRNG
//  - endian helpers, clamp/min/max
//  - safe integer ops (checked add/mul) for allocation sizing
//  - lightweight temp arena (bump allocator) for parsers under fuzz
//  - byte-slice helpers
//
// Dependencies: fuzz_assert.h

#ifndef VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_UTIL_H
#define VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_UTIL_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz_assert.h"

//------------------------------------------------------------------------------
// min/max/clamp
//------------------------------------------------------------------------------

FUZZ_INLINE static size_t
fuzz_min_zu(size_t a, size_t b) {
  return a < b ? a : b;
}
FUZZ_INLINE static size_t
fuzz_max_zu(size_t a, size_t b) {
  return a > b ? a : b;
}

FUZZ_INLINE static uint32_t
fuzz_min_u32(uint32_t a, uint32_t b) {
  return a < b ? a : b;
}
FUZZ_INLINE static uint32_t
fuzz_max_u32(uint32_t a, uint32_t b) {
  return a > b ? a : b;
}

FUZZ_INLINE static size_t
fuzz_clamp_zu(size_t v, size_t lo, size_t hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

FUZZ_INLINE static uint32_t
fuzz_clamp_u32(uint32_t v, uint32_t lo, uint32_t hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

//------------------------------------------------------------------------------
// Hash (FNV-1a)
//------------------------------------------------------------------------------

FUZZ_INLINE static uint32_t
fuzz_fnv1a32(const void* data, size_t n) {
  const uint8_t* p = (const uint8_t*)data;
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < n; ++i) {
    h ^= (uint32_t)p[i];
    h *= 16777619u;
  }
  return h ? h : 1u;
}

FUZZ_INLINE static uint64_t
fuzz_fnv1a64(const void* data, size_t n) {
  const uint8_t* p = (const uint8_t*)data;
  uint64_t h = 1469598103934665603ull;
  for (size_t i = 0; i < n; ++i) {
    h ^= (uint64_t)p[i];
    h *= 1099511628211ull;
  }
  return h ? h : 1ull;
}

FUZZ_INLINE static uint32_t
fuzz_hash_cstr32(const char* s) {
  if (!s)
    return 1u;
  uint32_t h = 2166136261u;
  for (size_t i = 0; s[i] && i < 256; ++i) {
    h ^= (uint32_t)(uint8_t)s[i];
    h *= 16777619u;
  }
  return h ? h : 1u;
}

//------------------------------------------------------------------------------
// PRNG (xorshift64*), deterministic
//------------------------------------------------------------------------------

typedef struct fuzz_rng {
  uint64_t s;
} fuzz_rng;

FUZZ_INLINE static void
fuzz_rng_seed(fuzz_rng* r, uint64_t seed) {
  FUZZ_ASSERT(r);
  r->s = seed ? seed : 0x9E3779B97F4A7C15ull;
}

FUZZ_INLINE static uint64_t
fuzz_rng_next_u64(fuzz_rng* r) {
  FUZZ_ASSERT(r);
  uint64_t x = r->s;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  r->s = x;
  return x * 2685821657736338717ull;
}

FUZZ_INLINE static uint32_t
fuzz_rng_next_u32(fuzz_rng* r) {
  return (uint32_t)fuzz_rng_next_u64(r);
}

FUZZ_INLINE static uint8_t
fuzz_rng_next_u8(fuzz_rng* r) {
  return (uint8_t)fuzz_rng_next_u64(r);
}

FUZZ_INLINE static size_t
fuzz_rng_range(fuzz_rng* r, size_t hi_exclusive) {
  FUZZ_ASSERT(r);
  if (hi_exclusive == 0)
    return 0;
  return (size_t)(fuzz_rng_next_u64(r) % (uint64_t)hi_exclusive);
}

//------------------------------------------------------------------------------
// Endian helpers
//------------------------------------------------------------------------------

FUZZ_INLINE static uint16_t
fuzz_bswap16(uint16_t v) {
  return (uint16_t)((v >> 8) | (v << 8));
}

FUZZ_INLINE static uint32_t
fuzz_bswap32(uint32_t v) {
  return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) | ((v & 0x00FF0000u) >> 8) |
         ((v & 0xFF000000u) >> 24);
}

FUZZ_INLINE static uint64_t
fuzz_bswap64(uint64_t v) {
  return ((v & 0x00000000000000FFull) << 56) | ((v & 0x000000000000FF00ull) << 40) |
         ((v & 0x0000000000FF0000ull) << 24) | ((v & 0x00000000FF000000ull) << 8) |
         ((v & 0x000000FF00000000ull) >> 8) | ((v & 0x0000FF0000000000ull) >> 24) |
         ((v & 0x00FF000000000000ull) >> 40) | ((v & 0xFF00000000000000ull) >> 56);
}

//------------------------------------------------------------------------------
// Checked arithmetic for allocation sizing
//------------------------------------------------------------------------------

FUZZ_INLINE static int
fuzz_add_overflow_zu(size_t a, size_t b, size_t* out) {
  FUZZ_ASSERT(out);
  if (a > (SIZE_MAX - b))
    return 1;
  *out = a + b;
  return 0;
}

FUZZ_INLINE static int
fuzz_mul_overflow_zu(size_t a, size_t b, size_t* out) {
  FUZZ_ASSERT(out);
  if (a == 0 || b == 0) {
    *out = 0;
    return 0;
  }
  if (a > (SIZE_MAX / b))
    return 1;
  *out = a * b;
  return 0;
}

//------------------------------------------------------------------------------
// Slice helpers
//------------------------------------------------------------------------------

typedef struct fuzz_bytes {
  uint8_t* data;
  size_t size;
} fuzz_bytes;

typedef struct fuzz_bytes_view {
  const uint8_t* data;
  size_t size;
} fuzz_bytes_view;

FUZZ_INLINE static fuzz_bytes_view
fuzz_bytes_view_from(const void* data, size_t size) {
  FUZZ_ASSERT(data || size == 0);
  fuzz_bytes_view v;
  v.data = (const uint8_t*)data;
  v.size = size;
  return v;
}

FUZZ_INLINE static int
fuzz_bytes_view_eq(fuzz_bytes_view a, fuzz_bytes_view b) {
  if (a.size != b.size)
    return 0;
  if (a.size == 0)
    return 1;
  return memcmp(a.data, b.data, a.size) == 0;
}

//------------------------------------------------------------------------------
// Simple bump arena (temp allocations during parsing)
//------------------------------------------------------------------------------

typedef struct fuzz_arena {
  uint8_t* mem;
  size_t cap;
  size_t off;
} fuzz_arena;

FUZZ_INLINE static void
fuzz_arena_init(fuzz_arena* a, void* backing, size_t cap) {
  FUZZ_ASSERT(a);
  FUZZ_ASSERT(backing || cap == 0);
  a->mem = (uint8_t*)backing;
  a->cap = cap;
  a->off = 0;
}

FUZZ_INLINE static void
fuzz_arena_reset(fuzz_arena* a) {
  FUZZ_ASSERT(a);
  a->off = 0;
}

FUZZ_INLINE static size_t
fuzz__align_up(size_t x, size_t align) {
  FUZZ_ASSERT(align && ((align & (align - 1u)) == 0u));
  return (x + (align - 1u)) & ~(align - 1u);
}

FUZZ_INLINE static void*
fuzz_arena_alloc(fuzz_arena* a, size_t size, size_t align) {
  FUZZ_ASSERT(a);
  if (align == 0)
    align = (size_t)sizeof(void*);
  if ((align & (align - 1u)) != 0u)
    return NULL; // must be pow2

  size_t off = fuzz__align_up(a->off, align);
  size_t end = 0;
  if (fuzz_add_overflow_zu(off, size, &end))
    return NULL;
  if (end > a->cap)
    return NULL;

  void* p = a->mem + off;
  a->off = end;
  return p;
}

FUZZ_INLINE static void*
fuzz_arena_alloc_zeroed(fuzz_arena* a, size_t size, size_t align) {
  void* p = fuzz_arena_alloc(a, size, align);
  if (!p)
    return NULL;
  memset(p, 0, size);
  return p;
}

// Convenience: “strndup” into arena (not NUL-required in source)
FUZZ_INLINE static char*
fuzz_arena_strndup(fuzz_arena* a, const char* s, size_t n) {
  FUZZ_ASSERT(a);
  FUZZ_ASSERT(s || n == 0);
  char* p = (char*)fuzz_arena_alloc(a, n + 1, 1);
  if (!p)
    return NULL;
  if (n)
    memcpy(p, s, n);
  p[n] = '\0';
  return p;
}

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_UTIL_H
