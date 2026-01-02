/*
  bench/platform.h

  Unified platform surface for the Vitte benchmark harness.

  Intent
  - Provide a single include that exposes the "platform layer" of bench.
  - Centralize OS/arch/compiler detection and small OS utilities.
  - Keep this header stable for downstream tools that embed the bench runner.

  Includes
  - bench/compiler.h : compiler/arch feature detection + low-level intrinsics.
  - bench/os.h       : OS helpers (pid/tid/sleep/env/cwd/temp/monotonic).
  - bench/cpu.h      : CPU probing + jitter mitigation hooks.
  - bench/path.h     : path manipulation (normalize/join/relativize).

  Policy
  - No filesystem backend is exposed here (see bench/detail/fs.h).
  - No dynamic allocation in the API, except arena-backed string helpers.

  Versioning
  - The `vitte_platform_desc` begins with (struct_size, struct_version) so callers
    can safely consume only the fields they know.
*/

#pragma once
#ifndef VITTE_BENCH_PLATFORM_H
#define VITTE_BENCH_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/compiler.h"
#include "bench/os.h"
#include "bench/cpu.h"
#include "bench/path.h"

/* -------------------------------------------------------------------------- */
/* Arch detection (coarse)                                                     */
/* -------------------------------------------------------------------------- */

#if defined(__x86_64__) || defined(_M_X64)
  #define VITTE_BENCH_ARCH_X64 1
#else
  #define VITTE_BENCH_ARCH_X64 0
#endif

#if defined(__i386__) || defined(_M_IX86)
  #define VITTE_BENCH_ARCH_X86 1
#else
  #define VITTE_BENCH_ARCH_X86 0
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
  #define VITTE_BENCH_ARCH_ARM64 1
#else
  #define VITTE_BENCH_ARCH_ARM64 0
#endif

#if defined(__arm__) || defined(_M_ARM)
  #define VITTE_BENCH_ARCH_ARM 1
#else
  #define VITTE_BENCH_ARCH_ARM 0
#endif

#if defined(__riscv)
  #define VITTE_BENCH_ARCH_RISCV 1
#else
  #define VITTE_BENCH_ARCH_RISCV 0
#endif

#if defined(__powerpc64__) || defined(__ppc64__)
  #define VITTE_BENCH_ARCH_PPC64 1
#else
  #define VITTE_BENCH_ARCH_PPC64 0
#endif

#if defined(__powerpc__) || defined(__ppc__)
  #define VITTE_BENCH_ARCH_PPC 1
#else
  #define VITTE_BENCH_ARCH_PPC 0
#endif

/* One-of summary */
#if VITTE_BENCH_ARCH_X64
  #define VITTE_BENCH_ARCH_NAME "x86_64"
#elif VITTE_BENCH_ARCH_X86
  #define VITTE_BENCH_ARCH_NAME "x86"
#elif VITTE_BENCH_ARCH_ARM64
  #define VITTE_BENCH_ARCH_NAME "aarch64"
#elif VITTE_BENCH_ARCH_ARM
  #define VITTE_BENCH_ARCH_NAME "arm"
#elif VITTE_BENCH_ARCH_RISCV
  #define VITTE_BENCH_ARCH_NAME "riscv"
#elif VITTE_BENCH_ARCH_PPC64
  #define VITTE_BENCH_ARCH_NAME "ppc64"
#elif VITTE_BENCH_ARCH_PPC
  #define VITTE_BENCH_ARCH_NAME "ppc"
#else
  #define VITTE_BENCH_ARCH_NAME "unknown"
#endif

/* -------------------------------------------------------------------------- */
/* OS name helper                                                              */
/* -------------------------------------------------------------------------- */

#if VITTE_BENCH_OS_WINDOWS
  #define VITTE_BENCH_OS_NAME "windows"
#elif VITTE_BENCH_OS_DARWIN
  #define VITTE_BENCH_OS_NAME "darwin"
#elif VITTE_BENCH_OS_LINUX
  #define VITTE_BENCH_OS_NAME "linux"
#elif VITTE_BENCH_OS_FREEBSD
  #define VITTE_BENCH_OS_NAME "freebsd"
#elif VITTE_BENCH_OS_NETBSD
  #define VITTE_BENCH_OS_NAME "netbsd"
#elif VITTE_BENCH_OS_OPENBSD
  #define VITTE_BENCH_OS_NAME "openbsd"
#elif VITTE_BENCH_OS_SOLARIS
  #define VITTE_BENCH_OS_NAME "solaris"
#else
  #define VITTE_BENCH_OS_NAME "unknown"
#endif

/* -------------------------------------------------------------------------- */
/* ABI / libc name helper                                                      */
/* -------------------------------------------------------------------------- */

#if VITTE_BENCH_OS_WINDOWS
  #if VITTE_BENCH_CC_MSVC
    #define VITTE_BENCH_ABI_NAME "msvc"
  #else
    #define VITTE_BENCH_ABI_NAME "mingw"
  #endif
#elif defined(__ANDROID__)
  #define VITTE_BENCH_ABI_NAME "android"
#elif defined(__APPLE__) && defined(__MACH__)
  #define VITTE_BENCH_ABI_NAME "apple"
#elif defined(__musl__)
  #define VITTE_BENCH_ABI_NAME "musl"
#elif defined(__GLIBC__)
  #define VITTE_BENCH_ABI_NAME "gnu"
#elif VITTE_BENCH_OS_BSD
  #define VITTE_BENCH_ABI_NAME "bsd"
#elif VITTE_BENCH_OS_SOLARIS
  #define VITTE_BENCH_ABI_NAME "solaris"
#else
  #define VITTE_BENCH_ABI_NAME "unknown"
#endif

/* -------------------------------------------------------------------------- */
/* Endianness                                                                  */
/* -------------------------------------------------------------------------- */

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
  #if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    #define VITTE_BENCH_ENDIAN_LITTLE 1
    #define VITTE_BENCH_ENDIAN_BIG    0
  #elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #define VITTE_BENCH_ENDIAN_LITTLE 0
    #define VITTE_BENCH_ENDIAN_BIG    1
  #else
    #define VITTE_BENCH_ENDIAN_LITTLE 0
    #define VITTE_BENCH_ENDIAN_BIG    0
  #endif
#elif defined(_WIN32)
  #define VITTE_BENCH_ENDIAN_LITTLE 1
  #define VITTE_BENCH_ENDIAN_BIG    0
#else
  /* Unknown at compile time */
  #define VITTE_BENCH_ENDIAN_LITTLE 0
  #define VITTE_BENCH_ENDIAN_BIG    0
#endif

#if VITTE_BENCH_ENDIAN_LITTLE
  #define VITTE_BENCH_ENDIAN_NAME "little"
#elif VITTE_BENCH_ENDIAN_BIG
  #define VITTE_BENCH_ENDIAN_NAME "big"
#else
  #define VITTE_BENCH_ENDIAN_NAME "unknown"
#endif

/* -------------------------------------------------------------------------- */
/* Compiler name helper                                                        */
/* -------------------------------------------------------------------------- */

#if VITTE_BENCH_CC_CLANG
  #define VITTE_BENCH_CC_NAME "clang"
#elif VITTE_BENCH_CC_GCC
  #define VITTE_BENCH_CC_NAME "gcc"
#elif VITTE_BENCH_CC_MSVC
  #define VITTE_BENCH_CC_NAME "msvc"
#else
  #define VITTE_BENCH_CC_NAME "unknown"
#endif

/* -------------------------------------------------------------------------- */
/* Build metadata (override from build system if desired)                      */
/* -------------------------------------------------------------------------- */

#ifndef VITTE_BENCH_BUILD_VERSION
  #define VITTE_BENCH_BUILD_VERSION "0"
#endif

#ifndef VITTE_BENCH_BUILD_GIT_SHA
  #define VITTE_BENCH_BUILD_GIT_SHA "unknown"
#endif

#ifndef VITTE_BENCH_BUILD_DATE
  #define VITTE_BENCH_BUILD_DATE __DATE__
#endif

#ifndef VITTE_BENCH_BUILD_TIME
  #define VITTE_BENCH_BUILD_TIME __TIME__
#endif

#ifndef VITTE_BENCH_BUILD_MODE
  #if defined(NDEBUG)
    #define VITTE_BENCH_BUILD_MODE "release"
  #else
    #define VITTE_BENCH_BUILD_MODE "debug"
  #endif
#endif

/* A conventional triple string. */
#ifndef VITTE_BENCH_PLATFORM_TRIPLE
  #define VITTE_BENCH_PLATFORM_TRIPLE VITTE_BENCH_OS_NAME "-" VITTE_BENCH_ARCH_NAME "-" VITTE_BENCH_ABI_NAME
#endif

/* -------------------------------------------------------------------------- */
/* Platform descriptor                                                         */
/* -------------------------------------------------------------------------- */

#define VITTE_PLATFORM_DESC_VERSION 2u

/* Flags */
#define VITTE_PLATFORM_F_NONE         0u
#define VITTE_PLATFORM_F_CPU_PROBED   (1u << 0)

typedef struct vitte_platform_desc {
    /* for forward/backward compatibility */
    uint32_t struct_size;     /* sizeof(vitte_platform_desc) */
    uint32_t struct_version;  /* VITTE_PLATFORM_DESC_VERSION */

    /* identity (static strings) */
    const char* os;      /* VITTE_BENCH_OS_NAME */
    const char* arch;    /* VITTE_BENCH_ARCH_NAME */
    const char* abi;     /* VITTE_BENCH_ABI_NAME */
    const char* endian;  /* VITTE_BENCH_ENDIAN_NAME */
    const char* cc;      /* VITTE_BENCH_CC_NAME */

    const char* triple;  /* VITTE_BENCH_PLATFORM_TRIPLE */

    /* build metadata */
    const char* build_version;
    const char* build_git_sha;
    const char* build_date;
    const char* build_time;
    const char* build_mode;

    /* runtime ids */
    uint32_t pid;
    uint64_t tid;

    /* fundamental properties */
    uint32_t ptr_bits;   /* sizeof(void*) * 8 */
    uint32_t page_size;  /* best effort, 0 if unknown */

    uint32_t flags;

    /* CPU info cache: optional */
    struct vitte_cpu_info cpu;
} vitte_platform_desc;

/* -------------------------------------------------------------------------- */
/* Best-effort queries                                                         */
/* -------------------------------------------------------------------------- */

static inline uint32_t vitte_platform_ptr_bits(void)
{
    return (uint32_t)(sizeof(void*) * 8u);
}

static inline uint32_t vitte_platform_page_size_best_effort(void)
{
#if VITTE_BENCH_OS_WINDOWS
    #include <windows.h>
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (uint32_t)si.dwPageSize;
#elif defined(_SC_PAGESIZE)
    #include <unistd.h>
    long v = sysconf(_SC_PAGESIZE);
    if (v <= 0) return 0u;
    return (uint32_t)v;
#elif defined(_SC_PAGE_SIZE)
    #include <unistd.h>
    long v = sysconf(_SC_PAGE_SIZE);
    if (v <= 0) return 0u;
    return (uint32_t)v;
#else
    return 0u;
#endif
}

/* Fill a descriptor (best effort, no allocation). */
static inline void vitte_platform_desc_fill(vitte_platform_desc* out)
{
    if (!out) return;

    out->struct_size = (uint32_t)sizeof(vitte_platform_desc);
    out->struct_version = VITTE_PLATFORM_DESC_VERSION;

    out->os = VITTE_BENCH_OS_NAME;
    out->arch = VITTE_BENCH_ARCH_NAME;
    out->abi = VITTE_BENCH_ABI_NAME;
    out->endian = VITTE_BENCH_ENDIAN_NAME;
    out->cc = VITTE_BENCH_CC_NAME;
    out->triple = VITTE_BENCH_PLATFORM_TRIPLE;

    out->build_version = VITTE_BENCH_BUILD_VERSION;
    out->build_git_sha = VITTE_BENCH_BUILD_GIT_SHA;
    out->build_date = VITTE_BENCH_BUILD_DATE;
    out->build_time = VITTE_BENCH_BUILD_TIME;
    out->build_mode = VITTE_BENCH_BUILD_MODE;

    out->pid = vitte_os_get_pid();
    out->tid = vitte_os_get_tid();

    out->ptr_bits = vitte_platform_ptr_bits();
    out->page_size = vitte_platform_page_size_best_effort();

    out->flags = VITTE_PLATFORM_F_NONE;

    /* best effort: may fail silently */
    if (vitte_cpu_probe(&out->cpu)) {
        out->flags |= VITTE_PLATFORM_F_CPU_PROBED;
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_PLATFORM_H */
