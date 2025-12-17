#pragma once

/*
  common.h

  Common utilities and macros used across benchmarks.

  Goals:
    - C17
    - Works on GCC/Clang/MSVC
    - Minimal dependencies
    - Provide portable optimization barriers + small helper macros
*/

#ifndef VITTE_BENCH_COMMON_H
#define VITTE_BENCH_COMMON_H

#include "config.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/* Compiler / platform detection                                              */
/* -------------------------------------------------------------------------- */

#if defined(_MSC_VER)
  #define VITTE_BENCH_MSVC 1
#else
  #define VITTE_BENCH_MSVC 0
#endif

#if defined(__clang__)
  #define VITTE_BENCH_CLANG 1
#else
  #define VITTE_BENCH_CLANG 0
#endif

#if defined(__GNUC__) && !VITTE_BENCH_CLANG
  #define VITTE_BENCH_GCC 1
#else
  #define VITTE_BENCH_GCC 0
#endif

/* -------------------------------------------------------------------------- */
/* Attributes / hints                                                         */
/* -------------------------------------------------------------------------- */

#if VITTE_BENCH_MSVC
  #define BENCH_INLINE __forceinline
  #define BENCH_NOINLINE __declspec(noinline)
  #define BENCH_ALIGNAS(N) __declspec(align(N))
  #define BENCH_UNUSED(x) (void)(x)
#elif VITTE_BENCH_GCC || VITTE_BENCH_CLANG
  #define BENCH_INLINE inline __attribute__((always_inline))
  #define BENCH_NOINLINE __attribute__((noinline))
  #define BENCH_ALIGNAS(N) __attribute__((aligned(N)))
  #define BENCH_UNUSED(x) (void)(x)
#else
  #define BENCH_INLINE inline
  #define BENCH_NOINLINE
  #define BENCH_ALIGNAS(N)
  #define BENCH_UNUSED(x) (void)(x)
#endif

#ifndef VITTE_BENCH_LIKELY
  #if (VITTE_BENCH_GCC || VITTE_BENCH_CLANG)
    #define VITTE_BENCH_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define VITTE_BENCH_UNLIKELY(x) __builtin_expect(!!(x), 0)
  #else
    #define VITTE_BENCH_LIKELY(x)   (x)
    #define VITTE_BENCH_UNLIKELY(x) (x)
  #endif
#endif

#ifndef BENCH_CACHELINE_SIZE
  #define BENCH_CACHELINE_SIZE 64u
#endif

/* -------------------------------------------------------------------------- */
/* Optimization barriers                                                      */
/* -------------------------------------------------------------------------- */

/*
  DO_NOT_OPTIMIZE:
    Prevents the compiler from assuming x is unused / constant.

  Notes:
    - GCC/Clang: uses inline asm.
    - MSVC: uses a volatile sink + compiler barrier.
*/

#if (VITTE_BENCH_GCC || VITTE_BENCH_CLANG)
  #define DO_NOT_OPTIMIZE(x) __asm__ volatile("" : "+r,m"(x) : : "memory")
  #define COMPILER_BARRIER() __asm__ volatile("" : : : "memory")
#elif VITTE_BENCH_MSVC
  #include <intrin.h>
  BENCH_NOINLINE static void vitte_bench__do_not_optimize_ptr_(const void* p) {
    static volatile const void* sink;
    sink = p;
    _ReadWriteBarrier();
  }
  BENCH_NOINLINE static void vitte_bench__do_not_optimize_u64_(uint64_t v) {
    static volatile uint64_t sink;
    sink ^= v;
    _ReadWriteBarrier();
  }
  #define DO_NOT_OPTIMIZE(x) do { \
    /* best-effort: route through u64 or pointer path */ \
    if (sizeof(x) <= sizeof(uint64_t)) { \
      uint64_t tmp__; \
      memcpy(&tmp__, &(x), sizeof(x)); \
      vitte_bench__do_not_optimize_u64_(tmp__); \
    } else { \
      vitte_bench__do_not_optimize_ptr_(&(x)); \
    } \
  } while (0)
  #define COMPILER_BARRIER() _ReadWriteBarrier()
#else
  /* Portable fallback (weaker): volatile touch */
  #define DO_NOT_OPTIMIZE(x) do { \
    volatile unsigned char* p__ = (volatile unsigned char*)&(x); \
    volatile unsigned char v__ = *p__; \
    (void)v__; \
  } while (0)
  #define COMPILER_BARRIER() do { } while (0)
#endif

/* Timing utilities */
typedef uint64_t bench_time_t;

/* -------------------------------------------------------------------------- */
/* Assertions                                                                 */
/* -------------------------------------------------------------------------- */

/*
  BENCH_ASSERT_RET:
    Assert helper suitable for benchmark functions returning an int error code.
*/
#define BENCH_ASSERT_RET(cond, msg, retcode) \
  do { \
    if (VITTE_BENCH_UNLIKELY(!(cond))) { \
      fprintf(stderr, "Assertion failed: %s (%s:%d)\n", (msg), __FILE__, __LINE__); \
      return (retcode); \
    } \
  } while (0)

/* Backward-compatible default */
#define BENCH_ASSERT(cond, msg) BENCH_ASSERT_RET((cond), (msg), 1)

/* -------------------------------------------------------------------------- */
/* Math helpers                                                               */
/* -------------------------------------------------------------------------- */

static BENCH_INLINE double safe_div(double num, double denom) {
  return (denom != 0.0) ? (num / denom) : 0.0;
}

/* Min/Max (beware side effects) */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/* Stringify */
#define STRINGIFY(x) #x
#define XSTRINGIFY(x) STRINGIFY(x)

#endif /* VITTE_BENCH_COMMON_H */
