// SPDX-License-Identifier: MIT
// span.c
//
// Span utilities (max).
//
// Provides:
//  - Constructors and normalization for `steel_span`
//  - Helpers to merge spans
//  - Safe clamping and validity checks
//  - Formatting helpers
//
// Assumes `steel_span` is defined in `lexer.h` (compiler) and contains:
//   uint32_t file_id;
//   uint32_t start;
//   uint32_t end;
//   uint32_t line;
//   uint32_t col;
//
// If your layout differs, adjust the accessors below.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "../compiler/lexer.h" // steel_span

//------------------------------------------------------------------------------
// Accessors (adjust if needed)
//------------------------------------------------------------------------------

static inline uint32_t sp_file_(steel_span s) { return s.file_id; }
static inline uint32_t sp_start_(steel_span s) { return s.start; }
static inline uint32_t sp_end_(steel_span s) { return s.end; }
static inline uint32_t sp_line_(steel_span s) { return s.line; }
static inline uint32_t sp_col_(steel_span s) { return s.col; }

static inline void sp_set_file_(steel_span* s, uint32_t v) { s->file_id = v; }
static inline void sp_set_start_(steel_span* s, uint32_t v) { s->start = v; }
static inline void sp_set_end_(steel_span* s, uint32_t v) { s->end = v; }
static inline void sp_set_line_(steel_span* s, uint32_t v) { s->line = v; }
static inline void sp_set_col_(steel_span* s, uint32_t v) { s->col = v; }

//------------------------------------------------------------------------------
// Core helpers
//------------------------------------------------------------------------------

steel_span steel_span_make(uint32_t file_id, uint32_t start, uint32_t end, uint32_t line, uint32_t col)
{
    steel_span s;
    memset(&s, 0, sizeof(s));
    s.file_id = file_id;
    s.start = start;
    s.end = end;
    s.line = line;
    s.col = col;

    // normalize
    if (s.end < s.start)
    {
        uint32_t t = s.start;
        s.start = s.end;
        s.end = t;
    }

    return s;
}

steel_span steel_span_zero(void)
{
    steel_span s;
    memset(&s, 0, sizeof(s));
    return s;
}

bool steel_span_is_valid(steel_span s)
{
    if (sp_file_(s) == 0) return false;
    if (sp_end_(s) < sp_start_(s)) return false;
    return true;
}

steel_span steel_span_normalize(steel_span s)
{
    if (sp_end_(s) < sp_start_(s))
    {
        uint32_t t = s.start;
        s.start = s.end;
        s.end = t;
    }

    return s;
}

steel_span steel_span_merge(steel_span a, steel_span b)
{
    // If one is invalid, return the other.
    bool va = steel_span_is_valid(a);
    bool vb = steel_span_is_valid(b);

    if (!va && !vb) return steel_span_zero();
    if (!va) return steel_span_normalize(b);
    if (!vb) return steel_span_normalize(a);

    // Different files: prefer `a` but widen offsets if reasonable.
    if (sp_file_(a) != sp_file_(b))
        return steel_span_normalize(a);

    steel_span r = a;

    if (sp_start_(b) < sp_start_(r)) sp_set_start_(&r, sp_start_(b));
    if (sp_end_(b) > sp_end_(r)) sp_set_end_(&r, sp_end_(b));

    // prefer earliest line/col if present
    if (sp_line_(r) == 0 || (sp_line_(b) != 0 && sp_line_(b) < sp_line_(r)))
    {
        sp_set_line_(&r, sp_line_(b));
        sp_set_col_(&r, sp_col_(b));
    }
    else if (sp_line_(r) == sp_line_(b) && sp_line_(r) != 0)
    {
        if (sp_col_(r) == 0 || (sp_col_(b) != 0 && sp_col_(b) < sp_col_(r)))
            sp_set_col_(&r, sp_col_(b));
    }

    return steel_span_normalize(r);
}

steel_span steel_span_clamp(steel_span s, uint32_t max_end)
{
    s = steel_span_normalize(s);

    if (s.start > max_end) s.start = max_end;
    if (s.end > max_end) s.end = max_end;

    return s;
}

bool steel_span_contains(steel_span outer, uint32_t offset)
{
    outer = steel_span_normalize(outer);
    return offset >= outer.start && offset <= outer.end;
}

bool steel_span_overlaps(steel_span a, steel_span b)
{
    a = steel_span_normalize(a);
    b = steel_span_normalize(b);

    if (!steel_span_is_valid(a) || !steel_span_is_valid(b))
        return false;

    if (a.file_id != b.file_id)
        return false;

    // inclusive overlap
    return !(a.end < b.start || b.end < a.start);
}

//------------------------------------------------------------------------------
// Formatting
//------------------------------------------------------------------------------

size_t steel_span_format(steel_span s, char* out, size_t out_cap)
{
    if (!out || out_cap == 0) return 0;

    if (!steel_span_is_valid(s))
    {
        int n = snprintf(out, out_cap, "<invalid-span>");
        if (n < 0) { out[0] = 0; return 0; }
        if ((size_t)n >= out_cap) { out[out_cap - 1] = 0; return out_cap - 1; }
        return (size_t)n;
    }

    // file:start..end line:col (line/col optional)
    if (s.line != 0 && s.col != 0)
    {
        int n = snprintf(out, out_cap, "%u:%u..%u @%u:%u",
                         (unsigned)s.file_id,
                         (unsigned)s.start,
                         (unsigned)s.end,
                         (unsigned)s.line,
                         (unsigned)s.col);
        if (n < 0) { out[0] = 0; return 0; }
        if ((size_t)n >= out_cap) { out[out_cap - 1] = 0; return out_cap - 1; }
        return (size_t)n;
    }
    else
    {
        int n = snprintf(out, out_cap, "%u:%u..%u",
                         (unsigned)s.file_id,
                         (unsigned)s.start,
                         (unsigned)s.end);
        if (n < 0) { out[0] = 0; return 0; }
        if ((size_t)n >= out_cap) { out[out_cap - 1] = 0; return out_cap - 1; }
        return (size_t)n;
    }
}
