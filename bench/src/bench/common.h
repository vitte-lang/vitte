#pragma once

/*
  common.h

  Common utilities and macros used across benchmarks.
*/

#ifndef VITTE_BENCH_COMMON_H
#define VITTE_BENCH_COMMON_H

#include "config.h"
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Prevent optimizations of values */
#define DO_NOT_OPTIMIZE(x) \
  __asm__ volatile("" : "+r"(x) : : "memory")

/* Compiler barriers */
#define COMPILER_BARRIER() \
  __asm__ volatile("" : : : "memory")

/* Timing utilities */
typedef uint64_t bench_time_t;

/* Assertion macro */
#define BENCH_ASSERT(cond, msg) \
  do { \
    if (VITTE_BENCH_UNLIKELY(!(cond))) { \
      fprintf(stderr, "Assertion failed: %s (%s:%d)\n", msg, __FILE__, __LINE__); \
      return 1; \
    } \
  } while (0)

/* Safe division */
static inline double safe_div(double num, double denom) {
  return (denom != 0.0) ? (num / denom) : 0.0;
}

/* Min/Max */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Stringify */
#define STRINGIFY(x) #x
#define XSTRINGIFY(x) STRINGIFY(x)

#endif /* VITTE_BENCH_COMMON_H */
