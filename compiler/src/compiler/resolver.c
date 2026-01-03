// SPDX-License-Identifier: MIT
// resolver.c
//
// Name resolver (max) for Vitte AST.
//
// Responsibilities:
//  - Build symbol tables (global + nested lexical scopes).
//  - Resolve identifier references in expressions.
//  - Validate duplicates and undefined names.
//  - Provide a side-table mapping AST node ids -> resolved symbol id.
//
// Notes:
//  - This resolver is intentionally representation-light. It only assumes a small
//    AST query API from ast.h. If your ast.h uses different names, add wrappers
//    or adjust the `AST API` section below.
//  - It is compatible with the parser we generated (ASTK_* numeric kinds).
//

#include "resolver.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "ast.h"
#include "../common/vec.h"

//------------------------------------------------------------------------------
// AST kinds (must match parser.c)
//------------------------------------------------------------------------------

typedef enum steel_ast_kind_internal
{
    ASTK_FILE = 1,

    ASTK_MOD,
    ASTK_USE,
    ASTK_EXPORT,

    ASTK_TYPE_ALIAS,
    ASTK_STRUCT,
    ASTK_UNION,
    ASTK_ENUM,

    ASTK_FN,
    ASTK_SCN,
    ASTK_ENTRY,

    ASTK_PATH,
    ASTK_PARAMS,
    ASTK_PARAM,
    ASTK_FIELDS,
    ASTK_FIELD,
    ASTK_ENUM_BODY,
    ASTK_ENUM_ENTRY,

    ASTK_BLOCK,

    ASTK_STMT_LET,
    ASTK_STMT_CONST,
    ASTK_STMT_SET,
    ASTK_STMT_SAY,
    ASTK_STMT_DO,
    ASTK_STMT_IF,
    ASTK_STMT_ELIF,
    ASTK_STMT_ELSE,
    ASTK_STMT_WHILE,
    ASTK_STMT_FOR,
    ASTK_STMT_MATCH,
    ASTK_MATCH_ARM,
    ASTK_STMT_BREAK,
    ASTK_STMT_CONTINUE,
    ASTK_STMT_RETURN,
    ASTK_STMT_EXPR,

    ASTK_IDENT,
    ASTK_INT,
    ASTK_FLOAT,
    ASTK_STRING,
    ASTK_CHAR,

    ASTK_UNARY,
    ASTK_BINARY,
    ASTK_CALL,
    ASTK_MEMBER,
    ASTK_INDEX,

    ASTK_PAT_WILDCARD,
    ASTK_PAT_IDENT,
    ASTK_PAT_LITERAL,
    ASTK_PAT_PATH,
    ASTK_PAT_TUPLE,

    ASTK_ERROR,

} steel_ast_kind_internal;

//------------------------------------------------------------------------------
// AST API (adjust here if your ast.h differs)
//------------------------------------------------------------------------------

// Expected API surface (thin wrappers are acceptable):
//   steel_ast_kind steel_ast_kindof(const steel_ast* a, steel_ast_id id);
//   steel_span     steel_ast_spanof(const steel_ast* a, steel_ast_id id);
//   const char*    steel_ast_nameof(const steel_ast* a, steel_ast_id id);
//   const char*    steel_ast_textof(const steel_ast* a, steel_ast_id id);
//   size_t         steel_ast_kid_countof(const steel_ast* a, steel_ast_id id);
//   steel_ast_id   steel_ast_kid_atof(const steel_ast* a, steel_ast_id id, size_t i);

#ifndef steel_ast_kindof
  #define steel_ast_kindof(a,id)        steel_ast_kind((a),(id))
#endif
#ifndef steel_ast_spanof
  #define steel_ast_spanof(a,id)        steel_ast_span((a),(id))
#endif
#ifndef steel_ast_nameof
  #define steel_ast_nameof(a,id)        steel_ast_name((a),(id))
#endif
#ifndef steel_ast_textof
  #define steel_ast_textof(a,id)        steel_ast_text((a),(id))
#endif
#ifndef steel_ast_kid_countof
  #define steel_ast_kid_countof(a,id)   steel_ast_kid_count((a),(id))
#endif
#ifndef steel_ast_kid_atof
  #define steel_ast_kid_atof(a,id,i)    steel_ast_kid_at((a),(id),(i))
#endif

//------------------------------------------------------------------------------
// Diagnostics
//------------------------------------------------------------------------------

typedef void (*steel_resolve_diag_fn)(void* user, steel_diag_sev sev, steel_span span, const char* msg);

static void steel_resolve_diag_default_(void* user, steel_diag_sev sev, steel_span span, const char* msg)
{
    (void)user;
    const char* ssev = "info";
    if (sev == STEEL_DIAG_WARN) ssev = "warn";
    if (sev == STEEL_DIAG_ERROR) ssev = "error";

    fprintf(stderr, "[resolver:%s] file=%u span=[%u..%u] line=%u col=%u: %s\n",
            ssev, span.file_id, span.start, span.end, span.line, span.col, msg ? msg : "(null)");
}

//------------------------------------------------------------------------------
// Small hash maps (interned string keys + u32 keys)
//------------------------------------------------------------------------------

static inline uint64_t steel_hash_ptr_(const void* p)
{
    uintptr_t x = (uintptr_t)p;
    // Mix
    x ^= x >> 33;
    x *= (uintptr_t)0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= (uintptr_t)0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return (uint64_t)x;
}

static inline uint64_t steel_hash_u32_(uint32_t x)
{
    uint64_t z = (uint64_t)x + 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    z = z ^ (z >> 31);
    return z;
}

typedef struct steel_map_str_u32
{
    const char** keys; // interned pointers
    uint32_t*    vals;
    uint32_t     cap;
    uint32_t     len;
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
    const char** old_k = m->keys;
    uint32_t* old_v = m->vals;
    uint32_t old_cap = m->cap;

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

    if (old_k)
    {
        for (uint32_t i = 0; i < old_cap; i++)
        {
            const char* k = old_k[i];
            uint32_t v = old_v[i];
            if (!k) continue;

            uint64_t h = steel_hash_ptr_(k);
            uint32_t mask = new_cap - 1;
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

    free((void*)old_k);
    free(old_v);

    return true;
}

static bool steel_map_str_u32_put(steel_map_str_u32* m, const char* key, uint32_t val)
{
    if (!key) return false;

    if (m->cap == 0)
    {
        if (!steel_map_str_u32_grow_(m, 64)) return false;
    }

    // load factor 0.7
    if ((m->len + 1) * 10 >= m->cap * 7)
    {
        if (!steel_map_str_u32_grow_(m, m->cap * 2)) return false;
    }

    uint64_t h = steel_hash_ptr_(key);
    uint32_t mask = m->cap - 1;

    for (uint32_t p = 0; p < m->cap; p++)
    {
        uint32_t idx = (uint32_t)((h + p) & mask);
        if (!m->keys[idx])
        {
            m->keys[idx] = key;
            m->vals[idx] = val;
            m->len++;
            return true;
        }
        if (m->keys[idx] == key)
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

typedef struct steel_map_u32_u32
{
    uint32_t* keys;
    uint32_t* vals;
    uint32_t cap;
    uint32_t len;
} steel_map_u32_u32;

static void steel_map_u32_u32_init(steel_map_u32_u32* m) { memset(m, 0, sizeof(*m)); }
static void steel_map_u32_u32_free(steel_map_u32_u32* m) { free(m->keys); free(m->vals); memset(m, 0, sizeof(*m)); }

static bool steel_map_u32_u32_grow_(steel_map_u32_u32* m, uint32_t new_cap)
{
    uint32_t* ok = m->keys;
    uint32_t* ov = m->vals;
    uint32_t oc = m->cap;

    uint32_t* nk = (uint32_t*)calloc((size_t)new_cap, sizeof(uint32_t));
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
        for (uint32_t i = 0; i < oc; i++)
        {
            uint32_t k = ok[i];
            uint32_t v = ov[i];
            if (k == 0) continue;

            uint64_t h = steel_hash_u32_(k);
            uint32_t mask = new_cap - 1;
            for (uint32_t p = 0; p < new_cap; p++)
            {
                uint32_t idx = (uint32_t)((h + p) & mask);
                if (m->keys[idx] == 0)
                {
                    m->keys[idx] = k;
                    m->vals[idx] = v;
                    m->len++;
                    break;
                }
            }
        }
    }

    free(ok);
    free(ov);
    return true;
}

static bool steel_map_u32_u32_put(steel_map_u32_u32* m, uint32_t key, uint32_t val)
{
    if (key == 0) return false;

    if (m->cap == 0)
    {
        if (!steel_map_u32_u32_grow_(m, 256)) return false;
    }
    if ((m->len + 1) * 10 >= m->cap * 7)
    {
        if (!steel_map_u32_u32_grow_(m, m->cap * 2)) return false;
    }

    uint64_t h = steel_hash_u32_(key);
    uint32_t mask = m->cap - 1;

    for (uint32_t p = 0; p < m->cap; p++)
    {
        uint32_t idx = (uint32_t)((h + p) & mask);
        if (m->keys[idx] == 0)
        {
            m->keys[idx] = key;
            m->vals[idx] = val;
            m->len++;
            return true;
        }
        if (m->keys[idx] == key)
        {
            m->vals[idx] = val;
            return true;
        }
    }

    return false;
}

static bool steel_map_u32_u32_get(const steel_map_u32_u32* m, uint32_t key, uint32_t* out)
{
    if (!m || !m->cap || key == 0) return false;

    uint64_t h = steel_hash_u32_(key);
    uint32_t mask = m->cap - 1;

    for (uint32_t p = 0; p < m->cap; p++)
    {
        uint32_t idx = (uint32_t)((h + p) & mask);
        uint32_t k = m->keys[idx];
        if (k == 0) return false;
        if (k == key)
        {
            if (out) *out = m->vals[idx];
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
// Symbols and scopes
//------------------------------------------------------------------------------

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

typedef struct steel_symbol
{
    uint32_t id;
    steel_symbol_kind kind;
    const char* name;      // interned
    steel_ast_id decl_node;
} steel_symbol;

typedef struct steel_scope
{
    steel_map_str_u32 map; // name -> sym id
} steel_scope;

typedef struct steel_resolver
{
    steel_ast* ast;

    steel_vec scopes; // steel_scope

    steel_vec symbols; // steel_symbol

    // Node resolution side-table:
    //   node_id -> symbol id (identifier usages, member base identifiers, etc.)
    steel_map_u32_u32 node_to_sym;

    // diagnostics
    steel_resolve_diag_fn diag;
    void* diag_user;

    bool ok;

} steel_resolver;

static void steel_scope_init_(steel_scope* s)
{
    steel_map_str_u32_init(&s->map);
}

static void steel_scope_free_(steel_scope* s)
{
    steel_map_str_u32_free(&s->map);
    memset(s, 0, sizeof(*s));
}

static steel_scope* steel_scope_push_(steel_resolver* r)
{
    steel_scope s;
    steel_scope_init_(&s);
    return (steel_scope*)steel_vec_push(&r->scopes, &s);
}

static void steel_scope_pop_(steel_resolver* r)
{
    size_t n = steel_vec_len(&r->scopes);
    if (n == 0) return;
    steel_scope* s = (steel_scope*)steel_vec_at(&r->scopes, n - 1);
    if (s) steel_scope_free_(s);
    steel_vec_pop(&r->scopes);
}

static steel_symbol* steel_sym_new_(steel_resolver* r, steel_symbol_kind kind, const char* name, steel_ast_id decl)
{
    steel_symbol sym;
    memset(&sym, 0, sizeof(sym));
    sym.id = (uint32_t)steel_vec_len(&r->symbols) + 1;
    sym.kind = kind;
    sym.name = name;
    sym.decl_node = decl;
    return (steel_symbol*)steel_vec_push(&r->symbols, &sym);
}

static steel_symbol* steel_sym_by_id_(steel_resolver* r, uint32_t id)
{
    if (id == 0) return NULL;
    size_t idx = (size_t)(id - 1);
    if (idx >= steel_vec_len(&r->symbols)) return NULL;
    return (steel_symbol*)steel_vec_at(&r->symbols, idx);
}

static void steel_resolve_emit_(steel_resolver* r, steel_diag_sev sev, steel_span sp, const char* msg)
{
    r->ok = false;
    if (!r->diag) r->diag = steel_resolve_diag_default_;
    r->diag(r->diag_user, sev, sp, msg);
}

static bool steel_define_in_scope_(steel_resolver* r, steel_scope* s, const char* name, uint32_t sym_id, steel_span sp)
{
    if (!name) return false;

    uint32_t prev = 0;
    if (steel_map_str_u32_get(&s->map, name, &prev))
    {
        steel_resolve_emit_(r, STEEL_DIAG_ERROR, sp, "duplicate definition in same scope");
        return false;
    }

    return steel_map_str_u32_put(&s->map, name, sym_id);
}

static bool steel_lookup_(steel_resolver* r, const char* name, uint32_t* out)
{
    if (!name) return false;
    // Search innermost to outermost
    for (size_t i = steel_vec_len(&r->scopes); i > 0; i--)
    {
        const steel_scope* s = (const steel_scope*)steel_vec_cat(&r->scopes, i - 1);
        uint32_t v = 0;
        if (s && steel_map_str_u32_get(&s->map, name, &v))
        {
            if (out) *out = v;
            return true;
        }
    }
    return false;
}

static void steel_bind_node_(steel_resolver* r, steel_ast_id node, uint32_t sym_id)
{
    (void)steel_map_u32_u32_put(&r->node_to_sym, (uint32_t)node, sym_id);
}

//------------------------------------------------------------------------------
// Resolver walkers
//------------------------------------------------------------------------------

static void resolve_node_(steel_resolver* r, steel_ast_id id);
static void resolve_block_(steel_resolver* r, steel_ast_id id);
static void resolve_stmt_(steel_resolver* r, steel_ast_id id);
static void resolve_expr_(steel_resolver* r, steel_ast_id id);
static void resolve_pattern_(steel_resolver* r, steel_ast_id pat, bool bind_names);

static steel_ast_id kid_(steel_resolver* r, steel_ast_id id, size_t i)
{
    return steel_ast_kid_atof(r->ast, id, i);
}

static size_t kid_count_(steel_resolver* r, steel_ast_id id)
{
    return steel_ast_kid_countof(r->ast, id);
}

static void resolve_children_(steel_resolver* r, steel_ast_id id)
{
    size_t n = kid_count_(r, id);
    for (size_t i = 0; i < n; i++)
        resolve_node_(r, kid_(r, id, i));
}

static void collect_global_decls_(steel_resolver* r, steel_ast_id root)
{
    // Global scope is scopes[0]
    steel_scope* g = (steel_scope*)steel_vec_at(&r->scopes, 0);
    if (!g) return;

    size_t n = kid_count_(r, root);
    for (size_t i = 0; i < n; i++)
    {
        steel_ast_id d = kid_(r, root, i);
        steel_ast_kind k = steel_ast_kindof(r->ast, d);
        steel_span sp = steel_ast_spanof(r->ast, d);

        const char* nm = steel_ast_nameof(r->ast, d);

        // For some decls, parser sets name in node->name; for others it might be in first ident kid.
        if (!nm)
        {
            // try first ident child
            size_t cn = kid_count_(r, d);
            if (cn > 0)
            {
                steel_ast_id c0 = kid_(r, d, 0);
                if (steel_ast_kindof(r->ast, c0) == (steel_ast_kind)ASTK_IDENT)
                    nm = steel_ast_textof(r->ast, c0);
            }
        }

        steel_symbol_kind sk = STEEL_SYM_GLOBAL;
        bool is_decl = false;

        if (k == (steel_ast_kind)ASTK_TYPE_ALIAS || k == (steel_ast_kind)ASTK_STRUCT || k == (steel_ast_kind)ASTK_UNION || k == (steel_ast_kind)ASTK_ENUM)
        {
            sk = STEEL_SYM_TYPE;
            is_decl = true;
        }
        else if (k == (steel_ast_kind)ASTK_FN || k == (steel_ast_kind)ASTK_SCN || k == (steel_ast_kind)ASTK_ENTRY)
        {
            sk = STEEL_SYM_FN;
            is_decl = true;
        }
        else if (k == (steel_ast_kind)ASTK_MOD)
        {
            sk = STEEL_SYM_MODULE;
            is_decl = true;
        }
        else if (k == (steel_ast_kind)ASTK_USE)
        {
            // optional alias is last ident kid
            sk = STEEL_SYM_USE_ALIAS;
            // do not require
            is_decl = false;

            // if use ... as Alias
            size_t cn = kid_count_(r, d);
            if (cn > 0)
            {
                steel_ast_id last = kid_(r, d, cn - 1);
                if (steel_ast_kindof(r->ast, last) == (steel_ast_kind)ASTK_IDENT)
                {
                    const char* anm = steel_ast_textof(r->ast, last);
                    if (anm)
                    {
                        steel_symbol* s = steel_sym_new_(r, STEEL_SYM_USE_ALIAS, anm, d);
                        (void)steel_define_in_scope_(r, g, anm, s->id, sp);
                    }
                }
            }
            continue;
        }
        else
        {
            continue;
        }

        if (is_decl)
        {
            if (!nm)
            {
                steel_resolve_emit_(r, STEEL_DIAG_ERROR, sp, "declaration has no name");
                continue;
            }

            steel_symbol* s = steel_sym_new_(r, sk, nm, d);
            (void)steel_define_in_scope_(r, g, nm, s->id, sp);
        }
    }
}

static void resolve_fn_like_(steel_resolver* r, steel_ast_id fn)
{
    // Enter function scope.
    steel_scope_push_(r);

    // params are usually first kid
    size_t n = kid_count_(r, fn);
    for (size_t i = 0; i < n; i++)
    {
        steel_ast_id c = kid_(r, fn, i);
        if (steel_ast_kindof(r->ast, c) == (steel_ast_kind)ASTK_PARAMS)
        {
            size_t pn = kid_count_(r, c);
            for (size_t j = 0; j < pn; j++)
            {
                steel_ast_id param = kid_(r, c, j);
                steel_span sp = steel_ast_spanof(r->ast, param);
                const char* pnm = steel_ast_nameof(r->ast, param);
                if (!pnm)
                {
                    // try ident child
                    size_t cn = kid_count_(r, param);
                    if (cn > 0)
                    {
                        steel_ast_id id0 = kid_(r, param, 0);
                        if (steel_ast_kindof(r->ast, id0) == (steel_ast_kind)ASTK_IDENT)
                            pnm = steel_ast_textof(r->ast, id0);
                    }
                }

                if (pnm)
                {
                    steel_symbol* s = steel_sym_new_(r, STEEL_SYM_PARAM, pnm, param);
                    steel_scope* cur = (steel_scope*)steel_vec_at(&r->scopes, steel_vec_len(&r->scopes) - 1);
                    (void)steel_define_in_scope_(r, cur, pnm, s->id, sp);
                }

                // resolve possible type annotation inside param
                resolve_children_(r, param);
            }
        }
    }

    // Body is last kid that is a block.
    for (size_t i = 0; i < n; i++)
    {
        steel_ast_id c = kid_(r, fn, i);
        if (steel_ast_kindof(r->ast, c) == (steel_ast_kind)ASTK_BLOCK)
        {
            resolve_block_(r, c);
        }
        else
        {
            // return type path, etc.
            resolve_node_(r, c);
        }
    }

    steel_scope_pop_(r);
}

static void resolve_block_(steel_resolver* r, steel_ast_id id)
{
    // Enter block scope.
    steel_scope_push_(r);

    size_t n = kid_count_(r, id);
    for (size_t i = 0; i < n; i++)
        resolve_stmt_(r, kid_(r, id, i));

    steel_scope_pop_(r);
}

static void resolve_stmt_(steel_resolver* r, steel_ast_id id)
{
    steel_ast_kind k = steel_ast_kindof(r->ast, id);

    switch ((int)k)
    {
        case ASTK_STMT_LET:
        case ASTK_STMT_CONST:
        {
            // children: ident, (optional) type, (optional) init
            size_t n = kid_count_(r, id);
            if (n > 0)
            {
                steel_ast_id nm_node = kid_(r, id, 0);
                const char* nm = NULL;
                if (steel_ast_kindof(r->ast, nm_node) == (steel_ast_kind)ASTK_IDENT)
                    nm = steel_ast_textof(r->ast, nm_node);

                if (nm)
                {
                    steel_scope* cur = (steel_scope*)steel_vec_at(&r->scopes, steel_vec_len(&r->scopes) - 1);
                    steel_span sp = steel_ast_spanof(r->ast, nm_node);
                    steel_symbol* s = steel_sym_new_(r, STEEL_SYM_LOCAL, nm, id);
                    (void)steel_define_in_scope_(r, cur, nm, s->id, sp);
                    // bind the ident node itself (definition)
                    steel_bind_node_(r, nm_node, s->id);
                }
                else
                {
                    steel_resolve_emit_(r, STEEL_DIAG_ERROR, steel_ast_spanof(r->ast, id), "let/const without identifier");
                }

                // resolve type/init
                for (size_t i = 1; i < n; i++)
                    resolve_node_(r, kid_(r, id, i));
            }
            return;
        }

        case ASTK_STMT_SET:
        case ASTK_STMT_SAY:
        case ASTK_STMT_DO:
        case ASTK_STMT_EXPR:
        {
            resolve_children_(r, id);
            return;
        }

        case ASTK_STMT_RETURN:
        case ASTK_STMT_BREAK:
        case ASTK_STMT_CONTINUE:
        {
            resolve_children_(r, id);
            return;
        }

        case ASTK_STMT_IF:
        {
            // kids: cond, then block, (elif...)*, (else ...)?
            size_t n = kid_count_(r, id);
            for (size_t i = 0; i < n; i++)
            {
                steel_ast_id c = kid_(r, id, i);
                steel_ast_kind ck = steel_ast_kindof(r->ast, c);
                if ((int)ck == ASTK_BLOCK)
                    resolve_block_(r, c);
                else
                    resolve_node_(r, c);
            }
            return;
        }

        case ASTK_STMT_ELIF:
        case ASTK_STMT_ELSE:
        {
            // cond + block, or only block
            size_t n = kid_count_(r, id);
            for (size_t i = 0; i < n; i++)
            {
                steel_ast_id c = kid_(r, id, i);
                if ((int)steel_ast_kindof(r->ast, c) == ASTK_BLOCK)
                    resolve_block_(r, c);
                else
                    resolve_node_(r, c);
            }
            return;
        }

        case ASTK_STMT_WHILE:
        {
            // cond, body block
            size_t n = kid_count_(r, id);
            if (n >= 1) resolve_node_(r, kid_(r, id, 0));
            if (n >= 2)
            {
                steel_ast_id body = kid_(r, id, 1);
                if ((int)steel_ast_kindof(r->ast, body) == ASTK_BLOCK)
                    resolve_block_(r, body);
                else
                    resolve_node_(r, body);
            }
            return;
        }

        case ASTK_STMT_FOR:
        {
            // minimal: ident (optional), iterable expr, body block
            steel_scope_push_(r);

            size_t n = kid_count_(r, id);
            size_t idx = 0;

            if (n > 0 && (int)steel_ast_kindof(r->ast, kid_(r, id, 0)) == ASTK_IDENT)
            {
                steel_ast_id it = kid_(r, id, 0);
                const char* nm = steel_ast_textof(r->ast, it);
                if (nm)
                {
                    steel_scope* cur = (steel_scope*)steel_vec_at(&r->scopes, steel_vec_len(&r->scopes) - 1);
                    steel_span sp = steel_ast_spanof(r->ast, it);
                    steel_symbol* s = steel_sym_new_(r, STEEL_SYM_LOCAL, nm, it);
                    (void)steel_define_in_scope_(r, cur, nm, s->id, sp);
                    steel_bind_node_(r, it, s->id);
                }
                idx = 1;
            }

            if (idx < n) resolve_node_(r, kid_(r, id, idx++));
            if (idx < n)
            {
                steel_ast_id body = kid_(r, id, idx);
                if ((int)steel_ast_kindof(r->ast, body) == ASTK_BLOCK)
                    resolve_block_(r, body);
                else
                    resolve_node_(r, body);
            }

            steel_scope_pop_(r);
            return;
        }

        case ASTK_STMT_MATCH:
        {
            // kids: scrutinee, arms...
            size_t n = kid_count_(r, id);
            if (n > 0) resolve_node_(r, kid_(r, id, 0));

            for (size_t i = 1; i < n; i++)
            {
                steel_ast_id arm = kid_(r, id, i);
                if ((int)steel_ast_kindof(r->ast, arm) != ASTK_MATCH_ARM)
                {
                    resolve_node_(r, arm);
                    continue;
                }

                // per-arm scope
                steel_scope_push_(r);

                size_t an = kid_count_(r, arm);
                if (an > 0)
                {
                    steel_ast_id pat = kid_(r, arm, 0);
                    resolve_pattern_(r, pat, true);
                }
                if (an > 1)
                {
                    steel_ast_id body = kid_(r, arm, 1);
                    if ((int)steel_ast_kindof(r->ast, body) == ASTK_BLOCK)
                        resolve_block_(r, body);
                    else
                        resolve_node_(r, body);
                }

                steel_scope_pop_(r);
            }
            return;
        }

        default:
            resolve_children_(r, id);
            return;
    }
}

static void resolve_expr_(steel_resolver* r, steel_ast_id id)
{
    steel_ast_kind k = steel_ast_kindof(r->ast, id);

    switch ((int)k)
    {
        case ASTK_IDENT:
        {
            const char* nm = steel_ast_textof(r->ast, id);
            uint32_t sym_id = 0;
            if (!steel_lookup_(r, nm, &sym_id))
            {
                steel_resolve_emit_(r, STEEL_DIAG_ERROR, steel_ast_spanof(r->ast, id), "undefined identifier");
                return;
            }
            steel_bind_node_(r, id, sym_id);
            return;
        }

        case ASTK_CALL:
        case ASTK_MEMBER:
        case ASTK_INDEX:
        case ASTK_UNARY:
        case ASTK_BINARY:
            resolve_children_(r, id);
            return;

        case ASTK_INT:
        case ASTK_FLOAT:
        case ASTK_STRING:
        case ASTK_CHAR:
            return;

        default:
            resolve_children_(r, id);
            return;
    }
}

static void resolve_pattern_(steel_resolver* r, steel_ast_id pat, bool bind_names)
{
    steel_ast_kind k = steel_ast_kindof(r->ast, pat);

    switch ((int)k)
    {
        case ASTK_PAT_WILDCARD:
        case ASTK_PAT_LITERAL:
            return;

        case ASTK_PAT_TUPLE:
        {
            size_t n = kid_count_(r, pat);
            for (size_t i = 0; i < n; i++)
                resolve_pattern_(r, kid_(r, pat, i), bind_names);
            return;
        }

        case ASTK_PAT_PATH:
        {
            // kid0 is a path node with ident segments
            if (!bind_names)
            {
                resolve_children_(r, pat);
                return;
            }

            steel_ast_id path = (kid_count_(r, pat) > 0) ? kid_(r, pat, 0) : 0;
            if (!path) return;

            // Determine if it's a binding or a constructor path.
            // Heuristic:
            //  - single segment: binds if not found in any outer scope as type/fn/module
            //  - multi segment: treat as constructor/reference; resolve as expression-ish.
            size_t segs = kid_count_(r, path);

            if (segs == 1)
            {
                steel_ast_id seg = kid_(r, path, 0);
                const char* nm = (seg && (int)steel_ast_kindof(r->ast, seg) == ASTK_IDENT) ? steel_ast_textof(r->ast, seg) : NULL;

                if (!nm) return;

                uint32_t existing = 0;
                if (steel_lookup_(r, nm, &existing))
                {
                    // name exists: treat as reference (not a new binding)
                    steel_bind_node_(r, seg, existing);
                    return;
                }

                // Bind a new local in current scope
                steel_scope* cur = (steel_scope*)steel_vec_at(&r->scopes, steel_vec_len(&r->scopes) - 1);
                steel_span sp = steel_ast_spanof(r->ast, seg);
                steel_symbol* s = steel_sym_new_(r, STEEL_SYM_LOCAL, nm, pat);
                (void)steel_define_in_scope_(r, cur, nm, s->id, sp);
                steel_bind_node_(r, seg, s->id);
                return;
            }

            // multi segment: resolve segments as references
            resolve_children_(r, path);
            return;
        }

        default:
            resolve_children_(r, pat);
            return;
    }
}

static void resolve_node_(steel_resolver* r, steel_ast_id id)
{
    if (!id) return;

    steel_ast_kind k = steel_ast_kindof(r->ast, id);

    switch ((int)k)
    {
        case ASTK_FILE:
            resolve_children_(r, id);
            return;

        case ASTK_FN:
        case ASTK_SCN:
        case ASTK_ENTRY:
            resolve_fn_like_(r, id);
            return;

        case ASTK_BLOCK:
            resolve_block_(r, id);
            return;

        // statements
        case ASTK_STMT_LET:
        case ASTK_STMT_CONST:
        case ASTK_STMT_SET:
        case ASTK_STMT_SAY:
        case ASTK_STMT_DO:
        case ASTK_STMT_IF:
        case ASTK_STMT_ELIF:
        case ASTK_STMT_ELSE:
        case ASTK_STMT_WHILE:
        case ASTK_STMT_FOR:
        case ASTK_STMT_MATCH:
        case ASTK_MATCH_ARM:
        case ASTK_STMT_BREAK:
        case ASTK_STMT_CONTINUE:
        case ASTK_STMT_RETURN:
        case ASTK_STMT_EXPR:
            resolve_stmt_(r, id);
            return;

        // expressions
        case ASTK_IDENT:
        case ASTK_INT:
        case ASTK_FLOAT:
        case ASTK_STRING:
        case ASTK_CHAR:
        case ASTK_UNARY:
        case ASTK_BINARY:
        case ASTK_CALL:
        case ASTK_MEMBER:
        case ASTK_INDEX:
            resolve_expr_(r, id);
            return;

        // patterns
        case ASTK_PAT_WILDCARD:
        case ASTK_PAT_LITERAL:
        case ASTK_PAT_TUPLE:
        case ASTK_PAT_PATH:
            resolve_pattern_(r, id, false);
            return;

        default:
            resolve_children_(r, id);
            return;
    }
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void steel_resolver_init(steel_resolver_ctx* ctx)
{
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->diag = NULL;
    ctx->diag_user = NULL;
}

bool steel_resolve_ast(steel_ast* ast, steel_ast_id root, const steel_resolver_ctx* ctx_in, steel_resolver_result* out)
{
    if (!ast || !root || !out)
        return false;

    steel_resolver r;
    memset(&r, 0, sizeof(r));

    r.ast = ast;
    r.ok = true;

    steel_vec_init(&r.scopes, sizeof(steel_scope));
    steel_vec_init(&r.symbols, sizeof(steel_symbol));
    steel_map_u32_u32_init(&r.node_to_sym);

    r.diag = ctx_in && ctx_in->diag ? (steel_resolve_diag_fn)ctx_in->diag : steel_resolve_diag_default_;
    r.diag_user = ctx_in ? ctx_in->diag_user : NULL;

    // push global scope
    steel_scope_push_(&r);

    // First: collect global decls for forward refs
    collect_global_decls_(&r, root);

    // Second: resolve all
    resolve_node_(&r, root);

    // Emit outputs
    out->ok = r.ok;

    // Transfer symbol vec and node map by shallow move.
    // The caller must free via steel_resolver_result_dispose.
    out->symbols = r.symbols;
    out->node_to_sym = r.node_to_sym;

    // Dispose remaining scopes
    while (steel_vec_len(&r.scopes) > 0)
        steel_scope_pop_(&r);
    steel_vec_free(&r.scopes);

    return out->ok;
}

void steel_resolver_result_dispose(steel_resolver_result* r)
{
    if (!r) return;

    // free symbol storage
    steel_vec_free(&r->symbols);

    // free map
    steel_map_u32_u32_free(&r->node_to_sym);

    memset(r, 0, sizeof(*r));
}

bool steel_resolver_lookup_node(const steel_resolver_result* r, steel_ast_id node, uint32_t* out_sym_id)
{
    if (!r) return false;
    return steel_map_u32_u32_get(&r->node_to_sym, (uint32_t)node, out_sym_id);
}

const steel_symbol_info* steel_resolver_symbol_info(const steel_resolver_result* r, uint32_t sym_id)
{
    if (!r || sym_id == 0) return NULL;
    size_t idx = (size_t)(sym_id - 1);
    if (idx >= steel_vec_len(&r->symbols)) return NULL;

    // Expose a stable view by aliasing the internal struct layout.
    return (const steel_symbol_info*)steel_vec_cat(&r->symbols, idx);
}
