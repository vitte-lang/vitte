

// platform.h - small cross-platform helpers for vitte/bench (C17)
//
// Provides:
//   - BENCH_OS_* and BENCH_ARCH_* detection macros
//   - BENCH_FORCE_INLINE / BENCH_NO_INLINE / BENCH_LIKELY / BENCH_UNLIKELY
//   - TSC / high-resolution time helpers (best-effort)
//   - CPU yield / pause primitive
//   - Cache line size macro (best-effort)
//
// SPDX-License-Identifier: MIT

#ifndef VITTE_BENCH_PLATFORM_H
#define VITTE_BENCH_PLATFORM_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

// -----------------------------------------------------------------------------
// OS detection
// -----------------------------------------------------------------------------

#if defined(_WIN32) || defined(_WIN64)
  #define BENCH_OS_WINDOWS 1
#else
  #define BENCH_OS_WINDOWS 0
#endif

#if defined(__APPLE__) && defined(__MACH__)
  #define BENCH_OS_APPLE 1
#else
  #define BENCH_OS_APPLE 0
#endif

#if defined(__linux__)
  #define BENCH_OS_LINUX 1
#else
  #define BENCH_OS_LINUX 0
#endif

#if defined(__unix__) || defined(__unix)
  #define BENCH_OS_UNIX 1
#else
  #define BENCH_OS_UNIX 0
#endif

// -----------------------------------------------------------------------------
// Arch detection
// -----------------------------------------------------------------------------

#if defined(__x86_64__) || defined(_M_X64)
  #define BENCH_ARCH_X86_64 1
#else
  #define BENCH_ARCH_X86_64 0
#endif

#if defined(__i386__) || defined(_M_IX86)
  #define BENCH_ARCH_X86_32 1
#else
  #define BENCH_ARCH_X86_32 0
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
  #define BENCH_ARCH_ARM64 1
#else
  #define BENCH_ARCH_ARM64 0
#endif

#if defined(__arm__) || defined(_M_ARM)
  #define BENCH_ARCH_ARM32 1
#else
  #define BENCH_ARCH_ARM32 0
#endif

#if defined(__riscv) || defined(__riscv__)
  #define BENCH_ARCH_RISCV 1
#else
  #define BENCH_ARCH_RISCV 0
#endif

#if defined(__ppc64__) || defined(__powerpc64__)
  #define BENCH_ARCH_PPC64 1
#else
  #define BENCH_ARCH_PPC64 0
#endif

#if BENCH_OS_WINDOWS
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#elif BENCH_OS_UNIX || BENCH_OS_LINUX || BENCH_OS_APPLE
  #include <sched.h>
#endif

// -----------------------------------------------------------------------------
// Compiler / attributes
// -----------------------------------------------------------------------------

#if defined(_MSC_VER)
  #define BENCH_COMPILER_MSVC 1
#else
  #define BENCH_COMPILER_MSVC 0
#endif

#if defined(__clang__)
  #define BENCH_COMPILER_CLANG 1
#else
  #define BENCH_COMPILER_CLANG 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
  #define BENCH_COMPILER_GCC 1
#else
  #define BENCH_COMPILER_GCC 0
#endif

#if BENCH_COMPILER_MSVC
  #define BENCH_FORCE_INLINE __forceinline
  #define BENCH_NO_INLINE __declspec(noinline)
#elif BENCH_COMPILER_CLANG || BENCH_COMPILER_GCC
  #define BENCH_FORCE_INLINE __attribute__((always_inline)) inline
  #define BENCH_NO_INLINE __attribute__((noinline))
#else
  #define BENCH_FORCE_INLINE inline
  #define BENCH_NO_INLINE
#endif

#if (BENCH_COMPILER_CLANG || BENCH_COMPILER_GCC)
  #define BENCH_LIKELY(x)   __builtin_expect(!!(x), 1)
  #define BENCH_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
  #define BENCH_LIKELY(x)   (x)
  #define BENCH_UNLIKELY(x) (x)
#endif

#if BENCH_COMPILER_MSVC
  #define BENCH_ALIGN(N) __declspec(align(N))
#elif (BENCH_COMPILER_CLANG || BENCH_COMPILER_GCC)
  #define BENCH_ALIGN(N) __attribute__((aligned(N)))
#else
  #define BENCH_ALIGN(N)
#endif

// -----------------------------------------------------------------------------
// Cache line size (best-effort)
// -----------------------------------------------------------------------------

#ifndef BENCH_CACHELINE
  #if BENCH_ARCH_X86_64 || BENCH_ARCH_X86_32 || BENCH_ARCH_ARM64 || BENCH_ARCH_ARM32
    #define BENCH_CACHELINE 64
  #else
    #define BENCH_CACHELINE 64
  #endif
#endif

// -----------------------------------------------------------------------------
// CPU pause / yield
// -----------------------------------------------------------------------------

BENCH_FORCE_INLINE void bench_cpu_pause(void)
{
#if BENCH_ARCH_X86_64 || BENCH_ARCH_X86_32
  #if BENCH_COMPILER_MSVC
    #include <immintrin.h>
    _mm_pause();
  #else
    __asm__ __volatile__("pause" ::: "memory");
  #endif
#elif BENCH_ARCH_ARM64 || BENCH_ARCH_ARM32
  #if defined(__aarch64__) || defined(__arm__)
    __asm__ __volatile__("yield" ::: "memory");
  #else
    (void)0;
  #endif
#else
    (void)0;
#endif
}

BENCH_FORCE_INLINE void bench_thread_yield(void)
{
#if BENCH_OS_WINDOWS
    Sleep(0);
#elif BENCH_OS_UNIX || BENCH_OS_LINUX || BENCH_OS_APPLE
    (void)sched_yield();
#else
    bench_cpu_pause();
#endif
}

// -----------------------------------------------------------------------------
// High-resolution monotonic time (ns)
// -----------------------------------------------------------------------------

// Returns monotonic time in nanoseconds when available; otherwise returns 0.
uint64_t bench_time_now_ns(void);

// Returns monotonic time in milliseconds.
BENCH_FORCE_INLINE uint64_t bench_time_now_ms(void)
{
    return bench_time_now_ns() / 1000000ull;
}

// -----------------------------------------------------------------------------
// Cycle counter (TSC-like) (best-effort)
// -----------------------------------------------------------------------------

// Returns a fast cycle counter when available; otherwise falls back to time.
uint64_t bench_cycles_now(void);

// Estimate cycles-per-second if possible; otherwise returns 0.
uint64_t bench_cycles_per_sec(void);

// -----------------------------------------------------------------------------
// Implementation
// -----------------------------------------------------------------------------

#ifdef BENCH_PLATFORM_IMPLEMENTATION

#if BENCH_OS_WINDOWS
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#elif BENCH_OS_APPLE
  #include <time.h>
  #include <mach/mach_time.h>
#elif BENCH_OS_LINUX || BENCH_OS_UNIX
  #include <time.h>
#endif

uint64_t bench_time_now_ns(void)
{
#if BENCH_OS_WINDOWS
    static LARGE_INTEGER freq = {0};
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    // ns = counter * 1e9 / freq
    return (uint64_t)((__int128)c.QuadPart * 1000000000ull / (uint64_t)freq.QuadPart);
#elif BENCH_OS_APPLE
    // mach_absolute_time with timebase
    static mach_timebase_info_data_t tb = {0, 0};
    if (tb.denom == 0) (void)mach_timebase_info(&tb);
    uint64_t t = mach_absolute_time();
    // to ns: t * numer / denom
    return (uint64_t)((__uint128_t)t * (uint64_t)tb.numer / (uint64_t)tb.denom);
#elif BENCH_OS_LINUX || BENCH_OS_UNIX
    struct timespec ts;
  #if defined(CLOCK_MONOTONIC_RAW)
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) return 0;
  #else
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
  #endif
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#else
    return 0;
#endif
}

uint64_t bench_cycles_now(void)
{
#if (BENCH_ARCH_X86_64 || BENCH_ARCH_X86_32)
  #if BENCH_COMPILER_MSVC
    #include <intrin.h>
    return (uint64_t)__rdtsc();
  #elif BENCH_COMPILER_GCC || BENCH_COMPILER_CLANG
    unsigned int hi = 0, lo = 0;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | (uint64_t)lo;
  #else
    return bench_time_now_ns();
  #endif
#elif BENCH_ARCH_ARM64
    // cntvct_el0 is available on AArch64 when enabled by kernel/hw.
  #if (BENCH_COMPILER_GCC || BENCH_COMPILER_CLANG)
    uint64_t v = 0;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(v));
    return v;
  #else
    return bench_time_now_ns();
  #endif
#else
    return bench_time_now_ns();
#endif
}

uint64_t bench_cycles_per_sec(void)
{
#if BENCH_ARCH_ARM64
  #if (BENCH_COMPILER_GCC || BENCH_COMPILER_CLANG)
    uint64_t f = 0;
    __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(f));
    return f;
  #else
    return 0;
  #endif
#else
    // For x86 TSC, portable CPS estimation is non-trivial without OS APIs.
    // Return 0 (caller can ignore or estimate via calibration).
    return 0;
#endif
}

#endif // BENCH_PLATFORM_IMPLEMENTATION

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_BENCH_PLATFORM_H
