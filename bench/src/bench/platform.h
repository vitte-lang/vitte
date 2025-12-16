#pragma once

/*
  platform.h

  Platform detection and abstraction macros.
*/

#ifndef VITTE_BENCH_PLATFORM_H
#define VITTE_BENCH_PLATFORM_H

/* OS Detection */
#if defined(_WIN32) || defined(_WIN64)
  #define VITTE_BENCH_OS_WINDOWS 1
  #define VITTE_BENCH_OS_NAME "Windows"
#elif defined(__APPLE__) && defined(__MACH__)
  #define VITTE_BENCH_OS_MACOS 1
  #define VITTE_BENCH_OS_NAME "macOS"
#elif defined(__linux__)
  #define VITTE_BENCH_OS_LINUX 1
  #define VITTE_BENCH_OS_NAME "Linux"
#elif defined(__unix__)
  #define VITTE_BENCH_OS_UNIX 1
  #define VITTE_BENCH_OS_NAME "Unix"
#else
  #define VITTE_BENCH_OS_UNKNOWN 1
  #define VITTE_BENCH_OS_NAME "Unknown"
#endif

/* Compiler Detection */
#if defined(__clang__)
  #define VITTE_BENCH_COMPILER_CLANG 1
  #define VITTE_BENCH_COMPILER_NAME "Clang"
  #define VITTE_BENCH_COMPILER_VERSION (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
  #define VITTE_BENCH_COMPILER_GCC 1
  #define VITTE_BENCH_COMPILER_NAME "GCC"
  #define VITTE_BENCH_COMPILER_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(_MSC_VER)
  #define VITTE_BENCH_COMPILER_MSVC 1
  #define VITTE_BENCH_COMPILER_NAME "MSVC"
  #define VITTE_BENCH_COMPILER_VERSION _MSC_VER
#else
  #define VITTE_BENCH_COMPILER_UNKNOWN 1
  #define VITTE_BENCH_COMPILER_NAME "Unknown"
#endif

/* Architecture Detection */
#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64)
  #define VITTE_BENCH_ARCH_X64 1
  #define VITTE_BENCH_ARCH_NAME "x86-64"
  #define VITTE_BENCH_POINTER_SIZE 8
#elif defined(__i386__) || defined(_M_IX86)
  #define VITTE_BENCH_ARCH_X86 1
  #define VITTE_BENCH_ARCH_NAME "x86"
  #define VITTE_BENCH_POINTER_SIZE 4
#elif defined(__aarch64__) || defined(_M_ARM64)
  #define VITTE_BENCH_ARCH_ARM64 1
  #define VITTE_BENCH_ARCH_NAME "ARM64"
  #define VITTE_BENCH_POINTER_SIZE 8
#elif defined(__arm__) || defined(_M_ARM)
  #define VITTE_BENCH_ARCH_ARM 1
  #define VITTE_BENCH_ARCH_NAME "ARM"
  #define VITTE_BENCH_POINTER_SIZE 4
#else
  #define VITTE_BENCH_ARCH_UNKNOWN 1
  #define VITTE_BENCH_ARCH_NAME "Unknown"
#endif

/* Inline Keyword */
#if defined(_MSC_VER)
  #define BENCH_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
  #define BENCH_INLINE inline __attribute__((always_inline))
#else
  #define BENCH_INLINE inline
#endif

/* Alignment */
#if defined(__GNUC__) || defined(__clang__)
  #define BENCH_ALIGNED(N) __attribute__((aligned(N)))
#elif defined(_MSC_VER)
  #define BENCH_ALIGNED(N) __declspec(align(N))
#else
  #define BENCH_ALIGNED(N)
#endif

/* Likely/Unlikely Hints */
#if defined(__GNUC__) || defined(__clang__)
  #define BENCH_LIKELY(x) __builtin_expect(!!(x), 1)
  #define BENCH_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
  #define BENCH_LIKELY(x) (x)
  #define BENCH_UNLIKELY(x) (x)
#endif

#endif /* VITTE_BENCH_PLATFORM_H */
