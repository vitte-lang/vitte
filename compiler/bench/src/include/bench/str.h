#ifndef VITTE_BENCH_STR_H
#define VITTE_BENCH_STR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

typedef struct {
    const char* ptr;
    size_t len;
} vitte_str;

typedef struct vitte_arena vitte_arena;
typedef struct vitte_diag vitte_diag;

/* Creation */

static inline vitte_str vitte_str_from_cstr(const char* cstr) {
    size_t len = 0;
    while (cstr[len]) len++;
    return (vitte_str){cstr, len};
}

static inline vitte_str vitte_str_from_ptr_len(const char* ptr, size_t len) {
    return (vitte_str){ptr, len};
}

/* Comparisons */

/* Prefix / suffix */

/* Trimming */

/* Search / split */

/* Arena-backed helpers */

#endif /* VITTE_BENCH_STR_H */

/*
  bench/str.h

  String utilities for the Vitte benchmark harness.

  Goals
  - Provide small, dependency-free helpers over UTF-8 byte strings.
  - Avoid libc locale behavior; be explicit about ASCII operations.
  - Provide arena-backed formatting and duplication helpers.

  Scope
  - ASCII case folding / comparisons.
  - Prefix/suffix checks.
  - Trim/split minimal utilities.
  - Deterministic hashing helper hooks (bench/hash.h).

  Memory
  - Functions returning strings allocate in vitte_arena.
  - Non-owning slices represented by (ptr,len).
*/

#pragma once
#ifndef VITTE_BENCH_STR_H
#define VITTE_BENCH_STR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/arena.h"
#include "bench/diag.h"
#include "bench/detail/format.h"
#include "bench/hash.h"

/* -------------------------------------------------------------------------- */
/* Slice                                                                        */
/* -------------------------------------------------------------------------- */

typedef struct vitte_str {
    const char* ptr;
    size_t len;
} vitte_str;

static inline vitte_str vitte_str_make(const char* p, size_t n)
{
    vitte_str s; s.ptr = p; s.len = n; return s;
}

static inline vitte_str vitte_str_cstr(const char* s)
{
    vitte_str out;
    out.ptr = s ? s : "";
    out.len = 0;
    while (out.ptr[out.len]) out.len++;
    return out;
}

static inline bool vitte_str_empty(vitte_str s) { return s.len == 0; }

/* -------------------------------------------------------------------------- */
/* ASCII utilities                                                              */
/* -------------------------------------------------------------------------- */

static inline char vitte_ascii_tolower(char c)
{
    return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

static inline char vitte_ascii_toupper(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - ('a' - 'A')) : c;
}

static inline bool vitte_ascii_is_space(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v');
}

static inline bool vitte_ascii_is_digit(char c)
{
    return (c >= '0' && c <= '9');
}

static inline bool vitte_ascii_is_alpha(char c)
{
    c = vitte_ascii_tolower(c);
    return (c >= 'a' && c <= 'z');
}

static inline bool vitte_ascii_is_alnum(char c)
{
    return vitte_ascii_is_alpha(c) || vitte_ascii_is_digit(c);
}

/* -------------------------------------------------------------------------- */
/* Comparisons                                                                  */
/* -------------------------------------------------------------------------- */

/* Compare two slices lexicographically (byte order). */
int vitte_str_cmp(vitte_str a, vitte_str b);

/* Compare two slices, ASCII-case-insensitive. */
int vitte_str_icmp(vitte_str a, vitte_str b);

/* Compare two C strings, case-sensitive. */
int vitte_str_cmp_cstr(const char* a, const char* b);

/* Compare two C strings, ASCII-case-insensitive. */
int vitte_str_icmp_cstr(const char* a, const char* b);

/* -------------------------------------------------------------------------- */
/* Prefix / suffix                                                              */
/* -------------------------------------------------------------------------- */

bool vitte_str_starts_with(vitte_str s, vitte_str prefix);
bool vitte_str_ends_with(vitte_str s, vitte_str suffix);

bool vitte_str_istarts_with(vitte_str s, vitte_str prefix);
bool vitte_str_iends_with(vitte_str s, vitte_str suffix);

/* -------------------------------------------------------------------------- */
/* Trimming                                                                     */
/* -------------------------------------------------------------------------- */

/* Trim ASCII whitespace from both ends (non-owning). */
vitte_str vitte_str_trim(vitte_str s);

/* Trim only left/right. */
vitte_str vitte_str_ltrim(vitte_str s);
vitte_str vitte_str_rtrim(vitte_str s);

/* -------------------------------------------------------------------------- */
/* Search / split                                                               */
/* -------------------------------------------------------------------------- */

/* Find first occurrence of a byte (returns index or SIZE_MAX). */
size_t vitte_str_find_byte(vitte_str s, char needle);

/* Find last occurrence of a byte (returns index or SIZE_MAX). */
size_t vitte_str_rfind_byte(vitte_str s, char needle);

/* Find substring (naive, deterministic). Returns index or SIZE_MAX. */
size_t vitte_str_find(vitte_str haystack, vitte_str needle);

/* Split once on delimiter; returns true if found. */
bool vitte_str_split_once(vitte_str s, char delim, vitte_str* left, vitte_str* right);

/* -------------------------------------------------------------------------- */
/* Arena-backed helpers                                                         */
/* -------------------------------------------------------------------------- */

/* Duplicate into arena; always NUL-terminates. */
const char* vitte_str_dup(vitte_arena* arena, vitte_str s, vitte_diag* d);

/* Format into arena via vitte_buf (printf-like). */
const char* vitte_str_printf(vitte_arena* arena, vitte_diag* d, const char* fmt, ...);

/* Join two slices with a separator (can be empty). */
const char* vitte_str_join2(vitte_arena* arena, vitte_str a, vitte_str sep, vitte_str b, vitte_diag* d);

/* -------------------------------------------------------------------------- */
/* Hashing                                                                      */
/* -------------------------------------------------------------------------- */

static inline uint64_t vitte_str_hash64(vitte_str s)
{
    /* Hash bytes deterministically */
    return vitte_bench_hash64_bytes((const uint8_t*)s.ptr, s.len);
}

/* -------------------------------------------------------------------------- */
/* Implementation (header-only for most helpers)                               */
/* -------------------------------------------------------------------------- */

static inline int vitte_str_cmp(vitte_str a, vitte_str b)
{
    size_t i = 0;
    const size_t n = (a.len < b.len) ? a.len : b.len;
    for (; i < n; ++i) {
        const unsigned char ca = (unsigned char)a.ptr[i];
        const unsigned char cb = (unsigned char)b.ptr[i];
        if (ca < cb) return -1;
        if (ca > cb) return 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

static inline int vitte_str_icmp(vitte_str a, vitte_str b)
{
    size_t i = 0;
    const size_t n = (a.len < b.len) ? a.len : b.len;
    for (; i < n; ++i) {
        unsigned char ca = (unsigned char)a.ptr[i];
        unsigned char cb = (unsigned char)b.ptr[i];
        ca = (unsigned char)vitte_ascii_tolower((char)ca);
        cb = (unsigned char)vitte_ascii_tolower((char)cb);
        if (ca < cb) return -1;
        if (ca > cb) return 1;
    }
    if (a.len < b.len) return -1;
    if (a.len > b.len) return 1;
    return 0;
}

static inline int vitte_str_cmp_cstr(const char* a, const char* b)
{
    if (!a) a = "";
    if (!b) b = "";
    while (*a && *b) {
        const unsigned char ca = (unsigned char)*a;
        const unsigned char cb = (unsigned char)*b;
        if (ca < cb) return -1;
        if (ca > cb) return 1;
        ++a; ++b;
    }
    if (*a) return 1;
    if (*b) return -1;
    return 0;
}

static inline int vitte_str_icmp_cstr(const char* a, const char* b)
{
    if (!a) a = "";
    if (!b) b = "";
    while (*a && *b) {
        unsigned char ca = (unsigned char)vitte_ascii_tolower(*a);
        unsigned char cb = (unsigned char)vitte_ascii_tolower(*b);
        if (ca < cb) return -1;
        if (ca > cb) return 1;
        ++a; ++b;
    }
    if (*a) return 1;
    if (*b) return -1;
    return 0;
}

static inline bool vitte_str_starts_with(vitte_str s, vitte_str prefix)
{
    if (prefix.len > s.len) return false;
    return vitte_str_cmp(vitte_str_make(s.ptr, prefix.len), prefix) == 0;
}

static inline bool vitte_str_ends_with(vitte_str s, vitte_str suffix)
{
    if (suffix.len > s.len) return false;
    return vitte_str_cmp(vitte_str_make(s.ptr + (s.len - suffix.len), suffix.len), suffix) == 0;
}

static inline bool vitte_str_istarts_with(vitte_str s, vitte_str prefix)
{
    if (prefix.len > s.len) return false;
    return vitte_str_icmp(vitte_str_make(s.ptr, prefix.len), prefix) == 0;
}

static inline bool vitte_str_iends_with(vitte_str s, vitte_str suffix)
{
    if (suffix.len > s.len) return false;
    return vitte_str_icmp(vitte_str_make(s.ptr + (s.len - suffix.len), suffix.len), suffix) == 0;
}

static inline vitte_str vitte_str_ltrim(vitte_str s)
{
    size_t i = 0;
    while (i < s.len && vitte_ascii_is_space(s.ptr[i])) i++;
    return vitte_str_make(s.ptr + i, s.len - i);
}

static inline vitte_str vitte_str_rtrim(vitte_str s)
{
    size_t n = s.len;
    while (n > 0 && vitte_ascii_is_space(s.ptr[n - 1])) n--;
    return vitte_str_make(s.ptr, n);
}

static inline vitte_str vitte_str_trim(vitte_str s)
{
    return vitte_str_rtrim(vitte_str_ltrim(s));
}

static inline size_t vitte_str_find_byte(vitte_str s, char needle)
{
    for (size_t i = 0; i < s.len; ++i) {
        if (s.ptr[i] == needle) return i;
    }
    return (size_t)-1;
}

static inline size_t vitte_str_rfind_byte(vitte_str s, char needle)
{
    for (size_t i = s.len; i > 0; --i) {
        if (s.ptr[i - 1] == needle) return i - 1;
    }
    return (size_t)-1;
}

static inline size_t vitte_str_find(vitte_str haystack, vitte_str needle)
{
    if (needle.len == 0) return 0;
    if (needle.len > haystack.len) return (size_t)-1;

    for (size_t i = 0; i + needle.len <= haystack.len; ++i) {
        if (vitte_str_cmp(vitte_str_make(haystack.ptr + i, needle.len), needle) == 0) {
            return i;
        }
    }
    return (size_t)-1;
}

static inline bool vitte_str_split_once(vitte_str s, char delim, vitte_str* left, vitte_str* right)
{
    size_t idx = vitte_str_find_byte(s, delim);
    if (idx == (size_t)-1) {
        if (left) *left = s;
        if (right) *right = vitte_str_make(s.ptr + s.len, 0);
        return false;
    }

    if (left) *left = vitte_str_make(s.ptr, idx);
    if (right) *right = vitte_str_make(s.ptr + idx + 1, s.len - idx - 1);
    return true;
}

static inline const char* vitte_str_dup(vitte_arena* arena, vitte_str s, vitte_diag* d)
{
    if (!arena) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_str_dup: arena required");
        return NULL;
    }

    char* p = (char*)vitte_arena_alloc(arena, s.len + 1, 1);
    if (!p) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_str_dup: oom");
        return NULL;
    }

    for (size_t i = 0; i < s.len; ++i) p[i] = s.ptr[i];
    p[s.len] = '\0';
    return p;
}

/* printf into arena using vitte_buf */
static inline const char* vitte_str_printf(vitte_arena* arena, vitte_diag* d, const char* fmt, ...)
{
    if (!arena || !fmt) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_str_printf: invalid args");
        return NULL;
    }

    vitte_buf b;
    vitte_buf_init_arena(&b, arena);

    va_list ap;
    va_start(ap, fmt);
    bool ok = vitte_buf_vappendf(&b, fmt, ap);
    va_end(ap);

    if (!ok) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_str_printf: oom");
        return NULL;
    }

    /* ensure NUL */
    if (!vitte_buf_append(&b, "\0", 1)) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_str_printf: oom(nul)");
        return NULL;
    }

    return b.data;
}

static inline const char* vitte_str_join2(vitte_arena* arena, vitte_str a, vitte_str sep, vitte_str b, vitte_diag* d)
{
    if (!arena) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_str_join2: arena required");
        return NULL;
    }

    const size_t n = a.len + sep.len + b.len;
    char* p = (char*)vitte_arena_alloc(arena, n + 1, 1);
    if (!p) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_str_join2: oom");
        return NULL;
    }

    size_t w = 0;
    for (size_t i = 0; i < a.len; ++i) p[w++] = a.ptr[i];
    for (size_t i = 0; i < sep.len; ++i) p[w++] = sep.ptr[i];
    for (size_t i = 0; i < b.len; ++i) p[w++] = b.ptr[i];
    p[w] = '\0';
    return p;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_STR_H */