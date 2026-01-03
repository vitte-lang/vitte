

// SPDX-License-Identifier: MIT
// ast.c
//
// Core AST storage for the vitte compiler.
//
// This implementation provides:
//  - An arena allocator for AST-owned memory (strings, small blobs)
//  - A string interner for stable symbol/name pointers
//  - A node store indexed by stable ids (u32)
//  - Generic child lists per node (using steel_vec)
//  - Dump utilities for debugging
//
// Important:
//  - This file intentionally avoids depending on parser/token internals.
//  - If `ast.h` already defines concrete structs/enums, keep them consistent.
//    This file assumes `ast.h` forward-declares the opaque structs and declares
//    the public API implemented here.

#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include "../common/vec.h"

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#ifndef STEEL_AST_ARENA_CHUNK_MIN
  #define STEEL_AST_ARENA_CHUNK_MIN (64u * 1024u)
#endif

#ifndef STEEL_AST_INTERN_LOAD_NUM
  #define STEEL_AST_INTERN_LOAD_NUM 7u
#endif
#ifndef STEEL_AST_INTERN_LOAD_DEN
  #define STEEL_AST_INTERN_LOAD_DEN 10u
#endif

#ifndef STEEL_AST_DUMP_MAX_NAME
  #define STEEL_AST_DUMP_MAX_NAME 256
#endif

//------------------------------------------------------------------------------
// Fallback types (only if ast.h is minimal/opaque)
//------------------------------------------------------------------------------

#ifndef STEEL_AST_ID_T
// If ast.h already defines steel_ast_id, do not define this macro.
// Some codebases typedef it; if not, we provide a default.
typedef uint32_t steel_ast_id;
#define STEEL_AST_ID_T 1
#endif

#ifndef STEEL_AST_KIND_T
// If ast.h defines a dedicated enum, it should typedef/alias to steel_ast_kind.
// Otherwise, treat it as a plain integer.
typedef uint16_t steel_ast_kind;
#define STEEL_AST_KIND_T 1
#endif

// If ast.h defines a span type, keep using it. Otherwise, store a minimal span.
#ifndef STEEL_SPAN_T
typedef struct steel_span
{
    uint32_t file_id;
    uint32_t start;
    uint32_t end;
    uint32_t line;
    uint32_t col;
} steel_span;
#define STEEL_SPAN_T 1
#endif

//------------------------------------------------------------------------------
// Arena allocator
//------------------------------------------------------------------------------

typedef struct steel_ast_arena_chunk
{
    struct steel_ast_arena_chunk* next;
    size_t cap;
    size_t used;
    // uint8_t data[]
} steel_ast_arena_chunk;

typedef struct steel_ast_arena
{
    steel_ast_arena_chunk* head;
} steel_ast_arena;

static void steel_ast_arena_init(steel_ast_arena* a)
{
    a->head = NULL;
}

static void steel_ast_arena_dispose(steel_ast_arena* a)
{
    steel_ast_arena_chunk* c = a->head;
    while (c)
    {
        steel_ast_arena_chunk* n = c->next;
        free(c);
        c = n;
    }
    a->head = NULL;
}

static size_t steel_align_up(size_t x, size_t a)
{
    return (x + (a - 1)) & ~(a - 1);
}

static void* steel_ast_arena_alloc(steel_ast_arena* a, size_t size, size_t align)
{
    if (size == 0) size = 1;
    if (align < 8) align = 8;

    size = steel_align_up(size, align);

    steel_ast_arena_chunk* c = a->head;
    if (!c || (steel_align_up(c->used, align) + size > c->cap))
    {
        size_t want = STEEL_AST_ARENA_CHUNK_MIN;
        if (want < size + 64) want = size + 64;

        size_t total = sizeof(steel_ast_arena_chunk) + want;
        steel_ast_arena_chunk* nc = (steel_ast_arena_chunk*)malloc(total);
        if (!nc) return NULL;

        nc->next = a->head;
        nc->cap = want;
        nc->used = 0;
        a->head = nc;
        c = nc;
    }

    size_t off = steel_align_up(c->used, align);
    uint8_t* base = (uint8_t*)(c + 1);
    void* p = base + off;
    c->used = off + size;
    return p;
}

static char* steel_ast_arena_strdup_n(steel_ast_arena* a, const char* s, size_t n)
{
    char* p = (char*)steel_ast_arena_alloc(a, n + 1, 1);
    if (!p) return NULL;
    if (n) memcpy(p, s, n);
    p[n] = 0;
    return p;
}

//------------------------------------------------------------------------------
// String interning
//------------------------------------------------------------------------------

typedef struct steel_intern_entry
{
    uint64_t hash;
    const char* str;
    size_t len;
} steel_intern_entry;

typedef struct steel_intern_table
{
    steel_intern_entry* entries;
    size_t cap;
    size_t len;
} steel_intern_table;

static uint64_t steel_fnv1a64(const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < len; i++)
    {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ull;
    }
    // avoid 0 sentinel
    return h ? h : 1ull;
}

static void steel_intern_init(steel_intern_table* t)
{
    t->entries = NULL;
    t->cap = 0;
    t->len = 0;
}

static void steel_intern_dispose(steel_intern_table* t)
{
    free(t->entries);
    t->entries = NULL;
    t->cap = 0;
    t->len = 0;
}

static bool steel_intern_rehash(steel_intern_table* t, size_t new_cap)
{
    steel_intern_entry* ne = (steel_intern_entry*)calloc(new_cap, sizeof(*ne));
    if (!ne) return false;

    for (size_t i = 0; i < t->cap; i++)
    {
        steel_intern_entry e = t->entries[i];
        if (!e.str) continue;

        size_t mask = new_cap - 1;
        size_t idx = (size_t)e.hash & mask;
        while (ne[idx].str)
            idx = (idx + 1) & mask;

        ne[idx] = e;
    }

    free(t->entries);
    t->entries = ne;
    t->cap = new_cap;
    // t->len unchanged
    return true;
}

static bool steel_is_pow2(size_t x)
{
    return x && ((x & (x - 1)) == 0);
}

static bool steel_intern_ensure_cap(steel_intern_table* t)
{
    // Ensure cap is power-of-two.
    if (t->cap == 0)
        return steel_intern_rehash(t, 1024);

    if (!steel_is_pow2(t->cap))
        return steel_intern_rehash(t, 1024);

    // Load factor check.
    if ((t->len + 1) * STEEL_AST_INTERN_LOAD_DEN < t->cap * STEEL_AST_INTERN_LOAD_NUM)
        return true;

    return steel_intern_rehash(t, t->cap * 2);
}

static const char* steel_intern_put(steel_intern_table* t, steel_ast_arena* arena, const char* s, size_t n)
{
    if (!s) { s = ""; n = 0; }

    if (!steel_intern_ensure_cap(t))
        return NULL;

    uint64_t h = steel_fnv1a64(s, n);
    size_t mask = t->cap - 1;
    size_t idx = (size_t)h & mask;

    while (t->entries[idx].str)
    {
        steel_intern_entry* e = &t->entries[idx];
        if (e->hash == h && e->len == n && memcmp(e->str, s, n) == 0)
            return e->str;
        idx = (idx + 1) & mask;
    }

    char* copy = steel_ast_arena_strdup_n(arena, s, n);
    if (!copy) return NULL;

    t->entries[idx].hash = h;
    t->entries[idx].str = copy;
    t->entries[idx].len = n;
    t->len++;
    return copy;
}

//------------------------------------------------------------------------------
// AST node storage
//------------------------------------------------------------------------------

// Generic node representation: it is intentionally loose to support both
// "core" and "phrase" AST layers without exploding the struct.
//
// Higher-level passes can interpret `a/b/c`, `name`, and `kids` according to kind.

struct steel_ast_node
{
    steel_ast_kind kind;
    uint16_t _pad0;
    uint32_t flags;

    steel_span span;

    const char* name;   // interned
    const char* text;   // interned; used for identifiers/strings/raw tokens

    // Generic link fields
    steel_ast_id a;
    steel_ast_id b;
    steel_ast_id c;

    int64_t  i64;
    double   f64;

    // Child lists
    steel_vec kids; // steel_ast_id
    steel_vec aux;  // steel_ast_id (optional)
};

struct steel_ast
{
    steel_ast_arena arena;
    steel_intern_table intern;

    steel_vec nodes; // steel_ast_node
};

static void steel_ast_node_init(struct steel_ast_node* n)
{
    memset(n, 0, sizeof(*n));
    steel_vec_init(&n->kids, sizeof(steel_ast_id));
    steel_vec_init(&n->aux, sizeof(steel_ast_id));
}

static void steel_ast_node_dispose(struct steel_ast_node* n)
{
    steel_vec_free(&n->kids);
    steel_vec_free(&n->aux);
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void steel_ast_init(steel_ast* ast)
{
    if (!ast) return;
    memset(ast, 0, sizeof(*ast));

    steel_ast_arena_init(&ast->arena);
    steel_intern_init(&ast->intern);
    steel_vec_init(&ast->nodes, sizeof(struct steel_ast_node));
}

void steel_ast_dispose(steel_ast* ast)
{
    if (!ast) return;

    // Dispose node vectors first (they own heap allocations via steel_vec).
    size_t n = steel_vec_len(&ast->nodes);
    for (size_t i = 0; i < n; i++)
    {
        struct steel_ast_node* node = (struct steel_ast_node*)steel_vec_at(&ast->nodes, i);
        if (node) steel_ast_node_dispose(node);
    }

    steel_vec_free(&ast->nodes);
    steel_intern_dispose(&ast->intern);
    steel_ast_arena_dispose(&ast->arena);

    memset(ast, 0, sizeof(*ast));
}

const char* steel_ast_intern(steel_ast* ast, const char* s)
{
    if (!ast) return NULL;
    if (!s) s = "";
    return steel_intern_put(&ast->intern, &ast->arena, s, strlen(s));
}

const char* steel_ast_intern_n(steel_ast* ast, const char* s, size_t n)
{
    if (!ast) return NULL;
    if (!s) { s = ""; n = 0; }
    return steel_intern_put(&ast->intern, &ast->arena, s, n);
}

// Node ids are 1-based. 0 is invalid.
steel_ast_id steel_ast_new(steel_ast* ast, steel_ast_kind kind, steel_span span)
{
    if (!ast) return 0;

    struct steel_ast_node node;
    steel_ast_node_init(&node);
    node.kind = kind;
    node.span = span;

    struct steel_ast_node* pushed = (struct steel_ast_node*)steel_vec_push(&ast->nodes, &node);
    if (!pushed)
        return 0;

    // Return id = index + 1
    size_t idx = steel_vec_len(&ast->nodes) - 1;
    if (idx >= UINT32_MAX) return 0;
    return (steel_ast_id)(idx + 1);
}

struct steel_ast_node* steel_ast_get(steel_ast* ast, steel_ast_id id)
{
    if (!ast || id == 0) return NULL;
    size_t idx = (size_t)id - 1;
    return (struct steel_ast_node*)steel_vec_at(&ast->nodes, idx);
}

const struct steel_ast_node* steel_ast_cget(const steel_ast* ast, steel_ast_id id)
{
    if (!ast || id == 0) return NULL;
    size_t idx = (size_t)id - 1;
    return (const struct steel_ast_node*)steel_vec_cat(&ast->nodes, idx);
}

size_t steel_ast_count(const steel_ast* ast)
{
    return ast ? steel_vec_len(&ast->nodes) : 0;
}

bool steel_ast_set_name(steel_ast* ast, steel_ast_id id, const char* name)
{
    struct steel_ast_node* n = steel_ast_get(ast, id);
    if (!n) return false;
    n->name = steel_ast_intern(ast, name ? name : "");
    return n->name != NULL;
}

bool steel_ast_set_text(steel_ast* ast, steel_ast_id id, const char* text)
{
    struct steel_ast_node* n = steel_ast_get(ast, id);
    if (!n) return false;
    n->text = steel_ast_intern(ast, text ? text : "");
    return n->text != NULL;
}

bool steel_ast_add_kid(steel_ast* ast, steel_ast_id parent, steel_ast_id child)
{
    struct steel_ast_node* p = steel_ast_get(ast, parent);
    if (!p) return false;
    return steel_vec_push(&p->kids, &child) != NULL;
}

bool steel_ast_add_aux(steel_ast* ast, steel_ast_id node, steel_ast_id item)
{
    struct steel_ast_node* p = steel_ast_get(ast, node);
    if (!p) return false;
    return steel_vec_push(&p->aux, &item) != NULL;
}

size_t steel_ast_kid_count(const steel_ast* ast, steel_ast_id id)
{
    const struct steel_ast_node* n = steel_ast_cget(ast, id);
    if (!n) return 0;
    return steel_vec_len(&n->kids);
}

steel_ast_id steel_ast_kid_at(const steel_ast* ast, steel_ast_id id, size_t i)
{
    const struct steel_ast_node* n = steel_ast_cget(ast, id);
    if (!n) return 0;
    const steel_ast_id* p = (const steel_ast_id*)steel_vec_cat(&n->kids, i);
    return p ? *p : 0;
}

size_t steel_ast_aux_count(const steel_ast* ast, steel_ast_id id)
{
    const struct steel_ast_node* n = steel_ast_cget(ast, id);
    if (!n) return 0;
    return steel_vec_len(&n->aux);
}

steel_ast_id steel_ast_aux_at(const steel_ast* ast, steel_ast_id id, size_t i)
{
    const struct steel_ast_node* n = steel_ast_cget(ast, id);
    if (!n) return 0;
    const steel_ast_id* p = (const steel_ast_id*)steel_vec_cat(&n->aux, i);
    return p ? *p : 0;
}

//------------------------------------------------------------------------------
// Dump helpers
//------------------------------------------------------------------------------

static void steel_ast_dump_indent(FILE* out, uint32_t depth)
{
    for (uint32_t i = 0; i < depth; i++)
        fputs("  ", out);
}

static const char* steel_ast_kind_debug_name(steel_ast_kind k)
{
    // Avoid depending on a specific enum definition.
    // Return a stable string like "k42".
    static char ring[8][16];
    static unsigned r = 0;
    char* buf = ring[r++ & 7u];
    snprintf(buf, 16, "k%u", (unsigned)k);
    return buf;
}

static void steel_ast_dump_node_rec(FILE* out, const steel_ast* ast, steel_ast_id id, uint32_t depth)
{
    const struct steel_ast_node* n = steel_ast_cget(ast, id);
    if (!n)
    {
        steel_ast_dump_indent(out, depth);
        fprintf(out, "(null:%u)\n", (unsigned)id);
        return;
    }

    steel_ast_dump_indent(out, depth);

    const char* kname = steel_ast_kind_debug_name(n->kind);
    fprintf(out, "[%u] %s", (unsigned)id, kname);

    if (n->name && n->name[0])
        fprintf(out, " name='%.*s'", STEEL_AST_DUMP_MAX_NAME, n->name);

    if (n->text && n->text[0])
        fprintf(out, " text='%.*s'", STEEL_AST_DUMP_MAX_NAME, n->text);

    // span (best-effort)
    if (n->span.end > n->span.start)
    {
        fprintf(out, " @f%u:%u-%u", (unsigned)n->span.file_id, (unsigned)n->span.start, (unsigned)n->span.end);
    }

    // generic fields
    if (n->a || n->b || n->c)
        fprintf(out, " a=%u b=%u c=%u", (unsigned)n->a, (unsigned)n->b, (unsigned)n->c);

    if (n->i64)
        fprintf(out, " i64=%lld", (long long)n->i64);

    if (n->f64 != 0.0)
        fprintf(out, " f64=%g", n->f64);

    fprintf(out, "\n");

    size_t kc = steel_vec_len(&n->kids);
    for (size_t i = 0; i < kc; i++)
    {
        const steel_ast_id* cid = (const steel_ast_id*)steel_vec_cat(&n->kids, i);
        if (!cid) continue;
        steel_ast_dump_node_rec(out, ast, *cid, depth + 1);
    }

    size_t ac = steel_vec_len(&n->aux);
    if (ac)
    {
        steel_ast_dump_indent(out, depth + 1);
        fputs("(aux)\n", out);
        for (size_t i = 0; i < ac; i++)
        {
            const steel_ast_id* cid = (const steel_ast_id*)steel_vec_cat(&n->aux, i);
            if (!cid) continue;
            steel_ast_dump_node_rec(out, ast, *cid, depth + 2);
        }
    }
}

void steel_ast_dump(FILE* out, const steel_ast* ast, steel_ast_id root)
{
    if (!out) out = stderr;
    if (!ast)
    {
        fputs("(ast=null)\n", out);
        return;
    }

    if (root == 0)
    {
        fprintf(out, "(ast nodes=%zu)\n", steel_vec_len(&ast->nodes));
        return;
    }

    steel_ast_dump_node_rec(out, ast, root, 0);
}