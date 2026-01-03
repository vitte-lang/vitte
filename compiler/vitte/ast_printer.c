// SPDX-License-Identifier: MIT
// ast_printer.c
//
// AST pretty printer (max).
//
// Purpose:
//  - Deterministic, readable debug output for the Vitte AST.
//  - Configurable formatting: indentation, colors, compact mode.
//  - Handles unknown node kinds gracefully.
//
// Notes:
//  - This file is designed to compile even if the full AST type system isn't
//    wired yet. It supports two modes:
//      (1) If `ast.h`/`ast_nodes.h` (or project equivalents) are present, it
//          will use the concrete node structures.
//      (2) Otherwise it falls back to a generic tagged-node interface that the
//          parser can provide later.
//
// Integration:
//  - If `ast_printer.h` exists, include it.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(__has_include)
  #if __has_include("ast_printer.h")
    #include "ast_printer.h"
    #define VITTE_HAS_AST_PRINTER_H 1
  #endif
#endif

#if defined(__has_include)
  #if __has_include("ast.h")
    #include "ast.h"
    #define VITTE_HAS_AST_H 1
  #elif __has_include("../vitte/ast.h")
    #include "../vitte/ast.h"
    #define VITTE_HAS_AST_H 1
  #endif
#endif

#if defined(__has_include)
  #if __has_include("../pal/cli/ansi.h")
    #include "../pal/cli/ansi.h"
    #define VITTE_HAS_ANSI 1
  #elif __has_include("pal/cli/ansi.h")
    #include "pal/cli/ansi.h"
    #define VITTE_HAS_ANSI 1
  #else
    #define VITTE_HAS_ANSI 0
  #endif
#else
  #define VITTE_HAS_ANSI 0
#endif

//------------------------------------------------------------------------------
// Fallback public API if header missing
//------------------------------------------------------------------------------

#ifndef VITTE_HAS_AST_PRINTER_H

typedef struct ast_printer_cfg
{
    FILE* out;
    uint32_t indent_spaces;
    uint32_t max_depth;
    bool compact;
    bool use_color;
} ast_printer_cfg;

void ast_printer_cfg_init(ast_printer_cfg* cfg);

// Generic printing entrypoints.
// If concrete AST is available, the overload will be used.
void ast_print_any(const void* root, const ast_printer_cfg* cfg);

#endif

//------------------------------------------------------------------------------
// Error
//------------------------------------------------------------------------------

static _Thread_local char g_ast_print_err_[256];

static void set_msg_(const char* msg)
{
    if (!msg) msg = "";
    size_t n = strlen(msg);
    if (n >= sizeof(g_ast_print_err_)) n = sizeof(g_ast_print_err_) - 1;
    memcpy(g_ast_print_err_, msg, n);
    g_ast_print_err_[n] = 0;
}

const char* ast_printer_last_error(void)
{
    return g_ast_print_err_;
}

//------------------------------------------------------------------------------
// Config
//------------------------------------------------------------------------------

void ast_printer_cfg_init(ast_printer_cfg* cfg)
{
    if (!cfg) return;
    cfg->out = stdout;
    cfg->indent_spaces = 2;
    cfg->max_depth = 128;
    cfg->compact = false;
    cfg->use_color = true;
}

//------------------------------------------------------------------------------
// Writer helpers
//------------------------------------------------------------------------------

typedef struct wctx
{
    FILE* out;
    ast_printer_cfg cfg;
    uint32_t depth;
    bool at_line_start;
} wctx;

static void w_init_(wctx* w, const ast_printer_cfg* cfg)
{
    memset(w, 0, sizeof(*w));
    if (cfg)
        w->cfg = *cfg;
    else
        ast_printer_cfg_init(&w->cfg);

    w->out = w->cfg.out ? w->cfg.out : stdout;
    w->depth = 0;
    w->at_line_start = true;

#if !VITTE_HAS_ANSI
    w->cfg.use_color = false;
#endif
}

static void w_indent_(wctx* w)
{
    if (!w->at_line_start) return;
    uint32_t n = w->depth * w->cfg.indent_spaces;
    for (uint32_t i = 0; i < n; i++)
        fputc(' ', w->out);
    w->at_line_start = false;
}

static void w_nl_(wctx* w)
{
    fputc('\n', w->out);
    w->at_line_start = true;
}

static void w_vprintf_(wctx* w, const char* fmt, va_list ap)
{
    w_indent_(w);
    vfprintf(w->out, fmt, ap);
}

static void w_printf_(wctx* w, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    w_vprintf_(w, fmt, ap);
    va_end(ap);
}

static const char* c_reset_(wctx* w)
{
    (void)w;
#if VITTE_HAS_ANSI
    return w->cfg.use_color ? ansi_reset() : "";
#else
    return "";
#endif
}

static const char* c_key_(wctx* w)
{
    (void)w;
#if VITTE_HAS_ANSI
    return w->cfg.use_color ? ansi_bold() : "";
#else
    return "";
#endif
}

static const char* c_kind_(wctx* w)
{
    (void)w;
#if VITTE_HAS_ANSI
    return w->cfg.use_color ? ansi_fg256(39) : ""; // blue-ish
#else
    return "";
#endif
}

static const char* c_str_(wctx* w)
{
    (void)w;
#if VITTE_HAS_ANSI
    return w->cfg.use_color ? ansi_fg256(214) : ""; // orange
#else
    return "";
#endif
}

static const char* c_num_(wctx* w)
{
    (void)w;
#if VITTE_HAS_ANSI
    return w->cfg.use_color ? ansi_fg256(82) : ""; // green
#else
    return "";
#endif
}

static void w_kv_str_(wctx* w, const char* key, const char* val)
{
    if (!val) val = "";
    if (w->cfg.compact)
        w_printf_(w, "%s%s%s=%s\"%s\"%s", c_key_(w), key, c_reset_(w), c_str_(w), val, c_reset_(w));
    else
        w_printf_(w, "%s%s%s: %s\"%s\"%s", c_key_(w), key, c_reset_(w), c_str_(w), val, c_reset_(w));
}

static void w_kv_u64_(wctx* w, const char* key, uint64_t v)
{
    if (w->cfg.compact)
        w_printf_(w, "%s%s%s=%s%llu%s", c_key_(w), key, c_reset_(w), c_num_(w), (unsigned long long)v, c_reset_(w));
    else
        w_printf_(w, "%s%s%s: %s%llu%s", c_key_(w), key, c_reset_(w), c_num_(w), (unsigned long long)v, c_reset_(w));
}

//------------------------------------------------------------------------------
// Generic node interface (fallback)
//------------------------------------------------------------------------------

// If concrete AST is not available, users can still dump a generic tree by
// providing a small vtable style interface.

typedef struct ast_any_vtbl
{
    const char* (*kind_name)(const void* node);
    uint32_t    (*child_count)(const void* node);
    const void* (*child_at)(const void* node, uint32_t i);
    const char* (*field_name)(const void* node, uint32_t i); // optional per child
    const char* (*node_text)(const void* node);              // optional
    uint64_t    (*node_id)(const void* node);                // optional
} ast_any_vtbl;

typedef struct ast_any
{
    const void* node;
    const ast_any_vtbl* v;
} ast_any;

static void print_any_node_(wctx* w, ast_any a);

static void print_any_children_(wctx* w, ast_any a)
{
    uint32_t n = a.v->child_count ? a.v->child_count(a.node) : 0;
    if (n == 0)
        return;

    if (!w->cfg.compact)
        w_nl_(w);

    w->depth++;
    if (w->depth > w->cfg.max_depth)
    {
        w_printf_(w, "%s<max-depth>%s", c_kind_(w), c_reset_(w));
        w_nl_(w);
        w->depth--;
        return;
    }

    for (uint32_t i = 0; i < n; i++)
    {
        const void* ch = a.v->child_at ? a.v->child_at(a.node, i) : NULL;
        const char* fname = a.v->field_name ? a.v->field_name(a.node, i) : NULL;

        if (fname && fname[0])
        {
            w_printf_(w, "%s%s%s ", c_key_(w), fname, c_reset_(w));
        }

        ast_any ca = { ch, a.v };
        print_any_node_(w, ca);

        if (i + 1 < n)
            w_nl_(w);
    }

    w->depth--;
}

static void print_any_node_(wctx* w, ast_any a)
{
    if (!a.node)
    {
        w_printf_(w, "%s<null>%s", c_kind_(w), c_reset_(w));
        return;
    }

    const char* kind = a.v->kind_name ? a.v->kind_name(a.node) : "<node>";

    // header: Kind (id=..., text="...")
    w_printf_(w, "%s%s%s", c_kind_(w), kind, c_reset_(w));

    bool has_any = false;

    if (a.v->node_id)
    {
        uint64_t id = a.v->node_id(a.node);
        if (id != 0)
        {
            w_printf_(w, w->cfg.compact ? "(" : " (" );
            w_kv_u64_(w, "id", id);
            has_any = true;
        }
    }

    if (a.v->node_text)
    {
        const char* t = a.v->node_text(a.node);
        if (t && t[0])
        {
            w_printf_(w, has_any ? ", " : (w->cfg.compact ? "(" : " ("));
            w_kv_str_(w, "text", t);
            has_any = true;
        }
    }

    if (has_any)
        w_printf_(w, ")");

    print_any_children_(w, a);
}

//------------------------------------------------------------------------------
// Concrete AST printing (optional)
//------------------------------------------------------------------------------

#if VITTE_HAS_AST_H

// Because project AST layout can evolve, we provide a conservative integration:
// we expect the following minimal helpers to exist (or we won't use them):
//  - ast_node_kind_name(kind)
//  - ast_node_child_count(node)
//  - ast_node_child_at(node, i)
//  - ast_node_id(node)
//  - ast_node_text(node)
//
// If your AST differs, adapt these adapters in one place.

#if defined(__has_include)
  #if __has_include("ast_adapters.h")
    #include "ast_adapters.h"
    #define VITTE_HAS_AST_ADAPTERS 1
  #elif __has_include("../vitte/ast_adapters.h")
    #include "../vitte/ast_adapters.h"
    #define VITTE_HAS_AST_ADAPTERS 1
  #endif
#endif

#ifndef VITTE_HAS_AST_ADAPTERS

// Default weak adapters (you can replace via ast_adapters.h)

// Expecting something like:
//   typedef struct ast_node ast_node;
//   typedef enum ast_kind ast_kind;

static const char* ast_kind_name_fallback_(const void* node)
{
    (void)node;
    return "ast_node";
}

static uint32_t ast_child_count_fallback_(const void* node)
{
    (void)node;
    return 0;
}

static const void* ast_child_at_fallback_(const void* node, uint32_t i)
{
    (void)node; (void)i;
    return NULL;
}

static const char* ast_field_name_fallback_(const void* node, uint32_t i)
{
    (void)node; (void)i;
    return NULL;
}

static const char* ast_text_fallback_(const void* node)
{
    (void)node;
    return NULL;
}

static uint64_t ast_id_fallback_(const void* node)
{
    (void)node;
    return 0;
}

#endif // !VITTE_HAS_AST_ADAPTERS

static const ast_any_vtbl g_ast_vtbl_ = {
#if defined(VITTE_HAS_AST_ADAPTERS)
    .kind_name = vitte_ast_kind_name,
    .child_count = vitte_ast_child_count,
    .child_at = vitte_ast_child_at,
    .field_name = vitte_ast_field_name,
    .node_text = vitte_ast_node_text,
    .node_id = vitte_ast_node_id,
#else
    .kind_name = ast_kind_name_fallback_,
    .child_count = ast_child_count_fallback_,
    .child_at = ast_child_at_fallback_,
    .field_name = ast_field_name_fallback_,
    .node_text = ast_text_fallback_,
    .node_id = ast_id_fallback_,
#endif
};

#endif // VITTE_HAS_AST_H

//------------------------------------------------------------------------------
// Public entrypoints
//------------------------------------------------------------------------------

void ast_print_any(const void* root, const ast_printer_cfg* cfg)
{
    wctx w;
    w_init_(&w, cfg);

    // If AST adapters exist and ast.h is present, use them.
#if VITTE_HAS_AST_H
    ast_any a = { root, &g_ast_vtbl_ };
#else
    // Without concrete AST, we can only print the pointer.
    // Users should provide a vtbl-based printer in the future.
    ast_any_vtbl v = {
        .kind_name = NULL,
        .child_count = NULL,
        .child_at = NULL,
        .field_name = NULL,
        .node_text = NULL,
        .node_id = NULL,
    };
    ast_any a = { root, &v };
#endif

    if (!a.node)
    {
        w_printf_(&w, "%s<null-root>%s", c_kind_(&w), c_reset_(&w));
        w_nl_(&w);
        return;
    }

    print_any_node_(&w, a);
    w_nl_(&w);
    fflush(w.out);
}

// Optional convenience if header defines concrete function name.
#if defined(VITTE_HAS_AST_PRINTER_H)

// If ast_printer.h declares `ast_print(const ast_node*, const ast_printer_cfg*)` we can
// provide it here via weak aliasing pattern; but to avoid symbol mismatch, we only
// provide a conservative wrapper name if requested.

#endif

