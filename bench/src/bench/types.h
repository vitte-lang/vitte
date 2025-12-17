// types.h - common bench types (C17)
//
// Centralizes small, stable ABI types shared across the bench subsystem.
//
// SPDX-License-Identifier: MIT

#ifndef VITTE_BENCH_TYPES_H
#define VITTE_BENCH_TYPES_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// -----------------------------------------------------------------------------
// Function signature
// -----------------------------------------------------------------------------

// Default benchmark calling convention used by runner.c:
//   - ctx: user context pointer supplied at registration
//   - iters: number of iterations to execute
//
// Return codes:
//   0  : success
//   2  : skipped (runner treats as skipped)
//   <0 : failure
//
// If your project wants a different signature, you can keep bench_fn_t as an
// opaque pointer in the registry and override BENCH_FN_CALL in runner.c.

typedef int (*bench_fn_t)(void* ctx, int64_t iters);

// -----------------------------------------------------------------------------
// Standard return codes
// -----------------------------------------------------------------------------

enum
{
    BENCH_RC_OK = 0,
    BENCH_RC_SKIPPED = 2,
    BENCH_RC_FAIL = -1
};

// -----------------------------------------------------------------------------
// Small helpers/macros
// -----------------------------------------------------------------------------

#ifndef BENCH_UNUSED
#define BENCH_UNUSED(x) ((void)(x))
#endif

#ifndef BENCH_ARRAY_COUNT
#define BENCH_ARRAY_COUNT(a) ((int32_t)(sizeof(a) / sizeof((a)[0])))
#endif

#ifndef BENCH_STATIC_ASSERT
  #if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
    #define BENCH_STATIC_ASSERT(expr, msg) _Static_assert((expr), msg)
  #else
    #define BENCH_STATIC_ASSERT(expr, msg) typedef char bench_static_assert_##__LINE__[(expr) ? 1 : -1]
  #endif
#endif

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_BENCH_TYPES_H
