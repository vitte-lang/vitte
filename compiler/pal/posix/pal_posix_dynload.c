// SPDX-License-Identifier: MIT
// pal_posix_dynload.c
//
// POSIX dynamic loader backend (max).
//
// Wraps:
//  - dlopen / dlsym / dlclose / dlerror
//
// Goals:
//  - Small, predictable API for the rest of the toolchain.
//  - Stable error string semantics (owned copy stored per handle).
//  - Standalone fallback types when `pal_dynload.h` is not available yet.
//
// Notes:
//  - On Linux, link with -ldl if not already provided by the build.
//  - On macOS, dlopen symbols are in libSystem; -ldl is typically not needed.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>
#include <errno.h>

//------------------------------------------------------------------------------
// Optional header integration
//------------------------------------------------------------------------------

#if defined(__has_include)
  #if __has_include("../pal_dynload.h")
    #include "../pal_dynload.h"
    #define STEEL_HAS_PAL_DYNLOAD_H 1
  #elif __has_include("pal_dynload.h")
    #include "pal_dynload.h"
    #define STEEL_HAS_PAL_DYNLOAD_H 1
  #endif
#endif

#ifndef STEEL_HAS_PAL_DYNLOAD_H

// Fallback API (keep it small; align pal_dynload.h later)

typedef struct pal_dynlib
{
    void* handle;
    char* last_error; // owned
} pal_dynlib;

typedef enum pal_dynload_flags
{
    PAL_DYNLOAD_DEFAULT = 0,

    // Mapping to dlopen flags.
    PAL_DYNLOAD_LAZY  = 1u << 0,
    PAL_DYNLOAD_NOW   = 1u << 1,
    PAL_DYNLOAD_LOCAL = 1u << 2,
    PAL_DYNLOAD_GLOBAL= 1u << 3,

} pal_dynload_flags;

void pal_dynlib_init(pal_dynlib* lib);
void pal_dynlib_dispose(pal_dynlib* lib);

bool pal_dynlib_open(pal_dynlib* lib, const char* path);
bool pal_dynlib_open_flags(pal_dynlib* lib, const char* path, uint32_t flags);

void* pal_dynlib_symbol(pal_dynlib* lib, const char* name);
bool  pal_dynlib_close(pal_dynlib* lib);

bool        pal_dynlib_is_open(const pal_dynlib* lib);
const char* pal_dynlib_last_error(const pal_dynlib* lib);

#endif // !STEEL_HAS_PAL_DYNLOAD_H

//------------------------------------------------------------------------------
// Internal helpers
//------------------------------------------------------------------------------

static void pal_free_err_(pal_dynlib* lib)
{
    if (!lib) return;
    free(lib->last_error);
    lib->last_error = NULL;
}

static char* pal_strdup_(const char* s)
{
    if (!s) s = "";
    size_t n = strlen(s);
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}

static void pal_set_err_(pal_dynlib* lib, const char* msg)
{
    if (!lib) return;
    pal_free_err_(lib);
    lib->last_error = pal_strdup_(msg);
}

static const char* pal_dlerror_or_errno_(void)
{
    const char* e = dlerror();
    if (e && e[0])
        return e;

    // dlerror may return NULL in some edge cases.
    // errno is best-effort here.
    return strerror(errno);
}

static int map_flags_(uint32_t flags)
{
    int f = 0;

    // Resolution mode
    if (flags & PAL_DYNLOAD_NOW) f |= RTLD_NOW;
    else if (flags & PAL_DYNLOAD_LAZY) f |= RTLD_LAZY;
    else f |= RTLD_NOW;

    // Symbol visibility
    if (flags & PAL_DYNLOAD_GLOBAL) f |= RTLD_GLOBAL;
    else if (flags & PAL_DYNLOAD_LOCAL) f |= RTLD_LOCAL;
    else f |= RTLD_LOCAL;

#ifdef RTLD_NODELETE
    // not exposed yet; keep room for future flags.
#endif

    return f;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void pal_dynlib_init(pal_dynlib* lib)
{
    if (!lib) return;
    memset(lib, 0, sizeof(*lib));
}

void pal_dynlib_dispose(pal_dynlib* lib)
{
    if (!lib) return;
    (void)pal_dynlib_close(lib);
    pal_free_err_(lib);
    memset(lib, 0, sizeof(*lib));
}

bool pal_dynlib_is_open(const pal_dynlib* lib)
{
    return lib && lib->handle != NULL;
}

const char* pal_dynlib_last_error(const pal_dynlib* lib)
{
    if (!lib) return "";
    return lib->last_error ? lib->last_error : "";
}

bool pal_dynlib_open(pal_dynlib* lib, const char* path)
{
    return pal_dynlib_open_flags(lib, path, PAL_DYNLOAD_DEFAULT);
}

bool pal_dynlib_open_flags(pal_dynlib* lib, const char* path, uint32_t flags)
{
    if (!lib)
        return false;

    pal_free_err_(lib);

    // Close existing handle if any.
    if (lib->handle)
        (void)pal_dynlib_close(lib);

    // Reset dlerror state before dlopen.
    (void)dlerror();

    int dl_flags = map_flags_(flags);
    void* h = dlopen(path, dl_flags);

    if (!h)
    {
        pal_set_err_(lib, pal_dlerror_or_errno_());
        lib->handle = NULL;
        return false;
    }

    lib->handle = h;
    return true;
}

void* pal_dynlib_symbol(pal_dynlib* lib, const char* name)
{
    if (!lib || !lib->handle || !name)
        return NULL;

    pal_free_err_(lib);

    // Reset dlerror state before dlsym.
    (void)dlerror();

    void* sym = dlsym(lib->handle, name);

    const char* e = dlerror();
    if (e && e[0])
    {
        pal_set_err_(lib, e);
        return NULL;
    }

    return sym;
}

bool pal_dynlib_close(pal_dynlib* lib)
{
    if (!lib)
        return false;

    pal_free_err_(lib);

    if (!lib->handle)
        return true;

    // Reset dlerror state before dlclose.
    (void)dlerror();

    int rc = dlclose(lib->handle);
    lib->handle = NULL;

    if (rc != 0)
    {
        pal_set_err_(lib, pal_dlerror_or_errno_());
        return false;
    }

    return true;
}
