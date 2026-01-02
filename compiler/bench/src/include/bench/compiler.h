/*
  bench/compiler.h

  Compiler/optimization and platform feature detection for the Vitte benchmark harness.

  Why
  - Bench code often needs:
      - consistent FORCEINLINE / NOINLINE / NORETURN
      - branch prediction hints
      - prefetch
      - alignment helpers
      - CPU pause/yield for spin loops
      - feature detection (SSE/AVX/NEON, etc.)
      - sanitizer / coverage toggles

  This header centralizes those primitives for the bench subsystem.

  Notes
  - This file is bench-scoped; the compiler project may have a broader `pal`.
  - This header is safe for C17 and C++.
*/

#pragma once
#ifndef VITTE_BENCH_COMPILER_H
#define VITTE_BENCH_COMPILER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/* -------------------------------------------------------------------------- */
/* Compiler identification                                                     */
/* -------------------------------------------------------------------------- */

#if defined(_MSC_VER)
  #define VITTE_BENCH_CC_MSVC 1
#else
  #define VITTE_BENCH_CC_MSVC 0
#endif

#if defined(__clang__)
  #define VITTE_BENCH_CC_CLANG 1
#else
  #define VITTE_BENCH_CC_CLANG 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
  #define VITTE_BENCH_CC_GCC 1
#else
  #define VITTE_BENCH_CC_GCC 0
#endif

/* -------------------------------------------------------------------------- */
/* Attributes                                                                  */
/* -------------------------------------------------------------------------- */

#if VITTE_BENCH_CC_MSVC
  #define VITTE_BENCH_FORCEINLINE __forceinline
  #define VITTE_BENCH_NOINLINE    __declspec(noinline)
  #define VITTE_BENCH_NORETURN    __declspec(noreturn)
  #define VITTE_BENCH_ALIGNAS(N)  __declspec(align(N))
  #define VITTE_BENCH_RESTRICT    __restrict
#else
  #define VITTE_BENCH_FORCEINLINE inline __attribute__((always_inline))
  #define VITTE_BENCH_NOINLINE    __attribute__((noinline))
  #define VITTE_BENCH_NORETURN    __attribute__((noreturn))
  #define VITTE_BENCH_ALIGNAS(N)  __attribute__((aligned(N)))
  #define VITTE_BENCH_RESTRICT    __restrict__
#endif

#if VITTE_BENCH_CC_MSVC
  #define VITTE_BENCH_UNUSED
#else
  #define VITTE_BENCH_UNUSED __attribute__((unused))
#endif

#if VITTE_BENCH_CC_MSVC
  #define VITTE_BENCH_LIKELY(x)   (x)
  #define VITTE_BENCH_UNLIKELY(x) (x)
#else
  #define VITTE_BENCH_LIKELY(x)   __builtin_expect(!!(x), 1)
  #define VITTE_BENCH_UNLIKELY(x) __builtin_expect(!!(x), 0)
#endif

/* printf-like format annotation */
#if !VITTE_BENCH_CC_MSVC
  #define VITTE_BENCH_PRINTF_FMT(fmt_idx, va_idx) __attribute__((format(printf, fmt_idx, va_idx)))
#else
  #define VITTE_BENCH_PRINTF_FMT(fmt_idx, va_idx)
#endif

/* -------------------------------------------------------------------------- */
/* Sanitizer / coverage detection                                              */
/* -------------------------------------------------------------------------- */

#if defined(__SANITIZE_ADDRESS__) || defined(__has_feature)
  #if defined(__has_feature)
    #if __has_feature(address_sanitizer)
      #define VITTE_BENCH_ASAN 1
    #else
      #define VITTE_BENCH_ASAN 0
    #endif
  #else
    #define VITTE_BENCH_ASAN 1
  #endif
#else
  #define VITTE_BENCH_ASAN 0
#endif

#if defined(__SANITIZE_THREAD__) || defined(__has_feature)
  #if defined(__has_feature)
    #if __has_feature(thread_sanitizer)
      #define VITTE_BENCH_TSAN 1
    #else
      #define VITTE_BENCH_TSAN 0
    #endif
  #else
    #define VITTE_BENCH_TSAN 1
  #endif
#else
  #define VITTE_BENCH_TSAN 0
#endif

#if defined(__SANITIZE_UNDEFINED__) || defined(__has_feature)
  #if defined(__has_feature)
    #if __has_feature(undefined_behavior_sanitizer)
      #define VITTE_BENCH_UBSAN 1
    #else
      #define VITTE_BENCH_UBSAN 0
    #endif
  #else
    #define VITTE_BENCH_UBSAN 1
  #endif
#else
  #define VITTE_BENCH_UBSAN 0
#endif

/* gcov/llvm-cov coverage builds */
#if defined(__COVERAGE__) || defined(__GCOV__) || defined(__llvm_profile_runtime)
  #define VITTE_BENCH_COVERAGE 1
#else
  #define VITTE_BENCH_COVERAGE 0
#endif

/* -------------------------------------------------------------------------- */
/* CPU feature macros (compile-time)                                           */
/* -------------------------------------------------------------------------- */

/* x86/x64 */
#if defined(__i386__) || defined(_M_IX86)
  #define VITTE_BENCH_ARCH_X86 1
#else
  #define VITTE_BENCH_ARCH_X86 0
#endif

#if defined(__x86_64__) || defined(_M_X64)
  #define VITTE_BENCH_ARCH_X64 1
#else
  #define VITTE_BENCH_ARCH_X64 0
#endif

/* arm */
#if defined(__arm__) || defined(_M_ARM)
  #define VITTE_BENCH_ARCH_ARM 1
#else
  #define VITTE_BENCH_ARCH_ARM 0
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
  #define VITTE_BENCH_ARCH_ARM64 1
#else
  #define VITTE_BENCH_ARCH_ARM64 0
#endif

/* SIMD */
#if VITTE_BENCH_ARCH_X86 || VITTE_BENCH_ARCH_X64
  #if defined(__SSE2__) || (VITTE_BENCH_CC_MSVC && (defined(_M_X64) || (_M_IX86_FP >= 2)))
    #define VITTE_BENCH_HAS_SSE2 1
  #else
    #define VITTE_BENCH_HAS_SSE2 0
  #endif

  #if defined(__AVX2__)
    #define VITTE_BENCH_HAS_AVX2 1
  #else
    #define VITTE_BENCH_HAS_AVX2 0
  #endif

  #if defined(__AVX512F__)
    #define VITTE_BENCH_HAS_AVX512F 1
  #else
    #define VITTE_BENCH_HAS_AVX512F 0
  #endif
#else
  #define VITTE_BENCH_HAS_SSE2 0
  #define VITTE_BENCH_HAS_AVX2 0
  #define VITTE_BENCH_HAS_AVX512F 0
#endif

#if VITTE_BENCH_ARCH_ARM || VITTE_BENCH_ARCH_ARM64
  #if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #define VITTE_BENCH_HAS_NEON 1
  #else
    #define VITTE_BENCH_HAS_NEON 0
  #endif
#else
  #define VITTE_BENCH_HAS_NEON 0
#endif

/* -------------------------------------------------------------------------- */
/* Low-level intrinsics                                                        */
/* -------------------------------------------------------------------------- */

/*
  Prefetch
  - rw: 0=read, 1=write
  - locality: 0..3, higher = keep in cache longer
*/
VITTE_BENCH_FORCEINLINE void vitte_bench_prefetch(const void* p, int rw, int locality)
{
#if (VITTE_BENCH_CC_CLANG || VITTE_BENCH_CC_GCC)
    __builtin_prefetch(p, rw, locality);
#else
    (void)p;
    (void)rw;
    (void)locality;
#endif
}

/*
  Pause / yield primitive for spin loops.
*/
VITTE_BENCH_FORCEINLINE void vitte_bench_cpu_relax(void)
{
#if (VITTE_BENCH_ARCH_X86 || VITTE_BENCH_ARCH_X64)
  #if VITTE_BENCH_CC_MSVC
    #include <immintrin.h>
    _mm_pause();
  #else
    __asm__ __volatile__("pause" ::: "memory");
  #endif
#elif (VITTE_BENCH_ARCH_ARM || VITTE_BENCH_ARCH_ARM64)
  #if (VITTE_BENCH_CC_CLANG || VITTE_BENCH_CC_GCC)
    __asm__ __volatile__("yield" ::: "memory");
  #else
    /* no-op */
  #endif
#else
    /* no-op */
#endif
}

/*
  Bit operations helpers (portable fallbacks).
*/
VITTE_BENCH_FORCEINLINE uint32_t vitte_bench_rotl32(uint32_t x, unsigned r)
{
    return (x << (r & 31u)) | (x >> ((32u - (r & 31u)) & 31u));
}

VITTE_BENCH_FORCEINLINE uint32_t vitte_bench_rotr32(uint32_t x, unsigned r)
{
    return (x >> (r & 31u)) | (x << ((32u - (r & 31u)) & 31u));
}

VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_rotl64(uint64_t x, unsigned r)
{
    return (x << (r & 63u)) | (x >> ((64u - (r & 63u)) & 63u));
}

VITTE_BENCH_FORCEINLINE uint64_t vitte_bench_rotr64(uint64_t x, unsigned r)
{
    return (x >> (r & 63u)) | (x << ((64u - (r & 63u)) & 63u));
}

/* -------------------------------------------------------------------------- */
/* Alignment helpers                                                           */
/* -------------------------------------------------------------------------- */

VITTE_BENCH_FORCEINLINE bool vitte_bench_is_pow2_u64(uint64_t x)
{
    return x && ((x & (x - 1u)) == 0u);
}

VITTE_BENCH_FORCEINLINE uintptr_t vitte_bench_align_up_uintptr(uintptr_t x, uintptr_t a)
{
    return (x + (a - 1u)) & ~(a - 1u);
}

VITTE_BENCH_FORCEINLINE void* vitte_bench_align_up_ptr(void* p, uintptr_t a)
{
    return (void*)vitte_bench_align_up_uintptr((uintptr_t)p, a);
}

VITTE_BENCH_FORCEINLINE size_t vitte_bench_align_up_size(size_t x, size_t a)
{
    return (size_t)vitte_bench_align_up_uintptr((uintptr_t)x, (uintptr_t)a);
}

/* -------------------------------------------------------------------------- */
/* Unreachable/assume/trap                                                     */
/* -------------------------------------------------------------------------- */

#if VITTE_BENCH_CC_MSVC
  #define VITTE_BENCH_TRAP() __debugbreak()
  #define VITTE_BENCH_UNREACHABLE() __assume(0)
  #define VITTE_BENCH_ASSUME(x) __assume(!!(x))
#elif (VITTE_BENCH_CC_CLANG || VITTE_BENCH_CC_GCC)
  #define VITTE_BENCH_TRAP() __builtin_trap()
  #define VITTE_BENCH_UNREACHABLE() __builtin_unreachable()
  #define VITTE_BENCH_ASSUME(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#else
  #define VITTE_BENCH_TRAP() do { *(volatile int*)0 = 0; } while (0)
  #define VITTE_BENCH_UNREACHABLE() do { } while (0)
  #define VITTE_BENCH_ASSUME(x) do { (void)(x); } while (0)
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_COMPILER_H */
