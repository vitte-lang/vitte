

/*
  bench/os.h

  OS / platform utilities for the Vitte benchmark harness.

  Responsibilities
  - OS detection macros (windows/linux/darwin/bsd/solaris).
  - Process/thread helpers: pid/tid, sleep, env access.
  - Time helpers: monotonic wall clock ns (delegates to detail/time where present).
  - Filesystem-ish helpers: temp dir, cwd, path separator.

  Goals
  - Portable C17.
  - No dynamic allocation in API (arena used where needed).
  - Deterministic behavior (no locale dependency).

  Notes
  - Some functions are best-effort and may return defaults if unsupported.
  - Prefer bench/detail/* backends in implementation units; this header provides
    a stable surface and simple inline fallbacks.
*/

#pragma once
#ifndef VITTE_BENCH_OS_H
#define VITTE_BENCH_OS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/arena.h"
#include "bench/diag.h"
#include "bench/detail/compat.h"

/* -------------------------------------------------------------------------- */
/* OS detection                                                                */
/* -------------------------------------------------------------------------- */

#if defined(_WIN32) || defined(_WIN64)
  #define VITTE_BENCH_OS_WINDOWS 1
#else
  #define VITTE_BENCH_OS_WINDOWS 0
#endif

#if defined(__linux__)
  #define VITTE_BENCH_OS_LINUX 1
#else
  #define VITTE_BENCH_OS_LINUX 0
#endif

#if defined(__APPLE__) && defined(__MACH__)
  #define VITTE_BENCH_OS_DARWIN 1
#else
  #define VITTE_BENCH_OS_DARWIN 0
#endif

#if defined(__FreeBSD__)
  #define VITTE_BENCH_OS_FREEBSD 1
#else
  #define VITTE_BENCH_OS_FREEBSD 0
#endif

#if defined(__NetBSD__)
  #define VITTE_BENCH_OS_NETBSD 1
#else
  #define VITTE_BENCH_OS_NETBSD 0
#endif

#if defined(__OpenBSD__)
  #define VITTE_BENCH_OS_OPENBSD 1
#else
  #define VITTE_BENCH_OS_OPENBSD 0
#endif

#if defined(__sun) && defined(__SVR4)
  #define VITTE_BENCH_OS_SOLARIS 1
#else
  #define VITTE_BENCH_OS_SOLARIS 0
#endif

#if (VITTE_BENCH_OS_FREEBSD || VITTE_BENCH_OS_NETBSD || VITTE_BENCH_OS_OPENBSD)
  #define VITTE_BENCH_OS_BSD 1
#else
  #define VITTE_BENCH_OS_BSD 0
#endif

#if defined(__unix__) || defined(__unix)
  #define VITTE_BENCH_OS_UNIX 1
#else
  #define VITTE_BENCH_OS_UNIX 0
#endif

/* -------------------------------------------------------------------------- */
/* Basic constants                                                             */
/* -------------------------------------------------------------------------- */

#if VITTE_BENCH_OS_WINDOWS
  #define VITTE_BENCH_PATH_SEP '\\'
  #define VITTE_BENCH_PATH_SEP_STR "\\"
#else
  #define VITTE_BENCH_PATH_SEP '/'
  #define VITTE_BENCH_PATH_SEP_STR "/"
#endif

/* best-effort max path used by helpers */
#ifndef VITTE_BENCH_PATH_MAX
  #define VITTE_BENCH_PATH_MAX 4096u
#endif

/* -------------------------------------------------------------------------- */
/* Process / thread ids                                                        */
/* -------------------------------------------------------------------------- */

uint32_t vitte_os_get_pid(void);
uint64_t vitte_os_get_tid(void);

/* -------------------------------------------------------------------------- */
/* Sleeping                                                                     */
/* -------------------------------------------------------------------------- */

/* Sleep for `ms` milliseconds (best effort). */
void vitte_os_sleep_ms(uint32_t ms);

/* Sleep for `ns` nanoseconds (best effort). */
void vitte_os_sleep_ns(uint64_t ns);

/* -------------------------------------------------------------------------- */
/* Environment                                                                  */
/* -------------------------------------------------------------------------- */

/*
  Get environment variable.

  Returns pointer to process environment storage when available (do not free).
  On Windows, returns arena-backed UTF-8 string when conversion is needed.
  Returns NULL if not present.
*/
const char* vitte_os_getenv(vitte_arena* arena, const char* key);

/* -------------------------------------------------------------------------- */
/* Working directory                                                            */
/* -------------------------------------------------------------------------- */

/*
  Get current working directory.
  Returns arena-backed normalized path (UTF-8).
*/
const char* vitte_os_getcwd(vitte_arena* arena, vitte_diag* d);

/*
  Get temp directory.
  Returns arena-backed normalized path (UTF-8).
*/
const char* vitte_os_temp_dir(vitte_arena* arena, vitte_diag* d);

/* -------------------------------------------------------------------------- */
/* Monotonic time (ns)                                                          */
/* -------------------------------------------------------------------------- */

/*
  Returns monotonic time in nanoseconds.

  This is used by the bench timer when a more specialized backend is not used.
*/
uint64_t vitte_os_monotonic_ns(void);

/* -------------------------------------------------------------------------- */
/* Implementation (header-only)                                                */
/* -------------------------------------------------------------------------- */

static inline size_t vitte_os__strlen(const char* s)
{
    size_t n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

static inline void vitte_os__cpy(char* dst, size_t cap, const char* src)
{
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t i = 0;
    for (; i + 1 < cap && src[i]; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

static inline const char* vitte_os__arena_dup(vitte_arena* a, const char* s)
{
    if (!a) return NULL;
    if (!s) s = "";
    size_t n = vitte_os__strlen(s);
    char* p = (char*)vitte_arena_alloc(a, n + 1, 1);
    if (!p) return NULL;
    for (size_t i = 0; i < n; ++i) p[i] = s[i];
    p[n] = '\0';
    return p;
}

uint32_t vitte_os_get_pid(void)
{
#if VITTE_BENCH_OS_WINDOWS
    #include <windows.h>
    return (uint32_t)GetCurrentProcessId();
#else
    #include <unistd.h>
    return (uint32_t)getpid();
#endif
}

uint64_t vitte_os_get_tid(void)
{
#if VITTE_BENCH_OS_WINDOWS
    #include <windows.h>
    return (uint64_t)GetCurrentThreadId();
#elif VITTE_BENCH_OS_DARWIN
    #include <pthread.h>
    uint64_t tid = 0;
    (void)pthread_threadid_np(NULL, &tid);
    return tid;
#elif VITTE_BENCH_OS_LINUX
    #include <sys/syscall.h>
    #include <unistd.h>
    return (uint64_t)syscall(SYS_gettid);
#elif VITTE_BENCH_OS_FREEBSD
    #include <pthread_np.h>
    return (uint64_t)pthread_getthreadid_np();
#elif VITTE_BENCH_OS_OPENBSD
    #include <unistd.h>
    return (uint64_t)getthrid();
#elif VITTE_BENCH_OS_NETBSD
    #include <lwp.h>
    return (uint64_t)_lwp_self();
#elif VITTE_BENCH_OS_SOLARIS
    #include <thread.h>
    return (uint64_t)thr_self();
#else
    /* fallback: pthread_self is opaque; hash it */
    #include <pthread.h>
    union { pthread_t t; uint64_t u; } u;
    u.u = 0;
    u.t = pthread_self();
    return u.u;
#endif
}

void vitte_os_sleep_ms(uint32_t ms)
{
#if VITTE_BENCH_OS_WINDOWS
    #include <windows.h>
    Sleep((DWORD)ms);
#else
    #include <time.h>
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000u);
    ts.tv_nsec = (long)((ms % 1000u) * 1000000ul);
    (void)nanosleep(&ts, NULL);
#endif
}

void vitte_os_sleep_ns(uint64_t ns)
{
#if VITTE_BENCH_OS_WINDOWS
    /* Windows Sleep is ms granularity by default */
    uint32_t ms = (uint32_t)(ns / 1000000ull);
    if (ms == 0 && ns) ms = 1;
    vitte_os_sleep_ms(ms);
#else
    #include <time.h>
    struct timespec ts;
    ts.tv_sec = (time_t)(ns / 1000000000ull);
    ts.tv_nsec = (long)(ns % 1000000000ull);
    (void)nanosleep(&ts, NULL);
#endif
}

const char* vitte_os_getenv(vitte_arena* arena, const char* key)
{
    if (!key || !key[0]) {
        return NULL;
    }

#if VITTE_BENCH_OS_WINDOWS
    /* Windows environment is UTF-16; use GetEnvironmentVariableW.
       We accept ASCII keys; for full UTF-8 keys, callers can provide a backend.
    */
    #include <windows.h>

    /* Convert key to wide (ASCII only) */
    wchar_t wkey[256];
    size_t klen = vitte_os__strlen(key);
    if (klen >= 255) return NULL;
    for (size_t i = 0; i < klen; ++i) wkey[i] = (wchar_t)(unsigned char)key[i];
    wkey[klen] = 0;

    DWORD need = GetEnvironmentVariableW(wkey, NULL, 0);
    if (need == 0) return NULL;

    wchar_t* wval = (wchar_t*)vitte_arena_alloc(arena, (size_t)need * sizeof(wchar_t), sizeof(wchar_t));
    if (!wval) return NULL;

    DWORD got = GetEnvironmentVariableW(wkey, wval, need);
    if (got == 0) return NULL;

    /* Convert UTF-16 to UTF-8 (best effort) */
    int u8need = WideCharToMultiByte(CP_UTF8, 0, wval, (int)got, NULL, 0, NULL, NULL);
    if (u8need <= 0) return NULL;

    char* u8 = (char*)vitte_arena_alloc(arena, (size_t)u8need + 1, 1);
    if (!u8) return NULL;

    int u8got = WideCharToMultiByte(CP_UTF8, 0, wval, (int)got, u8, u8need, NULL, NULL);
    if (u8got <= 0) return NULL;
    u8[u8got] = '\0';

    return u8;
#else
    #include <stdlib.h>
    (void)arena;
    return getenv(key);
#endif
}

const char* vitte_os_getcwd(vitte_arena* arena, vitte_diag* d)
{
    if (!arena) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_os_getcwd: arena required");
        return NULL;
    }

#if VITTE_BENCH_OS_WINDOWS
    #include <windows.h>
    wchar_t wbuf[VITTE_BENCH_PATH_MAX];
    DWORD n = GetCurrentDirectoryW((DWORD)VITTE_BENCH_PATH_MAX, wbuf);
    if (n == 0 || n >= (DWORD)VITTE_BENCH_PATH_MAX) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_IO, VITTE_DIAG_F_SYS, "GetCurrentDirectoryW failed");
        return NULL;
    }

    int u8need = WideCharToMultiByte(CP_UTF8, 0, wbuf, (int)n, NULL, 0, NULL, NULL);
    if (u8need <= 0) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_IO, VITTE_DIAG_F_SYS, "cwd utf16->utf8 conversion failed");
        return NULL;
    }

    char* u8 = (char*)vitte_arena_alloc(arena, (size_t)u8need + 1, 1);
    if (!u8) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "cwd oom");
        return NULL;
    }
    int u8got = WideCharToMultiByte(CP_UTF8, 0, wbuf, (int)n, u8, u8need, NULL, NULL);
    if (u8got <= 0) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_IO, VITTE_DIAG_F_SYS, "cwd utf16->utf8 conversion failed");
        return NULL;
    }
    u8[u8got] = '\0';
    return u8;
#else
    #include <unistd.h>
    char buf[VITTE_BENCH_PATH_MAX];
    if (!getcwd(buf, sizeof(buf))) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_IO, VITTE_DIAG_F_SYS, "getcwd failed");
        return NULL;
    }
    return vitte_os__arena_dup(arena, buf);
#endif
}

const char* vitte_os_temp_dir(vitte_arena* arena, vitte_diag* d)
{
    (void)d;
    if (!arena) {
        return NULL;
    }

#if VITTE_BENCH_OS_WINDOWS
    #include <windows.h>
    wchar_t wbuf[VITTE_BENCH_PATH_MAX];
    DWORD n = GetTempPathW((DWORD)VITTE_BENCH_PATH_MAX, wbuf);
    if (n == 0 || n >= (DWORD)VITTE_BENCH_PATH_MAX) {
        return vitte_os__arena_dup(arena, ".");
    }

    int u8need = WideCharToMultiByte(CP_UTF8, 0, wbuf, (int)n, NULL, 0, NULL, NULL);
    if (u8need <= 0) {
        return vitte_os__arena_dup(arena, ".");
    }
    char* u8 = (char*)vitte_arena_alloc(arena, (size_t)u8need + 1, 1);
    if (!u8) {
        return vitte_os__arena_dup(arena, ".");
    }
    int u8got = WideCharToMultiByte(CP_UTF8, 0, wbuf, (int)n, u8, u8need, NULL, NULL);
    if (u8got <= 0) {
        return vitte_os__arena_dup(arena, ".");
    }
    u8[u8got] = '\0';
    return u8;
#else
    const char* tmp = vitte_os_getenv(arena, "TMPDIR");
    if (!tmp || !tmp[0]) tmp = "/tmp";
    return vitte_os__arena_dup(arena, tmp);
#endif
}

uint64_t vitte_os_monotonic_ns(void)
{
#if VITTE_BENCH_OS_WINDOWS
    #include <windows.h>
    LARGE_INTEGER freq;
    LARGE_INTEGER ctr;
    if (!QueryPerformanceFrequency(&freq) || !QueryPerformanceCounter(&ctr) || freq.QuadPart == 0) {
        return 0;
    }
    /* convert to ns */
    return (uint64_t)((ctr.QuadPart * 1000000000ull) / (uint64_t)freq.QuadPart);
#else
    #include <time.h>
    #if defined(CLOCK_MONOTONIC)
      struct timespec ts;
      if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
          return 0;
      }
      return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
    #else
      /* worst-case fallback */
      struct timespec ts;
      if (timespec_get(&ts, TIME_UTC) != 0) {
          return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
      }
      return 0;
    #endif
#endif
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_OS_H */