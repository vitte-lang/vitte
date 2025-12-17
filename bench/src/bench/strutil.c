

// strutil.c - small string utilities for vitte/bench (C17)
//
// Provides:
//   - bench_strdup / bench_strndup
//   - bench_stricmp / bench_strnicmp
//   - bench_strstarts / bench_strends
//   - bench_strtrim_ascii (in-place)
//   - bench_glob_match (very small glob: '*' and '?' only)
//
// SPDX-License-Identifier: MIT

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#if defined(__has_include)
  #if __has_include("bench/strutil.h")
    #include "bench/strutil.h"
  #elif __has_include("strutil.h")
    #include "strutil.h"
  #endif
#endif

// -----------------------------------------------------------------------------
// Fallback public surface (only if strutil.h was not included)
// -----------------------------------------------------------------------------

#ifndef VITTE_BENCH_STRUTIL_H
#define VITTE_BENCH_STRUTIL_H

char* bench_strdup(const char* s);
char* bench_strndup(const char* s, size_t n);

int bench_stricmp(const char* a, const char* b);
int bench_strnicmp(const char* a, const char* b, size_t n);

bool bench_strstarts(const char* s, const char* prefix);
bool bench_strends(const char* s, const char* suffix);

// Trim ASCII whitespace in-place. Returns pointer to first non-ws char
// within the same buffer.
char* bench_strtrim_ascii(char* s);

// Tiny glob matcher supporting '*' and '?' only. Case-sensitive.
// Returns true if `text` matches `pattern`.
bool bench_glob_match(const char* pattern, const char* text);

#endif // VITTE_BENCH_STRUTIL_H

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

static unsigned char su__tolower_ascii(unsigned char c)
{
    if (c >= 'A' && c <= 'Z') return (unsigned char)(c - 'A' + 'a');
    return c;
}

static bool su__is_ws_ascii(unsigned char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

char* bench_strdup(const char* s)
{
    if (!s) return NULL;
    size_t n = strlen(s);
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

char* bench_strndup(const char* s, size_t n)
{
    if (!s) return NULL;
    size_t len = 0;
    while (len < n && s[len] != '\0') len++;

    char* p = (char*)malloc(len + 1);
    if (!p) return NULL;
    memcpy(p, s, len);
    p[len] = '\0';
    return p;
}

int bench_stricmp(const char* a, const char* b)
{
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    while (*a && *b)
    {
        unsigned char ca = su__tolower_ascii((unsigned char)*a);
        unsigned char cb = su__tolower_ascii((unsigned char)*b);
        if (ca != cb) return (ca < cb) ? -1 : 1;
        ++a;
        ++b;
    }

    if (*a == *b) return 0;
    return (*a == '\0') ? -1 : 1;
}

int bench_strnicmp(const char* a, const char* b, size_t n)
{
    if (n == 0) return 0;
    if (a == b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    for (size_t i = 0; i < n; ++i)
    {
        unsigned char ca = su__tolower_ascii((unsigned char)a[i]);
        unsigned char cb = su__tolower_ascii((unsigned char)b[i]);

        if (a[i] == '\0' || b[i] == '\0')
        {
            if (a[i] == b[i]) return 0;
            return (a[i] == '\0') ? -1 : 1;
        }

        if (ca != cb) return (ca < cb) ? -1 : 1;
    }

    return 0;
}

bool bench_strstarts(const char* s, const char* prefix)
{
    if (!s || !prefix) return false;
    size_t n = strlen(prefix);
    return strncmp(s, prefix, n) == 0;
}

bool bench_strends(const char* s, const char* suffix)
{
    if (!s || !suffix) return false;
    size_t ls = strlen(s);
    size_t lf = strlen(suffix);
    if (lf > ls) return false;
    return memcmp(s + (ls - lf), suffix, lf) == 0;
}

char* bench_strtrim_ascii(char* s)
{
    if (!s) return NULL;

    // leading
    char* p = s;
    while (*p && su__is_ws_ascii((unsigned char)*p)) p++;

    // all whitespace
    if (*p == '\0')
    {
        s[0] = '\0';
        return s;
    }

    // trailing
    char* end = p + strlen(p);
    while (end > p && su__is_ws_ascii((unsigned char)end[-1])) end--;
    *end = '\0';

    // if p != s, shift in-place
    if (p != s)
    {
        size_t len = (size_t)(end - p);
        memmove(s, p, len);
        s[len] = '\0';
        return s;
    }

    return p;
}

static bool su__glob_match_here(const char* pat, const char* txt)
{
    // Iterative with backtracking on '*'
    const char* p = pat;
    const char* t = txt;

    const char* star = NULL;
    const char* star_txt = NULL;

    while (*t)
    {
        if (*p == '*')
        {
            // collapse consecutive '*'
            while (*p == '*') p++;
            if (*p == '\0') return true; // trailing '*' matches rest
            star = p;
            star_txt = t;
            continue;
        }

        if (*p == '?' || *p == *t)
        {
            p++;
            t++;
            continue;
        }

        if (star)
        {
            // backtrack: let '*' consume one more char
            star_txt++;
            t = star_txt;
            p = star;
            continue;
        }

        return false;
    }

    // consume remaining '*' in pattern
    while (*p == '*') p++;
    return *p == '\0';
}

bool bench_glob_match(const char* pattern, const char* text)
{
    if (!pattern || !text) return false;
    return su__glob_match_here(pattern, text);
}