

// SPDX-License-Identifier: MIT
// muf_ast.c
//
// Muffin manifest AST (max).
//
// Provides a compact, arena-backed AST for `.muf` files, plus helper APIs to
// build, query, and pretty-print it.
//
// Design goals:
//  - Stable node ids (u32) with contiguous storage.
//  - Each node has: kind, span, optional interned text (for ident/string/int),
//    and a contiguous child list stored in a flat array.
//  - No recursion needed for storage; recursion is used only in pretty-printer.
//  - Zero external dependencies besides common vec + lexer span.
//
// Assumptions about existing project:
//  - `steel_span` is defined in `src/compiler/lexer.h`.
//  - `steel_vec` exists in `src/common/vec.h`.
//  - If a `muf_ast.h` header exists, this file will include it and use its
//    declarations. Otherwise, this file provides a fallback public API.
//

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../compiler/lexer.h"   // steel_span
#include "../common/vec.h"       // steel_vec

//------------------------------------------------------------------------------
// Optional header integration
//------------------------------------------------------------------------------

#if defined(__has_include)
  #if __has_include("muf_ast.h")
    #include "muf_ast.h"
    #define STEEL_HAS_MUF_AST_H 1
  #endif
#endif

#ifndef STEEL_HAS_MUF_AST_H

typedef uint32_t muf_ast_id;

typedef enum muf_ast_kind
{
    MUF_AST_FILE = 1,

    MUF_AST_HEADER,          // "muf" <version>
    MUF_AST_VERSION,         // int

    // Toplevel blocks
    MUF_AST_WORKSPACE_BLOCK, // workspace { ... }
    MUF_AST_PACKAGE_BLOCK,   // package { ... }
    MUF_AST_TARGET_BLOCK,    // target { ... }
    MUF_AST_DEPS_BLOCK,      // deps { ... }
    MUF_AST_PROFILE_BLOCK,   // profile { ... }
    MUF_AST_TOOLCHAIN_BLOCK, // toolchain { ... }
    MUF_AST_FEATURES_BLOCK,  // features { ... }
    MUF_AST_SCRIPTS_BLOCK,   // scripts { ... }
    MUF_AST_ABI_BLOCK,       // abi { ... }
    MUF_AST_INCLUDE_BLOCK,   // include { ... }
    MUF_AST_ENV_BLOCK,       // env { ... }
    MUF_AST_ON_BLOCK,        // on { ... }

    // Assignments / entries
    MUF_AST_KV,              // key = value
    MUF_AST_LIST,            // [a, b, ...]
    MUF_AST_TABLE,           // { kv* }

    // Values
    MUF_AST_IDENT,
    MUF_AST_STRING,
    MUF_AST_INT,
    MUF_AST_BOOL,
    MUF_AST_PATH,

    MUF_AST_ERROR,

} muf_ast_kind;

typedef struct muf_ast_node
{
    muf_ast_id id;
    muf_ast_kind kind;
    steel_span span;

    // Optional text payload (interned or stable storage pointer)
    const char* text;

    // Children range in a flat array
    uint32_t kids_ofs;
    uint32_t kids_len;

} muf_ast_node;

typedef struct muf_ast
{
    // nodes[0] is unused; ids are 1-based
    steel_vec nodes; // muf_ast_node

    // flat kids array of muf_ast_id
    steel_vec kids;  // muf_ast_id

} muf_ast;

void muf_ast_init(muf_ast* a);
void muf_ast_dispose(muf_ast* a);

muf_ast_id muf_ast_add_node(muf_ast* a, muf_ast_kind k, steel_span sp, const char* text, const muf_ast_id* children, uint32_t child_count);

const muf_ast_node* muf_ast_get(const muf_ast* a, muf_ast_id id);

muf_ast_kind muf_ast_kindof(const muf_ast* a, muf_ast_id id);
steel_span muf_ast_spanof(const muf_ast* a, muf_ast_id id);
const char* muf_ast_textof(const muf_ast* a, muf_ast_id id);

uint32_t muf_ast_kid_count(const muf_ast* a, muf_ast_id id);
muf_ast_id muf_ast_kid_at(const muf_ast* a, muf_ast_id id, uint32_t idx);

void muf_ast_dump(const muf_ast* a, muf_ast_id root, FILE* out);

#endif // !STEEL_HAS_MUF_AST_H

//------------------------------------------------------------------------------
// Implementation
//------------------------------------------------------------------------------

static const muf_ast_node* node_(const muf_ast* a, muf_ast_id id)
{
    if (!a || id == 0) return NULL;
    size_t idx = (size_t)id;
    if (idx >= steel_vec_len(&a->nodes)) return NULL;
    return (const muf_ast_node*)steel_vec_cat(&a->nodes, idx);
}

static muf_ast_node* node_mut_(muf_ast* a, muf_ast_id id)
{
    if (!a || id == 0) return NULL;
    size_t idx = (size_t)id;
    if (idx >= steel_vec_len(&a->nodes)) return NULL;
    return (muf_ast_node*)steel_vec_at(&a->nodes, idx);
}

static void push_dummy0_(muf_ast* a)
{
    // Ensure nodes[0] exists (unused) so ids are 1-based.
    if (steel_vec_len(&a->nodes) == 0)
    {
        muf_ast_node z;
        memset(&z, 0, sizeof(z));
        steel_vec_push(&a->nodes, &z);
    }
}

void muf_ast_init(muf_ast* a)
{
    if (!a) return;
    memset(a, 0, sizeof(*a));

    steel_vec_init(&a->nodes, sizeof(muf_ast_node));
    steel_vec_init(&a->kids, sizeof(muf_ast_id));

    push_dummy0_(a);
}

void muf_ast_dispose(muf_ast* a)
{
    if (!a) return;
    steel_vec_free(&a->nodes);
    steel_vec_free(&a->kids);
    memset(a, 0, sizeof(*a));
}

static uint32_t kids_push_many_(muf_ast* a, const muf_ast_id* xs, uint32_t n)
{
    uint32_t ofs = (uint32_t)steel_vec_len(&a->kids);
    for (uint32_t i = 0; i < n; i++)
        steel_vec_push(&a->kids, &xs[i]);
    return ofs;
}

muf_ast_id muf_ast_add_node(muf_ast* a, muf_ast_kind k, steel_span sp, const char* text, const muf_ast_id* children, uint32_t child_count)
{
    if (!a)
        return 0;

    push_dummy0_(a);

    muf_ast_node n;
    memset(&n, 0, sizeof(n));

    n.id = (uint32_t)steel_vec_len(&a->nodes); // next id (because nodes[0] exists)
    n.kind = k;
    n.span = sp;
    n.text = text;

    if (children && child_count)
    {
        n.kids_ofs = kids_push_many_(a, children, child_count);
        n.kids_len = child_count;
    }

    steel_vec_push(&a->nodes, &n);

    return n.id;
}

const muf_ast_node* muf_ast_get(const muf_ast* a, muf_ast_id id)
{
    return node_(a, id);
}

muf_ast_kind muf_ast_kindof(const muf_ast* a, muf_ast_id id)
{
    const muf_ast_node* n = node_(a, id);
    return n ? n->kind : MUF_AST_ERROR;
}

steel_span muf_ast_spanof(const muf_ast* a, muf_ast_id id)
{
    const muf_ast_node* n = node_(a, id);
    if (!n)
    {
        steel_span z;
        memset(&z, 0, sizeof(z));
        return z;
    }
    return n->span;
}

const char* muf_ast_textof(const muf_ast* a, muf_ast_id id)
{
    const muf_ast_node* n = node_(a, id);
    return n ? n->text : NULL;
}

uint32_t muf_ast_kid_count(const muf_ast* a, muf_ast_id id)
{
    const muf_ast_node* n = node_(a, id);
    return n ? n->kids_len : 0;
}

muf_ast_id muf_ast_kid_at(const muf_ast* a, muf_ast_id id, uint32_t idx)
{
    const muf_ast_node* n = node_(a, id);
    if (!n || idx >= n->kids_len)
        return 0;

    const muf_ast_id* p = (const muf_ast_id*)steel_vec_cat(&a->kids, (size_t)n->kids_ofs + idx);
    return p ? *p : 0;
}

//------------------------------------------------------------------------------
// Pretty printing / dumping
//------------------------------------------------------------------------------

static const char* kind_name_(muf_ast_kind k)
{
    switch ((int)k)
    {
        case MUF_AST_FILE: return "file";
        case MUF_AST_HEADER: return "header";
        case MUF_AST_VERSION: return "version";

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

        case MUF_AST_KV: return "kv";
        case MUF_AST_LIST: return "list";
        case MUF_AST_TABLE: return "table";

        case MUF_AST_IDENT: return "ident";
        case MUF_AST_STRING: return "string";
        case MUF_AST_INT: return "int";
        case MUF_AST_BOOL: return "bool";
        case MUF_AST_PATH: return "path";

        case MUF_AST_ERROR: return "error";
        default: return "?";
    }
}

static void dump_rec_(const muf_ast* a, muf_ast_id id, FILE* out, uint32_t depth)
{
    const muf_ast_node* n = node_(a, id);
    if (!n || !out) return;

    for (uint32_t i = 0; i < depth; i++)
        fputs("  ", out);

    fprintf(out, "%u:%s", (unsigned)n->id, kind_name_(n->kind));

    if (n->text && n->text[0] != 0)
        fprintf(out, " \"%s\"", n->text);

    fprintf(out, " span=%u:%u..%u", (unsigned)n->span.file_id, (unsigned)n->span.start, (unsigned)n->span.end);

    fputc('\n', out);

    for (uint32_t i = 0; i < n->kids_len; i++)
        dump_rec_(a, muf_ast_kid_at(a, id, i), out, depth + 1);
}

void muf_ast_dump(const muf_ast* a, muf_ast_id root, FILE* out)
{
    if (!a) return;
    if (!out) out = stderr;
    if (root == 0) root = 1;

    dump_rec_(a, root, out, 0);
}