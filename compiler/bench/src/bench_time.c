

// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// vitte/compiler/bench - time utilities
// -----------------------------------------------------------------------------
// Cross-platform monotonic clock helpers for benchmarking.
//
// Design goals:
//  - Monotonic: never goes backwards.
//  - High resolution when available.
//  - No dynamic allocation.
//  - Small API surface.
//
// Public API (linkable symbols):
//  - uint64_t vt_bench_time_now_ns(void)
//  - double   vt_bench_time_now_s(void)
//  - void     vt_bench_time_sleep_ms(uint32_t ms)
//  - uint64_t vt_bench_time_estimate_overhead_ns(uint32_t iters)
//  - uint64_t vt_bench_time_resolution_ns(void)
// -----------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#elif defined(__APPLE__)
  #include <time.h>
  #include <mach/mach_time.h>
  #include <unistd.h>
#else
  #include <time.h>
  #include <unistd.h>
  #include <errno.h>
#endif

// -----------------------------------------------------------------------------
// Internal: platform timebase
// -----------------------------------------------------------------------------

#if defined(_WIN32)

typedef struct vt__qpc_state {
    LARGE_INTEGER freq;
    int initialized;
} vt__qpc_state;

static vt__qpc_state g_vt__qpc;

static void vt__qpc_init(void)
{
    if (g_vt__qpc.initialized) return;
    QueryPerformanceFrequency(&g_vt__qpc.freq);
    g_vt__qpc.initialized = 1;
}

static uint64_t vt__qpc_now_ns(void)
{
    vt__qpc_init();

    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);

    // ns = (c * 1e9) / freq
    // Avoid overflow with 128-bit if available; otherwise split.
  #if defined(_MSC_VER) && defined(_M_X64)
    unsigned __int128 num = (unsigned __int128)(uint64_t)c.QuadPart * (unsigned __int128)1000000000ull;
    uint64_t ns = (uint64_t)(num / (uint64_t)g_vt__qpc.freq.QuadPart);
    return ns;
  #elif defined(__SIZEOF_INT128__)
    __uint128_t num = (__uint128_t)(uint64_t)c.QuadPart * (__uint128_t)1000000000ull;
    uint64_t ns = (uint64_t)(num / (uint64_t)g_vt__qpc.freq.QuadPart);
    return ns;
  #else
    // Fallback: do division in two steps.
    const uint64_t freq = (uint64_t)g_vt__qpc.freq.QuadPart;
    const uint64_t ticks = (uint64_t)c.QuadPart;
    const uint64_t sec = ticks / freq;
    const uint64_t rem = ticks % freq;
    return sec * 1000000000ull + (rem * 1000000000ull) / freq;
  #endif
}

static uint64_t vt__qpc_resolution_ns(void)
{
    vt__qpc_init();
    const uint64_t freq = (uint64_t)g_vt__qpc.freq.QuadPart;
    if (freq == 0) return 0;
    // Resolution is at best 1 tick.
    return (1000000000ull + (freq - 1)) / freq;
}

#elif defined(__APPLE__)

typedef struct vt__mach_state {
    mach_timebase_info_data_t tb;
    int initialized;
} vt__mach_state;

static vt__mach_state g_vt__mach;

static void vt__mach_init(void)
{
    if (g_vt__mach.initialized) return;
    (void)mach_timebase_info(&g_vt__mach.tb);
    if (g_vt__mach.tb.denom == 0) {
        g_vt__mach.tb.numer = 1;
        g_vt__mach.tb.denom = 1;
    }
    g_vt__mach.initialized = 1;
}

static uint64_t vt__mach_now_ns(void)
{
    vt__mach_init();

    const uint64_t t = mach_absolute_time();
    // ns = t * numer / denom
  #if defined(__SIZEOF_INT128__)
    __uint128_t num = (__uint128_t)t * (__uint128_t)g_vt__mach.tb.numer;
    uint64_t ns = (uint64_t)(num / (__uint128_t)g_vt__mach.tb.denom);
    return ns;
  #else
    // Fallback: reduce risk of overflow by dividing first if possible.
    const uint64_t numer = (uint64_t)g_vt__mach.tb.numer;
    const uint64_t denom = (uint64_t)g_vt__mach.tb.denom;
    if (denom == 0) return 0;
    const uint64_t q = t / denom;
    const uint64_t r = t % denom;
    return q * numer + (r * numer) / denom;
  #endif
}

static uint64_t vt__mach_resolution_ns(void)
{
    vt__mach_init();
    // Best-effort: 1 tick mapped through timebase.
    const uint64_t numer = (uint64_t)g_vt__mach.tb.numer;
    const uint64_t denom = (uint64_t)g_vt__mach.tb.denom;
    if (denom == 0) return 0;
    // ceil(numer/denom)
    return (numer + denom - 1) / denom;
}

#else

static uint64_t vt__posix_now_ns(void)
{
  #if defined(CLOCK_MONOTONIC)
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
  #else
    // Fallback: time() is not monotonic; return 0 to surface the limitation.
    return 0;
  #endif
}

static uint64_t vt__posix_resolution_ns(void)
{
  #if defined(CLOCK_MONOTONIC)
    struct timespec ts;
    if (clock_getres(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
  #else
    return 0;
  #endif
}

#endif

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

uint64_t vt_bench_time_now_ns(void)
{
#if defined(_WIN32)
    return vt__qpc_now_ns();
#elif defined(__APPLE__)
    return vt__mach_now_ns();
#else
    return vt__posix_now_ns();
#endif
}

double vt_bench_time_now_s(void)
{
    const uint64_t ns = vt_bench_time_now_ns();
    return (double)ns / 1000000000.0;
}

uint64_t vt_bench_time_resolution_ns(void)
{
#if defined(_WIN32)
    return vt__qpc_resolution_ns();
#elif defined(__APPLE__)
    return vt__mach_resolution_ns();
#else
    return vt__posix_resolution_ns();
#endif
}

void vt_bench_time_sleep_ms(uint32_t ms)
{
#if defined(_WIN32)
    Sleep((DWORD)ms);
#else
    // Sleep in a loop to handle signals.
    uint64_t ns = (uint64_t)ms * 1000000ull;
    struct timespec req;
    req.tv_sec = (time_t)(ns / 1000000000ull);
    req.tv_nsec = (long)(ns % 1000000000ull);

    for (;;) {
        struct timespec rem = {0};
        int rc = nanosleep(&req, &rem);
        if (rc == 0) break;
        // If interrupted, continue with remaining time.
        req = rem;
    }
#endif
}

// Estimate overhead of calling vt_bench_time_now_ns() by measuring iters calls
// and taking the minimum delta (mitigates scheduler noise).
uint64_t vt_bench_time_estimate_overhead_ns(uint32_t iters)
{
    if (iters == 0) iters = 1;

    // Warm-up
    (void)vt_bench_time_now_ns();
    (void)vt_bench_time_now_ns();

    uint64_t best = UINT64_MAX;

    // Run several trials; each trial measures a tight loop of `iters` calls.
    // Keep trials bounded to avoid long startup costs.
    const uint32_t trials = 32;

    for (uint32_t t = 0; t < trials; ++t) {
        const uint64_t a = vt_bench_time_now_ns();
        for (uint32_t i = 0; i < iters; ++i) {
            // Prevent the compiler from hoisting/merging calls.
#if defined(_MSC_VER)
            _ReadWriteBarrier();
#else
            __asm__ __volatile__("" ::: "memory");
#endif
            (void)vt_bench_time_now_ns();
        }
        const uint64_t b = vt_bench_time_now_ns();
        const uint64_t d = (b >= a) ? (b - a) : 0;
        if (d < best) best = d;
    }

    if (best == UINT64_MAX) return 0;

    // Per-call overhead: best delta divided by (iters + 2).
    // (+2) for the a/b timestamps which also call now_ns().
    const uint64_t denom = (uint64_t)iters + 2ull;
    return best / denom;
}
