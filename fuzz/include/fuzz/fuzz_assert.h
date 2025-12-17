// C:\Users\vince\Documents\GitHub\vitte\fuzz\include\fuzz\fuzz_assert.h
// fuzz_assert.h — assertions/utilities tailored for fuzzing (C17)
//
// Goals:
//  - deterministic crash on invariant violation
//  - minimal dependencies
//  - usable in libFuzzer/AFL/standalone harnesses
//  - keep messages short but structured

#ifndef VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_ASSERT_H
#define VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_ASSERT_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>   // size_t
#include <stdint.h>   // uint32_t, uint64_t
#include <stdio.h>    // fprintf
#include <stdlib.h>   // abort
#include <stdarg.h>   // va_list
#include <string.h>   // strlen

//------------------------------------------------------------------------------
// Compiler / platform helpers
//------------------------------------------------------------------------------

#if defined(_MSC_VER)
  #define FUZZ_INLINE __forceinline
  #define FUZZ_NORETURN __declspec(noreturn)
  #define FUZZ_LIKELY(x)   (x)
  #define FUZZ_UNLIKELY(x) (x)
  #define FUZZ_TRAP() __debugbreak()
#elif defined(__clang__) || defined(__GNUC__)
  #define FUZZ_INLINE inline __attribute__((always_inline))
  #define FUZZ_NORETURN __attribute__((noreturn))
  #define FUZZ_LIKELY(x)   __builtin_expect(!!(x), 1)
  #define FUZZ_UNLIKELY(x) __builtin_expect(!!(x), 0)
  #define FUZZ_TRAP() __builtin_trap()
#else
  #define FUZZ_INLINE inline
  #define FUZZ_NORETURN
  #define FUZZ_LIKELY(x)   (x)
  #define FUZZ_UNLIKELY(x) (x)
  #define FUZZ_TRAP() abort()
#endif

#if defined(__clang__) || defined(__GNUC__)
  #define FUZZ_UNREACHABLE_HINT() __builtin_unreachable()
#else
  #define FUZZ_UNREACHABLE_HINT() do { } while (0)
#endif

#ifndef FUZZ_ASSERT_ENABLED
  // For fuzzing you typically want asserts ON even in release.
  // Define FUZZ_ASSERT_ENABLED=0 to compile them out.
  #define FUZZ_ASSERT_ENABLED 1
#endif

//------------------------------------------------------------------------------
// Optional integration with sanitizers / coverage (no-op if unavailable)
//------------------------------------------------------------------------------

#if defined(__has_feature)
  #if __has_feature(address_sanitizer)
    #define FUZZ_HAS_ASAN 1
  #endif
  #if __has_feature(thread_sanitizer)
    #define FUZZ_HAS_TSAN 1
  #endif
  #if __has_feature(memory_sanitizer)
    #define FUZZ_HAS_MSAN 1
  #endif
  #if __has_feature(undefined_behavior_sanitizer)
    #define FUZZ_HAS_UBSAN 1
  #endif
#endif

// These symbols exist in sanitizer runtimes; optionally stub them during
// standalone builds that do not link sanitizer libs.
#if defined(FUZZ_DISABLE_SANITIZER_TRACE)
  FUZZ_INLINE static void __sanitizer_print_stack_trace(void) {
    // no-op stub for standalone builds
  }
#elif defined(__clang__) || defined(__GNUC__)
  __attribute__((weak)) void __sanitizer_print_stack_trace(void);
#endif

//------------------------------------------------------------------------------
// Crash policy
//------------------------------------------------------------------------------

typedef enum fuzz_crash_kind {
  FUZZ_CRASH_ASSERT = 1,
  FUZZ_CRASH_PANIC  = 2,
  FUZZ_CRASH_OOM    = 3,
  FUZZ_CRASH_BUG    = 4,
} fuzz_crash_kind;

// Custom crash hook: user can override by defining FUZZ_CRASH_HOOK
// or by linking their own fuzz_crash_hook implementation.
typedef void (*fuzz_crash_hook_fn)(
  fuzz_crash_kind kind,
  const char* file,
  int line,
  const char* func,
  const char* expr,
  const char* msg
);

#ifndef FUZZ_CRASH_HOOK
  #define FUZZ_CRASH_HOOK 0
#endif

#if (defined(__clang__) || defined(__GNUC__))
__attribute__((weak))
#endif
FUZZ_NORETURN void fuzz_crash_hook(
  fuzz_crash_kind kind,
  const char* file,
  int line,
  const char* func,
  const char* expr,
  const char* msg
);

//------------------------------------------------------------------------------
// Internal formatting helpers
//------------------------------------------------------------------------------

FUZZ_INLINE static void fuzz__vlog(FILE* out, const char* fmt, va_list ap) {
  (void)vfprintf(out, fmt, ap);
}

FUZZ_INLINE static void fuzz__log(FILE* out, const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fuzz__vlog(out, fmt, ap);
  va_end(ap);
}

FUZZ_INLINE static const char* fuzz__nonnull(const char* s) {
  return s ? s : "";
}

FUZZ_NORETURN FUZZ_INLINE static void fuzz__die(
  fuzz_crash_kind kind,
  const char* file,
  int line,
  const char* func,
  const char* expr,
  const char* msg
) {
#if FUZZ_CRASH_HOOK
  // If user wants an explicit hook macro, let it run first.
  // (They can still provide fuzz_crash_hook() too.)
  // NOLINTNEXTLINE(bugprone-macro-parentheses)
  FUZZ_CRASH_HOOK(kind, file, line, func, expr, msg);
#endif

  // Call weak hook if the user linked one.
  // If the default weak implementation is present, it will print+abort.
  fuzz_crash_hook(kind, file, line, func, expr, msg);
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

FUZZ_NORETURN FUZZ_INLINE static void fuzz_panic_at(
  const char* file,
  int line,
  const char* func,
  const char* msg
) {
  fuzz__die(FUZZ_CRASH_PANIC, file, line, func, NULL, msg);
}

FUZZ_NORETURN FUZZ_INLINE static void fuzz_bug_at(
  const char* file,
  int line,
  const char* func,
  const char* msg
) {
  fuzz__die(FUZZ_CRASH_BUG, file, line, func, NULL, msg);
}

FUZZ_NORETURN FUZZ_INLINE static void fuzz_oom_at(
  const char* file,
  int line,
  const char* func,
  const char* msg
) {
  fuzz__die(FUZZ_CRASH_OOM, file, line, func, NULL, msg);
}

// Assert with expression string.
FUZZ_NORETURN FUZZ_INLINE static void fuzz_assert_fail_at(
  const char* file,
  int line,
  const char* func,
  const char* expr,
  const char* msg
) {
  fuzz__die(FUZZ_CRASH_ASSERT, file, line, func, expr, msg);
}

// Unreachable helper.
FUZZ_NORETURN FUZZ_INLINE static void fuzz_unreachable_at(
  const char* file,
  int line,
  const char* func,
  const char* msg
) {
  fuzz__die(FUZZ_CRASH_BUG, file, line, func, "unreachable", msg);
}

// Convenience macros (capture file/line/func)
#ifndef FUZZ_FUNC
  #if defined(__GNUC__) || defined(__clang__)
    #define FUZZ_FUNC __PRETTY_FUNCTION__
  #elif defined(_MSC_VER)
    #define FUZZ_FUNC __FUNCSIG__
  #else
    #define FUZZ_FUNC __func__
  #endif
#endif

#define FUZZ_PANIC(msg) \
  fuzz_panic_at(__FILE__, (int)__LINE__, FUZZ_FUNC, (msg))

#define FUZZ_BUG(msg) \
  fuzz_bug_at(__FILE__, (int)__LINE__, FUZZ_FUNC, (msg))

#define FUZZ_OOM(msg) \
  fuzz_oom_at(__FILE__, (int)__LINE__, FUZZ_FUNC, (msg))

#define FUZZ_UNREACHABLE_MSG(msg) \
  do { \
    fuzz_unreachable_at(__FILE__, (int)__LINE__, FUZZ_FUNC, (msg)); \
    FUZZ_UNREACHABLE_HINT(); \
  } while (0)

#define FUZZ_UNREACHABLE() \
  do { \
    fuzz_unreachable_at(__FILE__, (int)__LINE__, FUZZ_FUNC, NULL); \
    FUZZ_UNREACHABLE_HINT(); \
  } while (0)

// Assertions
#if FUZZ_ASSERT_ENABLED
  #define FUZZ_ASSERT(expr) \
    do { \
      if (FUZZ_UNLIKELY(!(expr))) { \
        fuzz_assert_fail_at(__FILE__, (int)__LINE__, FUZZ_FUNC, #expr, NULL); \
      } \
    } while (0)

  #define FUZZ_ASSERT_MSG(expr, msg) \
    do { \
      if (FUZZ_UNLIKELY(!(expr))) { \
        fuzz_assert_fail_at(__FILE__, (int)__LINE__, FUZZ_FUNC, #expr, (msg)); \
      } \
    } while (0)

  // Like assert but intended for input validation in harnesses.
  #define FUZZ_CHECK(expr) FUZZ_ASSERT(expr)
  #define FUZZ_CHECK_MSG(expr, msg) FUZZ_ASSERT_MSG(expr, msg)
#else
  #define FUZZ_ASSERT(expr)        do { (void)sizeof(expr); } while (0)
  #define FUZZ_ASSERT_MSG(expr, m) do { (void)sizeof(expr); (void)(m); } while (0)
  #define FUZZ_CHECK(expr)         do { (void)sizeof(expr); } while (0)
  #define FUZZ_CHECK_MSG(expr, m)  do { (void)sizeof(expr); (void)(m); } while (0)
#endif

//------------------------------------------------------------------------------
// Default weak hook implementation
//------------------------------------------------------------------------------

#if (defined(__clang__) || defined(__GNUC__))
__attribute__((weak))
#endif
FUZZ_NORETURN void fuzz_crash_hook(
  fuzz_crash_kind kind,
  const char* file,
  int line,
  const char* func,
  const char* expr,
  const char* msg
) {
  const char* k = "CRASH";
  switch (kind) {
    case FUZZ_CRASH_ASSERT: k = "ASSERT"; break;
    case FUZZ_CRASH_PANIC:  k = "PANIC";  break;
    case FUZZ_CRASH_OOM:    k = "OOM";    break;
    case FUZZ_CRASH_BUG:    k = "BUG";    break;
    default: break;
  }

  // Minimal structured log (stderr); keep it stable for triage.
  fuzz__log(stderr, "[vitte:fuzz:%s] %s:%d\n", k, fuzz__nonnull(file), line);
  fuzz__log(stderr, "  func: %s\n", fuzz__nonnull(func));
  if (expr && expr[0]) fuzz__log(stderr, "  expr: %s\n", expr);
  if (msg && msg[0])   fuzz__log(stderr, "  msg : %s\n", msg);

#if defined(__clang__) || defined(__GNUC__)
  if (&__sanitizer_print_stack_trace) {
    __sanitizer_print_stack_trace();
  }
#endif

  // Prefer trap for better signal in fuzzers; fallback to abort.
#if defined(_MSC_VER)
  __debugbreak();
  abort();
#elif defined(__clang__) || defined(__GNUC__)
  FUZZ_TRAP();
  abort();
#else
  abort();
#endif
}

//------------------------------------------------------------------------------
// Small helpers commonly needed in fuzz targets
//------------------------------------------------------------------------------

FUZZ_INLINE static int fuzz_is_ascii_printable(unsigned char c) {
  return (c >= 0x20u && c <= 0x7Eu);
}

FUZZ_INLINE static size_t fuzz_strnlen_s(const char* s, size_t max_len) {
  if (!s) return 0;
  for (size_t i = 0; i < max_len; ++i) {
    if (s[i] == '\0') return i;
  }
  return max_len;
}

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_ASSERT_H
