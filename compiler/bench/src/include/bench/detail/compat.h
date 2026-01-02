/*
  bench/detail/compat.h

  Compatibility layer for the Vitte benchmark runtime.

  Goals
  - Provide a thin, predictable portability surface across:
    - OS: Windows, Linux, macOS, *BSD, Solaris
    - Compilers: Clang, GCC, MSVC
    - Architectures: x86/x64, ARM64, others
  - Keep this header self-contained and C17-friendly.
  - Avoid heavy platform headers in public includes.

  This header is designed to be included by both C and C++ translation units.

  Notes
  - The implementation of OS-specific calls is expected to live in
    `bench/detail/compat.c` (or similar). This header provides:
      - feature detection macros
      - compiler attributes
      - safe helpers
      - API declarations for time/cpu introspection
*/

#pragma once
#ifndef VITTE_BENCH_DETAIL_COMPAT_H
#define VITTE_BENCH_DETAIL_COMPAT_H

/* -------------------------------------------------------------------------- */
/* Language linkage                                                            */
/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/* Includes                                                                    */
/* -------------------------------------------------------------------------- */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

/* -------------------------------------------------------------------------- */
/* Versioning                                                                  */
/* -------------------------------------------------------------------------- */

#define VITTE_BENCH_COMPAT_API_VERSION 1

/* -------------------------------------------------------------------------- */
/* Compiler detection                                                          */
/* -------------------------------------------------------------------------- */

#if defined(__clang__)
  #define VITTE_CC_CLANG 1
#else
  #define VITTE_CC_CLANG 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
  #define VITTE_CC_GCC 1
#else
  #define VITTE_CC_GCC 0
#endif

#if defined(_MSC_VER)
  #define VITTE_CC_MSVC 1
#else
  #define VITTE_CC_MSVC 0
#endif

#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif
#ifndef __has_attribute
  #define __has_attribute(x) 0
#endif
#ifndef __has_feature
  #define __has_feature(x) 0
#endif
#ifndef __has_include
  #define __has_include(x) 0
#endif

/* -------------------------------------------------------------------------- */
/* OS detection                                                                */
/* -------------------------------------------------------------------------- */

#if defined(_WIN32) || defined(_WIN64)
  #define VITTE_OS_WINDOWS 1
#else
  #define VITTE_OS_WINDOWS 0
#endif

#if defined(__APPLE__) && defined(__MACH__)
  #define VITTE_OS_APPLE 1
#else
  #define VITTE_OS_APPLE 0
#endif

#if defined(__linux__)
  #define VITTE_OS_LINUX 1
#else
  #define VITTE_OS_LINUX 0
#endif

#if defined(__FreeBSD__)
  #define VITTE_OS_FREEBSD 1
#else
  #define VITTE_OS_FREEBSD 0
#endif

#if defined(__NetBSD__)
  #define VITTE_OS_NETBSD 1
#else
  #define VITTE_OS_NETBSD 0
#endif

#if defined(__OpenBSD__)
  #define VITTE_OS_OPENBSD 1
#else
  #define VITTE_OS_OPENBSD 0
#endif

#if defined(__sun) && defined(__SVR4)
  #define VITTE_OS_SOLARIS 1
#else
  #define VITTE_OS_SOLARIS 0
#endif

#if VITTE_OS_FREEBSD || VITTE_OS_NETBSD || VITTE_OS_OPENBSD
  #define VITTE_OS_BSD 1
#else
  #define VITTE_OS_BSD 0
#endif

#if !VITTE_OS_WINDOWS
  #define VITTE_OS_POSIX 1
#else
  #define VITTE_OS_POSIX 0
#endif

/* -------------------------------------------------------------------------- */
/* Architecture detection                                                      */
/* -------------------------------------------------------------------------- */

#if defined(__x86_64__) || defined(_M_X64)
  #define VITTE_ARCH_X64 1
#else
  #define VITTE_ARCH_X64 0
#endif

#if defined(__i386__) || defined(_M_IX86)
  #define VITTE_ARCH_X86 1
#else
  #define VITTE_ARCH_X86 0
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
  #define VITTE_ARCH_ARM64 1
#else
  #define VITTE_ARCH_ARM64 0
#endif

#if defined(__arm__) || defined(_M_ARM)
  #define VITTE_ARCH_ARM32 1
#else
  #define VITTE_ARCH_ARM32 0
#endif

#if VITTE_ARCH_X64 || VITTE_ARCH_X86
  #define VITTE_ARCH_X86_FAMILY 1
#else
  #define VITTE_ARCH_X86_FAMILY 0
#endif

#if VITTE_ARCH_ARM64 || VITTE_ARCH_ARM32
  #define VITTE_ARCH_ARM_FAMILY 1
#else
  #define VITTE_ARCH_ARM_FAMILY 0
#endif

/* -------------------------------------------------------------------------- */
/* Endianness                                                                  */
/* -------------------------------------------------------------------------- */

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
  #if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    #define VITTE_LITTLE_ENDIAN 1
    #define VITTE_BIG_ENDIAN 0
  #elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #define VITTE_LITTLE_ENDIAN 0
    #define VITTE_BIG_ENDIAN 1
  #else
    #define VITTE_LITTLE_ENDIAN 0
    #define VITTE_BIG_ENDIAN 0
  #endif
#elif VITTE_OS_WINDOWS
  #define VITTE_LITTLE_ENDIAN 1
  #define VITTE_BIG_ENDIAN 0
#else
  #define VITTE_LITTLE_ENDIAN 0
  #define VITTE_BIG_ENDIAN 0
#endif

/* -------------------------------------------------------------------------- */
/* Attributes and helpers                                                      */
/* -------------------------------------------------------------------------- */

#if VITTE_CC_MSVC
  #define VITTE_FORCEINLINE __forceinline
  #define VITTE_NOINLINE    __declspec(noinline)
  #define VITTE_NORETURN    __declspec(noreturn)
  #define VITTE_ALIGNAS(x)  __declspec(align(x))
  #define VITTE_THREADLOCAL __declspec(thread)
#elif VITTE_CC_CLANG || VITTE_CC_GCC
  #define VITTE_FORCEINLINE inline __attribute__((always_inline))
  #define VITTE_NOINLINE    __attribute__((noinline))
  #define VITTE_NORETURN    __attribute__((noreturn))
  #define VITTE_ALIGNAS(x)  __attribute__((aligned(x)))
  #define VITTE_THREADLOCAL __thread
#else
  #define VITTE_FORCEINLINE inline
  #define VITTE_NOINLINE
  #define VITTE_NORETURN
  #define VITTE_ALIGNAS(x)
  #define VITTE_THREADLOCAL
#endif

#if (VITTE_CC_CLANG || VITTE_CC_GCC) && __has_attribute(warn_unused_result)
  #define VITTE_WUR __attribute__((warn_unused_result))
#else
  #define VITTE_WUR
#endif

#if (VITTE_CC_CLANG || VITTE_CC_GCC) && __has_attribute(format)
  #define VITTE_PRINTF_FMT(fmt_index, first_arg) __attribute__((format(printf, fmt_index, first_arg)))
#else
  #define VITTE_PRINTF_FMT(fmt_index, first_arg)
#endif

#if (VITTE_CC_CLANG || VITTE_CC_GCC) && __has_builtin(__builtin_expect)
  #define VITTE_LIKELY(x)   (__builtin_expect(!!(x), 1))
  #define VITTE_UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
  #define VITTE_LIKELY(x)   (x)
  #define VITTE_UNLIKELY(x) (x)
#endif

#if VITTE_CC_MSVC
  #define VITTE_DEBUGBREAK() __debugbreak()
#elif (VITTE_CC_CLANG || VITTE_CC_GCC) && (VITTE_ARCH_X86_FAMILY || VITTE_ARCH_ARM_FAMILY)
  #define VITTE_DEBUGBREAK() __builtin_trap()
#else
  #define VITTE_DEBUGBREAK() do { *(volatile int*)0 = 0; } while (0)
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
  #define VITTE_STATIC_ASSERT(expr, msg) _Static_assert((expr), msg)
#else
  #define VITTE_STATIC_ASSERT(expr, msg) typedef char vitte_static_assert_##__LINE__[(expr) ? 1 : -1]
#endif

#define VITTE_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

/* Safe min/max without multiple evaluation pitfalls for basic scalar use. */
#define VITTE_MIN(a, b) (( (a) < (b) ) ? (a) : (b))
#define VITTE_MAX(a, b) (( (a) > (b) ) ? (a) : (b))

/* -------------------------------------------------------------------------- */
/* Path conventions                                                             */
/* -------------------------------------------------------------------------- */

#if VITTE_OS_WINDOWS
  #define VITTE_PATH_SEP_CHAR '\\'
  #define VITTE_PATH_SEP_STR  "\\"
#else
  #define VITTE_PATH_SEP_CHAR '/'
  #define VITTE_PATH_SEP_STR  "/"
#endif

/* -------------------------------------------------------------------------- */
/* CPU cycle counter                                                            */
/* -------------------------------------------------------------------------- */

/*
  Return a best-effort monotonically increasing cycle counter.

  - On x86/x64: uses RDTSC when available.
  - On ARM64: uses cntvct_el0 when accessible.
  - Otherwise: returns 0.

  IMPORTANT: cycle counters are not necessarily synchronized across cores.
*/
uint64_t vitte_bench_cycles_now(void);

/* -------------------------------------------------------------------------- */
/* Time API                                                                     */
/* -------------------------------------------------------------------------- */

/*
  Return a monotonic timestamp in nanoseconds.

  Requirements
  - Monotonic (no wall-clock adjustments)
  - Best available resolution on the platform
*/
uint64_t vitte_bench_time_now_ns(void);

/*
  Sleep for at least `ns` nanoseconds.

  Best-effort behavior:
  - On most platforms the sleep granularity will be >= 1ms.
  - Returns 0 on success, non-zero on failure.
*/
int vitte_bench_sleep_ns(uint64_t ns);

/*
  Yield execution to the scheduler.
  Returns 0 on success, non-zero on failure.
*/
int vitte_bench_yield(void);

/* -------------------------------------------------------------------------- */
/* Process/Thread helpers                                                       */
/* -------------------------------------------------------------------------- */

/*
  Best-effort pin the current thread to a single CPU core.

  - Returns 0 on success, non-zero on failure.
  - If unsupported, returns non-zero.
*/
int vitte_bench_pin_to_cpu(uint32_t cpu_index);

/*
  Best-effort set a name for the current thread.
  Returns 0 on success, non-zero on failure.
*/
int vitte_bench_thread_set_name(const char* name);

/* -------------------------------------------------------------------------- */
/* Memory helpers                                                               */
/* -------------------------------------------------------------------------- */

/*
  Prefetch memory to caches (best-effort).
*/
VITTE_FORCEINLINE void vitte_bench_prefetch_ro(const void* p)
{
#if (VITTE_CC_CLANG || VITTE_CC_GCC) && __has_builtin(__builtin_prefetch)
    __builtin_prefetch(p, 0 /*read*/, 3 /*high locality*/);
#else
    (void)p;
#endif
}

VITTE_FORCEINLINE void vitte_bench_prefetch_rw(const void* p)
{
#if (VITTE_CC_CLANG || VITTE_CC_GCC) && __has_builtin(__builtin_prefetch)
    __builtin_prefetch(p, 1 /*write*/, 3 /*high locality*/);
#else
    (void)p;
#endif
}

/*
  Compiler barrier: prevent reordering across this point.
*/
VITTE_FORCEINLINE void vitte_bench_compiler_barrier(void)
{
#if VITTE_CC_MSVC
    _ReadWriteBarrier();
#elif (VITTE_CC_CLANG || VITTE_CC_GCC)
    __asm__ __volatile__("" ::: "memory");
#else
    /* best effort */
#endif
}

/*
  Full fence (best-effort): prevents compiler and CPU reordering.

  Note: if <stdatomic.h> is available, prefer atomic_thread_fence in C code.
*/
VITTE_FORCEINLINE void vitte_bench_full_fence(void)
{
#if VITTE_CC_MSVC
    /* MSVC intrinsic */
    long barrier = 0;
    (void)barrier;
    vitte_bench_compiler_barrier();
#elif (VITTE_CC_CLANG || VITTE_CC_GCC)
    __sync_synchronize();
#else
    vitte_bench_compiler_barrier();
#endif
}

/* -------------------------------------------------------------------------- */
/* Formatting helpers                                                           */
/* -------------------------------------------------------------------------- */

/*
  Safe snprintf wrapper:
  - returns number of chars that would have been written (like snprintf)
  - always NUL-terminates when size > 0
*/
size_t vitte_bench_snprintf(char* dst, size_t dst_size, const char* fmt, ...) VITTE_PRINTF_FMT(3, 4);

size_t vitte_bench_vsnprintf(char* dst, size_t dst_size, const char* fmt, va_list ap) VITTE_PRINTF_FMT(3, 0);

/* -------------------------------------------------------------------------- */
/* Diagnostics                                                                  */
/* -------------------------------------------------------------------------- */

/*
  Minimal panic hook for bench code.

  - Prints a message (best-effort) and aborts the process.
  - Intended for unrecoverable invariants in microbench harnesses.
*/
VITTE_NORETURN void vitte_bench_panic(const char* fmt, ...) VITTE_PRINTF_FMT(1, 2);

/* -------------------------------------------------------------------------- */
/* Compile-time sanity                                                          */
/* -------------------------------------------------------------------------- */

VITTE_STATIC_ASSERT(sizeof(uint8_t)  == 1, "u8 size");
VITTE_STATIC_ASSERT(sizeof(uint16_t) == 2, "u16 size");
VITTE_STATIC_ASSERT(sizeof(uint32_t) == 4, "u32 size");
VITTE_STATIC_ASSERT(sizeof(uint64_t) == 8, "u64 size");

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_DETAIL_COMPAT_H */
