/*
  bench/rng.h

  Deterministic RNG utilities for the Vitte benchmark harness.

  Overview
  - `bench_rng`        : legacy tiny 32-bit xorshift RNG (ABI preserved).
  - `vitte_bench_rng64`: higher-quality 64-bit RNG for generators/fuzz inputs.

  Properties
  - Deterministic across platforms.
  - No external dependencies.
  - Not cryptographic.

  Notes
  - Use `vitte_bench_rng64` for data generators and fuzz corpora.
  - Keep `bench_rng` when ABI size matters or for legacy benchmarks.
*/

#pragma once
#ifndef VITTE_BENCH_RNG_H
#define VITTE_BENCH_RNG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/compiler.h"

/* -------------------------------------------------------------------------- */
/* Legacy RNG: xorshift32 (ABI preserved)                                      */
/* -------------------------------------------------------------------------- */

typedef struct bench_rng {
    uint32_t s;
} bench_rng;

/* Seed with non-zero. */
static inline void bench_rng_seed(bench_rng* r, uint32_t seed)
{
    if (!r) return;
    r->s = seed ? seed : 1u;
}

/* Next 32-bit value (xorshift32). */
static inline uint32_t bench_rng_next_u32(bench_rng* r)
{
    uint32_t x = r ? r->s : 1u;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    if (r) r->s = x ? x : 1u;
    return x;
}

/* Next 64-bit value (two draws). */
static inline uint64_t bench_rng_next_u64(bench_rng* r)
{
    uint64_t lo = (uint64_t)bench_rng_next_u32(r);
    uint64_t hi = (uint64_t)bench_rng_next_u32(r);
    return (hi << 32) | lo;
}

/*
  Uniform range [lo, hi) with rejection sampling to avoid modulo bias.
  - If hi <= lo, returns lo.
*/
static inline uint32_t bench_rng_range(bench_rng* r, uint32_t lo, uint32_t hi)
{
    if (hi <= lo) return lo;

    const uint32_t span = hi - lo;

    /* Rejection sampling: threshold is the largest multiple of span. */
    const uint32_t threshold = (uint32_t)(0u - span) % span;

    for (;;) {
        uint32_t x = bench_rng_next_u32(r);
        if (x >= threshold) {
            return lo + (x % span);
        }
    }
}

/* Uniform boolean (1 bit). */
static inline bool bench_rng_bool(bench_rng* r)
{
    return (bench_rng_next_u32(r) & 1u) != 0u;
}

/* Fill a buffer with deterministic bytes (legacy RNG). */
static inline void bench_rng_fill(bench_rng* r, void* dst, size_t n)
{
    uint8_t* p = (uint8_t*)dst;
    size_t i = 0;
    while (i + 4 <= n) {
        uint32_t x = bench_rng_next_u32(r);
        p[i + 0] = (uint8_t)(x & 0xffu);
        p[i + 1] = (uint8_t)((x >> 8) & 0xffu);
        p[i + 2] = (uint8_t)((x >> 16) & 0xffu);
        p[i + 3] = (uint8_t)((x >> 24) & 0xffu);
        i += 4;
    }
    if (i < n) {
        uint32_t x = bench_rng_next_u32(r);
        for (; i < n; ++i) {
            p[i] = (uint8_t)(x & 0xffu);
            x >>= 8;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* 64-bit RNG: splitmix64 seeding + xoroshiro128++                             */
/* -------------------------------------------------------------------------- */

/*
  xoroshiro128++ (David Blackman, Sebastiano Vigna)
  - very fast, good statistical properties for simulation/generation.
  - not cryptographic.

  splitmix64 is used for seeding.
*/

typedef struct vitte_bench_rng64 {
    uint64_t s0;
    uint64_t s1;
} vitte_bench_rng64;

VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_rng64__rotl(uint64_t x, int k)
{
    return (x << (uint32_t)k) | (x >> (uint32_t)(64 - k));
}

VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_rng64__splitmix64(uint64_t* x)
{
    uint64_t z = (*x += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

/* Seed from a single 64-bit value (deterministic). */
VITTE_BENCH_FORCEINLINE void vitte_bench_rng64_seed(vitte_bench_rng64* r, uint64_t seed)
{
    if (!r) return;
    uint64_t x = seed ? seed : 0x243f6a8885a308d3ull;
    r->s0 = vitte_bench_rng64__splitmix64(&x);
    r->s1 = vitte_bench_rng64__splitmix64(&x);
    /* xoroshiro forbids all-zero state */
    if (r->s0 == 0 && r->s1 == 0) {
        r->s0 = 0x9e3779b97f4a7c15ull;
        r->s1 = 0xbf58476d1ce4e5b9ull;
    }
}

/* Seed from two 64-bit values (useful for hashing inputs). */
VITTE_BENCH_FORCEINLINE void vitte_bench_rng64_seed2(vitte_bench_rng64* r, uint64_t a, uint64_t b)
{
    if (!r) return;
    /* Mix inputs through splitmix64 to avoid weak seeds */
    uint64_t x = a ^ (b + 0x9e3779b97f4a7c15ull);
    r->s0 = vitte_bench_rng64__splitmix64(&x);
    r->s1 = vitte_bench_rng64__splitmix64(&x);
    if (r->s0 == 0 && r->s1 == 0) {
        r->s0 = 0x9e3779b97f4a7c15ull;
        r->s1 = 0xbf58476d1ce4e5b9ull;
    }
}

/* Next 64-bit output. */
VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_rng64_next_u64(vitte_bench_rng64* r)
{
    const uint64_t s0 = r ? r->s0 : 0x9e3779b97f4a7c15ull;
    uint64_t s1 = r ? r->s1 : 0xbf58476d1ce4e5b9ull;

    const uint64_t result = vitte_bench_rng64__rotl(s0 + s1, 17) + s0;

    s1 ^= s0;

    const uint64_t ns0 = vitte_bench_rng64__rotl(s0, 49) ^ s1 ^ (s1 << 21);
    const uint64_t ns1 = vitte_bench_rng64__rotl(s1, 28);

    if (r) {
        r->s0 = ns0;
        r->s1 = ns1;
    }

    return result;
}

/* Next 32-bit output (high bits are generally better). */
VITTE_BENCH_FORCEINLINE uint32_t vitte_bench_rng64_next_u32(vitte_bench_rng64* r)
{
    return (uint32_t)(vitte_bench_rng64_next_u64(r) >> 32);
}

/* Uniform range [lo, hi) for 64-bit. */
VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_rng64_range_u64(vitte_bench_rng64* r, uint64_t lo, uint64_t hi)
{
    if (hi <= lo) return lo;
    const uint64_t span = hi - lo;

    /* Rejection sampling with 128-bit multiply if available. */
#if (VITTE_BENCH_CC_CLANG || VITTE_BENCH_CC_GCC)
    const uint64_t threshold = (uint64_t)(0ull - span) % span;
    for (;;) {
        uint64_t x = vitte_bench_rng64_next_u64(r);
        if (x >= threshold) return lo + (x % span);
    }
#else
    /* Portable fallback: use modulo with rejection using 64-bit threshold too. */
    const uint64_t threshold = (uint64_t)(0ull - span) % span;
    for (;;) {
        uint64_t x = vitte_bench_rng64_next_u64(r);
        if (x >= threshold) return lo + (x % span);
    }
#endif
}

/* Uniform range [lo, hi) for 32-bit (unbiased). */
VITTE_BENCH_FORCEINLINE uint32_t vitte_bench_rng64_range_u32(vitte_bench_rng64* r, uint32_t lo, uint32_t hi)
{
    if (hi <= lo) return lo;
    const uint32_t span = hi - lo;
    const uint32_t threshold = (uint32_t)(0u - span) % span;
    for (;;) {
        uint32_t x = vitte_bench_rng64_next_u32(r);
        if (x >= threshold) return lo + (x % span);
    }
}

/* Uniform double in [0,1). */
VITTE_BENCH_FORCEINLINE double vitte_bench_rng64_next_f64(vitte_bench_rng64* r)
{
    /* 53-bit mantissa from top bits */
    const uint64_t x = vitte_bench_rng64_next_u64(r);
    const uint64_t v = x >> 11; /* keep 53 bits */
    return (double)v * (1.0 / 9007199254740992.0); /* 2^53 */
}

/* Fill a buffer with deterministic bytes (64-bit RNG). */
VITTE_BENCH_FORCEINLINE void vitte_bench_rng64_fill(vitte_bench_rng64* r, void* dst, size_t n)
{
    uint8_t* p = (uint8_t*)dst;
    size_t i = 0;

    while (i + 8 <= n) {
        uint64_t x = vitte_bench_rng64_next_u64(r);
        p[i + 0] = (uint8_t)(x & 0xffu);
        p[i + 1] = (uint8_t)((x >> 8) & 0xffu);
        p[i + 2] = (uint8_t)((x >> 16) & 0xffu);
        p[i + 3] = (uint8_t)((x >> 24) & 0xffu);
        p[i + 4] = (uint8_t)((x >> 32) & 0xffu);
        p[i + 5] = (uint8_t)((x >> 40) & 0xffu);
        p[i + 6] = (uint8_t)((x >> 48) & 0xffu);
        p[i + 7] = (uint8_t)((x >> 56) & 0xffu);
        i += 8;
    }

    if (i < n) {
        uint64_t x = vitte_bench_rng64_next_u64(r);
        for (; i < n; ++i) {
            p[i] = (uint8_t)(x & 0xffu);
            x >>= 8;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Convenience: seed RNG64 from legacy RNG                                     */
/* -------------------------------------------------------------------------- */

VITTE_BENCH_FORCEINLINE void vitte_bench_rng64_seed_from_legacy(vitte_bench_rng64* out, bench_rng* legacy)
{
    uint64_t a = bench_rng_next_u64(legacy);
    uint64_t b = bench_rng_next_u64(legacy);
    vitte_bench_rng64_seed2(out, a, b);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_RNG_H */
