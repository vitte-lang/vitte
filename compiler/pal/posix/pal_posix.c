// SPDX-License-Identifier: MIT
// pal_posix.c
//
// POSIX platform glue (max).
//
// This unit provides cross-cutting platform utilities that don't naturally fit
// into pal_posix_fs/proc/thread/time/net/dynload.
//
// Typical toolchain needs covered here:
//  - platform name
//  - pid
//  - cpu/page size
//  - cwd get/set
//  - env get/set/unset
//  - home/tmp directories
//  - executable path (best-effort)
//
// Integration:
//  - If `pal_platform.h` (or similar) exists, it will be included.
//  - Otherwise, this file provides a fallback API (safe to call from C code).

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>

#if defined(__APPLE__)
  #include <TargetConditionals.h>
  #include <mach-o/dyld.h>
#endif

#if defined(__unix__) || defined(__APPLE__)
  #include <sys/param.h>
  #include <sys/stat.h>
  #include <pwd.h>
#endif

#ifndef PATH_MAX
  #define PATH_MAX 4096
#endif

//------------------------------------------------------------------------------
// Optional header integration
//------------------------------------------------------------------------------

#if defined(__has_include)
  #if __has_include("../pal_platform.h")
    #include "../pal_platform.h"
    #define STEEL_HAS_PAL_PLATFORM_H 1
  #elif __has_include("../pal.h")
    #include "../pal.h"
    #define STEEL_HAS_PAL_PLATFORM_H 1
  #elif __has_include("pal_platform.h")
    #include "pal_platform.h"
    #define STEEL_HAS_PAL_PLATFORM_H 1
  #elif __has_include("pal.h")
    #include "pal.h"
    #define STEEL_HAS_PAL_PLATFORM_H 1
  #endif
#endif

#ifndef STEEL_HAS_PAL_PLATFORM_H

//------------------------------------------------------------------------------
// Fallback API (align later with pal_platform.h)
//------------------------------------------------------------------------------

typedef struct pal_path_buf
{
    char*  data; // owned
    size_t len;
} pal_path_buf;

void pal_path_buf_dispose(pal_path_buf* b);

const char* pal_platform_name(void);

uint32_t pal_getpid_u32(void);

uint32_t pal_cpu_count(void);
uint32_t pal_page_size(void);

bool pal_get_cwd(char* out, size_t out_cap);
bool pal_set_cwd(const char* path);

// env
bool  pal_env_get(const char* key, char* out, size_t out_cap);
char* pal_env_get_copy(const char* key);
bool  pal_env_set(const char* key, const char* value, bool overwrite);
bool  pal_env_unset(const char* key);

// dirs
bool pal_dir_home(char* out, size_t out_cap);
bool pal_dir_tmp(char* out, size_t out_cap);

// executable path
bool pal_exe_path(char* out, size_t out_cap);

const char* pal_posix_last_error(void);

#endif // !STEEL_HAS_PAL_PLATFORM_H

//------------------------------------------------------------------------------
// Error
//------------------------------------------------------------------------------

static _Thread_local char g_posix_err_[256];

static void set_msg_(const char* msg)
{
    if (!msg) msg = "";
    size_t n = strlen(msg);
    if (n >= sizeof(g_posix_err_)) n = sizeof(g_posix_err_) - 1;
    memcpy(g_posix_err_, msg, n);
    g_posix_err_[n] = 0;
}

static void set_errno_(const char* prefix)
{
    const char* e = strerror(errno);
    if (!prefix) prefix = "";
    if (!e) e = "";

    if (prefix[0])
        snprintf(g_posix_err_, sizeof(g_posix_err_), "%s: %s", prefix, e);
    else
        snprintf(g_posix_err_, sizeof(g_posix_err_), "%s", e);
}

const char* pal_posix_last_error(void)
{
    return g_posix_err_;
}

//------------------------------------------------------------------------------
// Small helpers
//------------------------------------------------------------------------------

static size_t snprint_(char* out, size_t cap, const char* s)
{
    if (!out || cap == 0) return 0;
    if (!s) s = "";
    size_t n = strlen(s);
    if (n >= cap) n = cap - 1;
    memcpy(out, s, n);
    out[n] = 0;
    return n;
}

void pal_path_buf_dispose(pal_path_buf* b)
{
    if (!b) return;
    free(b->data);
    b->data = NULL;
    b->len = 0;
}

//------------------------------------------------------------------------------
// Platform name
//------------------------------------------------------------------------------

const char* pal_platform_name(void)
{
#if defined(__APPLE__)
    return "posix-macos";
#elif defined(__linux__)
    return "posix-linux";
#elif defined(__unix__)
    return "posix-unix";
#else
    return "posix";
#endif
}

//------------------------------------------------------------------------------
// PID
//------------------------------------------------------------------------------

uint32_t pal_getpid_u32(void)
{
    pid_t p = getpid();
    if (p < 0) return 0;
    return (uint32_t)p;
}

//------------------------------------------------------------------------------
// CPU count / page size
//------------------------------------------------------------------------------

uint32_t pal_cpu_count(void)
{
#ifdef _SC_NPROCESSORS_ONLN
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n > 0) return (uint32_t)n;
#endif
    return 1;
}

uint32_t pal_page_size(void)
{
#ifdef _SC_PAGESIZE
    long n = sysconf(_SC_PAGESIZE);
    if (n > 0) return (uint32_t)n;
#elif defined(_SC_PAGE_SIZE)
    long n2 = sysconf(_SC_PAGE_SIZE);
    if (n2 > 0) return (uint32_t)n2;
#endif
    return 4096;
}

//------------------------------------------------------------------------------
// CWD
//------------------------------------------------------------------------------

bool pal_get_cwd(char* out, size_t out_cap)
{
    if (!out || out_cap == 0)
    {
        set_msg_("invalid args");
        return false;
    }

    errno = 0;
    if (!getcwd(out, out_cap))
    {
        set_errno_("getcwd");
        out[0] = 0;
        return false;
    }

    return true;
}

bool pal_set_cwd(const char* path)
{
    if (!path || !path[0])
    {
        set_msg_("empty path");
        return false;
    }

    if (chdir(path) != 0)
    {
        set_errno_("chdir");
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
// Env
//------------------------------------------------------------------------------

bool pal_env_get(const char* key, char* out, size_t out_cap)
{
    if (!out || out_cap == 0)
    {
        set_msg_("invalid args");
        return false;
    }

    out[0] = 0;

    if (!key || !key[0])
    {
        set_msg_("empty key");
        return false;
    }

    const char* v = getenv(key);
    if (!v)
        return false;

    snprint_(out, out_cap, v);
    return true;
}

char* pal_env_get_copy(const char* key)
{
    if (!key || !key[0])
    {
        set_msg_("empty key");
        return NULL;
    }

    const char* v = getenv(key);
    if (!v)
        return NULL;

    size_t n = strlen(v);
    char* p = (char*)malloc(n + 1);
    if (!p)
    {
        set_msg_("out of memory");
        return NULL;
    }

    memcpy(p, v, n + 1);
    return p;
}

bool pal_env_set(const char* key, const char* value, bool overwrite)
{
    if (!key || !key[0])
    {
        set_msg_("empty key");
        return false;
    }

    if (!value) value = "";

    if (setenv(key, value, overwrite ? 1 : 0) != 0)
    {
        set_errno_("setenv");
        return false;
    }

    return true;
}

bool pal_env_unset(const char* key)
{
    if (!key || !key[0])
    {
        set_msg_("empty key");
        return false;
    }

    if (unsetenv(key) != 0)
    {
        set_errno_("unsetenv");
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
// Directories
//------------------------------------------------------------------------------

static bool home_from_pw_(char* out, size_t out_cap)
{
    struct passwd* pw = getpwuid(getuid());
    if (!pw || !pw->pw_dir)
        return false;

    snprint_(out, out_cap, pw->pw_dir);
    return out[0] != 0;
}

bool pal_dir_home(char* out, size_t out_cap)
{
    if (!out || out_cap == 0)
    {
        set_msg_("invalid args");
        return false;
    }

    out[0] = 0;

    const char* h = getenv("HOME");
    if (h && h[0])
    {
        snprint_(out, out_cap, h);
        return true;
    }

    if (home_from_pw_(out, out_cap))
        return true;

    set_msg_("HOME not set");
    return false;
}

bool pal_dir_tmp(char* out, size_t out_cap)
{
    if (!out || out_cap == 0)
    {
        set_msg_("invalid args");
        return false;
    }

    out[0] = 0;

    const char* t = getenv("TMPDIR");
    if (!t || !t[0]) t = getenv("TMP");
    if (!t || !t[0]) t = getenv("TEMP");

#if defined(__APPLE__)
    // TMPDIR is usually set. If not, fallback to /tmp.
#endif

    if (!t || !t[0])
        t = "/tmp";

    snprint_(out, out_cap, t);
    return true;
}

//------------------------------------------------------------------------------
// Executable path
//------------------------------------------------------------------------------

bool pal_exe_path(char* out, size_t out_cap)
{
    if (!out || out_cap == 0)
    {
        set_msg_("invalid args");
        return false;
    }

    out[0] = 0;

#if defined(__APPLE__)
    uint32_t size = (uint32_t)out_cap;
    int rc = _NSGetExecutablePath(out, &size);
    if (rc == 0)
        return true;

    // Buffer too small: return false but provide required size in error.
    char msg[128];
    snprintf(msg, sizeof(msg), "buffer too small (need %u)", (unsigned)size);
    set_msg_(msg);
    out[0] = 0;
    return false;

#elif defined(__linux__)
    // /proc/self/exe
    ssize_t n = readlink("/proc/self/exe", out, out_cap - 1);
    if (n < 0)
    {
        set_errno_("readlink(/proc/self/exe)");
        out[0] = 0;
        return false;
    }

    out[n] = 0;
    return true;

#else
    // Best-effort: not universally available.
    set_msg_("exe path unsupported on this platform");
    return false;
#endif
}
