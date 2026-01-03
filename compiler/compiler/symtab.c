// SPDX-License-Identifier: MIT
// symtab.c
//
// Symbol table (max):
//  - Interned-string keyed scopes (lexical).
//  - Symbol storage with stable ids.
//  - Duplicate/undefined diagnostics hooks.
//
// This file is written to be usable even if `symtab.h` is not ready yet.
// If you already have `symtab.h`, include it and remove the fallback block.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lexer.h"          // steel_span
#include "../common/vec.h" // steel_vec

//------------------------------------------------------------------------------
// Optional header integration
//------------------------------------------------------------------------------

#if defined(__has_include)
    #if __has_include("symtab.h")
        #include "symtab.h"
        #define STEEL_HAS_SYMTAB_H 1
    #endif
#endif

#ifndef STEEL_HAS_SYMTAB_H

typedef enum steel_diag_sev
{
    STEEL_DIAG_INFO = 0,
    STEEL_DIAG_WARN,
    STEEL_DIAG_ERROR,
} steel_diag_sev;

typedef enum steel_symbol_kind
{
    STEEL_SYM_GLOBAL = 1,
    STEEL_SYM_TYPE,
    STEEL_SYM_FN,
    STEEL_SYM_PARAM,
    STEEL_SYM_LOCAL,
    STEEL_SYM_MODULE,
    STEEL_SYM_USE_ALIAS,
} steel_symbol_kind;

typedef struct steel_symbol_info
{
    uint32_t id;
    steel_symbol_kind kind;
    const char* name;      // interned pointer
    uint32_t decl_node;    // AST node id (opaque)
} steel_symbol_info;

typedef void (*steel_symtab_diag_fn)(void* user, steel_diag_sev sev, steel_span span, const char* msg);

typedef struct steel_symtab steel_symtab;

struct steel_symtab
{
    void* impl;
};

void steel_symtab_init(steel_symtab* st, steel_symtab_diag_fn diag, void* diag_user);
void steel_symtab_dispose(steel_symtab* st);

void steel_symtab_push_scope(steel_symtab* st);
void steel_symtab_pop_scope(steel_symtab* st);

uint32_t steel_symtab_define(steel_symtab* st,
                            steel_symbol_kind kind,
                            const char* name,
                            uint32_t decl_node,
                            steel_span at);

bool steel_symtab_lookup(const steel_symtab* st, const char* name, uint32_t* out_sym_id);
bool steel_symtab_lookup_current(const steel_symtab* st, const char* name, uint32_t* out_sym_id);

const steel_symbol_info* steel_symtab_symbol_info(const steel_symtab* st, uint32_t sym_id);

#endif // !STEEL_HAS_SYMTAB_H

//------------------------------------------------------------------------------
// Small hash map: interned const char* -> u32
//------------------------------------------------------------------------------

static inline uint64_t steel_hash_ptr_(const void* p)
{
    uintptr_t x = (uintptr_t)p;
    x ^= x >> 33;
    x *= (uintptr_t)0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= (uintptr_t)0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return (uint64_t)x;
}

typedef struct steel_map_str_u32
{
    const char** keys; // interned pointers
    uint32_t* vals;
    uint32_t cap;
    uint32_t len;
} steel_map_str_u32;

static void steel_map_str_u32_init(steel_map_str_u32* m)
{
    memset(m, 0, sizeof(*m));
}

static void steel_map_str_u32_free(steel_map_str_u32* m)
{
    free(m->keys);
    free(m->vals);
    memset(m, 0, sizeof(*m));
}

static bool steel_map_str_u32_grow_(steel_map_str_u32* m, uint32_t new_cap)
{
    const char** ok = m->keys;
    uint32_t* ov = m->vals;
    uint32_t oc = m->cap;

    const char** nk = (const char**)calloc((size_t)new_cap, sizeof(const char*));
    uint32_t* nv = (uint32_t*)calloc((size_t)new_cap, sizeof(uint32_t));
    if (!nk || !nv)
    {
        free(nk);
        free(nv);
        return false;
    }

    m->keys = nk;
    m->vals = nv;
    m->cap = new_cap;
    m->len = 0;

    if (ok)
    {
        uint32_t mask = new_cap - 1;
        for (uint32_t i = 0; i < oc; i++)
        {
            const char* k = ok[i];
            uint32_t v = ov[i];
            if (!k) continue;

            uint64_t h = steel_hash_ptr_(k);
            for (uint32_t p = 0; p < new_cap; p++)
            {
                uint32_t idx = (uint32_t)((h + p) & mask);
                if (!m->keys[idx])
                {
                    m->keys[idx] = k;
                    m->vals[idx] = v;
                    m->len++;
                    break;
                }
            }
        }
    }

    free((void*)ok);
    free(ov);
    return true;
}

static bool steel_map_str_u32_put(steel_map_str_u32* m, const char* key, uint32_t val)
{
    if (!key) return false;

    if (m->cap == 0)
    {
        if (!steel_map_str_u32_grow_(m, 64)) return false;
    }

    // load factor ~0.70
    if ((m->len + 1) * 10 >= m->cap * 7)
    {
        if (!steel_map_str_u32_grow_(m, m->cap * 2)) return false;
    }

    uint64_t h = steel_hash_ptr_(key);
    uint32_t mask = m->cap - 1;

    for (uint32_t p = 0; p < m->cap; p++)
    {
        uint32_t idx = (uint32_t)((h + p) & mask);
        const char* k = m->keys[idx];
        if (!k)
        {
            m->keys[idx] = key;
            m->vals[idx] = val;
            m->len++;
            return true;
        }
        if (k == key)
        {
            m->vals[idx] = val;
            return true;
        }
    }

    return false;
}

static bool steel_map_str_u32_get(const steel_map_str_u32* m, const char* key, uint32_t* out)
{
    if (!m || !m->cap || !key) return false;

    uint64_t h = steel_hash_ptr_(key);
    uint32_t mask = m->cap - 1;

    for (uint32_t p = 0; p < m->cap; p++)
    {
        uint32_t idx = (uint32_t)((h + p) & mask);
        const char* k = m->keys[idx];
        if (!k) return false;
        if (k == key)
        {
            if (out) *out = m->vals[idx];
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
// Symtab implementation
//------------------------------------------------------------------------------

typedef struct steel_scope
{
    steel_map_str_u32 map; // name -> symbol id
} steel_scope;

typedef struct steel_symbol
{
    uint32_t id;
    steel_symbol_kind kind;
    const char* name;      // interned
    uint32_t decl_node;    // AST node id / opaque
} steel_symbol;

typedef struct steel_symtab_impl
{
    steel_vec scopes;   // steel_scope
    steel_vec symbols;  // steel_symbol

    steel_symtab_diag_fn diag;
    void* diag_user;

    bool ok;
} steel_symtab_impl;

static void steel_symtab_diag_default_(void* user, steel_diag_sev sev, steel_span span, const char* msg)
{
    (void)user;
    const char* ssev = "info";
    if (sev == STEEL_DIAG_WARN) ssev = "warn";
    if (sev == STEEL_DIAG_ERROR) ssev = "error";

    fprintf(stderr, "[symtab:%s] file=%u span=[%u..%u] line=%u col=%u: %s\n",
            ssev,
            span.file_id,
            span.start,
            span.end,
            span.line,
            span.col,
            msg ? msg : "(null)");
}

static steel_symtab_impl* impl_(const steel_symtab* st)
{
    return st ? (steel_symtab_impl*)st->impl : NULL;
}

static steel_scope* cur_scope_(steel_symtab_impl* s)
{
    size_t n = steel_vec_len(&s->scopes);
    if (n == 0) return NULL;
    return (steel_scope*)steel_vec_at(&s->scopes, n - 1);
}

static steel_symbol* sym_new_(steel_symtab_impl* s, steel_symbol_kind kind, const char* name, uint32_t decl_node)
{
    steel_symbol sym;
    memset(&sym, 0, sizeof(sym));
    sym.id = (uint32_t)steel_vec_len(&s->symbols) + 1;
    sym.kind = kind;
    sym.name = name;
    sym.decl_node = decl_node;
    return (steel_symbol*)steel_vec_push(&s->symbols, &sym);
}

static void emit_(steel_symtab_impl* s, steel_diag_sev sev, steel_span sp, const char* msg)
{
    s->ok = false;
    if (!s->diag) s->diag = steel_symtab_diag_default_;
    s->diag(s->diag_user, sev, sp, msg);
}

static void scope_init_(steel_scope* sc)
{
    steel_map_str_u32_init(&sc->map);
}

static void scope_free_(steel_scope* sc)
{
    steel_map_str_u32_free(&sc->map);
    memset(sc, 0, sizeof(*sc));
}

void steel_symtab_init(steel_symtab* st, steel_symtab_diag_fn diag, void* diag_user)
{
    if (!st) return;

    steel_symtab_impl* s = (steel_symtab_impl*)calloc(1, sizeof(*s));
    if (!s)
    {
        memset(st, 0, sizeof(*st));
        return;
    }

    steel_vec_init(&s->scopes, sizeof(steel_scope));
    steel_vec_init(&s->symbols, sizeof(steel_symbol));

    s->diag = diag ? diag : steel_symtab_diag_default_;
    s->diag_user = diag_user;
    s->ok = true;

    st->impl = s;

    // Push global scope by default
    steel_symtab_push_scope(st);
}

void steel_symtab_dispose(steel_symtab* st)
{
    steel_symtab_impl* s = impl_(st);
    if (!s)
    {
        if (st) st->impl = NULL;
        return;
    }

    // Free scopes
    while (steel_vec_len(&s->scopes) > 0)
        steel_symtab_pop_scope(st);

    steel_vec_free(&s->scopes);
    steel_vec_free(&s->symbols);

    free(s);
    st->impl = NULL;
}

void steel_symtab_push_scope(steel_symtab* st)
{
    steel_symtab_impl* s = impl_(st);
    if (!s) return;

    steel_scope sc;
    memset(&sc, 0, sizeof(sc));
    scope_init_(&sc);
    (void)steel_vec_push(&s->scopes, &sc);
}

void steel_symtab_pop_scope(steel_symtab* st)
{
    steel_symtab_impl* s = impl_(st);
    if (!s) return;

    size_t n = steel_vec_len(&s->scopes);
    if (n == 0) return;

    steel_scope* sc = (steel_scope*)steel_vec_at(&s->scopes, n - 1);
    if (sc) scope_free_(sc);

    steel_vec_pop(&s->scopes);
}

uint32_t steel_symtab_define(steel_symtab* st,
                            steel_symbol_kind kind,
                            const char* name,
                            uint32_t decl_node,
                            steel_span at)
{
    steel_symtab_impl* s = impl_(st);
    if (!s || !name)
        return 0;

    steel_scope* sc = cur_scope_(s);
    if (!sc)
        return 0;

    uint32_t prev = 0;
    if (steel_map_str_u32_get(&sc->map, name, &prev))
    {
        emit_(s, STEEL_DIAG_ERROR, at, "duplicate definition in current scope");
        return prev;
    }

    steel_symbol* sym = sym_new_(s, kind, name, decl_node);
    if (!sym)
    {
        emit_(s, STEEL_DIAG_ERROR, at, "out of memory defining symbol");
        return 0;
    }

    if (!steel_map_str_u32_put(&sc->map, name, sym->id))
    {
        emit_(s, STEEL_DIAG_ERROR, at, "failed to insert into scope map");
        return 0;
    }

    return sym->id;
}

bool steel_symtab_lookup_current(const steel_symtab* st, const char* name, uint32_t* out_sym_id)
{
    steel_symtab_impl* s = impl_(st);
    if (!s || !name) return false;

    steel_scope* sc = cur_scope_(s);
    if (!sc) return false;

    return steel_map_str_u32_get(&sc->map, name, out_sym_id);
}

bool steel_symtab_lookup(const steel_symtab* st, const char* name, uint32_t* out_sym_id)
{
    steel_symtab_impl* s = impl_(st);
    if (!s || !name) return false;

    // Search from inner to outer
    for (size_t i = steel_vec_len(&s->scopes); i > 0; i--)
    {
        const steel_scope* sc = (const steel_scope*)steel_vec_cat(&s->scopes, i - 1);
        uint32_t v = 0;
        if (sc && steel_map_str_u32_get(&sc->map, name, &v))
        {
            if (out_sym_id) *out_sym_id = v;
            return true;
        }
    }

    return false;
}

const steel_symbol_info* steel_symtab_symbol_info(const steel_symtab* st, uint32_t sym_id)
{
    steel_symtab_impl* s = impl_(st);
    if (!s || sym_id == 0) return NULL;

    size_t idx = (size_t)(sym_id - 1);
    if (idx >= steel_vec_len(&s->symbols))
        return NULL;

    // Expose as a stable layout (steel_symbol is compatible with steel_symbol_info)
    return (const steel_symbol_info*)steel_vec_cat(&s->symbols, idx);
}
