// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// bench_platform.c
// -----------------------------------------------------------------------------
// Platform metadata helpers for the compiler bench harness.
//
// This module provides strings suitable for CSV metadata fields:
//   - run_id      (e.g. git sha / CI build id)
//   - started_at  (ISO8601 UTC)
//   - host        (hostname)
//   - os          (platform)
//   - arch        (cpu arch)
//   - compiler    (compiler id/version)
//   - flags       (compile flags if injected)
//
// It is standalone and does not depend on the runner types. The runner can
// call the exported getters and fill its own metadata struct.
// -----------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#if !defined(_WIN32)
    #include <unistd.h>
#endif

// -----------------------------------------------------------------------------
// Build-time injection points (optional)
// -----------------------------------------------------------------------------
// You can define these from your build system:
//   -DVITTE_GIT_SHA="..."
//   -DVITTE_CI_RUN_ID="..."
//   -DVITTE_BENCH_CFLAGS="..."

#ifndef VITTE_GIT_SHA
#define VITTE_GIT_SHA ""
#endif

#ifndef VITTE_CI_RUN_ID
#define VITTE_CI_RUN_ID ""
#endif

#ifndef VITTE_BENCH_CFLAGS
#define VITTE_BENCH_CFLAGS ""
#endif

// -----------------------------------------------------------------------------
// Internal storage
// -----------------------------------------------------------------------------

enum {
    kHostCap    = 256,
    kTimeCap    = 64,
    kCompilerCap= 256,
    kOsCap      = 64,
    kArchCap    = 64,
    kRunIdCap   = 128,
};

static bool g_inited;

static char g_host[kHostCap];
static char g_started_at[kTimeCap];
static char g_compiler[kCompilerCap];
static char g_os[kOsCap];
static char g_arch[kArchCap];
static char g_run_id[kRunIdCap];

// -----------------------------------------------------------------------------
// Detect OS / arch / compiler
// -----------------------------------------------------------------------------

static const char* detect_os(void)
{
#if defined(__APPLE__) && defined(__MACH__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#elif defined(_WIN32)
    return "Windows";
#else
    return "Unknown";
#endif
}

static const char* detect_arch(void)
{
#if defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#elif defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#elif defined(__riscv)
    return "riscv";
#else
    return "unknown";
#endif
}

static void detect_compiler(char* out, size_t cap)
{
    if (!out || cap == 0) return;

#if defined(__clang__)
    // __clang_version__ is a string literal.
    snprintf(out, cap, "clang %s", __clang_version__);
#elif defined(__GNUC__)
    snprintf(out, cap, "gcc %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    snprintf(out, cap, "msvc %d", _MSC_VER);
#else
    // __VERSION__ is commonly provided.
    #ifdef __VERSION__
        snprintf(out, cap, "%s", __VERSION__);
    #else
        snprintf(out, cap, "unknown");
    #endif
#endif
}

// -----------------------------------------------------------------------------
// Hostname / time
// -----------------------------------------------------------------------------

static void detect_hostname(char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';

#if defined(_WIN32)
    // Keep empty on Windows for now.
    (void)cap;
#else
    if (gethostname(out, cap) != 0)
        out[0] = '\0';
    // Ensure NUL termination.
    out[cap - 1] = '\0';
#endif
}

static void detect_started_at_iso8601_utc(char* out, size_t cap)
{
    if (!out || cap == 0) return;

    const time_t t = time(NULL);

#if defined(_WIN32)
    // Windows variant omitted.
    (void)t;
    out[0] = '\0';
#else
    struct tm tmv;
    if (!gmtime_r(&t, &tmv))
    {
        out[0] = '\0';
        return;
    }
    // Example: 2026-01-01T00:00:00Z
    (void)strftime(out, cap, "%Y-%m-%dT%H:%M:%SZ", &tmv);
#endif
}

static void build_run_id(char* out, size_t cap)
{
    if (!out || cap == 0) return;
    out[0] = '\0';

    // Prefer CI id if provided, else git sha.
    if (VITTE_CI_RUN_ID[0] != '\0')
    {
        snprintf(out, cap, "%s", VITTE_CI_RUN_ID);
        return;
    }

    if (VITTE_GIT_SHA[0] != '\0')
    {
        snprintf(out, cap, "%s", VITTE_GIT_SHA);
        return;
    }

    // Otherwise empty.
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void vitte_bench_platform_init(void)
{
    if (g_inited) return;
    g_inited = true;

    snprintf(g_os, sizeof(g_os), "%s", detect_os());
    snprintf(g_arch, sizeof(g_arch), "%s", detect_arch());

    detect_compiler(g_compiler, sizeof(g_compiler));
    detect_hostname(g_host, sizeof(g_host));
    detect_started_at_iso8601_utc(g_started_at, sizeof(g_started_at));
    build_run_id(g_run_id, sizeof(g_run_id));
}

const char* vitte_bench_platform_run_id(void)
{
    vitte_bench_platform_init();
    return g_run_id[0] ? g_run_id : NULL;
}

const char* vitte_bench_platform_started_at(void)
{
    vitte_bench_platform_init();
    return g_started_at[0] ? g_started_at : NULL;
}

const char* vitte_bench_platform_host(void)
{
    vitte_bench_platform_init();
    return g_host[0] ? g_host : NULL;
}

const char* vitte_bench_platform_os(void)
{
    vitte_bench_platform_init();
    return g_os[0] ? g_os : NULL;
}

const char* vitte_bench_platform_arch(void)
{
    vitte_bench_platform_init();
    return g_arch[0] ? g_arch : NULL;
}

const char* vitte_bench_platform_compiler(void)
{
    vitte_bench_platform_init();
    return g_compiler[0] ? g_compiler : NULL;
}

const char* vitte_bench_platform_flags(void)
{
    // Flags are build-system injected; don't force init.
    return (VITTE_BENCH_CFLAGS[0] ? VITTE_BENCH_CFLAGS : NULL);
}

// Convenience: print to a stream (debug).
void vitte_bench_platform_print(FILE* f)
{
    if (!f) f = stdout;

    vitte_bench_platform_init();

    fprintf(f, "run_id: %s\n", g_run_id[0] ? g_run_id : "(none)");
    fprintf(f, "started_at: %s\n", g_started_at[0] ? g_started_at : "(none)");
    fprintf(f, "host: %s\n", g_host[0] ? g_host : "(none)");
    fprintf(f, "os: %s\n", g_os[0] ? g_os : "(none)");
    fprintf(f, "arch: %s\n", g_arch[0] ? g_arch : "(none)");
    fprintf(f, "compiler: %s\n", g_compiler[0] ? g_compiler : "(none)");
    fprintf(f, "flags: %s\n", VITTE_BENCH_CFLAGS[0] ? VITTE_BENCH_CFLAGS : "(none)");
}
