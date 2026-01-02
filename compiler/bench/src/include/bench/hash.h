
/*
  bench/hash.h

  Hashing utilities for the benchmark subsystem.

  Use cases
  - Stable identifiers for fixture/content bookkeeping.
  - File-set signatures (paths + kind).
  - Cheap change detection for generated artifacts.

  Properties
  - Portable C17, no external deps.
  - Deterministic across platforms (explicit little-endian loads).
  - Fast enough for bench metadata; not cryptographic.

  Notes
  - For security, use a real cryptographic hash (SHA-256, BLAKE3, ...).
*/

#pragma once
#ifndef VITTE_BENCH_HASH_H
#define VITTE_BENCH_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "bench/compiler.h"

/* -------------------------------------------------------------------------- */
/* Helpers                                                                     */
/* -------------------------------------------------------------------------- */

VITTE_BENCH_FORCEINLINE uint32_t vitte_bench_hash__rotl32(uint32_t x, uint32_t r)
{
    return (x << (r & 31u)) | (x >> ((32u - (r & 31u)) & 31u));
}

VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_hash__rotl64(uint64_t x, uint32_t r)
{
    return (x << (r & 63u)) | (x >> ((64u - (r & 63u)) & 63u));
}

VITTE_BENCH_FORCEINLINE uint32_t vitte_bench_hash__load_u32le(const void* p)
{
    const uint8_t* b = (const uint8_t*)p;
    return (uint32_t)b[0]
        | ((uint32_t)b[1] << 8u)
        | ((uint32_t)b[2] << 16u)
        | ((uint32_t)b[3] << 24u);
}

VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_hash__load_u64le(const void* p)
{
    const uint8_t* b = (const uint8_t*)p;
    return (uint64_t)b[0]
        | ((uint64_t)b[1] << 8u)
        | ((uint64_t)b[2] << 16u)
        | ((uint64_t)b[3] << 24u)
        | ((uint64_t)b[4] << 32u)
        | ((uint64_t)b[5] << 40u)
        | ((uint64_t)b[6] << 48u)
        | ((uint64_t)b[7] << 56u);
}

VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_hash__mul64(uint64_t a, uint64_t b)
{
#if (VITTE_BENCH_CC_CLANG || VITTE_BENCH_CC_GCC)
    return (uint64_t)((__uint128_t)a * (__uint128_t)b);
#else
    /* fallback: low 64-bit multiply is well-defined in C for unsigned */
    return a * b;
#endif
}

/* SplitMix64 finalizer / avalanche. */
VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_hash__mix64(uint64_t x)
{
    x ^= x >> 30u;
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27u;
    x *= 0x94d049bb133111ebull;
    x ^= x >> 31u;
    return x;
}

/* Murmur-like 32-bit finalizer. */
VITTE_BENCH_FORCEINLINE uint32_t vitte_bench_hash__fmix32(uint32_t h)
{
    h ^= h >> 16u;
    h *= 0x85ebca6bu;
    h ^= h >> 13u;
    h *= 0xc2b2ae35u;
    h ^= h >> 16u;
    return h;
}

/* -------------------------------------------------------------------------- */
/* FNV-1a                                                                      */
/* -------------------------------------------------------------------------- */

/* Classic FNV-1a 64 (portable, decent for short keys). */
VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_hash_fnv1a64_seed(uint64_t seed, const void* data, size_t n)
{
    const uint8_t* p = (const uint8_t*)data;
    uint64_t h = seed;
    for (size_t i = 0; i < n; ++i) {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ull;
    }
    return h;
}

VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_hash_fnv1a64(const void* data, size_t n)
{
    return vitte_bench_hash_fnv1a64_seed(1469598103934665603ull, data, n);
}

VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_hash_fnv1a64_str(const char* s)
{
    if (!s) {
        return vitte_bench_hash_fnv1a64("", 0);
    }
    uint64_t h = 1469598103934665603ull;
    for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
        h ^= (uint64_t)(*p);
        h *= 1099511628211ull;
    }
    return h;
}

/* -------------------------------------------------------------------------- */
/* Fast non-cryptographic hash for bytes (64-bit)                              */
/* -------------------------------------------------------------------------- */

/*
  A small, fast 64-bit hash for byte sequences.

  Construction
  - Absorbs 8-byte chunks (little endian) with mixing.
  - Handles tail bytes.
  - Finalizes with strong avalanche.

  This is intended for metadata (paths, small buffers). It is not a cryptographic hash.
*/
VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_hash64_bytes_seed(uint64_t seed, const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    const uint64_t c1 = 0x9e3779b185ebca87ull;
    const uint64_t c2 = 0xc2b2ae3d27d4eb4full;

    uint64_t h = vitte_bench_hash__mix64(seed ^ (uint64_t)len);

    /* body */
    while (len >= 8) {
        uint64_t k = vitte_bench_hash__load_u64le(p);
        k ^= vitte_bench_hash__rotl64(k, 25u);
        k *= c1;
        h ^= k;
        h = vitte_bench_hash__rotl64(h, 27u);
        h = vitte_bench_hash__mul64(h, c2) + 0x52dce729ull;

        p += 8;
        len -= 8;
    }

    /* tail */
    uint64_t t = 0;
    switch (len) {
        case 7: t ^= (uint64_t)p[6] << 48u; /* fallthrough */
        case 6: t ^= (uint64_t)p[5] << 40u; /* fallthrough */
        case 5: t ^= (uint64_t)p[4] << 32u; /* fallthrough */
        case 4: t ^= (uint64_t)p[3] << 24u; /* fallthrough */
        case 3: t ^= (uint64_t)p[2] << 16u; /* fallthrough */
        case 2: t ^= (uint64_t)p[1] << 8u;  /* fallthrough */
        case 1: t ^= (uint64_t)p[0];        /* fallthrough */
        default: break;
    }
    if (len) {
        t *= c1;
        t ^= vitte_bench_hash__rotl64(t, 23u);
        t *= c2;
        h ^= t;
    }

    return vitte_bench_hash__mix64(h);
}

VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_hash64_bytes(const void* data, size_t len)
{
    return vitte_bench_hash64_bytes_seed(0x243f6a8885a308d3ull, data, len);
}

VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_hash64_str(const char* s)
{
    if (!s) {
        return vitte_bench_hash64_bytes("", 0);
    }
    /* strlen without libc */
    const char* p = s;
    while (*p) p++;
    return vitte_bench_hash64_bytes(s, (size_t)(p - s));
}

/* -------------------------------------------------------------------------- */
/* Murmur3 32-bit (x86)                                                        */
/* -------------------------------------------------------------------------- */

VITTE_BENCH_FORCEINLINE uint32_t vitte_bench_hash_murmur3_32_seed(uint32_t seed, const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    const uint32_t c1 = 0xcc9e2d51u;
    const uint32_t c2 = 0x1b873593u;

    uint32_t h = seed;

    /* body */
    while (len >= 4) {
        uint32_t k = vitte_bench_hash__load_u32le(p);
        k *= c1;
        k = vitte_bench_hash__rotl32(k, 15u);
        k *= c2;

        h ^= k;
        h = vitte_bench_hash__rotl32(h, 13u);
        h = h * 5u + 0xe6546b64u;

        p += 4;
        len -= 4;
    }

    /* tail */
    uint32_t t = 0;
    switch (len) {
        case 3: t ^= (uint32_t)p[2] << 16u; /* fallthrough */
        case 2: t ^= (uint32_t)p[1] << 8u;  /* fallthrough */
        case 1: t ^= (uint32_t)p[0];        /* fallthrough */
        default: break;
    }
    if (len) {
        t *= c1;
        t = vitte_bench_hash__rotl32(t, 15u);
        t *= c2;
        h ^= t;
    }

    h ^= (uint32_t)((uint32_t)len);
    return vitte_bench_hash__fmix32(h);
}

VITTE_BENCH_FORCEINLINE uint32_t vitte_bench_hash_murmur3_32(const void* data, size_t len)
{
    return vitte_bench_hash_murmur3_32_seed(0x9747b28cu, data, len);
}

/* -------------------------------------------------------------------------- */
/* Hash combine                                                                 */
/* -------------------------------------------------------------------------- */

VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_hash_combine64(uint64_t h, uint64_t v)
{
    /* boost::hash_combine-like with stronger finalization */
    h ^= vitte_bench_hash__mix64(v + 0x9e3779b97f4a7c15ull + (h << 6u) + (h >> 2u));
    return vitte_bench_hash__mix64(h);
}

VITTE_BENCH_FORCEINLINE uint32_t vitte_bench_hash_combine32(uint32_t h, uint32_t v)
{
    h ^= v + 0x9e3779b9u + (h << 6u) + (h >> 2u);
    return vitte_bench_hash__fmix32(h);
}

/* -------------------------------------------------------------------------- */
/* Backward compatibility                                                      */
/* -------------------------------------------------------------------------- */

/* Historical name used by earlier bench fixtures. */
static inline uint64_t bench_hash_fnv1a64(const void* data, size_t n)
{
    return vitte_bench_hash_fnv1a64(data, n);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_HASH_H */
