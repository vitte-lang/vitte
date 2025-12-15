#ifndef VITTEC_INCLUDE_VITTEC_CONFIG_H
#define VITTEC_INCLUDE_VITTEC_CONFIG_H

/*
  config.h — vittec global build configuration (max)

  Purpose:
  - Centralize compile-time toggles and platform/compiler detection.
  - Provide stable defaults while allowing overrides via -D flags.

  Rule:
  - Every toggle is defined with #ifndef so projects can override.
  - This header must remain lightweight (no heavy includes).
*/

/* -------------------------------------------------------------------------
 * Versioning
 * ------------------------------------------------------------------------- */

#define VITTEC_CONFIG_API_VERSION 1u

/* Optional: project version (can be set by build system). */
#ifndef VITTEC_VERSION_MAJOR
  #define VITTEC_VERSION_MAJOR 0
#endif
#ifndef VITTEC_VERSION_MINOR
  #define VITTEC_VERSION_MINOR 0
#endif
#ifndef VITTEC_VERSION_PATCH
  #define VITTEC_VERSION_PATCH 0
#endif

/* -------------------------------------------------------------------------
 * Compiler detection
 * ------------------------------------------------------------------------- */

#if defined(_MSC_VER)
  #define VITTEC_COMPILER_MSVC 1
#else
  #define VITTEC_COMPILER_MSVC 0
#endif

#if defined(__clang__)
  #define VITTEC_COMPILER_CLANG 1
#else
  #define VITTEC_COMPILER_CLANG 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
  #define VITTEC_COMPILER_GCC 1
#else
  #define VITTEC_COMPILER_GCC 0
#endif

/* -------------------------------------------------------------------------
 * Platform / OS detection
 * ------------------------------------------------------------------------- */

#if defined(_WIN32) || defined(_WIN64)
  #define VITTEC_OS_WINDOWS 1
#else
  #define VITTEC_OS_WINDOWS 0
#endif

#if defined(__APPLE__) && defined(__MACH__)
  #define VITTEC_OS_DARWIN 1
#else
  #define VITTEC_OS_DARWIN 0
#endif

#if defined(__linux__)
  #define VITTEC_OS_LINUX 1
#else
  #define VITTEC_OS_LINUX 0
#endif

#if !VITTEC_OS_WINDOWS && !VITTEC_OS_DARWIN && !VITTEC_OS_LINUX
  #define VITTEC_OS_UNKNOWN 1
#else
  #define VITTEC_OS_UNKNOWN 0
#endif

/* Path separator helper */
#if VITTEC_OS_WINDOWS
  #define VITTEC_PATH_SEP '\\'
#else
  #define VITTEC_PATH_SEP '/'
#endif

/* -------------------------------------------------------------------------
 * Architecture detection
 * ------------------------------------------------------------------------- */

#if defined(__x86_64__) || defined(_M_X64)
  #define VITTEC_ARCH_X86_64 1
#else
  #define VITTEC_ARCH_X86_64 0
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
  #define VITTEC_ARCH_AARCH64 1
#else
  #define VITTEC_ARCH_AARCH64 0
#endif

#if defined(__i386__) || defined(_M_IX86)
  #define VITTEC_ARCH_X86 1
#else
  #define VITTEC_ARCH_X86 0
#endif

#if !VITTEC_ARCH_X86_64 && !VITTEC_ARCH_AARCH64 && !VITTEC_ARCH_X86
  #define VITTEC_ARCH_UNKNOWN 1
#else
  #define VITTEC_ARCH_UNKNOWN 0
#endif

/* Endianness (assume little unless explicitly known otherwise). */
#ifndef VITTEC_ENDIAN_LITTLE
  #if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
    #if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
      #define VITTEC_ENDIAN_LITTLE 0
      #define VITTEC_ENDIAN_BIG 1
    #else
      #define VITTEC_ENDIAN_LITTLE 1
      #define VITTEC_ENDIAN_BIG 0
    #endif
  #else
    #define VITTEC_ENDIAN_LITTLE 1
    #define VITTEC_ENDIAN_BIG 0
  #endif
#endif

/* -------------------------------------------------------------------------
 * Attributes / inline helpers
 * ------------------------------------------------------------------------- */

#if VITTEC_COMPILER_MSVC
  #define VITTEC_INLINE __forceinline
  #define VITTEC_NOINLINE __declspec(noinline)
  #define VITTEC_UNUSED
  #define VITTEC_RESTRICT __restrict
#else
  #define VITTEC_INLINE __attribute__((always_inline)) inline
  #define VITTEC_NOINLINE __attribute__((noinline))
  #define VITTEC_UNUSED __attribute__((unused))
  #define VITTEC_RESTRICT __restrict__
#endif

#if (VITTEC_COMPILER_CLANG || VITTEC_COMPILER_GCC)
  #define VITTEC_LIKELY(x)   __builtin_expect(!!(x), 1)
  #define VITTEC_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
  #define VITTEC_LIKELY(x)   (x)
  #define VITTEC_UNLIKELY(x) (x)
#endif

/* -------------------------------------------------------------------------
 * Build mode
 * ------------------------------------------------------------------------- */

/* Treat NDEBUG as release. */
#ifndef VITTEC_BUILD_DEBUG
  #if defined(NDEBUG)
    #define VITTEC_BUILD_DEBUG 0
  #else
    #define VITTEC_BUILD_DEBUG 1
  #endif
#endif

#ifndef VITTEC_ENABLE_ASSERT
  #define VITTEC_ENABLE_ASSERT (VITTEC_BUILD_DEBUG)
#endif

#ifndef VITTEC_ENABLE_TRACE
  #define VITTEC_ENABLE_TRACE 0
#endif

/* -------------------------------------------------------------------------
 * Feature toggles (modules)
 * ------------------------------------------------------------------------- */

/* JSON support (lexer/parser helpers, tools). */
#ifndef VITTEC_ENABLE_JSON
  #define VITTEC_ENABLE_JSON 1
#endif

/* Muffin (.muf) manifest parsing. */
#ifndef VITTEC_ENABLE_MUFFIN
  #define VITTEC_ENABLE_MUFFIN 1
#endif

/* ANSI colors in diagnostics (subject to runtime NO_COLOR / TTY). */
#ifndef VITTEC_ENABLE_COLOR
  #define VITTEC_ENABLE_COLOR 1
#endif

/* Emit JSON diagnostics (structured). */
#ifndef VITTEC_ENABLE_DIAG_JSON
  #define VITTEC_ENABLE_DIAG_JSON 1
#endif

/* Enable experimental features behind flags (must remain off by default). */
#ifndef VITTEC_ENABLE_EXPERIMENTAL
  #define VITTEC_ENABLE_EXPERIMENTAL 0
#endif

/* -------------------------------------------------------------------------
 * Backend toggles
 * ------------------------------------------------------------------------- */

/* C backend is the bootstrap backbone: keep enabled by default. */
#ifndef VITTEC_ENABLE_BACKEND_C
  #define VITTEC_ENABLE_BACKEND_C 1
#endif

/* VM backend (if/when present). */
#ifndef VITTEC_ENABLE_BACKEND_VM
  #define VITTEC_ENABLE_BACKEND_VM 0
#endif

/* Assembly fastpaths (memcpy/hash/utf8) — optional. */
#ifndef VITTEC_ENABLE_ASM
  #define VITTEC_ENABLE_ASM 0
#endif

/* Rust companion workspace (tools/libs) — optional; does not change C ABI. */
#ifndef VITTEC_ENABLE_RUST
  #define VITTEC_ENABLE_RUST 0
#endif

/* -------------------------------------------------------------------------
 * Safety limits (defaults)
 * ------------------------------------------------------------------------- */

#ifndef VITTEC_MAX_SOURCE_BYTES
  #define VITTEC_MAX_SOURCE_BYTES (128u * 1024u * 1024u)
#endif

#ifndef VITTEC_MAX_TOKEN_BYTES
  #define VITTEC_MAX_TOKEN_BYTES (1u * 1024u * 1024u)
#endif

#ifndef VITTEC_MAX_STRING_BYTES
  #define VITTEC_MAX_STRING_BYTES (16u * 1024u * 1024u)
#endif

#ifndef VITTEC_MAX_NESTING
  #define VITTEC_MAX_NESTING 4096u
#endif

/* -------------------------------------------------------------------------
 * Compatibility
 * ------------------------------------------------------------------------- */

/*
  Define VITTEC_NO_STDLIB if you are building in a freestanding environment.
  Most of vittec assumes a hosted C environment, so default is 0.
*/
#ifndef VITTEC_NO_STDLIB
  #define VITTEC_NO_STDLIB 0
#endif

#endif /* VITTEC_INCLUDE_VITTEC_CONFIG_H */
