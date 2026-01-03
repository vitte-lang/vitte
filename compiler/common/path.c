// SPDX-License-Identifier: MIT
// path.c
//
// Cross-platform path helpers.
//
// Scope:
//  - Join, normalize separators, basename/dirname
//  - Extension helpers
//  - Relative path computation (best-effort)
//  - Simple path classification (abs/rel)
//
// This file pairs with path.h.
// path.h is expected to declare the functions implemented here.
// If your path.h differs, adjust accordingly.

#include "path.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(_WIN32)
  #include <ctype.h>
#endif

#ifndef STEEL_PATH_MAX
  #define STEEL_PATH_MAX 4096
#endif

//------------------------------------------------------------------------------
// Internal helpers
//------------------------------------------------------------------------------

static inline bool steel_is_sep(char c)
{
    return (c == '/') || (c == '\\');
}

static inline char steel_native_sep(void)
{
#if defined(_WIN32)
    return '\\';
#else
    return '/';
#endif
}

static size_t steel_strlcpy(char* dst, const char* src, size_t cap)
{
    if (!dst || cap == 0) return 0;
    if (!src) { dst[0] = 0; return 0; }

    size_t n = strlen(src);
    size_t k = (n < cap - 1) ? n : (cap - 1);
    memcpy(dst, src, k);
    dst[k] = 0;
    return n;
}

static void steel_replace_seps(char* s, char sep)
{
    if (!s) return;
    for (char* p = s; *p; p++)
    {
        if (steel_is_sep(*p)) *p = sep;
    }
}

static void steel_trim_trailing_seps(char* s)
{
    if (!s) return;
    size_t n = strlen(s);
    while (n > 1 && steel_is_sep(s[n - 1]))
    {
        s[n - 1] = 0;
        n--;
    }
}

#if defined(_WIN32)
static bool steel_is_drive_prefix(const char* s)
{
    // "C:" style
    if (!s || !s[0] || !s[1]) return false;
    return ((s[0] >= 'A' && s[0] <= 'Z') || (s[0] >= 'a' && s[0] <= 'z')) && (s[1] == ':');
}
#endif

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

bool steel_path_is_abs(const char* path)
{
    if (!path || !path[0]) return false;

#if defined(_WIN32)
    // UNC: \\server\share
    if (steel_is_sep(path[0]) && steel_is_sep(path[1])) return true;
    // Drive: C:\ or C:/
    if (steel_is_drive_prefix(path) && steel_is_sep(path[2])) return true;
    return false;
#else
    return path[0] == '/';
#endif
}

bool steel_path_is_rel(const char* path)
{
    return !steel_path_is_abs(path);
}

bool steel_path_has_trailing_sep(const char* path)
{
    if (!path || !path[0]) return false;
    size_t n = strlen(path);
    return steel_is_sep(path[n - 1]);
}

size_t steel_path_basename(const char* path, char* out, size_t out_cap)
{
    if (!out || out_cap == 0) return 0;
    out[0] = 0;
    if (!path || !path[0]) return 0;

    const char* end = path + strlen(path);
    while (end > path && steel_is_sep(end[-1])) end--;

    const char* p = end;
    while (p > path && !steel_is_sep(p[-1])) p--;

    size_t n = (size_t)(end - p);
    if (n >= out_cap) n = out_cap - 1;
    memcpy(out, p, n);
    out[n] = 0;
    return n;
}

size_t steel_path_dirname(const char* path, char* out, size_t out_cap)
{
    if (!out || out_cap == 0) return 0;
    out[0] = 0;
    if (!path || !path[0]) return 0;

    char tmp[STEEL_PATH_MAX];
    steel_strlcpy(tmp, path, sizeof(tmp));

    // Normalize separators for processing.
    char sep = steel_native_sep();
    steel_replace_seps(tmp, sep);

    size_t n = strlen(tmp);
    while (n > 1 && tmp[n - 1] == sep) { tmp[n - 1] = 0; n--; }

#if defined(_WIN32)
    // Keep "C:\" as dirname of "C:\foo"
    if (steel_is_drive_prefix(tmp) && tmp[2] == 0)
    {
        steel_strlcpy(out, tmp, out_cap);
        return strlen(out);
    }
#endif

    char* last = strrchr(tmp, sep);
    if (!last)
    {
        // No separator => "."
        steel_strlcpy(out, ".", out_cap);
        return strlen(out);
    }

    // If root, keep one sep.
#if defined(_WIN32)
    if (last == tmp)
    {
        // "\foo" => "\"
        tmp[1] = 0;
        steel_strlcpy(out, tmp, out_cap);
        return strlen(out);
    }
    if (steel_is_drive_prefix(tmp) && last == tmp + 2)
    {
        // "C:\foo" => "C:\"
        tmp[3] = 0;
        steel_strlcpy(out, tmp, out_cap);
        return strlen(out);
    }
#else
    if (last == tmp)
    {
        tmp[1] = 0;
        steel_strlcpy(out, tmp, out_cap);
        return strlen(out);
    }
#endif

    *last = 0;
    steel_trim_trailing_seps(tmp);
    steel_strlcpy(out, tmp, out_cap);
    return strlen(out);
}

size_t steel_path_join2(const char* a, const char* b, char* out, size_t out_cap)
{
    if (!out || out_cap == 0) return 0;
    out[0] = 0;

    if (!a || !a[0])
    {
        return (size_t)steel_strlcpy(out, b ? b : "", out_cap);
    }

    if (!b || !b[0])
    {
        return (size_t)steel_strlcpy(out, a, out_cap);
    }

    // If b is absolute, return b.
    if (steel_path_is_abs(b))
    {
        return (size_t)steel_strlcpy(out, b, out_cap);
    }

    char sep = steel_native_sep();

    // Copy a
    size_t al = strlen(a);
    size_t pos = 0;
    if (al >= out_cap) al = out_cap - 1;
    memcpy(out, a, al);
    pos = al;
    out[pos] = 0;

    // Ensure single separator
    if (pos > 0 && !steel_is_sep(out[pos - 1]))
    {
        if (pos + 1 < out_cap)
        {
            out[pos++] = sep;
            out[pos] = 0;
        }
        else
        {
            return strlen(out);
        }
    }

    // Skip leading seps in b
    while (*b && steel_is_sep(*b)) b++;

    // Append b
    size_t bl = strlen(b);
    size_t room = (pos < out_cap) ? (out_cap - 1 - pos) : 0;
    if (bl > room) bl = room;
    memcpy(out + pos, b, bl);
    pos += bl;
    out[pos] = 0;

    // Normalize separators
    steel_replace_seps(out, sep);
    return pos;
}

size_t steel_path_join3(const char* a, const char* b, const char* c, char* out, size_t out_cap)
{
    char tmp[STEEL_PATH_MAX];
    steel_path_join2(a, b, tmp, sizeof(tmp));
    return steel_path_join2(tmp, c, out, out_cap);
}

size_t steel_path_normalize(char* path)
{
    if (!path) return 0;

    // Normalize to native sep and collapse multiple separators.
    char sep = steel_native_sep();

    // Convert all seps.
    steel_replace_seps(path, sep);

    // Collapse duplicates (but preserve UNC prefix on Windows).
    char* w = path;
    const char* r = path;

#if defined(_WIN32)
    // Preserve leading "\\" for UNC.
    if (r[0] == sep && r[1] == sep)
    {
        *w++ = sep;
        *w++ = sep;
        r += 2;
        while (*r == sep) r++; // collapse extra
    }
#endif

    bool prev_sep = false;
    while (*r)
    {
        char c = *r++;
        if (c == sep)
        {
            if (prev_sep) continue;
            prev_sep = true;
            *w++ = sep;
        }
        else
        {
            prev_sep = false;
            *w++ = c;
        }
    }
    *w = 0;

    return (size_t)(w - path);
}

bool steel_path_split_ext(const char* path, char* out_stem, size_t stem_cap, char* out_ext, size_t ext_cap)
{
    if (out_stem && stem_cap) out_stem[0] = 0;
    if (out_ext && ext_cap) out_ext[0] = 0;
    if (!path) return false;

    // Find last path separator.
    const char* base = path;
    const char* a = strrchr(path, '/');
    const char* b = strrchr(path, '\\');
    const char* sep = a;
    if (b && (!sep || b > sep)) sep = b;
    if (sep) base = sep + 1;

    // Find last dot in basename.
    const char* dot = strrchr(base, '.');
    if (!dot || dot == base)
    {
        // No extension
        if (out_stem && stem_cap) steel_strlcpy(out_stem, path, stem_cap);
        return false;
    }

    // Stem is full path up to dot
    size_t stem_len = (size_t)(dot - path);
    if (out_stem && stem_cap)
    {
        size_t n = (stem_len < stem_cap - 1) ? stem_len : (stem_cap - 1);
        memcpy(out_stem, path, n);
        out_stem[n] = 0;
    }

    // Ext is dot+...
    if (out_ext && ext_cap)
        steel_strlcpy(out_ext, dot, ext_cap);

    return true;
}

bool steel_path_change_ext(const char* path, const char* new_ext, char* out, size_t out_cap)
{
    if (!out || out_cap == 0) return false;
    out[0] = 0;
    if (!path) return false;

    char stem[STEEL_PATH_MAX];
    char ext[128];
    bool had = steel_path_split_ext(path, stem, sizeof(stem), ext, sizeof(ext));

    (void)had;

    if (!new_ext) new_ext = "";

    // Ensure new_ext starts with '.' if not empty.
    char ext2[128];
    if (new_ext[0] && new_ext[0] != '.')
        snprintf(ext2, sizeof(ext2), ".%s", new_ext);
    else
        steel_strlcpy(ext2, new_ext, sizeof(ext2));

    // If no extension existed, stem==path. If existed, stem is path without ext.
    snprintf(out, out_cap, "%s%s", stem, ext2);
    return true;
}

bool steel_path_strip_ext(const char* path, char* out, size_t out_cap)
{
    if (!out || out_cap == 0) return false;
    out[0] = 0;
    if (!path) return false;

    char stem[STEEL_PATH_MAX];
    char ext[128];
    if (!steel_path_split_ext(path, stem, sizeof(stem), ext, sizeof(ext)))
    {
        steel_strlcpy(out, path, out_cap);
        return true;
    }

    steel_strlcpy(out, stem, out_cap);
    return true;
}

// Best-effort relative path: if different root/prefix, return false.
bool steel_path_relpath(const char* from_dir, const char* to_path, char* out, size_t out_cap)
{
    if (!out || out_cap == 0) return false;
    out[0] = 0;
    if (!from_dir || !to_path) return false;

    char from[STEEL_PATH_MAX];
    char to[STEEL_PATH_MAX];
    steel_strlcpy(from, from_dir, sizeof(from));
    steel_strlcpy(to, to_path, sizeof(to));

    steel_path_normalize(from);
    steel_path_normalize(to);

    // If one is abs and other isn't, bail.
    if (steel_path_is_abs(from) != steel_path_is_abs(to)) return false;

#if defined(_WIN32)
    // Compare drive letters for absolute paths.
    if (steel_path_is_abs(from) && steel_is_drive_prefix(from) && steel_is_drive_prefix(to))
    {
        char f0 = (char)tolower((unsigned char)from[0]);
        char t0 = (char)tolower((unsigned char)to[0]);
        if (f0 != t0) return false;
    }
#endif

    char sep = steel_native_sep();

    // Split into segments.
    const char* f = from;
    const char* t = to;

    // Skip common prefix segments.
    const char* f_it = f;
    const char* t_it = t;
    const char* f_last = f;
    const char* t_last = t;

    while (*f_it && *t_it)
    {
        // Find next segment end
        const char* f_end = f_it;
        const char* t_end = t_it;
        while (*f_end && *f_end != sep) f_end++;
        while (*t_end && *t_end != sep) t_end++;

        size_t fl = (size_t)(f_end - f_it);
        size_t tl = (size_t)(t_end - t_it);

#if defined(_WIN32)
        // Case-insensitive compare for windows.
        bool eq = (fl == tl);
        if (eq)
        {
            for (size_t i = 0; i < fl; i++)
            {
                char cf = (char)tolower((unsigned char)f_it[i]);
                char ct = (char)tolower((unsigned char)t_it[i]);
                if (cf != ct) { eq = false; break; }
            }
        }
#else
        bool eq = (fl == tl) && (memcmp(f_it, t_it, fl) == 0);
#endif

        if (!eq) break;

        f_last = f_end;
        t_last = t_end;

        f_it = (*f_end == sep) ? f_end + 1 : f_end;
        t_it = (*t_end == sep) ? t_end + 1 : t_end;
    }

    // Count remaining segments in from (after common part)
    size_t ups = 0;
    const char* p = f_last;
    if (*p == sep) p++;
    while (*p)
    {
        const char* e = p;
        while (*e && *e != sep) e++;
        if (e != p) ups++;
        p = (*e == sep) ? e + 1 : e;
    }

    // Build output
    size_t pos = 0;
    for (size_t i = 0; i < ups; i++)
    {
        const char* frag = "..";
        size_t fl = 2;
        if (pos && pos + 1 < out_cap) out[pos++] = sep;
        if (pos + fl >= out_cap) return false;
        memcpy(out + pos, frag, fl);
        pos += fl;
        out[pos] = 0;
    }

    // Append remainder of to
    const char* rem = t_last;
    if (*rem == sep) rem++;
    if (!*rem)
    {
        if (pos == 0)
        {
            if (out_cap < 2) return false;
            out[0] = '.';
            out[1] = 0;
        }
        return true;
    }

    if (pos && pos + 1 < out_cap) out[pos++] = sep;

    size_t rl = strlen(rem);
    if (pos + rl >= out_cap) return false;
    memcpy(out + pos, rem, rl);
    pos += rl;
    out[pos] = 0;

    return true;
}
