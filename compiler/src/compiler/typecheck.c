// SPDX-License-Identifier: MIT
// typecheck.c
//
// Type checking (max) for Vitte (core + phrase) at the AST level.
//
// Goals:
//  - Infer and validate types for expressions and statements.
//  - Validate function bodies against declared return type.
//  - Validate let/const initializers against optional type annotations.
//  - Provide a side table: node_id -> type_id.
//
// Design notes:
//  - This module is representation-light and intentionally tolerant.
//  - It integrates optionally with the resolver (if resolver.h is present).
//  - It assumes a small AST query API from ast.h; adjust the AST API macros below if needed.
//

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lexer.h"          // steel_span
#include "ast.h"            // AST store
#include "../common/vec.h"  // steel_vec

// Optional resolver integration
#if defined(__has_include)
  #if __has_include("resolver.h")
    #include "resolver.h"
    #define STEEL_HAS_RESOLVER 1
  #endif
#endif

#ifndef STEEL_HAS_RESOLVER
  typedef struct steel_resolver_result steel_resolver_result;
  typedef struct steel_symbol_info steel_symbol_info;
  static inline bool steel_resolver_lookup_node(const steel_resolver_result* r, uint32_t node, uint32_t* out_sym_id)
  {
      (void)r; (void)node; (void)out_sym_id;
      return false;
  }
  static inline const steel_symbol_info* steel_resolver_symbol_info(const steel_resolver_result* r, uint32_t sym_id)
  {
      (void)r; (void)sym_id;
      return NULL;
  }
#endif

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

// Expected API surface:
//   steel_ast_kind steel_ast_kind(const steel_ast* a, steel_ast_id id);
//   steel_span     steel_ast_span(const steel_ast* a, steel_ast_id id);
//   const char*    steel_ast_name(const steel_ast* a, steel_ast_id id);
//   const char*    steel_ast_text(const steel_ast* a, steel_ast_id id);
//   size_t         steel_ast_kid_count(const steel_ast* a, steel_ast_id id);
//   steel_ast_id   steel_ast_kid_at(const steel_ast* a, steel_ast_id id, size_t i);

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

typedef enum steel_diag_sev
{
    STEEL_DIAG_INFO = 0,
    STEEL_DIAG_WARN,
    STEEL_DIAG_ERROR,
} steel_diag_sev;

typedef void (*steel_type_diag_fn)(void* user, steel_diag_sev sev, steel_span span, const char* msg);

static void steel_type_diag_default_(void* user, steel_diag_sev sev, steel_span span, const char* msg)
{
    (void)user;
    const char* ssev = "info";
    if (sev == STEEL_DIAG_WARN) ssev = "warn";
    if (sev == STEEL_DIAG_ERROR) ssev = "error";

    fprintf(stderr, "[typecheck:%s] file=%u span=[%u..%u] line=%u col=%u: %s\n",
            ssev, span.file_id, span.start, span.end, span.line, span.col, msg ? msg : "(null)");
}

//------------------------------------------------------------------------------
// Small hash map u32 -> u32
//------------------------------------------------------------------------------

static inline uint64_t steel_hash_u32_(uint32_t x)
{
    uint64_t z = (uint64_t)x + 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    z = z ^ (z >> 31);
    return z;
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
        uint32_t mask = new_cap - 1;
        for (uint32_t i = 0; i < oc; i++)
        {
            uint32_t k = ok[i];
            uint32_t v = ov[i];
            if (k == 0) continue;

            uint64_t h = steel_hash_u32_(k);
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
// Type system
//------------------------------------------------------------------------------

typedef enum steel_type_kind
{
    TY_ERROR = 1,
    TY_UNKNOWN,

    TY_UNIT,
    TY_NULL,

    TY_BOOL,
    TY_INT,
    TY_FLOAT,
    TY_CHAR,
    TY_STRING,

    TY_NOMINAL,   // user type: name
    TY_FN,        // fn(params...) -> ret

} steel_type_kind;

typedef struct steel_type
{
    uint32_t id;
    steel_type_kind kind;

    // For nominal types
    const char* name; // interned pointer

    // For fn types
    uint32_t ret;
    uint32_t params_ofs;
    uint32_t params_len;

} steel_type;

typedef struct steel_typecheck
{
    steel_ast* ast;
    const steel_resolver_result* rr;

    steel_vec types;       // steel_type
    steel_vec fn_params;   // uint32_t type ids (flat array)

    steel_map_u32_u32 node_to_type; // ast_id -> type_id

    // current function expected return type
    uint32_t cur_ret_ty;
    bool cur_ret_ty_set;

    // diagnostics
    steel_type_diag_fn diag;
    void* diag_user;

    bool ok;

} steel_typecheck;

// Builtin type ids
static uint32_t g_ty_error = 0;
static uint32_t g_ty_unknown = 0;
static uint32_t g_ty_unit = 0;
static uint32_t g_ty_null = 0;
static uint32_t g_ty_bool = 0;
static uint32_t g_ty_int = 0;
static uint32_t g_ty_float = 0;
static uint32_t g_ty_char = 0;
static uint32_t g_ty_string = 0;

static steel_type* ty_by_id_(steel_typecheck* tc, uint32_t id)
{
    if (id == 0) return NULL;
    size_t idx = (size_t)(id - 1);
    if (idx >= steel_vec_len(&tc->types)) return NULL;
    return (steel_type*)steel_vec_at(&tc->types, idx);
}

static uint32_t ty_new_(steel_typecheck* tc, steel_type t)
{
    t.id = (uint32_t)steel_vec_len(&tc->types) + 1;
    steel_vec_push(&tc->types, &t);
    return t.id;
}

static void emit_(steel_typecheck* tc, steel_diag_sev sev, steel_span sp, const char* msg)
{
    tc->ok = false;
    if (!tc->diag) tc->diag = steel_type_diag_default_;
    tc->diag(tc->diag_user, sev, sp, msg);
}

static void bind_type_(steel_typecheck* tc, uint32_t node, uint32_t ty)
{
    (void)steel_map_u32_u32_put(&tc->node_to_type, node, ty);
}

static uint32_t get_bound_type_(steel_typecheck* tc, uint32_t node)
{
    uint32_t v = 0;
    if (steel_map_u32_u32_get(&tc->node_to_type, node, &v))
        return v;
    return 0;
}

static bool is_same_ty_(steel_typecheck* tc, uint32_t a, uint32_t b)
{
    if (a == b) return true;
    if (a == g_ty_unknown || b == g_ty_unknown) return true;
    if (a == g_ty_error || b == g_ty_error) return true;

    steel_type* ta = ty_by_id_(tc, a);
    steel_type* tb = ty_by_id_(tc, b);
    if (!ta || !tb) return false;

    if (ta->kind != tb->kind) return false;

    if (ta->kind == TY_NOMINAL)
        return ta->name == tb->name;

    if (ta->kind == TY_FN)
    {
        if (!is_same_ty_(tc, ta->ret, tb->ret)) return false;
        if (ta->params_len != tb->params_len) return false;
        for (uint32_t i = 0; i < ta->params_len; i++)
        {
            uint32_t pa = ((uint32_t*)steel_vec_at(&tc->fn_params, ta->params_ofs + i))[0];
            uint32_t pb = ((uint32_t*)steel_vec_at(&tc->fn_params, tb->params_ofs + i))[0];
            if (!is_same_ty_(tc, pa, pb)) return false;
        }
        return true;
    }

    return true;
}

static bool is_numeric_(uint32_t ty)
{
    return ty == g_ty_int || ty == g_ty_float;
}

static uint32_t numeric_promote_(uint32_t a, uint32_t b)
{
    if (a == g_ty_float || b == g_ty_float) return g_ty_float;
    return g_ty_int;
}

static uint32_t intern_nominal_(steel_typecheck* tc, const char* name)
{
    if (!name) return g_ty_unknown;

    // Linear scan is fine for now.
    size_t n = steel_vec_len(&tc->types);
    for (size_t i = 0; i < n; i++)
    {
        steel_type* t = (steel_type*)steel_vec_at(&tc->types, i);
        if (t && t->kind == TY_NOMINAL && t->name == name)
            return t->id;
    }

    steel_type t;
    memset(&t, 0, sizeof(t));
    t.kind = TY_NOMINAL;
    t.name = name;
    return ty_new_(tc, t);
}

static uint32_t parse_type_from_path_(steel_typecheck* tc, steel_ast_id path)
{
    if (!path) return g_ty_unknown;
    if ((int)steel_ast_kindof(tc->ast, path) != ASTK_PATH)
        return g_ty_unknown;

    // Type name is last segment ident
    size_t n = steel_ast_kid_countof(tc->ast, path);
    if (n == 0) return g_ty_unknown;

    steel_ast_id last = steel_ast_kid_atof(tc->ast, path, n - 1);
    if (!last) return g_ty_unknown;

    const char* nm = NULL;
    if ((int)steel_ast_kindof(tc->ast, last) == ASTK_IDENT)
        nm = steel_ast_textof(tc->ast, last);

    if (!nm) return g_ty_unknown;

    // Builtins by name (interned pointer is ideal; fallback to strcmp for safety).
    if (strcmp(nm, "int") == 0 || strcmp(nm, "i32") == 0 || strcmp(nm, "i64") == 0) return g_ty_int;
    if (strcmp(nm, "float") == 0 || strcmp(nm, "f32") == 0 || strcmp(nm, "f64") == 0) return g_ty_float;
    if (strcmp(nm, "bool") == 0) return g_ty_bool;
    if (strcmp(nm, "char") == 0) return g_ty_char;
    if (strcmp(nm, "string") == 0 || strcmp(nm, "str") == 0) return g_ty_string;
    if (strcmp(nm, "null") == 0) return g_ty_null;
    if (strcmp(nm, "unit") == 0 || strcmp(nm, "void") == 0) return g_ty_unit;

    return intern_nominal_(tc, nm);
}

//------------------------------------------------------------------------------
// AST traversal helpers
//------------------------------------------------------------------------------

static steel_ast_id kid_(steel_typecheck* tc, steel_ast_id id, size_t i)
{
    return steel_ast_kid_atof(tc->ast, id, i);
}

static size_t kid_count_(steel_typecheck* tc, steel_ast_id id)
{
    return steel_ast_kid_countof(tc->ast, id);
}

//------------------------------------------------------------------------------
// Typecheck core
//------------------------------------------------------------------------------

static uint32_t typecheck_expr_(steel_typecheck* tc, steel_ast_id e);
static void typecheck_stmt_(steel_typecheck* tc, steel_ast_id s);
static void typecheck_block_(steel_typecheck* tc, steel_ast_id b);
static void typecheck_fn_like_(steel_typecheck* tc, steel_ast_id fn);

static bool is_bool_ident_(const char* s, bool* out_val)
{
    if (!s) return false;
    if (strcmp(s, "true") == 0) { if (out_val) *out_val = true; return true; }
    if (strcmp(s, "false") == 0) { if (out_val) *out_val = false; return true; }
    return false;
}

static bool is_null_ident_(const char* s)
{
    return s && strcmp(s, "null") == 0;
}

static uint32_t type_of_ident_(steel_typecheck* tc, steel_ast_id id)
{
    const char* nm = steel_ast_textof(tc->ast, id);
    bool bv = false;
    if (is_bool_ident_(nm, &bv))
        return g_ty_bool;
    if (is_null_ident_(nm))
        return g_ty_null;

#if STEEL_HAS_RESOLVER
    if (tc->rr)
    {
        uint32_t sym_id = 0;
        if (steel_resolver_lookup_node(tc->rr, id, &sym_id))
        {
            const steel_symbol_info* si = steel_resolver_symbol_info(tc->rr, sym_id);
            (void)si;
            // If symbol info provides kind-specific typing, plug it here.
            // For now, treat unresolved symbol types as unknown.
            return g_ty_unknown;
        }
    }
#endif

    return g_ty_unknown;
}

static uint32_t typecheck_binary_(steel_typecheck* tc, steel_ast_id node)
{
    // node.text = operator string (see parser.c)
    const char* op = steel_ast_textof(tc->ast, node);

    size_t n = kid_count_(tc, node);
    if (n < 2)
        return g_ty_error;

    uint32_t lt = typecheck_expr_(tc, kid_(tc, node, 0));
    uint32_t rt = typecheck_expr_(tc, kid_(tc, node, 1));

    if (!op) return g_ty_unknown;

    // logical
    if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0)
    {
        if (lt != g_ty_bool && lt != g_ty_unknown)
            emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, node), "lhs of logical op must be bool");
        if (rt != g_ty_bool && rt != g_ty_unknown)
            emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, node), "rhs of logical op must be bool");
        return g_ty_bool;
    }

    // equality
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0)
    {
        // allow, but warn on obvious mismatch
        if (!is_same_ty_(tc, lt, rt) && lt != g_ty_unknown && rt != g_ty_unknown)
            emit_(tc, STEEL_DIAG_WARN, steel_ast_spanof(tc->ast, node), "equality compare between different types");
        return g_ty_bool;
    }

    // comparison
    if (strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 || strcmp(op, ">") == 0 || strcmp(op, ">=") == 0)
    {
        if (!is_numeric_(lt) && lt != g_ty_unknown)
            emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, node), "lhs of comparison must be numeric");
        if (!is_numeric_(rt) && rt != g_ty_unknown)
            emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, node), "rhs of comparison must be numeric");
        return g_ty_bool;
    }

    // arithmetic
    if (strcmp(op, "+") == 0)
    {
        if (lt == g_ty_string || rt == g_ty_string)
        {
            if (lt != g_ty_string && lt != g_ty_unknown)
                emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, node), "string concatenation requires string lhs");
            if (rt != g_ty_string && rt != g_ty_unknown)
                emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, node), "string concatenation requires string rhs");
            return g_ty_string;
        }

        if (!is_numeric_(lt) && lt != g_ty_unknown)
            emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, node), "lhs of + must be numeric");
        if (!is_numeric_(rt) && rt != g_ty_unknown)
            emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, node), "rhs of + must be numeric");

        return numeric_promote_(lt, rt);
    }

    if (strcmp(op, "-") == 0 || strcmp(op, "*") == 0 || strcmp(op, "/") == 0 || strcmp(op, "%") == 0)
    {
        if (!is_numeric_(lt) && lt != g_ty_unknown)
            emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, node), "lhs of arithmetic op must be numeric");
        if (!is_numeric_(rt) && rt != g_ty_unknown)
            emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, node), "rhs of arithmetic op must be numeric");
        return numeric_promote_(lt, rt);
    }

    return g_ty_unknown;
}

static uint32_t typecheck_unary_(steel_typecheck* tc, steel_ast_id node)
{
    const char* op = steel_ast_textof(tc->ast, node);

    size_t n = kid_count_(tc, node);
    if (n < 1)
        return g_ty_error;

    uint32_t xt = typecheck_expr_(tc, kid_(tc, node, 0));

    if (!op) return g_ty_unknown;

    if (strcmp(op, "!") == 0)
    {
        if (xt != g_ty_bool && xt != g_ty_unknown)
            emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, node), "operand of ! must be bool");
        return g_ty_bool;
    }

    if (strcmp(op, "-") == 0)
    {
        if (!is_numeric_(xt) && xt != g_ty_unknown)
            emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, node), "operand of unary - must be numeric");
        return xt;
    }

    return g_ty_unknown;
}

static uint32_t typecheck_call_(steel_typecheck* tc, steel_ast_id node)
{
    // call children: callee, arg1, arg2, ...
    size_t n = kid_count_(tc, node);
    if (n == 0) return g_ty_error;

    uint32_t callee_ty = typecheck_expr_(tc, kid_(tc, node, 0));

    // Typecheck args (even if we can't check signature yet)
    for (size_t i = 1; i < n; i++)
        (void)typecheck_expr_(tc, kid_(tc, node, i));

    steel_type* ct = ty_by_id_(tc, callee_ty);
    if (ct && ct->kind == TY_FN)
        return ct->ret;

    // Unknown callee => unknown result
    return g_ty_unknown;
}

static uint32_t typecheck_expr_(steel_typecheck* tc, steel_ast_id e)
{
    if (!e) return g_ty_error;

    uint32_t cached = get_bound_type_(tc, (uint32_t)e);
    if (cached) return cached;

    steel_ast_kind k = steel_ast_kindof(tc->ast, e);
    uint32_t ty = g_ty_unknown;

    switch ((int)k)
    {
        case ASTK_INT: ty = g_ty_int; break;
        case ASTK_FLOAT: ty = g_ty_float; break;
        case ASTK_STRING: ty = g_ty_string; break;
        case ASTK_CHAR: ty = g_ty_char; break;

        case ASTK_IDENT:
            ty = type_of_ident_(tc, e);
            break;

        case ASTK_UNARY:
            ty = typecheck_unary_(tc, e);
            break;

        case ASTK_BINARY:
            ty = typecheck_binary_(tc, e);
            break;

        case ASTK_CALL:
            ty = typecheck_call_(tc, e);
            break;

        case ASTK_MEMBER:
        {
            // children: base, member ident
            size_t n = kid_count_(tc, e);
            if (n > 0)
                (void)typecheck_expr_(tc, kid_(tc, e, 0));
            ty = g_ty_unknown;
            break;
        }

        case ASTK_INDEX:
        {
            // children: base, index
            size_t n = kid_count_(tc, e);
            if (n > 0) (void)typecheck_expr_(tc, kid_(tc, e, 0));
            if (n > 1) (void)typecheck_expr_(tc, kid_(tc, e, 1));
            ty = g_ty_unknown;
            break;
        }

        default:
        {
            // best effort: typecheck children to accumulate errors
            size_t n = kid_count_(tc, e);
            for (size_t i = 0; i < n; i++)
                (void)typecheck_expr_(tc, kid_(tc, e, i));
            ty = g_ty_unknown;
            break;
        }
    }

    bind_type_(tc, (uint32_t)e, ty);
    return ty;
}

static void typecheck_stmt_(steel_typecheck* tc, steel_ast_id s)
{
    if (!s) return;

    steel_ast_kind k = steel_ast_kindof(tc->ast, s);

    switch ((int)k)
    {
        case ASTK_STMT_LET:
        case ASTK_STMT_CONST:
        {
            // children: ident, (optional) type_path, (optional) init_expr
            size_t n = kid_count_(tc, s);
            uint32_t anno = 0;
            uint32_t init = 0;

            if (n >= 2 && (int)steel_ast_kindof(tc->ast, kid_(tc, s, 1)) == ASTK_PATH)
                anno = parse_type_from_path_(tc, kid_(tc, s, 1));

            // init expression can be index 1 or 2 depending on annotation
            for (size_t i = 1; i < n; i++)
            {
                steel_ast_id c = kid_(tc, s, i);
                int ck = (int)steel_ast_kindof(tc->ast, c);
                if (ck == ASTK_PATH)
                    continue;

                init = typecheck_expr_(tc, c);
                break;
            }

            if (anno && init)
            {
                if (!is_same_ty_(tc, anno, init))
                    emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, s), "initializer type does not match annotation");
            }

            // bind let/const node as unit
            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;
        }

        case ASTK_STMT_SET:
        {
            // children: lhs, rhs
            size_t n = kid_count_(tc, s);
            if (n > 0) (void)typecheck_expr_(tc, kid_(tc, s, 0));
            if (n > 1) (void)typecheck_expr_(tc, kid_(tc, s, 1));
            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;
        }

        case ASTK_STMT_SAY:
        case ASTK_STMT_DO:
        case ASTK_STMT_EXPR:
        {
            size_t n = kid_count_(tc, s);
            for (size_t i = 0; i < n; i++)
                (void)typecheck_expr_(tc, kid_(tc, s, i));
            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;
        }

        case ASTK_STMT_IF:
        {
            // kids: cond, then block, (elif/else nodes)
            size_t n = kid_count_(tc, s);
            if (n > 0)
            {
                uint32_t ct = typecheck_expr_(tc, kid_(tc, s, 0));
                if (ct != g_ty_bool && ct != g_ty_unknown)
                    emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, kid_(tc, s, 0)), "if condition must be bool");
            }

            for (size_t i = 1; i < n; i++)
            {
                steel_ast_id c = kid_(tc, s, i);
                int ck = (int)steel_ast_kindof(tc->ast, c);
                if (ck == ASTK_BLOCK) typecheck_block_(tc, c);
                else typecheck_stmt_(tc, c);
            }

            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;
        }

        case ASTK_STMT_ELIF:
        {
            size_t n = kid_count_(tc, s);
            if (n > 0)
            {
                uint32_t ct = typecheck_expr_(tc, kid_(tc, s, 0));
                if (ct != g_ty_bool && ct != g_ty_unknown)
                    emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, kid_(tc, s, 0)), "elif condition must be bool");
            }
            if (n > 1)
            {
                steel_ast_id b = kid_(tc, s, 1);
                if ((int)steel_ast_kindof(tc->ast, b) == ASTK_BLOCK) typecheck_block_(tc, b);
            }
            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;
        }

        case ASTK_STMT_ELSE:
        {
            size_t n = kid_count_(tc, s);
            if (n > 0)
            {
                steel_ast_id b = kid_(tc, s, 0);
                if ((int)steel_ast_kindof(tc->ast, b) == ASTK_BLOCK) typecheck_block_(tc, b);
            }
            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;
        }

        case ASTK_STMT_WHILE:
        {
            size_t n = kid_count_(tc, s);
            if (n > 0)
            {
                uint32_t ct = typecheck_expr_(tc, kid_(tc, s, 0));
                if (ct != g_ty_bool && ct != g_ty_unknown)
                    emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, kid_(tc, s, 0)), "while condition must be bool");
            }
            if (n > 1)
            {
                steel_ast_id b = kid_(tc, s, 1);
                if ((int)steel_ast_kindof(tc->ast, b) == ASTK_BLOCK) typecheck_block_(tc, b);
            }
            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;
        }

        case ASTK_STMT_FOR:
        {
            // minimal: iter var (ident), iterable expr, block
            size_t n = kid_count_(tc, s);
            size_t idx = 0;
            if (n > 0 && (int)steel_ast_kindof(tc->ast, kid_(tc, s, 0)) == ASTK_IDENT)
                idx = 1;

            if (idx < n)
                (void)typecheck_expr_(tc, kid_(tc, s, idx++));

            if (idx < n)
            {
                steel_ast_id b = kid_(tc, s, idx);
                if ((int)steel_ast_kindof(tc->ast, b) == ASTK_BLOCK) typecheck_block_(tc, b);
            }

            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;
        }

        case ASTK_STMT_MATCH:
        {
            // scrutinee + arms
            size_t n = kid_count_(tc, s);
            if (n > 0) (void)typecheck_expr_(tc, kid_(tc, s, 0));

            for (size_t i = 1; i < n; i++)
                typecheck_stmt_(tc, kid_(tc, s, i));

            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;
        }

        case ASTK_MATCH_ARM:
        {
            // kids: pattern, block
            size_t n = kid_count_(tc, s);
            if (n > 1)
            {
                steel_ast_id b = kid_(tc, s, 1);
                if ((int)steel_ast_kindof(tc->ast, b) == ASTK_BLOCK) typecheck_block_(tc, b);
            }
            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;
        }

        case ASTK_STMT_RETURN:
        {
            size_t n = kid_count_(tc, s);
            uint32_t rt = g_ty_unit;
            if (n > 0)
                rt = typecheck_expr_(tc, kid_(tc, s, 0));

            if (tc->cur_ret_ty_set)
            {
                if (!is_same_ty_(tc, tc->cur_ret_ty, rt))
                    emit_(tc, STEEL_DIAG_ERROR, steel_ast_spanof(tc->ast, s), "return type does not match function signature");
            }

            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;
        }

        case ASTK_STMT_BREAK:
        case ASTK_STMT_CONTINUE:
            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;

        case ASTK_BLOCK:
            typecheck_block_(tc, s);
            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;

        default:
        {
            // Fallback: if it's an expression node used as a statement.
            (void)typecheck_expr_(tc, s);
            bind_type_(tc, (uint32_t)s, g_ty_unit);
            return;
        }
    }
}

static void typecheck_block_(steel_typecheck* tc, steel_ast_id b)
{
    if (!b) return;
    size_t n = kid_count_(tc, b);
    for (size_t i = 0; i < n; i++)
        typecheck_stmt_(tc, kid_(tc, b, i));

    bind_type_(tc, (uint32_t)b, g_ty_unit);
}

static uint32_t parse_fn_return_type_(steel_typecheck* tc, steel_ast_id fn)
{
    // children: params, (optional) ret type path, block
    size_t n = kid_count_(tc, fn);
    for (size_t i = 0; i < n; i++)
    {
        steel_ast_id c = kid_(tc, fn, i);
        if ((int)steel_ast_kindof(tc->ast, c) == ASTK_PATH)
            return parse_type_from_path_(tc, c);
    }

    // default: unit
    return g_ty_unit;
}

static void typecheck_fn_like_(steel_typecheck* tc, steel_ast_id fn)
{
    uint32_t prev_ret = tc->cur_ret_ty;
    bool prev_ret_set = tc->cur_ret_ty_set;

    tc->cur_ret_ty = parse_fn_return_type_(tc, fn);
    tc->cur_ret_ty_set = true;

    // Typecheck body block
    size_t n = kid_count_(tc, fn);
    for (size_t i = 0; i < n; i++)
    {
        steel_ast_id c = kid_(tc, fn, i);
        int ck = (int)steel_ast_kindof(tc->ast, c);
        if (ck == ASTK_BLOCK)
            typecheck_block_(tc, c);
    }

    // Restore
    tc->cur_ret_ty = prev_ret;
    tc->cur_ret_ty_set = prev_ret_set;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

typedef struct steel_typecheck_ctx
{
    steel_type_diag_fn diag;
    void* diag_user;
} steel_typecheck_ctx;

typedef struct steel_typecheck_result
{
    bool ok;
    steel_map_u32_u32 node_to_type; // ast_id -> type_id
    steel_vec types;                // steel_type
} steel_typecheck_result;

static void typecheck_result_init_(steel_typecheck_result* r)
{
    memset(r, 0, sizeof(*r));
    steel_map_u32_u32_init(&r->node_to_type);
    steel_vec_init(&r->types, sizeof(steel_type));
}

void steel_typecheck_result_dispose(steel_typecheck_result* r)
{
    if (!r) return;
    steel_map_u32_u32_free(&r->node_to_type);
    steel_vec_free(&r->types);
    memset(r, 0, sizeof(*r));
}

static void init_builtins_(steel_typecheck* tc)
{
    // Builtins are inserted once per run.
    steel_type t;

    memset(&t, 0, sizeof(t)); t.kind = TY_ERROR;   g_ty_error   = ty_new_(tc, t);
    memset(&t, 0, sizeof(t)); t.kind = TY_UNKNOWN; g_ty_unknown = ty_new_(tc, t);
    memset(&t, 0, sizeof(t)); t.kind = TY_UNIT;    g_ty_unit    = ty_new_(tc, t);
    memset(&t, 0, sizeof(t)); t.kind = TY_NULL;    g_ty_null    = ty_new_(tc, t);

    memset(&t, 0, sizeof(t)); t.kind = TY_BOOL;    g_ty_bool    = ty_new_(tc, t);
    memset(&t, 0, sizeof(t)); t.kind = TY_INT;     g_ty_int     = ty_new_(tc, t);
    memset(&t, 0, sizeof(t)); t.kind = TY_FLOAT;   g_ty_float   = ty_new_(tc, t);
    memset(&t, 0, sizeof(t)); t.kind = TY_CHAR;    g_ty_char    = ty_new_(tc, t);
    memset(&t, 0, sizeof(t)); t.kind = TY_STRING;  g_ty_string  = ty_new_(tc, t);
}

bool steel_typecheck_ast(steel_ast* ast,
                         steel_ast_id root,
                         const steel_resolver_result* rr,
                         const steel_typecheck_ctx* ctx_in,
                         steel_typecheck_result* out)
{
    if (!ast || !root || !out)
        return false;

    steel_typecheck tc;
    memset(&tc, 0, sizeof(tc));

    tc.ast = ast;
    tc.rr = rr;

    tc.diag = (ctx_in && ctx_in->diag) ? ctx_in->diag : steel_type_diag_default_;
    tc.diag_user = ctx_in ? ctx_in->diag_user : NULL;

    tc.ok = true;

    steel_vec_init(&tc.types, sizeof(steel_type));
    steel_vec_init(&tc.fn_params, sizeof(uint32_t));
    steel_map_u32_u32_init(&tc.node_to_type);

    init_builtins_(&tc);

    // Walk toplevel
    size_t n = steel_ast_kid_countof(ast, root);
    for (size_t i = 0; i < n; i++)
    {
        steel_ast_id d = steel_ast_kid_atof(ast, root, i);
        int k = (int)steel_ast_kindof(ast, d);

        if (k == ASTK_FN || k == ASTK_SCN || k == ASTK_ENTRY)
            typecheck_fn_like_(&tc, d);
        else if (k == ASTK_STMT_EXPR || k == ASTK_STMT_SAY || k == ASTK_STMT_DO)
            typecheck_stmt_(&tc, d);
        else
        {
            // Other decls: typecheck children lightly
            size_t cn = steel_ast_kid_countof(ast, d);
            for (size_t j = 0; j < cn; j++)
                (void)typecheck_expr_(&tc, steel_ast_kid_atof(ast, d, j));
        }
    }

    // Move results out
    typecheck_result_init_(out);
    out->ok = tc.ok;

    out->node_to_type = tc.node_to_type; // move
    out->types = tc.types;               // move

    // Dispose leftovers
    steel_vec_free(&tc.fn_params);

    return out->ok;
}

bool steel_typecheck_lookup_node(const steel_typecheck_result* r, steel_ast_id node, uint32_t* out_type_id)
{
    if (!r) return false;
    return steel_map_u32_u32_get(&r->node_to_type, (uint32_t)node, out_type_id);
}

const char* steel_type_name(const steel_typecheck_result* r, uint32_t type_id)
{
    if (!r || type_id == 0) return "<none>";

    size_t idx = (size_t)(type_id - 1);
    if (idx >= steel_vec_len(&r->types)) return "<bad-type-id>";

    const steel_type* t = (const steel_type*)steel_vec_cat(&r->types, idx);
    if (!t) return "<bad-type>";

    switch (t->kind)
    {
        case TY_ERROR: return "<error>";
        case TY_UNKNOWN: return "<unknown>";
        case TY_UNIT: return "unit";
        case TY_NULL: return "null";
        case TY_BOOL: return "bool";
        case TY_INT: return "int";
        case TY_FLOAT: return "float";
        case TY_CHAR: return "char";
        case TY_STRING: return "string";
        case TY_NOMINAL: return t->name ? t->name : "<nominal>";
        case TY_FN: return "fn";
        default: return "<type>";
    }
}
