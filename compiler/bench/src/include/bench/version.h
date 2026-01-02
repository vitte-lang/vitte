/*
  bench/version.h

  Versioning information for the Vitte benchmark harness.

  Goals
  - Provide a stable semantic version for the bench framework.
  - Provide build metadata (git sha, build date/time, compiler id) when available.
  - Provide helpers for feature gating and deterministic report stamps.

  Integration
  - These macros are intended to be overridden by the build system (Muffin/CMake/etc.).

  Notes
  - This is distinct from the Vitte compiler version.
  - String macros are "stable": they must not allocate, and should be deterministic.
*/

#pragma once
#ifndef VITTE_BENCH_VERSION_H
#define VITTE_BENCH_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "bench/platform.h" /* VITTE_BENCH_PLATFORM_TRIPLE, cpu/os/arch */

/* -------------------------------------------------------------------------- */
/* Semantic version                                                             */
/* -------------------------------------------------------------------------- */

#ifndef VITTE_BENCH_VERSION_MAJOR
#define VITTE_BENCH_VERSION_MAJOR 0
#endif

#ifndef VITTE_BENCH_VERSION_MINOR
#define VITTE_BENCH_VERSION_MINOR 1
#endif

#ifndef VITTE_BENCH_VERSION_PATCH
#define VITTE_BENCH_VERSION_PATCH 0
#endif

/*
  Optional prerelease/build strings.

  IMPORTANT: These are concatenated verbatim into VITTE_BENCH_VERSION_FULL_STR.
  Provide any separators you want (e.g. "-alpha", "+exp.sha.5114f85").
*/
#ifndef VITTE_BENCH_VERSION_PRERELEASE
#define VITTE_BENCH_VERSION_PRERELEASE ""
#endif

#ifndef VITTE_BENCH_VERSION_BUILD
#define VITTE_BENCH_VERSION_BUILD ""
#endif

/* Packed numeric version for comparisons: MMMmmpp (e.g., 001002003). */
#define VITTE_BENCH_VERSION_NUM \
    ((uint32_t)(VITTE_BENCH_VERSION_MAJOR) * 1000000u + \
     (uint32_t)(VITTE_BENCH_VERSION_MINOR) * 1000u + \
     (uint32_t)(VITTE_BENCH_VERSION_PATCH))

/* -------------------------------------------------------------------------- */
/* Build metadata (overridable)                                                 */
/* -------------------------------------------------------------------------- */

/* Git SHA (short or full). */
#ifndef VITTE_BENCH_GIT_SHA
#define VITTE_BENCH_GIT_SHA "unknown"
#endif

/* Optional: git describe string (tag distance), e.g. "v0.1.0-12-g<sha>" */
#ifndef VITTE_BENCH_GIT_DESCRIBE
#define VITTE_BENCH_GIT_DESCRIBE ""
#endif

/* Optional: repository dirty flag. */
#ifndef VITTE_BENCH_GIT_DIRTY
#define VITTE_BENCH_GIT_DIRTY 0
#endif

/* Build date/time (prefer UTC if your CI provides it). */
#ifndef VITTE_BENCH_BUILD_DATE
#define VITTE_BENCH_BUILD_DATE __DATE__
#endif

#ifndef VITTE_BENCH_BUILD_TIME
#define VITTE_BENCH_BUILD_TIME __TIME__
#endif

/* Build mode, e.g. "debug" / "release" */
#ifndef VITTE_BENCH_BUILD_MODE
#define VITTE_BENCH_BUILD_MODE "unknown"
#endif

/*
  Compiler id string.
  Recommended values: "clang", "gcc", "msvc", "tcc", etc.
*/
#ifndef VITTE_BENCH_BUILD_COMPILER
#define VITTE_BENCH_BUILD_COMPILER "unknown"
#endif

/* Optional compiler version string (e.g. "clang-18.1.2"). */
#ifndef VITTE_BENCH_BUILD_COMPILER_VERSION
#if defined(__clang__)
#define VITTE_BENCH_BUILD_COMPILER_VERSION "clang-" __clang_version__
#elif defined(__GNUC__) && !defined(__clang__)
#define VITTE_BENCH_BUILD_COMPILER_VERSION "gcc-" __VERSION__
#elif defined(_MSC_VER)
#define VITTE_BENCH__STR2(x) #x
#define VITTE_BENCH__STR(x)  VITTE_BENCH__STR2(x)
#define VITTE_BENCH_BUILD_COMPILER_VERSION "msvc-" VITTE_BENCH__STR(_MSC_VER)
#undef VITTE_BENCH__STR
#undef VITTE_BENCH__STR2
#else
#define VITTE_BENCH_BUILD_COMPILER_VERSION "unknown"
#endif
#endif

/* Optional: build id (CI run number). */
#ifndef VITTE_BENCH_BUILD_ID
#define VITTE_BENCH_BUILD_ID ""
#endif

/* Optional: build host identifier (CI agent). */
#ifndef VITTE_BENCH_BUILD_HOST
#define VITTE_BENCH_BUILD_HOST ""
#endif

/* -------------------------------------------------------------------------- */
/* Stringification helpers                                                      */
/* -------------------------------------------------------------------------- */

#define VITTE_BENCH__STR2(x) #x
#define VITTE_BENCH__STR(x)  VITTE_BENCH__STR2(x)

/* Base version string (major.minor.patch). */
#define VITTE_BENCH_VERSION_STR \
    VITTE_BENCH__STR(VITTE_BENCH_VERSION_MAJOR) \
    "." VITTE_BENCH__STR(VITTE_BENCH_VERSION_MINOR) \
    "." VITTE_BENCH__STR(VITTE_BENCH_VERSION_PATCH)

/* Full version string with optional prerelease/build metadata (verbatim concat). */
#define VITTE_BENCH_VERSION_FULL_STR \
    VITTE_BENCH_VERSION_STR \
    VITTE_BENCH_VERSION_PRERELEASE \
    VITTE_BENCH_VERSION_BUILD

/* Human-friendly stamp for logs/reports (single-line, deterministic). */
#ifndef VITTE_BENCH_VERSION_STAMP
#define VITTE_BENCH_VERSION_STAMP \
    "bench/" VITTE_BENCH_VERSION_FULL_STR \
    " (" VITTE_BENCH_BUILD_COMPILER \
    ", " VITTE_BENCH_BUILD_MODE \
    ", " VITTE_BENCH_PLATFORM_TRIPLE \
    ", sha=" VITTE_BENCH_GIT_SHA \
    ")"
#endif

/* -------------------------------------------------------------------------- */
/* Feature gating                                                               */
/* -------------------------------------------------------------------------- */

/*
  Compare current bench version against a minimum.

  Example:
    #if VITTE_BENCH_VERSION_AT_LEAST(0,2,0)
      ...
    #endif
*/
#define VITTE_BENCH_VERSION_AT_LEAST(MAJ, MIN, PAT) \
    (VITTE_BENCH_VERSION_NUM >= ((uint32_t)(MAJ) * 1000000u + (uint32_t)(MIN) * 1000u + (uint32_t)(PAT)))

/* -------------------------------------------------------------------------- */
/* ABI markers                                                                  */
/* -------------------------------------------------------------------------- */

/*
  Bench report ABI version.
  Keep in sync with bench/report.h `VITTE_REPORT_VERSION`.
*/
#ifndef VITTE_BENCH_REPORT_ABI_VERSION
#define VITTE_BENCH_REPORT_ABI_VERSION 1u
#endif

/* -------------------------------------------------------------------------- */
/* Structured info (stable ABI)                                                */
/* -------------------------------------------------------------------------- */

typedef struct vitte_bench_version_info {
    uint32_t struct_size;    /* sizeof(vitte_bench_version_info) */
    uint32_t struct_version; /* 1 */

    uint32_t semver_num;     /* VITTE_BENCH_VERSION_NUM */

    const char* version_str;       /* VITTE_BENCH_VERSION_STR */
    const char* version_full_str;  /* VITTE_BENCH_VERSION_FULL_STR */

    const char* git_sha;           /* VITTE_BENCH_GIT_SHA */
    const char* git_describe;      /* VITTE_BENCH_GIT_DESCRIBE */
    uint32_t git_dirty;            /* VITTE_BENCH_GIT_DIRTY (0/1) */

    const char* build_date;        /* VITTE_BENCH_BUILD_DATE */
    const char* build_time;        /* VITTE_BENCH_BUILD_TIME */
    const char* build_mode;        /* VITTE_BENCH_BUILD_MODE */

    const char* compiler;          /* VITTE_BENCH_BUILD_COMPILER */
    const char* compiler_version;  /* VITTE_BENCH_BUILD_COMPILER_VERSION */

    const char* build_id;          /* VITTE_BENCH_BUILD_ID */
    const char* build_host;        /* VITTE_BENCH_BUILD_HOST */

    const char* platform_triple;   /* VITTE_BENCH_PLATFORM_TRIPLE */

    const char* stamp;             /* VITTE_BENCH_VERSION_STAMP */
} vitte_bench_version_info;

/* Fill version info (no allocations). */
static inline void vitte_bench_version_info_get(vitte_bench_version_info* out)
{
    if (!out) return;

    out->struct_size = (uint32_t)sizeof(vitte_bench_version_info);
    out->struct_version = 1u;

    out->semver_num = (uint32_t)VITTE_BENCH_VERSION_NUM;

    out->version_str = VITTE_BENCH_VERSION_STR;
    out->version_full_str = VITTE_BENCH_VERSION_FULL_STR;

    out->git_sha = VITTE_BENCH_GIT_SHA;
    out->git_describe = VITTE_BENCH_GIT_DESCRIBE;
    out->git_dirty = (uint32_t)(VITTE_BENCH_GIT_DIRTY ? 1u : 0u);

    out->build_date = VITTE_BENCH_BUILD_DATE;
    out->build_time = VITTE_BENCH_BUILD_TIME;
    out->build_mode = VITTE_BENCH_BUILD_MODE;

    out->compiler = VITTE_BENCH_BUILD_COMPILER;
    out->compiler_version = VITTE_BENCH_BUILD_COMPILER_VERSION;

    out->build_id = VITTE_BENCH_BUILD_ID;
    out->build_host = VITTE_BENCH_BUILD_HOST;

    out->platform_triple = VITTE_BENCH_PLATFORM_TRIPLE;

    out->stamp = VITTE_BENCH_VERSION_STAMP;
}

/* -------------------------------------------------------------------------- */
/* Backward-compatible tiny API (kept)                                         */
/* -------------------------------------------------------------------------- */

static inline uint32_t vitte_bench_version_num(void)
{
    return (uint32_t)VITTE_BENCH_VERSION_NUM;
}

static inline const char* vitte_bench_version_str(void)
{
    return VITTE_BENCH_VERSION_STR;
}

static inline const char* vitte_bench_version_full_str(void)
{
    return VITTE_BENCH_VERSION_FULL_STR;
}

static inline const char* vitte_bench_git_sha(void)
{
    return VITTE_BENCH_GIT_SHA;
}

static inline const char* vitte_bench_build_date(void)
{
    return VITTE_BENCH_BUILD_DATE;
}

static inline const char* vitte_bench_build_time(void)
{
    return VITTE_BENCH_BUILD_TIME;
}

static inline const char* vitte_bench_build_compiler(void)
{
    return VITTE_BENCH_BUILD_COMPILER;
}

static inline const char* vitte_bench_version_stamp(void)
{
    return VITTE_BENCH_VERSION_STAMP;
}

#undef VITTE_BENCH__STR
#undef VITTE_BENCH__STR2

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_VERSION_H */
