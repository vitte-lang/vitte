// SPDX-License-Identifier: MIT
// rt_slice.c
//
// Runtime slice utilities (max).
//
// A "slice" is a non-owning view over a contiguous memory range.
// This file provides:
//  - byte slice, char slice
//  - constructors and bounds-checked slicing
//  - comparisons, prefix/suffix, find, trim
//  - hashing (FNV-1a 64)
//  - safe copy helpers
//
// Integration:
//  - If `rt_slice.h` exists, include it.
//  - Otherwise provide a fallback API.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#if defined(__has_include)
  #if __has_include("rt_slice.h")
    #include "rt_slice.h"
    #define RT_HAVE_RT_SLICE_H 1
  #elif __has_include("../runtime/rt_slice.h")
    #include "../runtime/rt_slice.h"
    #define RT_HAVE_RT_SLICE_H 1
  #endif
#endif

#ifndef RT_HAVE_RT_SLICE_H

typedef struct rt_bytes
{
    const uint8_t* ptr;
    size_t len;
} rt_bytes;

typedef struct rt_str
{
    const char* ptr;
    size_t len;
} rt_str;

// constructors
rt_bytes rt_bytes_from(const void* p, size_t n);
rt_bytes rt_bytes_from_cstr(const char* s);
rt_str   rt_str_from(const char* p, size_t n);
rt_str   rt_str_from_cstr(const char* s);

// slicing (bounds-checked; returns empty on OOB)
rt_bytes rt_bytes_slice(rt_bytes b, size_t off, size_t n);
rt_str   rt_str_slice(rt_str s, size_t off, size_t n);

// comparisons
int  rt_bytes_cmp(rt_bytes a, rt_bytes b);
bool rt_bytes_eq(rt_bytes a, rt_bytes b);

int  rt_str_cmp(rt_str a, rt_str b);
bool rt_str_eq(rt_str a, rt_str b);

// prefix/suffix
bool rt_bytes_starts_with(rt_bytes a, rt_bytes prefix);
bool rt_bytes_ends_with(rt_bytes a, rt_bytes suffix);

bool rt_str_starts_with(rt_str a, rt_str prefix);
bool rt_str_ends_with(rt_str a, rt_str suffix);

// find
ssize_t rt_bytes_find(rt_bytes hay, rt_bytes needle);
ssize_t rt_str_find(rt_str hay, rt_str needle);

// trim (ASCII whitespace)
rt_str rt_str_trim(rt_str s);
rt_str rt_str_trim_left(rt_str s);
rt_str rt_str_trim_right(rt_str s);

// hash (FNV-1a 64)
uint64_t rt_bytes_hash64(rt_bytes b);
uint64_t rt_str_hash64(rt_str s);

// copy helpers
size_t rt_bytes_copy_to(rt_bytes b, void* out, size_t out_cap);
size_t rt_str_copy_to_cstr(rt_str s, char* out, size_t out_cap);

#endif // !RT_HAVE_RT_SLICE_H

//------------------------------------------------------------------------------
// Error
//------------------------------------------------------------------------------

static _Thread_local char g_slice_err_[256];

static void set_msg_(const char* msg)
{
    if (!msg) msg = "";
    size_t n = strlen(msg);
    if (n >= sizeof(g_slice_err_)) n = sizeof(g_slice_err_) - 1;
    memcpy(g_slice_err_, msg, n);
    g_slice_err_[n] = 0;
}

const char* rt_slice_last_error(void)
{
    return g_slice_err_;
}

//------------------------------------------------------------------------------
// Constructors
//------------------------------------------------------------------------------

rt_bytes rt_bytes_from(const void* p, size_t n)
{
    rt_bytes b;
    b.ptr = (const uint8_t*)p;
    b.len = (p && n) ? n : 0;
    return b;
}

rt_bytes rt_bytes_from_cstr(const char* s)
{
    if (!s)
        return rt_bytes_from(NULL, 0);
    return rt_bytes_from((const void*)s, strlen(s));
}

rt_str rt_str_from(const char* p, size_t n)
{
    rt_str s;
    s.ptr = p;
    s.len = (p && n) ? n : 0;
    return s;
}

rt_str rt_str_from_cstr(const char* s)
{
    if (!s)
        return rt_str_from(NULL, 0);
    return rt_str_from(s, strlen(s));
}

//------------------------------------------------------------------------------
// Slicing
//------------------------------------------------------------------------------

static bool range_ok_(size_t len, size_t off, size_t n)
{
    if (off > len) return false;
    if (n > len - off) return false;
    return true;
}

rt_bytes rt_bytes_slice(rt_bytes b, size_t off, size_t n)
{
    if (!range_ok_(b.len, off, n))
        return rt_bytes_from(NULL, 0);

    return rt_bytes_from(b.ptr + off, n);
}

rt_str rt_str_slice(rt_str s, size_t off, size_t n)
{
    if (!range_ok_(s.len, off, n))
        return rt_str_from(NULL, 0);

    return rt_str_from(s.ptr + off, n);
}

//------------------------------------------------------------------------------
// Comparisons
//------------------------------------------------------------------------------

int rt_bytes_cmp(rt_bytes a, rt_bytes b)
{
    size_t n = (a.len < b.len) ? a.len : b.len;
    if (n)
    {
        int c = memcmp(a.ptr, b.ptr, n);
        if (c != 0) return c;
    }

    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

bool rt_bytes_eq(rt_bytes a, rt_bytes b)
{
    return rt_bytes_cmp(a, b) == 0;
}

int rt_str_cmp(rt_str a, rt_str b)
{
    size_t n = (a.len < b.len) ? a.len : b.len;
    if (n)
    {
        int c = memcmp(a.ptr, b.ptr, n);
        if (c != 0) return c;
    }

    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

bool rt_str_eq(rt_str a, rt_str b)
{
    return rt_str_cmp(a, b) == 0;
}

//------------------------------------------------------------------------------
// Prefix/suffix
//------------------------------------------------------------------------------

bool rt_bytes_starts_with(rt_bytes a, rt_bytes prefix)
{
    if (prefix.len > a.len) return false;
    return (prefix.len == 0) || memcmp(a.ptr, prefix.ptr, prefix.len) == 0;
}

bool rt_bytes_ends_with(rt_bytes a, rt_bytes suffix)
{
    if (suffix.len > a.len) return false;
    if (suffix.len == 0) return true;
    return memcmp(a.ptr + (a.len - suffix.len), suffix.ptr, suffix.len) == 0;
}

bool rt_str_starts_with(rt_str a, rt_str prefix)
{
    if (prefix.len > a.len) return false;
    return (prefix.len == 0) || memcmp(a.ptr, prefix.ptr, prefix.len) == 0;
}

bool rt_str_ends_with(rt_str a, rt_str suffix)
{
    if (suffix.len > a.len) return false;
    if (suffix.len == 0) return true;
    return memcmp(a.ptr + (a.len - suffix.len), suffix.ptr, suffix.len) == 0;
}

//------------------------------------------------------------------------------
// Find
//------------------------------------------------------------------------------

static ssize_t find_bytes_(const uint8_t* h, size_t hlen, const uint8_t* n, size_t nlen)
{
    if (nlen == 0) return 0;
    if (nlen > hlen) return -1;

    // naive scan; can be replaced with KMP if needed.
    for (size_t i = 0; i + nlen <= hlen; i++)
    {
        if (h[i] == n[0] && memcmp(h + i, n, nlen) == 0)
            return (ssize_t)i;
    }

    return -1;
}

ssize_t rt_bytes_find(rt_bytes hay, rt_bytes needle)
{
    if (!hay.ptr && hay.len) return -1;
    if (!needle.ptr && needle.len) return -1;
    return find_bytes_(hay.ptr, hay.len, needle.ptr, needle.len);
}

ssize_t rt_str_find(rt_str hay, rt_str needle)
{
    rt_bytes h = rt_bytes_from(hay.ptr, hay.len);
    rt_bytes n = rt_bytes_from(needle.ptr, needle.len);
    return rt_bytes_find(h, n);
}

//------------------------------------------------------------------------------
// Trim
//------------------------------------------------------------------------------

static bool is_ws_(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}

rt_str rt_str_trim_left(rt_str s)
{
    size_t i = 0;
    while (i < s.len && is_ws_(s.ptr[i])) i++;
    return rt_str_slice(s, i, s.len - i);
}

rt_str rt_str_trim_right(rt_str s)
{
    if (s.len == 0) return s;

    size_t end = s.len;
    while (end > 0 && is_ws_(s.ptr[end - 1])) end--;
    return rt_str_slice(s, 0, end);
}

rt_str rt_str_trim(rt_str s)
{
    return rt_str_trim_right(rt_str_trim_left(s));
}

//------------------------------------------------------------------------------
// Hash
//------------------------------------------------------------------------------

uint64_t rt_bytes_hash64(rt_bytes b)
{
    // FNV-1a 64
    const uint64_t FNV_OFF = 1469598103934665603ull;
    const uint64_t FNV_PRIME = 1099511628211ull;

    uint64_t h = FNV_OFF;
    for (size_t i = 0; i < b.len; i++)
    {
        h ^= (uint64_t)b.ptr[i];
        h *= FNV_PRIME;
    }

    return h;
}

uint64_t rt_str_hash64(rt_str s)
{
    rt_bytes b = rt_bytes_from(s.ptr, s.len);
    return rt_bytes_hash64(b);
}

//------------------------------------------------------------------------------
// Copy helpers
//------------------------------------------------------------------------------

size_t rt_bytes_copy_to(rt_bytes b, void* out, size_t out_cap)
{
    if (!out || out_cap == 0)
        return 0;

    size_t n = (b.len < out_cap) ? b.len : out_cap;
    if (n)
        memcpy(out, b.ptr, n);

    return n;
}

size_t rt_str_copy_to_cstr(rt_str s, char* out, size_t out_cap)
{
    if (!out || out_cap == 0)
        return 0;

    size_t n = (s.len < (out_cap - 1)) ? s.len : (out_cap - 1);
    if (n)
        memcpy(out, s.ptr, n);

    out[n] = 0;
    return n;
}
