/*
  bench/assert.h

  Assertions and hard checks for the Vitte benchmark harness.

  Goals
  - Very low overhead in hot paths.
  - Actionable failures: file/line/function + expression + optional message.
  - Deterministic output (no libc assert formatting).
  - Works in C17 and is safe to include from C++.

  Policy
  - VITTE_ASSERT: debug-only by default (enabled when !NDEBUG).
  - VITTE_CHECK : enabled in all builds by default.

  Single-header implementation
  - This header provides declarations by default.
  - Define `VITTE_BENCH_ASSERT_IMPLEMENTATION` in exactly one .c/.cpp file
    before including this header to emit the global storage + implementations.

    Example:
      // bench_assert_impl.c
      #define VITTE_BENCH_ASSERT_IMPLEMENTATION
      #include "bench/assert.h"

  Dependencies
  - bench/detail/compat.h provides:
      - VITTE_FORCEINLINE, VITTE_NORETURN
      - VITTE_LIKELY/VITTE_UNLIKELY
      - VITTE_CC_* detection
      - vitte_bench_panic(), vitte_bench_snprintf(), vitte_bench_vsnprintf()
*/

#pragma once
#ifndef VITTE_BENCH_ASSERT_H
#define VITTE_BENCH_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#include "bench/detail/compat.h"

/* -------------------------------------------------------------------------- */
/* Build configuration                                                         */
/* -------------------------------------------------------------------------- */

/*
  VITTE_ASSERT_ENABLED
  - Defaults to enabled when !NDEBUG.
  - Can be forced on/off by defining VITTE_ASSERT_ENABLED to 1/0.
*/
#ifndef VITTE_ASSERT_ENABLED
  #if defined(NDEBUG)
    #define VITTE_ASSERT_ENABLED 0
  #else
    #define VITTE_ASSERT_ENABLED 1
  #endif
#endif

/*
  VITTE_CHECK_ENABLED
  - Checks that stay enabled in release builds.
  - Defaults to 1.
*/
#ifndef VITTE_CHECK_ENABLED
  #define VITTE_CHECK_ENABLED 1
#endif

/* Message buffer size for *_F macros. */
#ifndef VITTE_ASSERT_MSG_CAP
  #define VITTE_ASSERT_MSG_CAP 256
#endif

/* -------------------------------------------------------------------------- */
/* Hook                                                                         */
/* -------------------------------------------------------------------------- */

/*
  Hook signature invoked on failure.

  Notes
  - If you set a hook, it should not return.
  - If it returns, the default failure path executes.
*/
typedef void (*vitte_assert_hook_fn)(
    const char* expr,
    const char* msg,
    const char* file,
    int line,
    const char* func
);

/* Global hook storage (defined in exactly one TU when implementation is enabled). */
#if defined(VITTE_BENCH_ASSERT_IMPLEMENTATION)
  vitte_assert_hook_fn vitte__bench_assert_hook;
#else
  extern vitte_assert_hook_fn vitte__bench_assert_hook;
#endif

/* Get/set global assertion hook (process-wide). */
VITTE_FORCEINLINE vitte_assert_hook_fn vitte_assert_get_hook(void)
{
    return vitte__bench_assert_hook;
}

VITTE_FORCEINLINE void vitte_assert_set_hook(vitte_assert_hook_fn hook)
{
    vitte__bench_assert_hook = hook;
}

/* -------------------------------------------------------------------------- */
/* Failure path                                                                 */
/* -------------------------------------------------------------------------- */

/*
  Fail with (expr,msg,file,line,func).
  Always aborts.
*/
VITTE_NORETURN void vitte_assert_fail(
    const char* expr,
    const char* msg,
    const char* file,
    int line,
    const char* func
);

/* Fail with printf-style message (formatted into a fixed buffer). */
VITTE_NORETURN void vitte_assert_failf(
    const char* expr,
    const char* file,
    int line,
    const char* func,
    const char* fmt,
    ...
) VITTE_PRINTF_FMT(5, 6);

/* Implementation: emitted when VITTE_BENCH_ASSERT_IMPLEMENTATION is defined. */
#if defined(VITTE_BENCH_ASSERT_IMPLEMENTATION)

VITTE_NORETURN void vitte_assert_fail(
    const char* expr,
    const char* msg,
    const char* file,
    int line,
    const char* func
)
{
    const char* m = msg ? msg : "";
    /* Deterministic, single-line. */
    vitte_bench_panic("ASSERT: %s (%s:%d %s) %s", expr ? expr : "?", file ? file : "?", line, func ? func : "?", m);
}

VITTE_NORETURN void vitte_assert_failf(
    const char* expr,
    const char* file,
    int line,
    const char* func,
    const char* fmt,
    ...
)
{
    char buf[VITTE_ASSERT_MSG_CAP];
    buf[0] = '\0';

    if (fmt && fmt[0]) {
        va_list ap;
        va_start(ap, fmt);
        (void)vitte_bench_vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
    }

    vitte_assert_fail(expr, buf[0] ? buf : NULL, file, line, func);
}

#endif /* VITTE_BENCH_ASSERT_IMPLEMENTATION */

/* -------------------------------------------------------------------------- */
/* Internal helper used by macros                                              */
/* -------------------------------------------------------------------------- */

VITTE_FORCEINLINE void vitte_assert__maybe_fail(
    bool cond,
    const char* expr,
    const char* msg,
    const char* file,
    int line,
    const char* func
)
{
    if (VITTE_UNLIKELY(!cond)) {
        vitte_assert_hook_fn hook = vitte_assert_get_hook();
        if (hook) {
            hook(expr, msg, file, line, func);
        }
        vitte_assert_fail(expr, msg, file, line, func);
    }
}

/* -------------------------------------------------------------------------- */
/* Compiler hints / traps                                                      */
/* -------------------------------------------------------------------------- */

/*
  VITTE_TRAP
  - immediate termination primitive.
*/
#if VITTE_CC_MSVC
  #define VITTE_TRAP() __debugbreak()
#elif (VITTE_CC_CLANG || VITTE_CC_GCC)
  #define VITTE_TRAP() __builtin_trap()
#else
  #define VITTE_TRAP() do { *(volatile int*)0 = 0; } while (0)
#endif

/*
  VITTE_ASSUME
  - informs the compiler of an invariant.
  - in debug builds, also asserts.
*/
#if VITTE_CC_MSVC
  #define VITTE_ASSUME(expr) do { VITTE_ASSERT(expr); __assume(!!(expr)); } while (0)
#elif (VITTE_CC_CLANG || VITTE_CC_GCC)
  #define VITTE_ASSUME(expr) do { VITTE_ASSERT(expr); if (!(expr)) __builtin_unreachable(); } while (0)
#else
  #define VITTE_ASSUME(expr) VITTE_ASSERT(expr)
#endif

/*
  VITTE_UNREACHABLE
  - marks a code path as unreachable.
*/
#if VITTE_ASSERT_ENABLED
  #define VITTE_UNREACHABLE() \
    do { \
      vitte_assert__maybe_fail(false, "unreachable", NULL, __FILE__, (int)__LINE__, __func__); \
    } while (0)
#else
  #if VITTE_CC_MSVC
    #define VITTE_UNREACHABLE() __assume(0)
  #elif (VITTE_CC_CLANG || VITTE_CC_GCC)
    #define VITTE_UNREACHABLE() __builtin_unreachable()
  #else
    #define VITTE_UNREACHABLE() do { } while (0)
  #endif
#endif

/* -------------------------------------------------------------------------- */
/* Public macros                                                               */
/* -------------------------------------------------------------------------- */

/*
  VITTE_ASSERT / VITTE_ASSERT_MSG / VITTE_ASSERTF
  - Debug-only by default.
*/
#if VITTE_ASSERT_ENABLED
  #define VITTE_ASSERT(expr) \
    do { \
      vitte_assert__maybe_fail(!!(expr), #expr, NULL, __FILE__, (int)__LINE__, __func__); \
    } while (0)

  #define VITTE_ASSERT_MSG(expr, msg) \
    do { \
      vitte_assert__maybe_fail(!!(expr), #expr, (msg), __FILE__, (int)__LINE__, __func__); \
    } while (0)

  #define VITTE_ASSERTF(expr, fmt, ...) \
    do { \
      if (VITTE_UNLIKELY(!(expr))) { \
        vitte_assert_failf(#expr, __FILE__, (int)__LINE__, __func__, (fmt), __VA_ARGS__); \
      } \
    } while (0)
#else
  #define VITTE_ASSERT(expr) do { (void)sizeof(expr); } while (0)
  #define VITTE_ASSERT_MSG(expr, msg) do { (void)sizeof(expr); (void)(msg); } while (0)
  #define VITTE_ASSERTF(expr, fmt, ...) do { (void)sizeof(expr); } while (0)
#endif

/*
  VITTE_CHECK / VITTE_CHECK_MSG / VITTE_CHECKF
  - Intended to remain active in release builds.
*/
#if VITTE_CHECK_ENABLED
  #define VITTE_CHECK(expr) \
    do { \
      vitte_assert__maybe_fail(!!(expr), #expr, NULL, __FILE__, (int)__LINE__, __func__); \
    } while (0)

  #define VITTE_CHECK_MSG(expr, msg) \
    do { \
      vitte_assert__maybe_fail(!!(expr), #expr, (msg), __FILE__, (int)__LINE__, __func__); \
    } while (0)

  #define VITTE_CHECKF(expr, fmt, ...) \
    do { \
      if (VITTE_UNLIKELY(!(expr))) { \
        vitte_assert_failf(#expr, __FILE__, (int)__LINE__, __func__, (fmt), __VA_ARGS__); \
      } \
    } while (0)
#else
  #define VITTE_CHECK(expr) do { (void)sizeof(expr); } while (0)
  #define VITTE_CHECK_MSG(expr, msg) do { (void)sizeof(expr); (void)(msg); } while (0)
  #define VITTE_CHECKF(expr, fmt, ...) do { (void)sizeof(expr); } while (0)
#endif

/*
  VITTE_PANIC / VITTE_PANICF
  - unconditional failure.
*/
#define VITTE_PANIC(msg) \
  do { \
    vitte_assert_fail("panic", (msg), __FILE__, (int)__LINE__, __func__); \
  } while (0)

#define VITTE_PANICF(fmt, ...) \
  do { \
    vitte_assert_failf("panic", __FILE__, (int)__LINE__, __func__, (fmt), __VA_ARGS__); \
  } while (0)

/*
  VITTE_VERIFY
  - Like CHECK in debug, but evaluates expr in all builds.
  - Useful when expr has side-effects.
*/
#if VITTE_ASSERT_ENABLED
  #define VITTE_VERIFY(expr) VITTE_CHECK(expr)
#else
  #define VITTE_VERIFY(expr) do { (void)(expr); } while (0)
#endif

/* -------------------------------------------------------------------------- */
/* Static assertions                                                           */
/* -------------------------------------------------------------------------- */

#ifndef VITTE_STATIC_ASSERT
  #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    #define VITTE_STATIC_ASSERT(cond, msg) _Static_assert((cond), msg)
  #else
    #define VITTE_STATIC_ASSERT(cond, msg) typedef char vitte_static_assert__[(cond) ? 1 : -1]
  #endif
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_ASSERT_H */