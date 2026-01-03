// SPDX-License-Identifier: MIT
// source_map.c
//
// Source map (max): tracks loaded source files and provides span -> (path, line, col)
// resolution utilities.
//
// Responsibilities:
//  - Store source buffers (optionally owned) and their file paths.
//  - Precompute line start offsets for O(log n) position mapping.
//  - Provide helpers to format spans for diagnostics.
//
// Assumptions:
//  - `steel_span` contains { file_id, start, end, line, col }.
//    If your span differs, adjust the accessors below.
//

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../common/vec.h"      // steel_vec
#include "../compiler/lexer.h"  // steel_span

//------------------------------------------------------------------------------
// Optional header integration
//------------------------------------------------------------------------------

#if defined(__has_include)
  #if __has_include("source_map.h")
    #include "source_map.h"
    #define STEEL_HAS_SOURCE_MAP_H 1
  #endif
#endif

#ifndef STEEL_HAS_SOURCE_MAP_H

typedef struct steel_source_file
{
    uint32_t id;
    const char* path;      // owned copy
    const char* text;      // owned or borrowed
    size_t text_len;

    // line starts (uint32_t offsets)
    steel_vec line_starts;

    bool owns_text;

} steel_source_file;

typedef struct steel_source_map
{
    steel_vec files; // steel_source_file

} steel_source_map;

void steel_source_map_init(steel_source_map* sm);
void steel_source_map_dispose(steel_source_map* sm);

uint32_t steel_source_map_add_owned(steel_source_map* sm, const char* path, const char* text, size_t text_len);
uint32_t steel_source_map_add_borrowed(steel_source_map* sm, const char* path, const char* text, size_t text_len);

const steel_source_file* steel_source_map_get(const steel_source_map* sm, uint32_t file_id);

// Resolve absolute offset -> (1-based line, 1-based col)
bool steel_source_map_linecol(const steel_source_map* sm, uint32_t file_id, uint32_t offset, uint32_t* out_line, uint32_t* out_col);

// Format span as "path:line:col" (or "<unknown>" if missing)
size_t steel_source_map_format_loc(const steel_source_map* sm, steel_span sp, char* out, size_t out_cap);

#endif // !STEEL_HAS_SOURCE_MAP_H

//------------------------------------------------------------------------------
// Utilities
//------------------------------------------------------------------------------

static char* steel_strdup_n_(const char* s, size_t n)
{
    if (!s) { s = ""; n = 0; }
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}

static char* steel_strdup_(const char* s)
{
    if (!s) s = "";
    return steel_strdup_n_(s, strlen(s));
}

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

static size_t snprintf_(char* out, size_t cap, const char* fmt, ...)
{
    if (!out || cap == 0) return 0;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(out, cap, fmt, ap);
    va_end(ap);
    if (n < 0) { out[0] = 0; return 0; }
    if ((size_t)n >= cap) { out[cap - 1] = 0; return cap - 1; }
    return (size_t)n;
}

static void build_line_starts_(steel_source_file* f)
{
    steel_vec_init(&f->line_starts, sizeof(uint32_t));

    // Line 1 always starts at offset 0.
    uint32_t z = 0;
    steel_vec_push(&f->line_starts, &z);

    const char* s = f->text;
    size_t n = f->text_len;

    for (size_t i = 0; i < n; i++)
    {
        char c = s[i];
        if (c == '\n')
        {
            uint32_t off = (uint32_t)(i + 1);
            steel_vec_push(&f->line_starts, &off);
        }
        else if (c == '\r')
        {
            // Handle CRLF: treat as a single newline.
            if (i + 1 < n && s[i + 1] == '\n')
            {
                uint32_t off = (uint32_t)(i + 2);
                steel_vec_push(&f->line_starts, &off);
                i++;
            }
            else
            {
                uint32_t off = (uint32_t)(i + 1);
                steel_vec_push(&f->line_starts, &off);
            }
        }
    }
}

static uint32_t upper_bound_u32_(const uint32_t* xs, uint32_t n, uint32_t key)
{
    // returns first index i where xs[i] > key
    uint32_t lo = 0, hi = n;
    while (lo < hi)
    {
        uint32_t mid = lo + ((hi - lo) >> 1);
        if (xs[mid] <= key) lo = mid + 1;
        else hi = mid;
    }
    return lo;
}

static const steel_source_file* file_by_id_(const steel_source_map* sm, uint32_t file_id)
{
    if (!sm || file_id == 0) return NULL;
    size_t idx = (size_t)(file_id - 1);
    if (idx >= steel_vec_len(&sm->files)) return NULL;
    return (const steel_source_file*)steel_vec_cat(&sm->files, idx);
}

static steel_source_file* file_by_id_mut_(steel_source_map* sm, uint32_t file_id)
{
    if (!sm || file_id == 0) return NULL;
    size_t idx = (size_t)(file_id - 1);
    if (idx >= steel_vec_len(&sm->files)) return NULL;
    return (steel_source_file*)steel_vec_at(&sm->files, idx);
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void steel_source_map_init(steel_source_map* sm)
{
    if (!sm) return;
    memset(sm, 0, sizeof(*sm));
    steel_vec_init(&sm->files, sizeof(steel_source_file));
}

void steel_source_map_dispose(steel_source_map* sm)
{
    if (!sm) return;

    size_t n = steel_vec_len(&sm->files);
    for (size_t i = 0; i < n; i++)
    {
        steel_source_file* f = (steel_source_file*)steel_vec_at(&sm->files, i);
        if (!f) continue;

        if (f->path) free((void*)f->path);
        if (f->owns_text && f->text) free((void*)f->text);

        steel_vec_free(&f->line_starts);

        memset(f, 0, sizeof(*f));
    }

    steel_vec_free(&sm->files);
    memset(sm, 0, sizeof(*sm));
}

static uint32_t add_file_(steel_source_map* sm, const char* path, const char* text, size_t text_len, bool owns_text)
{
    if (!sm) return 0;

    steel_source_file f;
    memset(&f, 0, sizeof(f));
    f.id = (uint32_t)steel_vec_len(&sm->files) + 1;

    f.path = steel_strdup_(path);
    if (!f.path) return 0;

    f.text = text;
    f.text_len = text_len;
    f.owns_text = owns_text;

    build_line_starts_(&f);

    steel_vec_push(&sm->files, &f);

    return f.id;
}

uint32_t steel_source_map_add_owned(steel_source_map* sm, const char* path, const char* text, size_t text_len)
{
    // If caller passes non-owned buffer, copy it.
    if (!text) text = "";
    if (text_len == 0) text_len = strlen(text);

    char* copy = steel_strdup_n_(text, text_len);
    if (!copy) return 0;

    uint32_t id = add_file_(sm, path, copy, text_len, true);
    if (id == 0) free(copy);
    return id;
}

uint32_t steel_source_map_add_borrowed(steel_source_map* sm, const char* path, const char* text, size_t text_len)
{
    if (!text) text = "";
    if (text_len == 0) text_len = strlen(text);
    return add_file_(sm, path, text, text_len, false);
}

const steel_source_file* steel_source_map_get(const steel_source_map* sm, uint32_t file_id)
{
    return file_by_id_(sm, file_id);
}

bool steel_source_map_linecol(const steel_source_map* sm, uint32_t file_id, uint32_t offset, uint32_t* out_line, uint32_t* out_col)
{
    const steel_source_file* f = file_by_id_(sm, file_id);
    if (!f) return false;

    if (offset > (uint32_t)f->text_len)
        offset = (uint32_t)f->text_len;

    const uint32_t* ls = (const uint32_t*)steel_vec_data(&f->line_starts);
    uint32_t ln = (uint32_t)steel_vec_len(&f->line_starts);
    if (!ls || ln == 0)
        return false;

    // Find first line start > offset; line index is ub-1.
    uint32_t ub = upper_bound_u32_(ls, ln, offset);
    uint32_t line_idx = (ub == 0) ? 0 : (ub - 1);

    uint32_t line_start = ls[line_idx];
    uint32_t col0 = offset - line_start;

    if (out_line) *out_line = line_idx + 1; // 1-based
    if (out_col)  *out_col  = col0 + 1;     // 1-based

    return true;
}

size_t steel_source_map_format_loc(const steel_source_map* sm, steel_span sp, char* out, size_t out_cap)
{
    if (!out || out_cap == 0)
        return 0;

    const steel_source_file* f = file_by_id_(sm, sp.file_id);
    const char* path = f ? f->path : "<unknown>";

    // Prefer span's line/col if already filled; otherwise compute from start.
    uint32_t line = sp.line;
    uint32_t col  = sp.col;

    if ((line == 0 || col == 0) && f)
        (void)steel_source_map_linecol(sm, sp.file_id, sp.start, &line, &col);

    return snprintf_(out, out_cap, "%s:%u:%u", path, (unsigned)line, (unsigned)col);
}
