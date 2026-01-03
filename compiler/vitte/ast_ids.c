// SPDX-License-Identifier: MIT
// ast_ids.c
//
// AST ID allocator / interning utilities (max).
//
// This module provides deterministic, compact IDs for AST nodes (and optionally
// symbols/strings) used by the Vitte front-end.
//
// Features:
//  - Monotonic ID allocation for nodes (u32) with reset per-compilation.
//  - Optional interning table for string -> symbol id (u32) with stable hashing.
//  - Debug name lookup for interned strings.
//  - Thread-local last error string.
//
// Design:
//  - Node IDs: 1..N. 0 is invalid.
//  - Symbol IDs: 1..N. 0 is invalid.
//  - String interning uses open addressing with linear probing.
//
// Integration:
//  - If `ast_ids.h` exists, include it.
//  - Otherwise a fallback API is provided.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(__has_include)
  #if __has_include("ast_ids.h")
    #include "ast_ids.h"
    #define VITTE_HAS_AST_IDS_H 1
  #elif __has_include("../vitte/ast_ids.h")
    #include "../vitte/ast_ids.h"
    #define VITTE_HAS_AST_IDS_H 1
  #endif
#endif

#ifndef VITTE_HAS_AST_IDS_H

//------------------------------------------------------------------------------
// Fallback API (align later with ast_ids.h)
//------------------------------------------------------------------------------

typedef uint32_t ast_node_id;
typedef uint32_t ast_sym_id;

typedef struct ast_ids
{
    // node
    uint32_t next_node;

    // sym intern
    char**    sym_strs;   // owned strings, indexed by sym id (1..sym_count)
    uint32_t  sym_count;
    uint32_t  sym_cap;

    // hash table (open addressing)
    uint32_t* ht;         // stores sym_id; 0=empty
    uint32_t  ht_cap;
    uint32_t  ht_used;
} ast_ids;

void ast_ids_init(ast_ids* ids);
void ast_ids_dispose(ast_ids* ids);
void ast_ids_reset(ast_ids* ids);

ast_node_id ast_ids_new_node(ast_ids* ids);

// string interning
ast_sym_id  ast_ids_intern(ast_ids* ids, const char* s);
ast_sym_id  ast_ids_intern_n(ast_ids* ids, const char* s, size_t n);

const char* ast_ids_sym_cstr(const ast_ids* ids, ast_sym_id sym);

uint32_t ast_ids_node_count(const ast_ids* ids);
uint32_t ast_ids_sym_count(const ast_ids* ids);

const char* ast_ids_last_error(void);

#endif // !VITTE_HAS_AST_IDS_H

//------------------------------------------------------------------------------
// Error
//------------------------------------------------------------------------------

static _Thread_local char g_ast_ids_err_[256];

static void set_msg_(const char* msg)
{
    if (!msg) msg = "";
    size_t n = strlen(msg);
    if (n >= sizeof(g_ast_ids_err_)) n = sizeof(g_ast_ids_err_) - 1;
    memcpy(g_ast_ids_err_, msg, n);
    g_ast_ids_err_[n] = 0;
}

const char* ast_ids_last_error(void)
{
    return g_ast_ids_err_;
}

//------------------------------------------------------------------------------
// Hash (FNV-1a 64)
//------------------------------------------------------------------------------

static uint64_t fnv1a64_(const void* p, size_t n)
{
    const uint8_t* b = (const uint8_t*)p;
    const uint64_t OFF = 1469598103934665603ull;
    const uint64_t PRIME = 1099511628211ull;

    uint64_t h = OFF;
    for (size_t i = 0; i < n; i++)
    {
        h ^= (uint64_t)b[i];
        h *= PRIME;
    }

    return h;
}

static uint32_t hash32_(const void* p, size_t n)
{
    uint64_t h = fnv1a64_(p, n);
    // xor-fold
    return (uint32_t)(h ^ (h >> 32));
}

//------------------------------------------------------------------------------
// Intern table
//------------------------------------------------------------------------------

static void table_reset_(ast_ids* ids)
{
    ids->next_node = 1;

    ids->sym_strs = NULL;
    ids->sym_count = 0;
    ids->sym_cap = 0;

    ids->ht = NULL;
    ids->ht_cap = 0;
    ids->ht_used = 0;
}

static bool ensure_sym_cap_(ast_ids* ids, uint32_t want)
{
    if (ids->sym_cap >= want)
        return true;

    uint32_t new_cap = ids->sym_cap ? ids->sym_cap : 32u;
    while (new_cap < want)
    {
        uint32_t next = new_cap * 2u;
        if (next < new_cap)
        {
            set_msg_("sym capacity overflow");
            return false;
        }
        new_cap = next;
    }

    char** ns = (char**)realloc(ids->sym_strs, (size_t)new_cap * sizeof(char*));
    if (!ns)
    {
        set_msg_("out of memory");
        return false;
    }

    // init new region
    for (uint32_t i = ids->sym_cap; i < new_cap; i++)
        ns[i] = NULL;

    ids->sym_strs = ns;
    ids->sym_cap = new_cap;
    return true;
}

static bool ensure_ht_cap_(ast_ids* ids, uint32_t want_cap)
{
    if (ids->ht_cap >= want_cap)
        return true;

    uint32_t new_cap = ids->ht_cap ? ids->ht_cap : 64u;
    while (new_cap < want_cap)
    {
        uint32_t next = new_cap * 2u;
        if (next < new_cap)
        {
            set_msg_("hash capacity overflow");
            return false;
        }
        new_cap = next;
    }

    uint32_t* nh = (uint32_t*)malloc((size_t)new_cap * sizeof(uint32_t));
    if (!nh)
    {
        set_msg_("out of memory");
        return false;
    }

    for (uint32_t i = 0; i < new_cap; i++)
        nh[i] = 0;

    // rehash old
    if (ids->ht && ids->ht_cap)
    {
        for (uint32_t i = 0; i < ids->ht_cap; i++)
        {
            uint32_t sym = ids->ht[i];
            if (sym == 0)
                continue;

            const char* s = ast_ids_sym_cstr(ids, sym);
            if (!s)
                continue;

            size_t n = strlen(s);
            uint32_t h = hash32_(s, n);
            uint32_t mask = new_cap - 1u;
            uint32_t pos = h & mask;

            while (nh[pos] != 0)
                pos = (pos + 1u) & mask;

            nh[pos] = sym;
        }

        free(ids->ht);
    }

    ids->ht = nh;
    ids->ht_cap = new_cap;

    // ht_used stays same
    return true;
}

static bool ensure_load_(ast_ids* ids)
{
    // keep load factor <= ~0.7
    if (ids->ht_cap == 0)
        return ensure_ht_cap_(ids, 64u);

    uint32_t limit = (ids->ht_cap * 7u) / 10u;
    if (ids->ht_used + 1u <= limit)
        return true;

    return ensure_ht_cap_(ids, ids->ht_cap * 2u);
}

static bool str_eq_n_(const char* a, const char* b, size_t n)
{
    return memcmp(a, b, n) == 0 && b[n] == 0;
}

static ast_sym_id intern_impl_(ast_ids* ids, const char* s, size_t n)
{
    if (!ids)
    {
        set_msg_("invalid ids");
        return 0;
    }

    if (!s && n)
    {
        set_msg_("null string");
        return 0;
    }

    if (!ensure_load_(ids))
        return 0;

    // probe
    uint32_t h = hash32_(s, n);
    uint32_t mask = ids->ht_cap - 1u;
    uint32_t pos = h & mask;

    for (;;)
    {
        uint32_t sym = ids->ht[pos];
        if (sym == 0)
            break;

        const char* existing = ast_ids_sym_cstr(ids, sym);
        if (existing)
        {
            // compare length first cheaply
            if (strlen(existing) == n && memcmp(existing, s, n) == 0)
                return sym;
        }

        pos = (pos + 1u) & mask;
    }

    // insert new
    if (!ensure_sym_cap_(ids, ids->sym_count + 2u))
        return 0;

    char* copy = (char*)malloc(n + 1);
    if (!copy)
    {
        set_msg_("out of memory");
        return 0;
    }

    if (n) memcpy(copy, s, n);
    copy[n] = 0;

    uint32_t new_sym = ids->sym_count + 1u;
    ids->sym_strs[new_sym] = copy;
    ids->sym_count = new_sym;

    ids->ht[pos] = new_sym;
    ids->ht_used++;

    return new_sym;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void ast_ids_init(ast_ids* ids)
{
    if (!ids) return;
    memset(ids, 0, sizeof(*ids));
    table_reset_(ids);

    // allocate base hash table
    (void)ensure_ht_cap_(ids, 64u);
    // sym_strs uses index 1..N; slot 0 unused but allocated for simplicity
    (void)ensure_sym_cap_(ids, 32u);
}

void ast_ids_dispose(ast_ids* ids)
{
    if (!ids) return;

    if (ids->sym_strs)
    {
        for (uint32_t i = 1; i <= ids->sym_count; i++)
            free(ids->sym_strs[i]);
        free(ids->sym_strs);
    }

    free(ids->ht);

    table_reset_(ids);
    memset(ids, 0, sizeof(*ids));
}

void ast_ids_reset(ast_ids* ids)
{
    if (!ids) return;

    // keep interned strings by default? For front-end reuse, usually reset everything.
    // Here: reset nodes only, keep symbols to preserve dedup across compilation unit.
    ids->next_node = 1;
}

ast_node_id ast_ids_new_node(ast_ids* ids)
{
    if (!ids)
    {
        set_msg_("invalid ids");
        return 0;
    }

    uint32_t id = ids->next_node;
    if (id == 0)
    {
        set_msg_("node id overflow");
        return 0;
    }

    ids->next_node++;
    if (ids->next_node == 0)
    {
        // wrap (reserve 0)
        ids->next_node = 1;
        set_msg_("node id overflow");
        return 0;
    }

    return id;
}

ast_sym_id ast_ids_intern(ast_ids* ids, const char* s)
{
    if (!s) s = "";
    return intern_impl_(ids, s, strlen(s));
}

ast_sym_id ast_ids_intern_n(ast_ids* ids, const char* s, size_t n)
{
    return intern_impl_(ids, s, n);
}

const char* ast_ids_sym_cstr(const ast_ids* ids, ast_sym_id sym)
{
    if (!ids || !ids->sym_strs)
        return NULL;

    if (sym == 0 || sym > ids->sym_count)
        return NULL;

    return ids->sym_strs[sym];
}

uint32_t ast_ids_node_count(const ast_ids* ids)
{
    if (!ids) return 0;
    // next_node starts at 1
    return (ids->next_node > 0) ? (ids->next_node - 1u) : 0u;
}

uint32_t ast_ids_sym_count(const ast_ids* ids)
{
    if (!ids) return 0;
    return ids->sym_count;
}
