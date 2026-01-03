// SPDX-License-Identifier: MIT
// rt_string.c
//
// Runtime string utilities (max).
//
// This module provides:
//  - Owned, growable string buffer (rt_string)
//  - Append/insert/erase/replace
//  - Formatting append (printf-style)
//  - UTF-8 aware validation + codepoint iteration helpers (minimal)
//  - Small helpers: starts/ends/contains, split (iterator-style)
//
// Integration:
//  - If `rt_string.h` exists, include it.
//  - Otherwise provide a fallback API.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#if defined(__has_include)
  #if __has_include("rt_string.h")
    #include "rt_string.h"
    #define RT_HAVE_RT_STRING_H 1
  #elif __has_include("../runtime/rt_string.h")
    #include "../runtime/rt_string.h"
    #define RT_HAVE_RT_STRING_H 1
  #endif
#endif

#ifndef RT_HAVE_RT_STRING_H

// Forward decl allocator (rt_alloc)
typedef struct rt_allocator rt_allocator;

// Owned string

typedef struct rt_string
{
    rt_allocator* alloc; // not owned
    char*  data;         // owned
    size_t len;          // excluding NUL
    size_t cap;          // including NUL
} rt_string;

typedef struct rt_str_view
{
    const char* ptr;
    size_t len;
} rt_str_view;

typedef struct rt_split_iter
{
    rt_str_view s;
    rt_str_view sep;
    size_t pos;
    bool done;
} rt_split_iter;

void rt_string_init(rt_string* s, rt_allocator* a);
void rt_string_dispose(rt_string* s);

bool rt_string_reserve(rt_string* s, size_t cap);
void rt_string_clear(rt_string* s);

bool rt_string_set(rt_string* s, const char* cstr);
bool rt_string_set_n(rt_string* s, const char* p, size_t n);

bool rt_string_append(rt_string* s, const char* cstr);
bool rt_string_append_n(rt_string* s, const char* p, size_t n);

bool rt_string_append_char(rt_string* s, char c);

bool rt_string_insert_n(rt_string* s, size_t off, const char* p, size_t n);
bool rt_string_erase(rt_string* s, size_t off, size_t n);
bool rt_string_replace_n(rt_string* s, size_t off, size_t n, const char* p, size_t rep_n);

bool rt_string_appendf(rt_string* s, const char* fmt, ...);
bool rt_string_appendfv(rt_string* s, const char* fmt, va_list ap);

rt_str_view rt_str_view_from(const char* p, size_t n);
rt_str_view rt_str_view_cstr(const char* s);

bool rt_str_view_eq(rt_str_view a, rt_str_view b);
bool rt_str_view_starts_with(rt_str_view a, rt_str_view prefix);
bool rt_str_view_ends_with(rt_str_view a, rt_str_view suffix);
ssize_t rt_str_view_find(rt_str_view hay, rt_str_view needle);

// split iterator
rt_split_iter rt_split_iter_make(rt_str_view s, rt_str_view sep);
bool rt_split_next(rt_split_iter* it, rt_str_view* out);

// UTF-8
bool rt_utf8_validate(rt_str_view s);
uint32_t rt_utf8_next(const char* p, size_t len, size_t* io_off); // returns codepoint or 0xFFFFFFFF on error

const char* rt_string_last_error(void);

#endif // !RT_HAVE_RT_STRING_H

//------------------------------------------------------------------------------
// Dependencies (allocator)
//------------------------------------------------------------------------------

#if !defined(RT_HAVE_RT_ALLOC_H)
void* rt_malloc(rt_allocator* a, size_t n);
void* rt_realloc(rt_allocator* a, void* p, size_t n);
void  rt_free(rt_allocator* a, void* p);
#endif

//------------------------------------------------------------------------------
// Error
//------------------------------------------------------------------------------

static _Thread_local char g_str_err_[256];

static void set_msg_(const char* msg)
{
    if (!msg) msg = "";
    size_t n = strlen(msg);
    if (n >= sizeof(g_str_err_)) n = sizeof(g_str_err_) - 1;
    memcpy(g_str_err_, msg, n);
    g_str_err_[n] = 0;
}

const char* rt_string_last_error(void)
{
    return g_str_err_;
}

//------------------------------------------------------------------------------
// Internal helpers
//------------------------------------------------------------------------------

static size_t max_(size_t a, size_t b) { return a > b ? a : b; }

static bool ensure_cap_(rt_string* s, size_t want_cap)
{
    if (!s)
        return false;

    if (want_cap < 1) want_cap = 1;

    if (s->cap >= want_cap)
        return true;

    size_t new_cap = s->cap ? s->cap : 16;
    while (new_cap < want_cap)
    {
        size_t next = new_cap * 2;
        if (next < new_cap)
        {
            set_msg_("capacity overflow");
            return false;
        }
        new_cap = next;
    }

    char* nd = (char*)rt_realloc(s->alloc, s->data, new_cap);
    if (!nd)
    {
        set_msg_("out of memory");
        return false;
    }

    s->data = nd;
    s->cap = new_cap;

    if (s->len + 1 <= s->cap)
        s->data[s->len] = 0;

    return true;
}

static void normalize_(rt_string* s)
{
    if (!s) return;
    if (!s->data)
    {
        s->cap = 0;
        s->len = 0;
        return;
    }

    if (s->cap == 0)
        s->cap = 1;

    if (s->len + 1 > s->cap)
        s->len = s->cap ? (s->cap - 1) : 0;

    s->data[s->len] = 0;
}

//------------------------------------------------------------------------------
// Public: string buffer
//------------------------------------------------------------------------------

void rt_string_init(rt_string* s, rt_allocator* a)
{
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->alloc = a;
}

void rt_string_dispose(rt_string* s)
{
    if (!s) return;
    if (s->data)
        rt_free(s->alloc, s->data);
    memset(s, 0, sizeof(*s));
}

bool rt_string_reserve(rt_string* s, size_t cap)
{
    if (!s)
    {
        set_msg_("invalid string");
        return false;
    }
    return ensure_cap_(s, cap);
}

void rt_string_clear(rt_string* s)
{
    if (!s) return;
    s->len = 0;
    if (s->data && s->cap)
        s->data[0] = 0;
}

bool rt_string_set_n(rt_string* s, const char* p, size_t n)
{
    if (!s)
    {
        set_msg_("invalid string");
        return false;
    }

    if (!p && n)
    {
        set_msg_("null pointer");
        return false;
    }

    if (!ensure_cap_(s, n + 1))
        return false;

    if (n)
        memcpy(s->data, p, n);

    s->len = n;
    s->data[s->len] = 0;
    return true;
}

bool rt_string_set(rt_string* s, const char* cstr)
{
    if (!cstr) cstr = "";
    return rt_string_set_n(s, cstr, strlen(cstr));
}

bool rt_string_append_n(rt_string* s, const char* p, size_t n)
{
    if (!s)
    {
        set_msg_("invalid string");
        return false;
    }

    if (!p && n)
    {
        set_msg_("null pointer");
        return false;
    }

    if (!ensure_cap_(s, s->len + n + 1))
        return false;

    if (n)
        memcpy(s->data + s->len, p, n);

    s->len += n;
    s->data[s->len] = 0;
    return true;
}

bool rt_string_append(rt_string* s, const char* cstr)
{
    if (!cstr) cstr = "";
    return rt_string_append_n(s, cstr, strlen(cstr));
}

bool rt_string_append_char(rt_string* s, char c)
{
    return rt_string_append_n(s, &c, 1);
}

bool rt_string_insert_n(rt_string* s, size_t off, const char* p, size_t n)
{
    if (!s)
    {
        set_msg_("invalid string");
        return false;
    }

    if (off > s->len)
    {
        set_msg_("offset out of range");
        return false;
    }

    if (!p && n)
    {
        set_msg_("null pointer");
        return false;
    }

    if (n == 0)
        return true;

    if (!ensure_cap_(s, s->len + n + 1))
        return false;

    memmove(s->data + off + n, s->data + off, (s->len - off) + 1); // include NUL
    memcpy(s->data + off, p, n);
    s->len += n;
    return true;
}

bool rt_string_erase(rt_string* s, size_t off, size_t n)
{
    if (!s)
    {
        set_msg_("invalid string");
        return false;
    }

    if (off > s->len)
    {
        set_msg_("offset out of range");
        return false;
    }

    if (n > s->len - off)
        n = s->len - off;

    if (n == 0)
        return true;

    memmove(s->data + off, s->data + off + n, (s->len - (off + n)) + 1);
    s->len -= n;
    return true;
}

bool rt_string_replace_n(rt_string* s, size_t off, size_t n, const char* p, size_t rep_n)
{
    if (!s)
    {
        set_msg_("invalid string");
        return false;
    }

    if (off > s->len)
    {
        set_msg_("offset out of range");
        return false;
    }

    if (n > s->len - off)
        n = s->len - off;

    if (!p && rep_n)
    {
        set_msg_("null pointer");
        return false;
    }

    // Common cases
    if (n == rep_n)
    {
        if (rep_n) memcpy(s->data + off, p, rep_n);
        return true;
    }

    // Erase then insert with capacity planning.
    size_t new_len = s->len - n + rep_n;
    if (!ensure_cap_(s, new_len + 1))
        return false;

    if (rep_n > n)
    {
        // grow gap
        size_t delta = rep_n - n;
        memmove(s->data + off + rep_n, s->data + off + n, (s->len - (off + n)) + 1);
        if (rep_n) memcpy(s->data + off, p, rep_n);
        s->len += delta;
        return true;
    }
    else
    {
        // shrink gap
        size_t delta = n - rep_n;
        if (rep_n) memcpy(s->data + off, p, rep_n);
        memmove(s->data + off + rep_n, s->data + off + n, (s->len - (off + n)) + 1);
        s->len -= delta;
        return true;
    }
}

bool rt_string_appendfv(rt_string* s, const char* fmt, va_list ap)
{
    if (!s)
    {
        set_msg_("invalid string");
        return false;
    }

    if (!fmt) fmt = "";

    // Try into a small stack buffer first.
    char tmp[512];

    va_list aq;
    va_copy(aq, ap);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, aq);
    va_end(aq);

    if (n < 0)
    {
        set_msg_("format error");
        return false;
    }

    if ((size_t)n < sizeof(tmp))
    {
        return rt_string_append_n(s, tmp, (size_t)n);
    }

    // Need bigger buffer.
    size_t need = (size_t)n + 1;
    char* buf = (char*)rt_malloc(s->alloc, need);
    if (!buf)
    {
        set_msg_("out of memory");
        return false;
    }

    va_list ar;
    va_copy(ar, ap);
    int n2 = vsnprintf(buf, need, fmt, ar);
    va_end(ar);

    if (n2 < 0)
    {
        rt_free(s->alloc, buf);
        set_msg_("format error");
        return false;
    }

    bool ok = rt_string_append_n(s, buf, (size_t)n2);
    rt_free(s->alloc, buf);
    return ok;
}

bool rt_string_appendf(rt_string* s, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    bool ok = rt_string_appendfv(s, fmt, ap);
    va_end(ap);
    return ok;
}

//------------------------------------------------------------------------------
// Views
//------------------------------------------------------------------------------

rt_str_view rt_str_view_from(const char* p, size_t n)
{
    rt_str_view v;
    v.ptr = p;
    v.len = (p && n) ? n : 0;
    return v;
}

rt_str_view rt_str_view_cstr(const char* s)
{
    if (!s) return rt_str_view_from(NULL, 0);
    return rt_str_view_from(s, strlen(s));
}

static int view_cmp_(rt_str_view a, rt_str_view b)
{
    size_t n = a.len < b.len ? a.len : b.len;
    if (n)
    {
        int c = memcmp(a.ptr, b.ptr, n);
        if (c != 0) return c;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

bool rt_str_view_eq(rt_str_view a, rt_str_view b)
{
    return view_cmp_(a, b) == 0;
}

bool rt_str_view_starts_with(rt_str_view a, rt_str_view prefix)
{
    if (prefix.len > a.len) return false;
    return prefix.len == 0 || memcmp(a.ptr, prefix.ptr, prefix.len) == 0;
}

bool rt_str_view_ends_with(rt_str_view a, rt_str_view suffix)
{
    if (suffix.len > a.len) return false;
    if (suffix.len == 0) return true;
    return memcmp(a.ptr + (a.len - suffix.len), suffix.ptr, suffix.len) == 0;
}

static ssize_t view_find_(rt_str_view hay, rt_str_view needle)
{
    if (needle.len == 0) return 0;
    if (needle.len > hay.len) return -1;

    const char* h = hay.ptr;
    const char* n = needle.ptr;

    for (size_t i = 0; i + needle.len <= hay.len; i++)
    {
        if (h[i] == n[0] && memcmp(h + i, n, needle.len) == 0)
            return (ssize_t)i;
    }

    return -1;
}

ssize_t rt_str_view_find(rt_str_view hay, rt_str_view needle)
{
    if ((!hay.ptr && hay.len) || (!needle.ptr && needle.len))
        return -1;
    return view_find_(hay, needle);
}

//------------------------------------------------------------------------------
// Split iterator
//------------------------------------------------------------------------------

rt_split_iter rt_split_iter_make(rt_str_view s, rt_str_view sep)
{
    rt_split_iter it;
    it.s = s;
    it.sep = sep;
    it.pos = 0;
    it.done = false;
    return it;
}

bool rt_split_next(rt_split_iter* it, rt_str_view* out)
{
    if (!it || !out)
        return false;

    if (it->done)
        return false;

    // If sep empty: yield whole once.
    if (it->sep.len == 0)
    {
        *out = it->s;
        it->done = true;
        return true;
    }

    if (it->pos > it->s.len)
    {
        it->done = true;
        return false;
    }

    rt_str_view rest = rt_str_view_from(it->s.ptr + it->pos, it->s.len - it->pos);
    ssize_t at = view_find_(rest, it->sep);

    if (at < 0)
    {
        // last token
        *out = rest;
        it->done = true;
        return true;
    }

    size_t off = (size_t)at;
    *out = rt_str_view_from(rest.ptr, off);
    it->pos += off + it->sep.len;
    return true;
}

//------------------------------------------------------------------------------
// UTF-8 (minimal)
//------------------------------------------------------------------------------

static bool utf8_is_cont_(uint8_t b)
{
    return (b & 0xC0u) == 0x80u;
}

uint32_t rt_utf8_next(const char* p, size_t len, size_t* io_off)
{
    if (!p || !io_off) return 0xFFFFFFFFu;
    size_t i = *io_off;
    if (i >= len) return 0xFFFFFFFFu;

    uint8_t b0 = (uint8_t)p[i++];
    if (b0 < 0x80u)
    {
        *io_off = i;
        return (uint32_t)b0;
    }

    // 2-byte
    if ((b0 & 0xE0u) == 0xC0u)
    {
        if (i >= len) return 0xFFFFFFFFu;
        uint8_t b1 = (uint8_t)p[i++];
        if (!utf8_is_cont_(b1)) return 0xFFFFFFFFu;

        uint32_t cp = ((uint32_t)(b0 & 0x1Fu) << 6) | (uint32_t)(b1 & 0x3Fu);
        // overlong
        if (cp < 0x80u) return 0xFFFFFFFFu;
        *io_off = i;
        return cp;
    }

    // 3-byte
    if ((b0 & 0xF0u) == 0xE0u)
    {
        if (i + 1 > len) return 0xFFFFFFFFu;
        if (i >= len) return 0xFFFFFFFFu;
        uint8_t b1 = (uint8_t)p[i++];
        if (i >= len) return 0xFFFFFFFFu;
        uint8_t b2 = (uint8_t)p[i++];
        if (!utf8_is_cont_(b1) || !utf8_is_cont_(b2)) return 0xFFFFFFFFu;

        uint32_t cp = ((uint32_t)(b0 & 0x0Fu) << 12) |
                      ((uint32_t)(b1 & 0x3Fu) << 6) |
                      (uint32_t)(b2 & 0x3Fu);
        // overlong
        if (cp < 0x800u) return 0xFFFFFFFFu;
        // surrogate
        if (cp >= 0xD800u && cp <= 0xDFFFu) return 0xFFFFFFFFu;

        *io_off = i;
        return cp;
    }

    // 4-byte
    if ((b0 & 0xF8u) == 0xF0u)
    {
        if (i + 2 > len) return 0xFFFFFFFFu;
        if (i >= len) return 0xFFFFFFFFu;
        uint8_t b1 = (uint8_t)p[i++];
        if (i >= len) return 0xFFFFFFFFu;
        uint8_t b2 = (uint8_t)p[i++];
        if (i >= len) return 0xFFFFFFFFu;
        uint8_t b3 = (uint8_t)p[i++];
        if (!utf8_is_cont_(b1) || !utf8_is_cont_(b2) || !utf8_is_cont_(b3)) return 0xFFFFFFFFu;

        uint32_t cp = ((uint32_t)(b0 & 0x07u) << 18) |
                      ((uint32_t)(b1 & 0x3Fu) << 12) |
                      ((uint32_t)(b2 & 0x3Fu) << 6) |
                      (uint32_t)(b3 & 0x3Fu);
        // range
        if (cp < 0x10000u || cp > 0x10FFFFu) return 0xFFFFFFFFu;

        *io_off = i;
        return cp;
    }

    return 0xFFFFFFFFu;
}

bool rt_utf8_validate(rt_str_view s)
{
    if (!s.ptr && s.len) return false;

    size_t off = 0;
    while (off < s.len)
    {
        uint32_t cp = rt_utf8_next(s.ptr, s.len, &off);
        if (cp == 0xFFFFFFFFu)
            return false;
    }
    return true;
}

