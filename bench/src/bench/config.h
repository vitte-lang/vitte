#pragma once

/*
  config.h

  Benchmark configuration and feature detection.
*/

#ifndef VITTE_BENCH_CONFIG_H
#define VITTE_BENCH_CONFIG_H

#include <stdint.h>
#include <stddef.h>

/* Platform detection */
#if defined(__APPLE__)
  #define VITTE_BENCH_PLATFORM_MACOS 1
#elif defined(__linux__)
  #define VITTE_BENCH_PLATFORM_LINUX 1
#elif defined(_WIN32) || defined(_WIN64)
  #define VITTE_BENCH_PLATFORM_WINDOWS 1
#else
  #define VITTE_BENCH_PLATFORM_POSIX 1
#endif

/* Compiler detection */
#if defined(__clang__)
  #define VITTE_BENCH_COMPILER_CLANG 1
#elif defined(__GNUC__)
  #define VITTE_BENCH_COMPILER_GCC 1
#elif defined(_MSC_VER)
  #define VITTE_BENCH_COMPILER_MSVC 1
#endif

/* C standard compliance */
#if __STDC_VERSION__ >= 201112L
  #define VITTE_BENCH_C11 1
#endif

/* Alignment macros */
#if defined(__GNUC__) || defined(__clang__)
  #define VITTE_BENCH_ALIGNED(N) __attribute__((aligned(N)))
#elif defined(_MSC_VER)
  #define VITTE_BENCH_ALIGNED(N) __declspec(align(N))
#else
  #define VITTE_BENCH_ALIGNED(N)
#endif

/* Likely/Unlikely hints */
#if defined(__GNUC__) || defined(__clang__)
  #define VITTE_BENCH_LIKELY(x) __builtin_expect(!!(x), 1)
  #define VITTE_BENCH_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
  #define VITTE_BENCH_LIKELY(x) (x)
  #define VITTE_BENCH_UNLIKELY(x) (x)
#endif

/* Feature flags */
#ifndef VITTE_BENCH_EXTRA
  #define VITTE_BENCH_EXTRA 0
#endif

#ifndef VITTE_BENCH_EXPERIMENTAL
  #define VITTE_BENCH_EXPERIMENTAL 0
#endif

#ifndef VITTE_BENCH_USE_ASM_MEMCPY
  #define VITTE_BENCH_USE_ASM_MEMCPY 0
#endif

#ifndef VITTE_BENCH_USE_ASM_HASH
  #define VITTE_BENCH_USE_ASM_HASH 0
#endif

#endif /* VITTE_BENCH_CONFIG_H */
