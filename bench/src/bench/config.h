

// config.h — build configuration for vitte/bench (C17)
//
// Centralized feature toggles and compile-time configuration.
//
// This header is included by common.h and can be included anywhere.
// Keep it dependency-free (only preprocessor).

#ifndef VITTE_BENCH_CONFIG_H
#define VITTE_BENCH_CONFIG_H

// -----------------------------------------------------------------------------
// Build mode
// -----------------------------------------------------------------------------

#if !defined(VITTE_BENCH_DEBUG)
  // Define to 1 for extra checks/logging in bench runtime.
  #define VITTE_BENCH_DEBUG 0
#endif

#if !defined(VITTE_BENCH_ENABLE_ASSERTS)
  #define VITTE_BENCH_ENABLE_ASSERTS 1
#endif

// -----------------------------------------------------------------------------
// Output defaults
// -----------------------------------------------------------------------------

#if !defined(VITTE_BENCH_DEFAULT_ITERS)
  #define VITTE_BENCH_DEFAULT_ITERS 500000ULL
#endif

#if !defined(VITTE_BENCH_DEFAULT_REPEATS)
  #define VITTE_BENCH_DEFAULT_REPEATS 5u
#endif

#if !defined(VITTE_BENCH_DEFAULT_WARMUP)
  #define VITTE_BENCH_DEFAULT_WARMUP 1u
#endif

#if !defined(VITTE_BENCH_DEFAULT_SIZE)
  #define VITTE_BENCH_DEFAULT_SIZE 64u
#endif

#if !defined(VITTE_BENCH_DEFAULT_SEED)
  #define VITTE_BENCH_DEFAULT_SEED 0x123456789ABCDEF0ULL
#endif

// -----------------------------------------------------------------------------
// Providers (registry)
// -----------------------------------------------------------------------------

// By default, optional providers are weak on GCC/Clang and auto-detected.
// On MSVC, weak is not portable; enable suites explicitly via these defines.

#if !defined(VITTE_BENCH_ENABLE_MICRO)
  #define VITTE_BENCH_ENABLE_MICRO 0
#endif

#if !defined(VITTE_BENCH_ENABLE_MACRO)
  #define VITTE_BENCH_ENABLE_MACRO 0
#endif

#if !defined(VITTE_BENCH_ENABLE_JSON)
  #define VITTE_BENCH_ENABLE_JSON 0
#endif

// -----------------------------------------------------------------------------
// Cache line size (best-effort default)
// -----------------------------------------------------------------------------

#if !defined(BENCH_CACHELINE_SIZE)
  #define BENCH_CACHELINE_SIZE 64u
#endif

// -----------------------------------------------------------------------------
// Time / cycles
// -----------------------------------------------------------------------------

// If you want to force-disable cycle counters and use only ns:
//   - define VITTE_BENCH_DISABLE_CYCLES to 1

#if !defined(VITTE_BENCH_DISABLE_CYCLES)
  #define VITTE_BENCH_DISABLE_CYCLES 0
#endif

// -----------------------------------------------------------------------------
// Allocator hooks
// -----------------------------------------------------------------------------

// Allow overriding malloc/free for benchmarks.
// Define these to custom functions/macros before including bench code.

#if !defined(VITTE_BENCH_MALLOC)
  #define VITTE_BENCH_MALLOC(sz) malloc((sz))
#endif

#if !defined(VITTE_BENCH_FREE)
  #define VITTE_BENCH_FREE(p) free((p))
#endif

// -----------------------------------------------------------------------------
// Platform specifics
// -----------------------------------------------------------------------------

// If you need to disable OS affinity/priority tweaks:
#if !defined(VITTE_BENCH_ENABLE_RUNTIME_TWEAKS)
  #define VITTE_BENCH_ENABLE_RUNTIME_TWEAKS 1
#endif

#endif // VITTE_BENCH_CONFIG_H