

/*
  bench/path.h

  Path utilities for the Vitte benchmark harness.

  Rationale
  - Bench needs portable path manipulation for fixtures, generated outputs,
    baselines, and reports.
  - We keep rules deterministic and avoid OS-specific surprising behaviors.

  Scope
  - Normalize / join / split helpers (UTF-8 strings).
  - No filesystem calls (those are in bench/os.h and bench/detail/fs.h).
  - No symlink resolution.

  Conventions
  - Internally we normalize to forward slashes '/'.
  - Windows drive prefixes and UNC paths are preserved best-effort.

  Memory
  - Functions returning strings allocate in vitte_arena.
*/

#pragma once
#ifndef VITTE_BENCH_PATH_H
#define VITTE_BENCH_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/arena.h"
#include "bench/diag.h"

/* -------------------------------------------------------------------------- */
/* Separators                                                                   */
/* -------------------------------------------------------------------------- */

static inline bool vitte_path_is_sep(char c) { return c == '/' || c == '\\'; }

/* Normalize separator for internal use (always '/'). */
static inline char vitte_path_norm_sep(char c) { return (c == '\\') ? '/' : c; }

/* -------------------------------------------------------------------------- */
/* Small string helpers                                                         */
/* -------------------------------------------------------------------------- */

static inline size_t vitte_path__strlen(const char* s)
{
    size_t n = 0;
    if (!s) return 0;
    while (s[n]) n++;
    return n;
}

static inline void vitte_path__cpy(char* dst, size_t cap, const char* src)
{
    if (!dst || cap == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t i = 0;
    for (; i + 1 < cap && src[i]; ++i) dst[i] = src[i];
    dst[i] = '\0';
}

/* -------------------------------------------------------------------------- */
/* Parse prefixes (drive/UNC)                                                   */
/* -------------------------------------------------------------------------- */

typedef struct vitte_path_prefix {
    /* number of bytes of prefix in normalized path string */
    size_t bytes;

    /* prefix kind */
    enum {
        VITTE_PATH_PREFIX_NONE = 0,
        VITTE_PATH_PREFIX_UNC,
        VITTE_PATH_PREFIX_DRIVE
    } kind;
} vitte_path_prefix;

static inline vitte_path_prefix vitte_path_prefix_parse(const char* path)
{
    vitte_path_prefix p;
    p.bytes = 0;
    p.kind = VITTE_PATH_PREFIX_NONE;

    if (!path) {
        return p;
    }

    /* UNC: starts with // or \\ */
    if ((path[0] == '/' || path[0] == '\\') && (path[1] == '/' || path[1] == '\\')) {
        p.kind = VITTE_PATH_PREFIX_UNC;
        p.bytes = 2;
        return p;
    }

    /* Drive: "C:" or "c:" */
    const unsigned char c0 = (unsigned char)path[0];
    if (((c0 >= 'A' && c0 <= 'Z') || (c0 >= 'a' && c0 <= 'z')) && path[1] == ':') {
        p.kind = VITTE_PATH_PREFIX_DRIVE;
        p.bytes = 2;
        return p;
    }

    return p;
}

/* -------------------------------------------------------------------------- */
/* Basename / dirname                                                           */
/* -------------------------------------------------------------------------- */

/* Pointer into input. */
static inline const char* vitte_path_basename(const char* path)
{
    if (!path) return "";
    const char* last = path;
    for (const char* p = path; *p; ++p) {
        if (vitte_path_is_sep(*p)) {
            last = p + 1;
        }
    }
    return last;
}

/* Allocate dirname into arena.
   - Returns "" if no separator.
   - Preserves drive/UNC prefix.
*/
static inline const char* vitte_path_dirname(vitte_arena* arena, const char* path, vitte_diag* d)
{
    if (!arena || !path) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_path_dirname: invalid args");
        return NULL;
    }

    size_t n = vitte_path__strlen(path);
    if (n == 0) {
        return "";
    }

    const char* last = NULL;
    for (const char* p = path; *p; ++p) {
        if (vitte_path_is_sep(*p)) last = p;
    }

    if (!last) {
        return "";
    }

    size_t dn = (size_t)(last - path);
    if (dn == 0) {
        /* root */
        char* s = (char*)vitte_arena_alloc(arena, 2, 1);
        if (!s) {
            VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_path_dirname: oom");
            return NULL;
        }
        s[0] = '/';
        s[1] = '\0';
        return s;
    }

    char* out = (char*)vitte_arena_alloc(arena, dn + 1, 1);
    if (!out) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_path_dirname: oom");
        return NULL;
    }

    for (size_t i = 0; i < dn; ++i) {
        out[i] = vitte_path_norm_sep(path[i]);
    }
    out[dn] = '\0';
    return out;
}

/* -------------------------------------------------------------------------- */
/* Normalization                                                                */
/* -------------------------------------------------------------------------- */

/*
  Normalize path:
  - converts backslashes to '/'
  - collapses repeated '/'
  - resolves '.' segments
  - preserves '..' segments (does not walk above root)
  - trims trailing '/' except root or UNC prefix

  Returns arena-backed string.
*/
const char* vitte_path_normalize(vitte_arena* arena, const char* path, vitte_diag* d);

/* Join two paths with a single '/' and normalize. */
const char* vitte_path_join(vitte_arena* arena, const char* a, const char* b, vitte_diag* d);

/*
  Relativize `path` against `base`.
  - Both inputs are normalized internally.
  - If path is not under base, returns normalized path.

  Examples:
    base=/a/b, path=/a/b/c.txt -> c.txt
    base=/a/b, path=/a/x.txt   -> /a/x.txt

  Returns arena-backed string.
*/
const char* vitte_path_relativize(vitte_arena* arena, const char* base, const char* path, vitte_diag* d);

/* -------------------------------------------------------------------------- */
/* Implementation (header-only)                                                */
/* -------------------------------------------------------------------------- */

static inline bool vitte_path__is_unc(const char* p)
{
    return p && vitte_path_is_sep(p[0]) && vitte_path_is_sep(p[1]);
}

static inline bool vitte_path__is_drive(const char* p)
{
    if (!p) return false;
    const unsigned char c0 = (unsigned char)p[0];
    return ((c0 >= 'A' && c0 <= 'Z') || (c0 >= 'a' && c0 <= 'z')) && p[1] == ':';
}

const char* vitte_path_normalize(vitte_arena* arena, const char* path, vitte_diag* d)
{
    if (!arena || !path) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_path_normalize: invalid args");
        return NULL;
    }

    size_t n = vitte_path__strlen(path);

    /* allocate temporary buffer in arena (worst case n+1) */
    char* tmp = (char*)vitte_arena_alloc(arena, n + 1, 1);
    if (!tmp) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_path_normalize: oom");
        return NULL;
    }

    /* pass 1: normalize separators, collapse repeats */
    size_t w = 0;
    bool prev_sep = false;
    const bool unc = vitte_path__is_unc(path);

    for (size_t i = 0; i < n; ++i) {
        char c = vitte_path_norm_sep(path[i]);
        if (c == '/') {
            if (prev_sep) continue;
            prev_sep = true;
            tmp[w++] = '/';
        } else {
            prev_sep = false;
            tmp[w++] = c;
        }
    }

    /* preserve UNC double-slash */
    if (unc && w > 0 && tmp[0] == '/' && (w == 1 || tmp[1] != '/')) {
        if (w + 1 <= n) {
            for (size_t i = w; i > 0; --i) tmp[i] = tmp[i - 1];
            tmp[0] = '/';
            tmp[1] = '/';
            w++;
        }
    }

    /* pass 2: resolve '.' segments, keep '..' */
    char* out = tmp;
    size_t out_w = 0;

    size_t i = 0;

    /* preserve drive prefix like C: */
    if (vitte_path__is_drive(out)) {
        out[out_w++] = (char)out[0];
        out[out_w++] = ':';
        i = 2;
        if (out[i] == '/') {
            out[out_w++] = '/';
            i++;
        }
    }

    while (i < w) {
        /* skip separators */
        while (i < w && out[i] == '/') {
            if (out_w == 0 || out[out_w - 1] != '/') {
                out[out_w++] = '/';
            }
            i++;
        }
        if (i >= w) break;

        /* segment */
        size_t seg_start = i;
        while (i < w && out[i] != '/') i++;
        size_t seg_len = i - seg_start;

        const bool is_dot = (seg_len == 1 && out[seg_start] == '.');

        if (!is_dot) {
            for (size_t k = 0; k < seg_len; ++k) {
                out[out_w++] = out[seg_start + k];
            }
        } else {
            /* skip '.' */
            if (out_w == 0) {
                /* nothing */
            }
        }
    }

    /* trim trailing '/' except root or UNC prefix */
    while (out_w > 1 && out[out_w - 1] == '/') {
        if (out_w >= 2 && out[0] == '/' && out[1] == '/') {
            break;
        }
        /* keep "C:/" */
        if (out_w == 3 && out[1] == ':' && out[2] == '/') {
            break;
        }
        out_w--;
    }

    out[out_w] = '\0';
    return out;
}

const char* vitte_path_join(vitte_arena* arena, const char* a, const char* b, vitte_diag* d)
{
    if (!arena || !a || !b) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_path_join: invalid args");
        return NULL;
    }

    size_t na = vitte_path__strlen(a);
    size_t nb = vitte_path__strlen(b);

    char* tmp = (char*)vitte_arena_alloc(arena, na + 1 + nb + 1, 1);
    if (!tmp) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_path_join: oom");
        return NULL;
    }

    size_t w = 0;
    for (size_t i = 0; i < na; ++i) tmp[w++] = a[i];
    if (w && !vitte_path_is_sep(tmp[w - 1])) tmp[w++] = '/';
    for (size_t i = 0; i < nb; ++i) tmp[w++] = b[i];
    tmp[w] = '\0';

    return vitte_path_normalize(arena, tmp, d);
}

static inline bool vitte_path__starts_with(const char* s, const char* pfx)
{
    if (!s || !pfx) return false;
    while (*pfx) {
        if (*s != *pfx) return false;
        ++s; ++pfx;
    }
    return true;
}

const char* vitte_path_relativize(vitte_arena* arena, const char* base, const char* path, vitte_diag* d)
{
    if (!arena || !base || !path) {
        VITTE_DIAG_SET(d, VITTE_DIAG_ERR_INVALID_ARG, VITTE_DIAG_F_PERM, "vitte_path_relativize: invalid args");
        return NULL;
    }

    const char* nb = vitte_path_normalize(arena, base, d);
    const char* np = vitte_path_normalize(arena, path, d);
    if (!nb || !np) {
        return NULL;
    }

    size_t nbl = vitte_path__strlen(nb);

    /* ensure base ends with '/' for prefix match, except root */
    bool need_slash = (nbl > 1 && nb[nbl - 1] != '/');

    if (need_slash) {
        const char* base2 = vitte_path_join(arena, nb, "", d);
        if (!base2) return NULL;
        nb = base2;
        nbl = vitte_path__strlen(nb);
    }

    if (vitte_path__starts_with(np, nb)) {
        const char* rel = np + nbl;
        if (!rel[0]) return "";
        /* skip leading '/' */
        if (rel[0] == '/') rel++;
        /* duplicate into arena */
        size_t n = vitte_path__strlen(rel);
        char* out = (char*)vitte_arena_alloc(arena, n + 1, 1);
        if (!out) {
            VITTE_DIAG_SET(d, VITTE_DIAG_ERR_OOM, VITTE_DIAG_F_PERM, "vitte_path_relativize: oom");
            return NULL;
        }
        for (size_t i = 0; i < n; ++i) out[i] = rel[i];
        out[n] = '\0';
        return out;
    }

    return np;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_PATH_H */