// SPDX-License-Identifier: MIT
// muf_emit.c
//
// Muffin manifest emitter (max).
//
// Responsibilities:
//  - Convert a Muffin AST into a canonical `.muf` textual representation.
//  - Provide stable formatting: deterministic key ordering (optional),
//    consistent indentation, quoting, and list/table syntax.
//  - Offer a small writer abstraction to emit either to FILE* or an in-memory
//    buffer.
//
// Assumes:
//  - `muf_ast` from muf_ast.c
//  - `steel_span` available but unused for emission.
//

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "../common/vec.h"
#include "muf_ast.h" // if present; otherwise relies on muf_ast.c fallback types

//------------------------------------------------------------------------------
// Writer
//------------------------------------------------------------------------------

typedef struct muf_writer
{
    // If file != NULL, stream to file.
    FILE* file;

    // Otherwise accumulate to buffer.
    char* buf;
    size_t len;
    size_t cap;

    bool oom;

} muf_writer;

static void w_init_file_(muf_writer* w, FILE* f)
{
    memset(w, 0, sizeof(*w));
    w->file = f;
}

static void w_init_buf_(muf_writer* w)
{
    memset(w, 0, sizeof(*w));
}

static void w_free_(muf_writer* w)
{
    if (!w) return;
    if (!w->file)
        free(w->buf);
    memset(w, 0, sizeof(*w));
}

static bool w_grow_(muf_writer* w, size_t need)
{
    if (w->file) return true;
    if (w->oom) return false;

    size_t req = w->len + need + 1;
    if (req <= w->cap) return true;

    size_t ncap = (w->cap == 0) ? 256 : w->cap;
    while (ncap < req)
        ncap *= 2;

    char* nb = (char*)realloc(w->buf, ncap);
    if (!nb)
    {
        w->oom = true;
        return false;
    }

    w->buf = nb;
    w->cap = ncap;
    return true;
}

static void w_putn_(muf_writer* w, const char* s, size_t n)
{
    if (!w || !s) return;

    if (w->file)
    {
        fwrite(s, 1, n, w->file);
        return;
    }

    if (!w_grow_(w, n)) return;
    memcpy(w->buf + w->len, s, n);
    w->len += n;
    w->buf[w->len] = 0;
}

static void w_puts_(muf_writer* w, const char* s)
{
    if (!s) s = "";
    w_putn_(w, s, strlen(s));
}

static void w_putc_(muf_writer* w, char c)
{
    w_putn_(w, &c, 1);
}

static void w_printf_(muf_writer* w, const char* fmt, ...)
{
    if (!w || !fmt) return;

    va_list ap;
    va_start(ap, fmt);

    if (w->file)
    {
        vfprintf(w->file, fmt, ap);
        va_end(ap);
        return;
    }

    va_list ap2;
    va_copy(ap2, ap);
    int need = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);

    if (need < 0)
    {
        va_end(ap);
        return;
    }

    if (!w_grow_(w, (size_t)need))
    {
        va_end(ap);
        return;
    }

    vsnprintf(w->buf + w->len, w->cap - w->len, fmt, ap);
    w->len += (size_t)need;
    w->buf[w->len] = 0;

    va_end(ap);
}

//------------------------------------------------------------------------------
// Emission options
//------------------------------------------------------------------------------

typedef struct muf_emit_opts
{
    uint32_t indent_width;    // spaces per indent level
    bool sort_kv_keys;        // sort table entries by key for deterministic output
    bool trailing_newline;    // ensure file ends with newline

} muf_emit_opts;

static muf_emit_opts muf_emit_opts_default_(void)
{
    muf_emit_opts o;
    o.indent_width = 2;
    o.sort_kv_keys = true;
    o.trailing_newline = true;
    return o;
}

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

static void indent_(muf_writer* w, const muf_emit_opts* o, uint32_t depth)
{
    uint32_t n = o->indent_width * depth;
    for (uint32_t i = 0; i < n; i++)
        w_putc_(w, ' ');
}

static bool is_ident_char_(char c, bool first)
{
    if (first)
        return (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
    return (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-');
}

static bool is_ident_(const char* s)
{
    if (!s || !s[0]) return false;
    if (!is_ident_char_(s[0], true)) return false;
    for (size_t i = 1; s[i]; i++)
        if (!is_ident_char_(s[i], false)) return false;
    return true;
}

static void emit_string_escaped_(muf_writer* w, const char* s)
{
    if (!s) s = "";
    w_putc_(w, '"');

    for (size_t i = 0; s[i]; i++)
    {
        unsigned char c = (unsigned char)s[i];
        switch (c)
        {
            case '"': w_puts_(w, "\\\""); break;
            case '\\': w_puts_(w, "\\\\"); break;
            case '\n': w_puts_(w, "\\n"); break;
            case '\r': w_puts_(w, "\\r"); break;
            case '\t': w_puts_(w, "\\t"); break;
            default:
                if (c < 0x20)
                    w_printf_(w, "\\u%04x", (unsigned)c);
                else
                    w_putc_(w, (char)c);
                break;
        }
    }

    w_putc_(w, '"');
}

//------------------------------------------------------------------------------
// AST helpers (compat with fallback API)
//------------------------------------------------------------------------------

#ifndef MUF_AST_ERROR
// If muf_ast.h is not present, we assume muf_ast.c fallback is compiled.
// We redeclare the minimum signatures used here.

typedef uint32_t muf_ast_id;

typedef enum muf_ast_kind
{
    MUF_AST_FILE = 1,
    MUF_AST_HEADER,
    MUF_AST_VERSION,

    MUF_AST_WORKSPACE_BLOCK,
    MUF_AST_PACKAGE_BLOCK,
    MUF_AST_TARGET_BLOCK,
    MUF_AST_DEPS_BLOCK,
    MUF_AST_PROFILE_BLOCK,
    MUF_AST_TOOLCHAIN_BLOCK,
    MUF_AST_FEATURES_BLOCK,
    MUF_AST_SCRIPTS_BLOCK,
    MUF_AST_ABI_BLOCK,
    MUF_AST_INCLUDE_BLOCK,
    MUF_AST_ENV_BLOCK,
    MUF_AST_ON_BLOCK,

    MUF_AST_KV,
    MUF_AST_LIST,
    MUF_AST_TABLE,

    MUF_AST_IDENT,
    MUF_AST_STRING,
    MUF_AST_INT,
    MUF_AST_BOOL,
    MUF_AST_PATH,

    MUF_AST_ERROR,

} muf_ast_kind;

typedef struct muf_ast muf_ast;

typedef struct muf_ast_node
{
    muf_ast_id id;
    muf_ast_kind kind;
    steel_span span;
    const char* text;
    uint32_t kids_ofs;
    uint32_t kids_len;
} muf_ast_node;

const muf_ast_node* muf_ast_get(const muf_ast* a, muf_ast_id id);
muf_ast_kind muf_ast_kindof(const muf_ast* a, muf_ast_id id);
const char* muf_ast_textof(const muf_ast* a, muf_ast_id id);
uint32_t muf_ast_kid_count(const muf_ast* a, muf_ast_id id);
muf_ast_id muf_ast_kid_at(const muf_ast* a, muf_ast_id id, uint32_t idx);

#endif

static const char* node_text_(const muf_ast* a, muf_ast_id id)
{
    const char* t = muf_ast_textof(a, id);
    return t ? t : "";
}

static muf_ast_kind kind_(const muf_ast* a, muf_ast_id id)
{
    return muf_ast_kindof(a, id);
}

static uint32_t kids_(const muf_ast* a, muf_ast_id id)
{
    return muf_ast_kid_count(a, id);
}

static muf_ast_id kid_(const muf_ast* a, muf_ast_id id, uint32_t i)
{
    return muf_ast_kid_at(a, id, i);
}

//------------------------------------------------------------------------------
// Sorting KV entries
//------------------------------------------------------------------------------

typedef struct kv_ref
{
    muf_ast_id kv;
    const char* key;
} kv_ref;

static int kv_ref_cmp_(const void* a, const void* b)
{
    const kv_ref* x = (const kv_ref*)a;
    const kv_ref* y = (const kv_ref*)b;

    // NULL-safe
    const char* xs = x->key ? x->key : "";
    const char* ys = y->key ? y->key : "";

    return strcmp(xs, ys);
}

static uint32_t collect_table_kvs_(const muf_ast* a, muf_ast_id table, kv_ref** out_refs)
{
    *out_refs = NULL;

    uint32_t n = kids_(a, table);
    if (n == 0) return 0;

    kv_ref* refs = (kv_ref*)calloc((size_t)n, sizeof(kv_ref));
    if (!refs) return 0;

    uint32_t used = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        muf_ast_id kv = kid_(a, table, i);
        if (kind_(a, kv) != MUF_AST_KV) continue;

        // kv kids: key, value
        muf_ast_id key = kids_(a, kv) > 0 ? kid_(a, kv, 0) : 0;
        const char* ktxt = node_text_(a, key);

        refs[used].kv = kv;
        refs[used].key = ktxt;
        used++;
    }

    *out_refs = refs;
    return used;
}

//------------------------------------------------------------------------------
// Emit value
//------------------------------------------------------------------------------

static void emit_value_(muf_writer* w, const muf_emit_opts* o, const muf_ast* a, muf_ast_id v, uint32_t depth);

static void emit_list_(muf_writer* w, const muf_emit_opts* o, const muf_ast* a, muf_ast_id list, uint32_t depth)
{
    (void)depth;

    w_putc_(w, '[');

    uint32_t n = kids_(a, list);
    for (uint32_t i = 0; i < n; i++)
    {
        if (i > 0) w_puts_(w, ", ");
        emit_value_(w, o, a, kid_(a, list, i), depth);
    }

    w_putc_(w, ']');
}

static void emit_table_(muf_writer* w, const muf_emit_opts* o, const muf_ast* a, muf_ast_id table, uint32_t depth)
{
    // Table is emitted as multi-line if it contains at least one KV.
    uint32_t n = kids_(a, table);

    w_puts_(w, "{\n");

    kv_ref* refs = NULL;
    uint32_t used = 0;

    if (o->sort_kv_keys)
    {
        used = collect_table_kvs_(a, table, &refs);
        if (used > 1 && refs)
            qsort(refs, used, sizeof(kv_ref), kv_ref_cmp_);

        for (uint32_t i = 0; i < used; i++)
        {
            muf_ast_id kv = refs[i].kv;
            muf_ast_id key = kids_(a, kv) > 0 ? kid_(a, kv, 0) : 0;
            muf_ast_id val = kids_(a, kv) > 1 ? kid_(a, kv, 1) : 0;

            indent_(w, o, depth + 1);

            const char* ktxt = node_text_(a, key);
            if (is_ident_(ktxt)) w_puts_(w, ktxt);
            else emit_string_escaped_(w, ktxt);

            w_puts_(w, " = ");
            emit_value_(w, o, a, val, depth + 1);
            w_puts_(w, "\n");
        }

        free(refs);
    }
    else
    {
        for (uint32_t i = 0; i < n; i++)
        {
            muf_ast_id kv = kid_(a, table, i);
            if (kind_(a, kv) != MUF_AST_KV) continue;

            muf_ast_id key = kids_(a, kv) > 0 ? kid_(a, kv, 0) : 0;
            muf_ast_id val = kids_(a, kv) > 1 ? kid_(a, kv, 1) : 0;

            indent_(w, o, depth + 1);

            const char* ktxt = node_text_(a, key);
            if (is_ident_(ktxt)) w_puts_(w, ktxt);
            else emit_string_escaped_(w, ktxt);

            w_puts_(w, " = ");
            emit_value_(w, o, a, val, depth + 1);
            w_puts_(w, "\n");
        }
    }

    indent_(w, o, depth);
    w_putc_(w, '}');
}

static void emit_value_(muf_writer* w, const muf_emit_opts* o, const muf_ast* a, muf_ast_id v, uint32_t depth)
{
    (void)depth;

    muf_ast_kind k = kind_(a, v);

    switch ((int)k)
    {
        case MUF_AST_IDENT:
        case MUF_AST_PATH:
        {
            const char* t = node_text_(a, v);
            if (is_ident_(t) || k == MUF_AST_PATH)
                w_puts_(w, t);
            else
                emit_string_escaped_(w, t);
            return;
        }

        case MUF_AST_STRING:
            emit_string_escaped_(w, node_text_(a, v));
            return;

        case MUF_AST_INT:
            w_puts_(w, node_text_(a, v));
            return;

        case MUF_AST_BOOL:
        {
            const char* t = node_text_(a, v);
            if (t && (strcmp(t, "true") == 0 || strcmp(t, "false") == 0))
                w_puts_(w, t);
            else
                w_puts_(w, "false");
            return;
        }

        case MUF_AST_LIST:
            emit_list_(w, o, a, v, depth);
            return;

        case MUF_AST_TABLE:
            emit_table_(w, o, a, v, depth);
            return;

        default:
            w_puts_(w, "<error>");
            return;
    }
}

//------------------------------------------------------------------------------
// Emit blocks
//------------------------------------------------------------------------------

static const char* block_name_(muf_ast_kind k)
{
    switch ((int)k)
    {
        case MUF_AST_WORKSPACE_BLOCK: return "workspace";
        case MUF_AST_PACKAGE_BLOCK: return "package";
        case MUF_AST_TARGET_BLOCK: return "target";
        case MUF_AST_DEPS_BLOCK: return "deps";
        case MUF_AST_PROFILE_BLOCK: return "profile";
        case MUF_AST_TOOLCHAIN_BLOCK: return "toolchain";
        case MUF_AST_FEATURES_BLOCK: return "features";
        case MUF_AST_SCRIPTS_BLOCK: return "scripts";
        case MUF_AST_ABI_BLOCK: return "abi";
        case MUF_AST_INCLUDE_BLOCK: return "include";
        case MUF_AST_ENV_BLOCK: return "env";
        case MUF_AST_ON_BLOCK: return "on";
        default: return "block";
    }
}

static void emit_block_(muf_writer* w, const muf_emit_opts* o, const muf_ast* a, muf_ast_id blk)
{
    muf_ast_kind k = kind_(a, blk);
    const char* name = block_name_(k);

    // Expect first child to be MUF_AST_TABLE (body), but tolerate.
    muf_ast_id body = 0;
    uint32_t n = kids_(a, blk);

    for (uint32_t i = 0; i < n; i++)
    {
        muf_ast_id c = kid_(a, blk, i);
        if (kind_(a, c) == MUF_AST_TABLE)
        {
            body = c;
            break;
        }
    }

    w_puts_(w, name);
    w_putc_(w, ' ');

    if (body)
        emit_table_(w, o, a, body, 0);
    else
        w_puts_(w, "{\n}\n");

    w_puts_(w, "\n\n");
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

typedef struct muf_emit_result
{
    bool ok;
    char* text;     // owned (malloc)
    size_t len;

} muf_emit_result;

static void result_zero_(muf_emit_result* r)
{
    if (!r) return;
    r->ok = false;
    r->text = NULL;
    r->len = 0;
}

void muf_emit_result_dispose(muf_emit_result* r)
{
    if (!r) return;
    free(r->text);
    result_zero_(r);
}

// Emit to file
bool muf_emit_to_file(const muf_ast* a, muf_ast_id root, FILE* out, const muf_emit_opts* opt_in)
{
    if (!a || !out)
        return false;

    muf_emit_opts o = opt_in ? *opt_in : muf_emit_opts_default_();

    muf_writer w;
    w_init_file_(&w, out);

    // Root is MUF_AST_FILE: children = header + blocks
    uint32_t n = kids_(a, root);
    for (uint32_t i = 0; i < n; i++)
    {
        muf_ast_id c = kid_(a, root, i);
        muf_ast_kind k = kind_(a, c);

        if (k == MUF_AST_HEADER)
        {
            // header kids: version
            muf_ast_id ver = kids_(a, c) > 0 ? kid_(a, c, 0) : 0;
            w_puts_(&w, "muf ");
            if (ver) w_puts_(&w, node_text_(a, ver));
            else w_puts_(&w, "1");
            w_putc_(&w, '\n');
            w_putc_(&w, '\n');
        }
        else if (k >= MUF_AST_WORKSPACE_BLOCK && k <= MUF_AST_ON_BLOCK)
        {
            emit_block_(&w, &o, a, c);
        }
        else
        {
            // ignore
        }
    }

    if (o.trailing_newline)
        w_putc_(&w, '\n');

    // file writer ignores oom
    return true;
}

// Emit to string
bool muf_emit_to_string(const muf_ast* a, muf_ast_id root, muf_emit_result* out, const muf_emit_opts* opt_in)
{
    if (!out)
        return false;
    result_zero_(out);

    if (!a)
        return false;

    muf_emit_opts o = opt_in ? *opt_in : muf_emit_opts_default_();

    muf_writer w;
    w_init_buf_(&w);

    uint32_t n = kids_(a, root);
    for (uint32_t i = 0; i < n; i++)
    {
        muf_ast_id c = kid_(a, root, i);
        muf_ast_kind k = kind_(a, c);

        if (k == MUF_AST_HEADER)
        {
            muf_ast_id ver = kids_(a, c) > 0 ? kid_(a, c, 0) : 0;
            w_puts_(&w, "muf ");
            if (ver) w_puts_(&w, node_text_(a, ver));
            else w_puts_(&w, "1");
            w_putc_(&w, '\n');
            w_putc_(&w, '\n');
        }
        else if (k >= MUF_AST_WORKSPACE_BLOCK && k <= MUF_AST_ON_BLOCK)
        {
            emit_block_(&w, &o, a, c);
        }
        else
        {
            // ignore
        }
    }

    if (o.trailing_newline)
        w_putc_(&w, '\n');

    if (w.oom)
    {
        w_free_(&w);
        return false;
    }

    out->ok = true;
    out->text = w.buf;
    out->len = w.len;

    // detach buffer
    w.buf = NULL;
    w_free_(&w);

    return true;
}

