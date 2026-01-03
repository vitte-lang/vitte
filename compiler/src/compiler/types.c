// SPDX-License-Identifier: MIT
// types.c
//
// Type interner and canonical type representation (max).
//
// Provides:
//  - Stable type ids
//  - Builtin types
//  - Nominal types (by interned name)
//  - Function types (params -> ret)
//  - Minimal formatting helpers
//
// This module is intentionally standalone and can be used by resolver/typecheck/IR.
// If you already have a `types.h`, align it to the public API below.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../common/vec.h"

//------------------------------------------------------------------------------
// Public API (keep in sync with types.h)
//------------------------------------------------------------------------------

typedef enum steel_type_kind
{
    STEEL_TY_ERROR = 1,
    STEEL_TY_UNKNOWN,

    STEEL_TY_UNIT,
    STEEL_TY_NULL,

    STEEL_TY_BOOL,
    STEEL_TY_INT,
    STEEL_TY_FLOAT,
    STEEL_TY_CHAR,
    STEEL_TY_STRING,

    STEEL_TY_NOMINAL, // user type by name
    STEEL_TY_FN,      // fn(params...) -> ret

} steel_type_kind;

typedef struct steel_type
{
    uint32_t id;
    steel_type_kind kind;

    // nominal
    const char* name; // interned pointer (or stable storage)

    // fn
    uint32_t ret;
    uint32_t params_ofs;
    uint32_t params_len;

} steel_type;

typedef struct steel_types
{
    steel_vec types;      // steel_type
    steel_vec fn_params;  // uint32_t

    // cached builtin ids
    uint32_t ty_error;
    uint32_t ty_unknown;
    uint32_t ty_unit;
    uint32_t ty_null;
    uint32_t ty_bool;
    uint32_t ty_int;
    uint32_t ty_float;
    uint32_t ty_char;
    uint32_t ty_string;

} steel_types;

// init/dispose
void steel_types_init(steel_types* t);
void steel_types_dispose(steel_types* t);

// builtins
uint32_t steel_types_error(const steel_types* t);
uint32_t steel_types_unknown(const steel_types* t);
uint32_t steel_types_unit(const steel_types* t);
uint32_t steel_types_null(const steel_types* t);
uint32_t steel_types_bool(const steel_types* t);
uint32_t steel_types_int(const steel_types* t);
uint32_t steel_types_float(const steel_types* t);
uint32_t steel_types_char(const steel_types* t);
uint32_t steel_types_string(const steel_types* t);

// constructors
uint32_t steel_types_nominal(steel_types* t, const char* interned_name);
uint32_t steel_types_fn(steel_types* t, const uint32_t* param_types, uint32_t param_count, uint32_t ret_type);

// queries
const steel_type* steel_types_get(const steel_types* t, uint32_t type_id);
bool steel_types_equal(const steel_types* t, uint32_t a, uint32_t b);

// formatting
const char* steel_types_kind_name(steel_type_kind k);
size_t steel_types_format(const steel_types* t, uint32_t type_id, char* out, size_t out_cap);

//------------------------------------------------------------------------------
// Internal helpers
//------------------------------------------------------------------------------

static steel_type* ty_mut_(steel_types* ts, uint32_t id)
{
    if (!ts || id == 0) return NULL;
    size_t idx = (size_t)(id - 1);
    if (idx >= steel_vec_len(&ts->types)) return NULL;
    return (steel_type*)steel_vec_at(&ts->types, idx);
}

static const steel_type* ty_(const steel_types* ts, uint32_t id)
{
    if (!ts || id == 0) return NULL;
    size_t idx = (size_t)(id - 1);
    if (idx >= steel_vec_len(&ts->types)) return NULL;
    return (const steel_type*)steel_vec_cat(&ts->types, idx);
}

static uint32_t ty_push_(steel_types* ts, steel_type t)
{
    t.id = (uint32_t)steel_vec_len(&ts->types) + 1;
    steel_vec_push(&ts->types, &t);
    return t.id;
}

static uint32_t u32_push_many_(steel_vec* v, const uint32_t* xs, uint32_t n)
{
    uint32_t ofs = (uint32_t)steel_vec_len(v);
    for (uint32_t i = 0; i < n; i++)
        steel_vec_push(v, &xs[i]);
    return ofs;
}

//------------------------------------------------------------------------------
// Init/dispose
//------------------------------------------------------------------------------

void steel_types_init(steel_types* ts)
{
    if (!ts) return;
    memset(ts, 0, sizeof(*ts));

    steel_vec_init(&ts->types, sizeof(steel_type));
    steel_vec_init(&ts->fn_params, sizeof(uint32_t));

    steel_type t;

    memset(&t, 0, sizeof(t)); t.kind = STEEL_TY_ERROR;   ts->ty_error   = ty_push_(ts, t);
    memset(&t, 0, sizeof(t)); t.kind = STEEL_TY_UNKNOWN; ts->ty_unknown = ty_push_(ts, t);
    memset(&t, 0, sizeof(t)); t.kind = STEEL_TY_UNIT;    ts->ty_unit    = ty_push_(ts, t);
    memset(&t, 0, sizeof(t)); t.kind = STEEL_TY_NULL;    ts->ty_null    = ty_push_(ts, t);

    memset(&t, 0, sizeof(t)); t.kind = STEEL_TY_BOOL;    ts->ty_bool    = ty_push_(ts, t);
    memset(&t, 0, sizeof(t)); t.kind = STEEL_TY_INT;     ts->ty_int     = ty_push_(ts, t);
    memset(&t, 0, sizeof(t)); t.kind = STEEL_TY_FLOAT;   ts->ty_float   = ty_push_(ts, t);
    memset(&t, 0, sizeof(t)); t.kind = STEEL_TY_CHAR;    ts->ty_char    = ty_push_(ts, t);
    memset(&t, 0, sizeof(t)); t.kind = STEEL_TY_STRING;  ts->ty_string  = ty_push_(ts, t);
}

void steel_types_dispose(steel_types* ts)
{
    if (!ts) return;

    steel_vec_free(&ts->types);
    steel_vec_free(&ts->fn_params);

    memset(ts, 0, sizeof(*ts));
}

//------------------------------------------------------------------------------
// Builtin accessors
//------------------------------------------------------------------------------

uint32_t steel_types_error(const steel_types* t)   { return t ? t->ty_error : 0; }
uint32_t steel_types_unknown(const steel_types* t) { return t ? t->ty_unknown : 0; }
uint32_t steel_types_unit(const steel_types* t)    { return t ? t->ty_unit : 0; }
uint32_t steel_types_null(const steel_types* t)    { return t ? t->ty_null : 0; }
uint32_t steel_types_bool(const steel_types* t)    { return t ? t->ty_bool : 0; }
uint32_t steel_types_int(const steel_types* t)     { return t ? t->ty_int : 0; }
uint32_t steel_types_float(const steel_types* t)   { return t ? t->ty_float : 0; }
uint32_t steel_types_char(const steel_types* t)    { return t ? t->ty_char : 0; }
uint32_t steel_types_string(const steel_types* t)  { return t ? t->ty_string : 0; }

//------------------------------------------------------------------------------
// Constructors
//------------------------------------------------------------------------------

uint32_t steel_types_nominal(steel_types* ts, const char* interned_name)
{
    if (!ts || !interned_name)
        return ts ? ts->ty_unknown : 0;

    // Dedup by pointer identity (interned name). If not interned, dedup is weaker.
    size_t n = steel_vec_len(&ts->types);
    for (size_t i = 0; i < n; i++)
    {
        const steel_type* t = (const steel_type*)steel_vec_cat(&ts->types, i);
        if (t && t->kind == STEEL_TY_NOMINAL && t->name == interned_name)
            return t->id;
    }

    steel_type t;
    memset(&t, 0, sizeof(t));
    t.kind = STEEL_TY_NOMINAL;
    t.name = interned_name;

    return ty_push_(ts, t);
}

static bool same_fn_sig_(const steel_types* ts, const steel_type* a, const steel_type* b)
{
    if (!ts || !a || !b) return false;
    if (a->kind != STEEL_TY_FN || b->kind != STEEL_TY_FN) return false;
    if (a->ret != b->ret) return false;
    if (a->params_len != b->params_len) return false;

    for (uint32_t i = 0; i < a->params_len; i++)
    {
        const uint32_t* pa = (const uint32_t*)steel_vec_cat((steel_vec*)&ts->fn_params, a->params_ofs + i);
        const uint32_t* pb = (const uint32_t*)steel_vec_cat((steel_vec*)&ts->fn_params, b->params_ofs + i);
        if (!pa || !pb) return false;
        if (*pa != *pb) return false;
    }

    return true;
}

uint32_t steel_types_fn(steel_types* ts, const uint32_t* param_types, uint32_t param_count, uint32_t ret_type)
{
    if (!ts) return 0;

    // Dedup: linear scan over existing function types.
    size_t n = steel_vec_len(&ts->types);
    for (size_t i = 0; i < n; i++)
    {
        const steel_type* t = (const steel_type*)steel_vec_cat(&ts->types, i);
        if (!t || t->kind != STEEL_TY_FN) continue;

        if (t->ret != ret_type) continue;
        if (t->params_len != param_count) continue;

        bool same = true;
        for (uint32_t j = 0; j < param_count; j++)
        {
            const uint32_t* pj = (const uint32_t*)steel_vec_cat(&ts->fn_params, t->params_ofs + j);
            if (!pj || *pj != param_types[j]) { same = false; break; }
        }

        if (same) return t->id;
    }

    uint32_t ofs = 0;
    if (param_count > 0)
        ofs = u32_push_many_(&ts->fn_params, param_types, param_count);

    steel_type t;
    memset(&t, 0, sizeof(t));
    t.kind = STEEL_TY_FN;
    t.ret = ret_type;
    t.params_ofs = ofs;
    t.params_len = param_count;

    return ty_push_(ts, t);
}

//------------------------------------------------------------------------------
// Queries
//------------------------------------------------------------------------------

const steel_type* steel_types_get(const steel_types* ts, uint32_t type_id)
{
    return ty_(ts, type_id);
}

bool steel_types_equal(const steel_types* ts, uint32_t a, uint32_t b)
{
    if (a == b) return true;
    if (!ts) return false;

    // unknown and error are compatible-ish at the boundary; keep strict by default.
    const steel_type* ta = ty_(ts, a);
    const steel_type* tb = ty_(ts, b);
    if (!ta || !tb) return false;

    if (ta->kind != tb->kind) return false;

    switch (ta->kind)
    {
        case STEEL_TY_NOMINAL:
            return ta->name == tb->name;

        case STEEL_TY_FN:
            return same_fn_sig_(ts, ta, tb);

        default:
            return true;
    }
}

//------------------------------------------------------------------------------
// Formatting
//------------------------------------------------------------------------------

const char* steel_types_kind_name(steel_type_kind k)
{
    switch (k)
    {
        case STEEL_TY_ERROR: return "error";
        case STEEL_TY_UNKNOWN: return "unknown";
        case STEEL_TY_UNIT: return "unit";
        case STEEL_TY_NULL: return "null";
        case STEEL_TY_BOOL: return "bool";
        case STEEL_TY_INT: return "int";
        case STEEL_TY_FLOAT: return "float";
        case STEEL_TY_CHAR: return "char";
        case STEEL_TY_STRING: return "string";
        case STEEL_TY_NOMINAL: return "nominal";
        case STEEL_TY_FN: return "fn";
        default: return "<type>";
    }
}

static size_t snprint_(char* out, size_t cap, const char* s)
{
    if (!out || cap == 0) return 0;
    if (!s) s = "";
    size_t n = strlen(s);
    if (n >= cap) n = cap - 1;
    memcpy(out, s, n);
    out[n] = 0;
    return n;
}

static size_t snprintf_(char* out, size_t cap, const char* fmt, ...)
{
    if (!out || cap == 0) return 0;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(out, cap, fmt, ap);
    va_end(ap);
    if (n < 0) { out[0] = 0; return 0; }
    if ((size_t)n >= cap) { out[cap - 1] = 0; return cap - 1; }
    return (size_t)n;
}

size_t steel_types_format(const steel_types* ts, uint32_t type_id, char* out, size_t out_cap)
{
    if (!ts) return snprint_(out, out_cap, "<no-types>");

    const steel_type* t = ty_(ts, type_id);
    if (!t) return snprint_(out, out_cap, "<bad-type-id>");

    switch (t->kind)
    {
        case STEEL_TY_ERROR: return snprint_(out, out_cap, "<error>");
        case STEEL_TY_UNKNOWN: return snprint_(out, out_cap, "<unknown>");
        case STEEL_TY_UNIT: return snprint_(out, out_cap, "unit");
        case STEEL_TY_NULL: return snprint_(out, out_cap, "null");
        case STEEL_TY_BOOL: return snprint_(out, out_cap, "bool");
        case STEEL_TY_INT: return snprint_(out, out_cap, "int");
        case STEEL_TY_FLOAT: return snprint_(out, out_cap, "float");
        case STEEL_TY_CHAR: return snprint_(out, out_cap, "char");
        case STEEL_TY_STRING: return snprint_(out, out_cap, "string");

        case STEEL_TY_NOMINAL:
            return snprint_(out, out_cap, t->name ? t->name : "<nominal>");

        case STEEL_TY_FN:
        {
            // "fn(T1, T2) -> R"
            size_t used = 0;
            used += snprint_(out + used, (used < out_cap ? out_cap - used : 0), "fn(");

            for (uint32_t i = 0; i < t->params_len; i++)
            {
                if (i > 0)
                    used += snprint_(out + used, (used < out_cap ? out_cap - used : 0), ", ");

                const uint32_t* pid = (const uint32_t*)steel_vec_cat((steel_vec*)&ts->fn_params, t->params_ofs + i);
                char tmp[64];
                tmp[0] = 0;
                if (pid)
                    steel_types_format(ts, *pid, tmp, sizeof(tmp));
                used += snprint_(out + used, (used < out_cap ? out_cap - used : 0), tmp);
            }

            used += snprint_(out + used, (used < out_cap ? out_cap - used : 0), ") -> ");

            char rt[64];
            rt[0] = 0;
            steel_types_format(ts, t->ret, rt, sizeof(rt));
            used += snprint_(out + used, (used < out_cap ? out_cap - used : 0), rt);

            return used;
        }

        default:
            return snprint_(out, out_cap, "<type>");
    }
}
