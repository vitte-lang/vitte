

// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// suite_common.h
// -----------------------------------------------------------------------------
// Common definitions for compiler bench suites.
//
// Goals:
//  - Provide a stable ABI between suite translation units and the bench runner.
//  - Keep dependencies minimal (C17-compatible).
//  - Offer portable attributes (constructor/weak) and small utilities.
//
// Notes:
//  - Suites may be fully standalone and not include this header.
//  - The runner should define/implement `bench_register_suite`.
// -----------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Attributes
// -----------------------------------------------------------------------------

#if defined(__GNUC__) || defined(__clang__)
    #define VITTE_BENCH_CONSTRUCTOR __attribute__((constructor))
    #define VITTE_BENCH_WEAK        __attribute__((weak))
    #define VITTE_BENCH_NOINLINE    __attribute__((noinline))
    #define VITTE_BENCH_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define VITTE_BENCH_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define VITTE_BENCH_CONSTRUCTOR
    #define VITTE_BENCH_WEAK
    #define VITTE_BENCH_NOINLINE
    #define VITTE_BENCH_LIKELY(x)   (x)
    #define VITTE_BENCH_UNLIKELY(x) (x)
#endif

// -----------------------------------------------------------------------------
// Runner <-> Suite ABI
// -----------------------------------------------------------------------------

typedef void (*bench_fn)(uint64_t iters, void* user);

typedef struct bench_case
{
    const char* name;
    bench_fn fn;
    void* user;
} bench_case_t;

typedef struct bench_suite
{
    const char* name;
    const bench_case_t* cases;
    size_t case_count;
} bench_suite_t;

// Implemented by the bench runner.
void bench_register_suite(const bench_suite_t* suite);

// Optional runner helpers (not required).
// If your runner provides these, suites/tools may use them.
VITTE_BENCH_WEAK const bench_suite_t* bench_get_suites(size_t* out_count);

// -----------------------------------------------------------------------------
// Blackhole (optional)
// -----------------------------------------------------------------------------
// Suites frequently need to prevent dead-code elimination. The runner may
// provide these symbols. If not present, suites can define their own.

VITTE_BENCH_WEAK void bench_blackhole_u64(uint64_t v);
VITTE_BENCH_WEAK void bench_blackhole_bytes(const void* p, size_t n);

// Safe fallback inline implementations, used only if the runner does not
// provide strong definitions.
static inline void bench_blackhole_u64_fallback(uint64_t v)
{
    // Volatile XOR accumulator.
    static volatile uint64_t sink;
    sink ^= (v + 0x9e3779b97f4a7c15ULL) ^ (sink << 7) ^ (sink >> 3);
}

static inline void bench_blackhole_bytes_fallback(const void* p, size_t n)
{
    static volatile uint8_t sink;
    const uint8_t* b = (const uint8_t*)p;
    uint8_t acc = sink;
    if (n)
    {
        acc ^= b[0];
        acc ^= b[n >> 1];
        acc ^= b[n - 1];
    }
    sink = (uint8_t)(acc + 0x2B);
}

static inline void bench_blackhole_u64_safe(uint64_t v)
{
    if (bench_blackhole_u64)
        bench_blackhole_u64(v);
    else
        bench_blackhole_u64_fallback(v);
}

static inline void bench_blackhole_bytes_safe(const void* p, size_t n)
{
    if (bench_blackhole_bytes)
        bench_blackhole_bytes(p, n);
    else
        bench_blackhole_bytes_fallback(p, n);
}

#ifdef __cplusplus
} // extern "C"
#endif
