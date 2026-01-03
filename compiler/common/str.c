// SPDX-License-Identifier: MIT
// str.c
//
// Small string utilities for vitte.
//
// Goals:
//  - Dependency-light helpers frequently needed across CLI/runtime.
//  - No allocations unless explicitly requested.
//
// This file pairs with str.h.
// str.h is expected to declare the functions implemented here.
// If your str.h differs, adjust accordingly.

#include "str.h"

#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#ifndef STEEL_STR_TMP_MAX
  #define STEEL_STR_TMP_MAX 4096
#endif

//------------------------------------------------------------------------------
// Basic helpers
//------------------------------------------------------------------------------

bool steel_str_eq(const char* a, const char* b)
{
    if (a == b) return true;
    if (!a || !b) return false;
    return strcmp(a, b) == 0;
}

bool steel_str_ieq(const char* a, const char* b)
{
    if (a == b) return true;
    if (!a || !b) return false;

    while (*a && *b)
    {
        unsigned char ca = (unsigned char)*a++;
        unsigned char cb = (unsigned char)*b++;
        if (tolower(ca) != tolower(cb)) return false;
    }
    return *a == 0 && *b == 0;
}

bool steel_str_starts_with(const char* s, const char* prefix)
{
    if (!s || !prefix) return false;
    size_t n = strlen(prefix);
    return strncmp(s, prefix, n) == 0;
}

bool steel_str_ends_with(const char* s, const char* suffix)
{
    if (!s || !suffix) return false;
    size_t n = strlen(s);
    size_t m = strlen(suffix);
    if (m > n) return false;
    return memcmp(s + (n - m), suffix, m) == 0;
}

//------------------------------------------------------------------------------
// Trimming
//------------------------------------------------------------------------------

size_t steel_str_trim_left(const char* s, size_t len)
{
    size_t i = 0;
    while (i < len && s[i] && isspace((unsigned char)s[i])) i++;
    return i;
}

size_t steel_str_trim_right(const char* s, size_t len)
{
    size_t n = len;
    while (n > 0)
    {
        unsigned char c = (unsigned char)s[n - 1];
        if (c == 0) { n--; continue; }
        if (!isspace(c)) break;
        n--;
    }
    return n;
}

size_t steel_str_trim(const char* s, size_t len, size_t* out_begin)
{
    if (!s) { if (out_begin) *out_begin = 0; return 0; }
    size_t b = steel_str_trim_left(s, len);
    size_t e = steel_str_trim_right(s, len);
    if (e < b) e = b;
    if (out_begin) *out_begin = b;
    return e;
}

//------------------------------------------------------------------------------
// Safe copy/concat
//------------------------------------------------------------------------------

size_t steel_strlcpy(char* dst, const char* src, size_t cap)
{
    if (!dst || cap == 0) return 0;
    if (!src) { dst[0] = 0; return 0; }

    size_t n = strlen(src);
    size_t k = (n < cap - 1) ? n : (cap - 1);
    memcpy(dst, src, k);
    dst[k] = 0;
    return n;
}

size_t steel_strlcat(char* dst, const char* src, size_t cap)
{
    if (!dst || cap == 0) return 0;
    if (!src) return strlen(dst);

    size_t dlen = strnlen(dst, cap);
    size_t slen = strlen(src);

    if (dlen == cap) return cap + slen;

    size_t room = cap - dlen - 1;
    size_t k = (slen < room) ? slen : room;
    memcpy(dst + dlen, src, k);
    dst[dlen + k] = 0;
    return dlen + slen;
}

//------------------------------------------------------------------------------
// Split
//------------------------------------------------------------------------------

bool steel_str_split1(const char* s, char sep, steel_str_view* out_a, steel_str_view* out_b)
{
    if (!s || !out_a || !out_b) return false;

    const char* p = strchr(s, (int)sep);
    if (!p) return false;

    out_a->ptr = s;
    out_a->len = (size_t)(p - s);

    out_b->ptr = p + 1;
    out_b->len = strlen(p + 1);

    return true;
}

//------------------------------------------------------------------------------
// ASCII helpers
//------------------------------------------------------------------------------

void steel_str_to_lower_ascii(char* s)
{
    if (!s) return;
    for (; *s; s++)
        *s = (char)tolower((unsigned char)*s);
}

void steel_str_to_upper_ascii(char* s)
{
    if (!s) return;
    for (; *s; s++)
        *s = (char)toupper((unsigned char)*s);
}

//------------------------------------------------------------------------------
// Parsing
//------------------------------------------------------------------------------

bool steel_str_parse_i64(const char* s, int64_t* out)
{
    if (out) *out = 0;
    if (!s || !*s) return false;

    char* end = NULL;
    errno = 0;
    long long v = strtoll(s, &end, 10);
    if (errno != 0) return false;
    if (end == s || *end != 0) return false;
    if (out) *out = (int64_t)v;
    return true;
}

bool steel_str_parse_u64(const char* s, uint64_t* out)
{
    if (out) *out = 0;
    if (!s || !*s) return false;

    char* end = NULL;
    errno = 0;
    unsigned long long v = strtoull(s, &end, 10);
    if (errno != 0) return false;
    if (end == s || *end != 0) return false;
    if (out) *out = (uint64_t)v;
    return true;
}

bool steel_str_parse_bool(const char* s, bool* out)
{
    if (out) *out = false;
    if (!s) return false;

    if (steel_str_ieq(s, "1") || steel_str_ieq(s, "true") || steel_str_ieq(s, "yes") || steel_str_ieq(s, "on"))
    {
        if (out) *out = true;
        return true;
    }

    if (steel_str_ieq(s, "0") || steel_str_ieq(s, "false") || steel_str_ieq(s, "no") || steel_str_ieq(s, "off"))
    {
        if (out) *out = false;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
// Allocate helpers
//------------------------------------------------------------------------------

char* steel_strdup(const char* s)
{
    if (!s) return NULL;
    size_t n = strlen(s);
    char* d = (char*)malloc(n + 1);
    if (!d) return NULL;
    memcpy(d, s, n + 1);
    return d;
}

char* steel_strndup(const char* s, size_t n)
{
    if (!s) return NULL;
    size_t sl = strlen(s);
    if (n > sl) n = sl;

    char* d = (char*)malloc(n + 1);
    if (!d) return NULL;
    memcpy(d, s, n);
    d[n] = 0;
    return d;
}

//------------------------------------------------------------------------------
// Formatting helpers
//------------------------------------------------------------------------------

int steel_snprintf(char* out, size_t out_cap, const char* fmt, ...)
{
    if (!out || out_cap == 0) return -1;

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(out, out_cap, fmt, ap);
    va_end(ap);

    // Ensure NUL termination even on truncation.
    out[out_cap - 1] = 0;
    return n;
}
