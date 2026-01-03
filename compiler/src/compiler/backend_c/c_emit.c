// SPDX-License-Identifier: MIT
// c_emit.c
//
// C code emission helpers for the C backend.
//
// This module is intentionally small and dependency-light: it provides a
// deterministic emitter with indentation, token/line management, and safe
// escaping for C string/char literals.
//
// Pairing:
//   - c_emit.h should forward-declare `struct steel_c_emit` (opaque) and declare
//     the public API implemented here.
//   - If your c_emit.h uses different names, either adjust the header or add
//     wrapper functions there.

#include "c_emit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>

#ifndef STEEL_C_EMIT_INDENT_SPACES
  #define STEEL_C_EMIT_INDENT_SPACES 4
#endif

#ifndef STEEL_C_EMIT_LINEBUF_MAX
  #define STEEL_C_EMIT_LINEBUF_MAX 4096
#endif

#ifndef STEEL_C_EMIT_TMP_MAX
  #define STEEL_C_EMIT_TMP_MAX 8192
#endif

//------------------------------------------------------------------------------
// Opaque type definition (header should forward-declare this struct).
//------------------------------------------------------------------------------

struct steel_c_emit
{
    FILE* out;
    bool owns_file;

    // Formatting
    uint32_t indent;
    bool at_bol;             // beginning-of-line
    bool emit_line_markers;  // emit #line directives

    // Diagnostics
    int last_err;            // errno-style code (0=ok)

    // Optional source tracking for #line.
    const char* cur_file;
    uint32_t cur_line;
};

//------------------------------------------------------------------------------
// Internal helpers
//------------------------------------------------------------------------------

static void steel_c_emit_set_err(steel_c_emit* e, int err)
{
    if (!e) return;
    if (e->last_err == 0) e->last_err = err;
}

static bool steel_c_emit_ok(const steel_c_emit* e)
{
    return e && e->last_err == 0;
}

static bool steel_c_emit_put_raw(steel_c_emit* e, const void* data, size_t n)
{
    if (!e || !e->out) return false;
    if (n == 0) return true;
    if (fwrite(data, 1, n, e->out) != n)
    {
        steel_c_emit_set_err(e, errno ? errno : EIO);
        return false;
    }
    return true;
}

static bool steel_c_emit_put_ch(steel_c_emit* e, char c)
{
    return steel_c_emit_put_raw(e, &c, 1);
}

static bool steel_c_emit_write_indent_if_needed(steel_c_emit* e)
{
    if (!e || !e->out) return false;
    if (!e->at_bol) return true;

    e->at_bol = false;

    if (e->indent == 0) return true;

    // Write indent spaces
    char buf[STEEL_C_EMIT_INDENT_SPACES];
    memset(buf, ' ', sizeof(buf));

    for (uint32_t i = 0; i < e->indent; i++)
    {
        if (!steel_c_emit_put_raw(e, buf, sizeof(buf))) return false;
    }

    return true;
}

static bool steel_c_emit_vprintf(steel_c_emit* e, const char* fmt, va_list ap)
{
    if (!e || !e->out) return false;
    if (!fmt) fmt = "";

    if (!steel_c_emit_write_indent_if_needed(e)) return false;

    char tmp[STEEL_C_EMIT_TMP_MAX];
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    if (n < 0)
    {
        steel_c_emit_set_err(e, EINVAL);
        return false;
    }

    // If truncated, allocate dynamically for full output.
    if ((size_t)n >= sizeof(tmp))
    {
        size_t need = (size_t)n + 1;
        char* big = (char*)malloc(need);
        if (!big)
        {
            steel_c_emit_set_err(e, ENOMEM);
            return false;
        }

        va_list ap2;
        va_copy(ap2, ap);
        vsnprintf(big, need, fmt, ap2);
        va_end(ap2);

        bool ok = steel_c_emit_put_raw(e, big, (size_t)n);
        free(big);
        return ok;
    }

    return steel_c_emit_put_raw(e, tmp, (size_t)n);
}

static inline bool steel_c_is_ident_start(unsigned char c)
{
    return (c == '_') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static inline bool steel_c_is_ident_cont(unsigned char c)
{
    return steel_c_is_ident_start(c) || (c >= '0' && c <= '9');
}

static void steel_c_emit_escape_byte(char* out, size_t out_cap, uint8_t b)
{
    // out must have >= 5 bytes: "\\xHH" + NUL
    static const char* hex = "0123456789ABCDEF";
    if (!out || out_cap < 5) return;
    out[0] = '\\';
    out[1] = 'x';
    out[2] = hex[(b >> 4) & 0xF];
    out[3] = hex[b & 0xF];
    out[4] = 0;
}

static bool steel_c_emit_write_c_escaped(steel_c_emit* e, const uint8_t* bytes, size_t len, bool is_char)
{
    if (!e || !e->out) return false;

    if (!steel_c_emit_write_indent_if_needed(e)) return false;

    if (!steel_c_emit_put_ch(e, is_char ? '\'' : '"')) return false;

    for (size_t i = 0; i < len; i++)
    {
        uint8_t b = bytes[i];

        // Common escapes
        switch (b)
        {
            case '\\': if (!steel_c_emit_put_raw(e, "\\\\", 2)) return false; continue;
            case '"':  if (!is_char) { if (!steel_c_emit_put_raw(e, "\\\"", 2)) return false; continue; } break;
            case '\'': if (is_char)  { if (!steel_c_emit_put_raw(e, "\\\'", 2)) return false; continue; } break;
            case '\n': if (!steel_c_emit_put_raw(e, "\\n", 2)) return false; continue;
            case '\r': if (!steel_c_emit_put_raw(e, "\\r", 2)) return false; continue;
            case '\t': if (!steel_c_emit_put_raw(e, "\\t", 2)) return false; continue;
            case '\v': if (!steel_c_emit_put_raw(e, "\\v", 2)) return false; continue;
            case '\b': if (!steel_c_emit_put_raw(e, "\\b", 2)) return false; continue;
            case '\f': if (!steel_c_emit_put_raw(e, "\\f", 2)) return false; continue;
            case 0:     if (!steel_c_emit_put_raw(e, "\\0", 2)) return false; continue;
            default: break;
        }

        // Printable ASCII
        if (b >= 0x20 && b <= 0x7E)
        {
            char c = (char)b;
            if (!steel_c_emit_put_ch(e, c)) return false;
            continue;
        }

        // Non-printable => \xHH
        char esc[8];
        steel_c_emit_escape_byte(esc, sizeof(esc), b);
        if (!steel_c_emit_put_raw(e, esc, 4)) return false;

        // If next byte is a hex digit, add string break to avoid \xHH... consuming
        // further hex digits.
        if (!is_char && i + 1 < len)
        {
            uint8_t n = bytes[i + 1];
            bool is_hex = (n >= '0' && n <= '9') || (n >= 'A' && n <= 'F') || (n >= 'a' && n <= 'f');
            if (is_hex)
            {
                if (!steel_c_emit_put_raw(e, "\"\"", 2)) return false;
            }
        }
    }

    if (!steel_c_emit_put_ch(e, is_char ? '\'' : '"')) return false;
    return true;
}

static bool steel_c_emit_emit_line_marker(steel_c_emit* e, const char* file, uint32_t line)
{
    if (!e || !e->out) return false;
    if (!e->emit_line_markers) return true;

    // Emit as its own line at BOL.
    if (!steel_c_emit_nl(e)) return false;

    // Force BOL indentation off for preprocessor directives.
    bool saved_bol = e->at_bol;
    uint32_t saved_indent = e->indent;
    e->at_bol = true;
    e->indent = 0;

    if (!steel_c_emit_printf(e, "#line %u ", (unsigned)line)) return false;
    if (!steel_c_emit_cstr_lit(e, file ? file : "")) return false;
    if (!steel_c_emit_nl(e)) return false;

    e->indent = saved_indent;
    e->at_bol = saved_bol;

    e->cur_file = file;
    e->cur_line = line;
    return true;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void steel_c_emit_init(steel_c_emit* e, FILE* out)
{
    if (!e) return;
    memset(e, 0, sizeof(*e));
    e->out = out;
    e->owns_file = false;
    e->indent = 0;
    e->at_bol = true;
    e->emit_line_markers = false;
    e->last_err = 0;
    e->cur_file = NULL;
    e->cur_line = 1;
}

bool steel_c_emit_init_path(steel_c_emit* e, const char* path)
{
    if (!e) return false;
    steel_c_emit_init(e, NULL);

    FILE* f = fopen(path, "wb");
    if (!f)
    {
        steel_c_emit_set_err(e, errno ? errno : EIO);
        return false;
    }

    e->out = f;
    e->owns_file = true;
    return true;
}

void steel_c_emit_dispose(steel_c_emit* e)
{
    if (!e) return;
    if (e->owns_file && e->out)
    {
        fclose(e->out);
    }
    e->out = NULL;
    e->owns_file = false;
}

int steel_c_emit_last_error(const steel_c_emit* e)
{
    return e ? e->last_err : EINVAL;
}

bool steel_c_emit_flush(steel_c_emit* e)
{
    if (!e || !e->out) return false;
    if (fflush(e->out) != 0)
    {
        steel_c_emit_set_err(e, errno ? errno : EIO);
        return false;
    }
    return true;
}

void steel_c_emit_set_line_markers(steel_c_emit* e, bool enabled)
{
    if (!e) return;
    e->emit_line_markers = enabled;
}

void steel_c_emit_set_source_pos(steel_c_emit* e, const char* file, uint32_t line)
{
    if (!e) return;
    (void)steel_c_emit_emit_line_marker(e, file, line);
}

void steel_c_emit_indent_push(steel_c_emit* e)
{
    if (!e) return;
    if (e->indent < 0x3FFFFFFFu) e->indent++;
}

void steel_c_emit_indent_pop(steel_c_emit* e)
{
    if (!e) return;
    if (e->indent > 0) e->indent--;
}

bool steel_c_emit_nl(steel_c_emit* e)
{
    if (!e || !e->out) return false;
    if (!steel_c_emit_put_ch(e, '\n')) return false;
    e->at_bol = true;
    return true;
}

bool steel_c_emit_write(steel_c_emit* e, const char* s)
{
    if (!e || !e->out) return false;
    if (!s) return true;

    // Write with indentation on first chunk, and keep BOL tracking.
    const char* p = s;
    while (*p)
    {
        const char* nl = strchr(p, '\n');
        if (!nl)
        {
            if (!steel_c_emit_write_indent_if_needed(e)) return false;
            if (!steel_c_emit_put_raw(e, p, strlen(p))) return false;
            return true;
        }

        // Up to newline
        size_t n = (size_t)(nl - p);
        if (n)
        {
            if (!steel_c_emit_write_indent_if_needed(e)) return false;
            if (!steel_c_emit_put_raw(e, p, n)) return false;
        }
        if (!steel_c_emit_put_ch(e, '\n')) return false;
        e->at_bol = true;
        p = nl + 1;
    }

    return true;
}

bool steel_c_emit_write_n(steel_c_emit* e, const char* s, size_t n)
{
    if (!e || !e->out) return false;
    if (!s || n == 0) return true;

    // No special newline handling here; caller can split if needed.
    if (!steel_c_emit_write_indent_if_needed(e)) return false;
    return steel_c_emit_put_raw(e, s, n);
}

bool steel_c_emit_printf(steel_c_emit* e, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    bool ok = steel_c_emit_vprintf(e, fmt, ap);
    va_end(ap);
    return ok;
}

bool steel_c_emit_ident(steel_c_emit* e, const char* ident)
{
    if (!e || !e->out) return false;
    if (!ident) ident = "";

    if (!steel_c_emit_write_indent_if_needed(e)) return false;

    // Sanitize identifier to a valid C identifier.
    // Strategy:
    //  - If first char invalid, prefix '_'
    //  - Replace invalid chars with '_'
    const unsigned char* p = (const unsigned char*)ident;

    if (!*p)
    {
        return steel_c_emit_put_ch(e, '_');
    }

    if (!steel_c_is_ident_start(*p))
    {
        if (!steel_c_emit_put_ch(e, '_')) return false;
    }

    for (; *p; p++)
    {
        unsigned char c = *p;
        if (steel_c_is_ident_cont(c))
        {
            if (!steel_c_emit_put_ch(e, (char)c)) return false;
        }
        else
        {
            if (!steel_c_emit_put_ch(e, '_')) return false;
        }
    }

    return true;
}

bool steel_c_emit_cstr_lit(steel_c_emit* e, const char* s)
{
    if (!s) s = "";
    return steel_c_emit_write_c_escaped(e, (const uint8_t*)s, strlen(s), false);
}

bool steel_c_emit_bytes_lit(steel_c_emit* e, const uint8_t* bytes, size_t len)
{
    if (!bytes && len != 0) return false;
    return steel_c_emit_write_c_escaped(e, bytes ? bytes : (const uint8_t*)"", len, false);
}

bool steel_c_emit_char_lit(steel_c_emit* e, uint32_t ch)
{
    uint8_t b;
    if (ch > 0xFFu) b = (uint8_t)'?';
    else b = (uint8_t)ch;
    return steel_c_emit_write_c_escaped(e, &b, 1, true);
}

bool steel_c_emit_comment_line(steel_c_emit* e, const char* text)
{
    if (!e || !e->out) return false;
    if (!text) text = "";

    if (!steel_c_emit_write_indent_if_needed(e)) return false;
    if (!steel_c_emit_put_raw(e, "// ", 3)) return false;

    // Avoid embedding newlines in line comments; replace with spaces.
    for (const char* p = text; *p; p++)
    {
        char c = *p;
        if (c == '\n' || c == '\r') c = ' ';
        if (!steel_c_emit_put_ch(e, c)) return false;
    }

    return steel_c_emit_nl(e);
}

bool steel_c_emit_comment_block(steel_c_emit* e, const char* text)
{
    if (!e || !e->out) return false;
    if (!text) text = "";

    if (!steel_c_emit_write_indent_if_needed(e)) return false;
    if (!steel_c_emit_put_raw(e, "/* ", 3)) return false;

    // Prevent closing the comment early: turn "*/" into "* /".
    const char* p = text;
    while (*p)
    {
        if (p[0] == '*' && p[1] == '/')
        {
            if (!steel_c_emit_put_raw(e, "* /", 3)) return false;
            p += 2;
            continue;
        }
        if (!steel_c_emit_put_ch(e, *p++)) return false;
    }

    if (!steel_c_emit_put_raw(e, " */", 3)) return false;
    return steel_c_emit_nl(e);
}

bool steel_c_emit_pp_include(steel_c_emit* e, const char* path, bool system_header)
{
    if (!e || !e->out) return false;
    if (!path) path = "";

    // Preprocessor directives should not be indented.
    bool saved_bol = e->at_bol;
    uint32_t saved_indent = e->indent;
    e->at_bol = true;
    e->indent = 0;

    if (!steel_c_emit_write(e, "#include ")) return false;
    if (system_header)
    {
        if (!steel_c_emit_put_ch(e, '<')) return false;
        if (!steel_c_emit_write(e, path)) return false;
        if (!steel_c_emit_put_ch(e, '>')) return false;
    }
    else
    {
        if (!steel_c_emit_put_ch(e, '"')) return false;
        // Emit as-is; caller should provide a valid include path.
        if (!steel_c_emit_write(e, path)) return false;
        if (!steel_c_emit_put_ch(e, '"')) return false;
    }
    if (!steel_c_emit_nl(e)) return false;

    e->indent = saved_indent;
    e->at_bol = saved_bol;

    return true;
}

bool steel_c_emit_pp_define(steel_c_emit* e, const char* name, const char* value)
{
    if (!e || !e->out) return false;
    if (!name) name = "";
    if (!value) value = "";

    bool saved_bol = e->at_bol;
    uint32_t saved_indent = e->indent;
    e->at_bol = true;
    e->indent = 0;

    if (!steel_c_emit_write(e, "#define ")) return false;
    if (!steel_c_emit_write(e, name)) return false;
    if (value[0])
    {
        if (!steel_c_emit_put_ch(e, ' ')) return false;
        if (!steel_c_emit_write(e, value)) return false;
    }
    if (!steel_c_emit_nl(e)) return false;

    e->indent = saved_indent;
    e->at_bol = saved_bol;

    return true;
}

bool steel_c_emit_guard_begin(steel_c_emit* e, const char* macro_name)
{
    if (!e || !e->out) return false;
    if (!macro_name) macro_name = "";

    bool saved_bol = e->at_bol;
    uint32_t saved_indent = e->indent;
    e->at_bol = true;
    e->indent = 0;

    if (!steel_c_emit_write(e, "#ifndef ")) return false;
    if (!steel_c_emit_write(e, macro_name)) return false;
    if (!steel_c_emit_nl(e)) return false;

    if (!steel_c_emit_write(e, "#define ")) return false;
    if (!steel_c_emit_write(e, macro_name)) return false;
    if (!steel_c_emit_nl(e)) return false;

    e->indent = saved_indent;
    e->at_bol = saved_bol;

    return true;
}

bool steel_c_emit_guard_end(steel_c_emit* e, const char* macro_name)
{
    if (!e || !e->out) return false;
    if (!macro_name) macro_name = "";

    bool saved_bol = e->at_bol;
    uint32_t saved_indent = e->indent;
    e->at_bol = true;
    e->indent = 0;

    if (!steel_c_emit_write(e, "#endif /* ")) return false;
    if (!steel_c_emit_write(e, macro_name)) return false;
    if (!steel_c_emit_write(e, " */")) return false;
    if (!steel_c_emit_nl(e)) return false;

    e->indent = saved_indent;
    e->at_bol = saved_bol;

    return true;
}

// Block helpers

bool steel_c_emit_block_begin(steel_c_emit* e)
{
    if (!steel_c_emit_write(e, "{")) return false;
    if (!steel_c_emit_nl(e)) return false;
    steel_c_emit_indent_push(e);
    return true;
}

bool steel_c_emit_block_end(steel_c_emit* e)
{
    steel_c_emit_indent_pop(e);
    if (!steel_c_emit_write(e, "}")) return false;
    return true;
}

bool steel_c_emit_block_end_ln(steel_c_emit* e)
{
    if (!steel_c_emit_block_end(e)) return false;
    return steel_c_emit_nl(e);
}

//------------------------------------------------------------------------------
// Compatibility wrappers (optional)
// If your codebase uses shorter names, keep these.
//------------------------------------------------------------------------------

#ifdef STEEL_C_EMIT_ENABLE_SHORT_NAMES

void c_emit_init(steel_c_emit* e, FILE* out) { steel_c_emit_init(e, out); }
bool c_emit_init_path(steel_c_emit* e, const char* path) { return steel_c_emit_init_path(e, path); }
void c_emit_dispose(steel_c_emit* e) { steel_c_emit_dispose(e); }
int  c_emit_last_error(const steel_c_emit* e) { return steel_c_emit_last_error(e); }
bool c_emit_flush(steel_c_emit* e) { return steel_c_emit_flush(e); }

void c_emit_indent_push(steel_c_emit* e) { steel_c_emit_indent_push(e); }
void c_emit_indent_pop(steel_c_emit* e) { steel_c_emit_indent_pop(e); }
bool c_emit_nl(steel_c_emit* e) { return steel_c_emit_nl(e); }
bool c_emit_write(steel_c_emit* e, const char* s) { return steel_c_emit_write(e, s); }
bool c_emit_write_n(steel_c_emit* e, const char* s, size_t n) { return steel_c_emit_write_n(e, s, n); }
bool c_emit_printf(steel_c_emit* e, const char* fmt, ...) { va_list ap; va_start(ap, fmt); bool ok = steel_c_emit_vprintf(e, fmt, ap); va_end(ap); return ok; }

bool c_emit_ident(steel_c_emit* e, const char* ident) { return steel_c_emit_ident(e, ident); }
bool c_emit_cstr_lit(steel_c_emit* e, const char* s) { return steel_c_emit_cstr_lit(e, s); }
bool c_emit_bytes_lit(steel_c_emit* e, const uint8_t* b, size_t n) { return steel_c_emit_bytes_lit(e, b, n); }
bool c_emit_char_lit(steel_c_emit* e, uint32_t ch) { return steel_c_emit_char_lit(e, ch); }

bool c_emit_comment_line(steel_c_emit* e, const char* t) { return steel_c_emit_comment_line(e, t); }
bool c_emit_comment_block(steel_c_emit* e, const char* t) { return steel_c_emit_comment_block(e, t); }

bool c_emit_pp_include(steel_c_emit* e, const char* p, bool sys) { return steel_c_emit_pp_include(e, p, sys); }
bool c_emit_pp_define(steel_c_emit* e, const char* n, const char* v) { return steel_c_emit_pp_define(e, n, v); }

bool c_emit_guard_begin(steel_c_emit* e, const char* m) { return steel_c_emit_guard_begin(e, m); }
bool c_emit_guard_end(steel_c_emit* e, const char* m) { return steel_c_emit_guard_end(e, m); }

bool c_emit_block_begin(steel_c_emit* e) { return steel_c_emit_block_begin(e); }
bool c_emit_block_end(steel_c_emit* e) { return steel_c_emit_block_end(e); }
bool c_emit_block_end_ln(steel_c_emit* e) { return steel_c_emit_block_end_ln(e); }

#endif
