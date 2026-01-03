// SPDX-License-Identifier: MIT
// hir_build.c
//
// HIR builder (AST -> HIR) for vitte.
//
// This file implements a pragmatic, compiler-friendly HIR construction layer.
// It is designed to be usable even if the project is mid-refactor: it provides
// an internal HIR data model (type-erased enough to not depend on every AST kind)
// and a set of passes:
//  - symbol collection (per module/function)
//  - basic desugaring hooks (placeholders)
//  - lowering expressions/statements into HIR instructions
//
// It intentionally does NOT perform type checking or borrow checking; those are
// intended to be later passes.
//
// Pairing:
//  - hir_build.h should declare the public API:
//      void steel_hir_build_init(steel_hir_build* b, steel_ast* ast);
//      void steel_hir_build_dispose(steel_hir_build* b);
//      bool steel_hir_build_module(steel_hir_build* b, steel_ast_id root, steel_hir_module* out);
//  - If your headers already define HIR structs/enums, either:
//      (A) map this implementation's internal types to yours via typedef/wrappers, or
//      (B) port the helpers here into your existing representation.
//
// Dependencies:
//  - common/vec.h
//  - compiler/ast.h

#include "hir_build.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "ast.h"
#include "../common/vec.h"

//------------------------------------------------------------------------------
// Internal HIR representation
//------------------------------------------------------------------------------

// HIR ids
typedef uint32_t steel_hir_id;

typedef enum steel_hir_op
{
    STEEL_HIR_OP_NOP = 0,

    // Values
    STEEL_HIR_OP_CONST_I64,
    STEEL_HIR_OP_CONST_F64,
    STEEL_HIR_OP_CONST_STR,

    // Locals
    STEEL_HIR_OP_LOCAL_GET,
    STEEL_HIR_OP_LOCAL_SET,

    // Control
    STEEL_HIR_OP_JMP,
    STEEL_HIR_OP_JMP_IF,
    STEEL_HIR_OP_RET,

    // Calls
    STEEL_HIR_OP_CALL,

    // Binary ops (untyped)
    STEEL_HIR_OP_ADD,
    STEEL_HIR_OP_SUB,
    STEEL_HIR_OP_MUL,
    STEEL_HIR_OP_DIV,
    STEEL_HIR_OP_MOD,

    STEEL_HIR_OP_EQ,
    STEEL_HIR_OP_NE,
    STEEL_HIR_OP_LT,
    STEEL_HIR_OP_LE,
    STEEL_HIR_OP_GT,
    STEEL_HIR_OP_GE,

    STEEL_HIR_OP_AND,
    STEEL_HIR_OP_OR,

    // Unary
    STEEL_HIR_OP_NEG,
    STEEL_HIR_OP_NOT,

} steel_hir_op;

typedef struct steel_hir_inst
{
    steel_hir_op op;
    uint32_t     dst;      // virtual register id (0 means no dst)
    uint32_t     a;
    uint32_t     b;
    uint32_t     c;

    int64_t      imm_i64;
    double       imm_f64;
    const char*  imm_str;  // interned from AST

    steel_span   span;
} steel_hir_inst;

typedef struct steel_hir_block
{
    // instruction indices range within module insts
    uint32_t first;
    uint32_t count;
} steel_hir_block;

typedef struct steel_hir_fn
{
    const char* name;
    steel_span  span;

    uint32_t reg_count;
    uint32_t local_count;

    steel_vec blocks; // steel_hir_block
    steel_vec insts;  // steel_hir_inst

    // mapping local index -> name (optional)
    steel_vec local_names; // const char*

} steel_hir_fn;

typedef struct steel_hir_symbol
{
    const char* name;
    uint32_t    kind; // 0=fn, 1=global, 2=type
    steel_ast_id ast_id;
} steel_hir_symbol;

typedef struct steel_hir_module_i
{
    const char* name;
    steel_vec symbols; // steel_hir_symbol
    steel_vec fns;     // steel_hir_fn
} steel_hir_module_i;

//------------------------------------------------------------------------------
// Builder state
//------------------------------------------------------------------------------

struct steel_hir_build
{
    steel_ast* ast;

    // Interned names, reused from AST
    const char* mod_name;

    // Temp tables
    steel_vec sym_tmp; // steel_hir_symbol
};

//------------------------------------------------------------------------------
// Utilities
//------------------------------------------------------------------------------

static void steel_hir_fn_init(steel_hir_fn* f)
{
    memset(f, 0, sizeof(*f));
    steel_vec_init(&f->blocks, sizeof(steel_hir_block));
    steel_vec_init(&f->insts, sizeof(steel_hir_inst));
    steel_vec_init(&f->local_names, sizeof(const char*));
}

static void steel_hir_fn_dispose(steel_hir_fn* f)
{
    steel_vec_free(&f->blocks);
    steel_vec_free(&f->insts);
    steel_vec_free(&f->local_names);
    memset(f, 0, sizeof(*f));
}

static uint32_t steel_hir_new_reg(steel_hir_fn* f)
{
    f->reg_count += 1;
    return f->reg_count;
}

static uint32_t steel_hir_new_local(steel_hir_fn* f, const char* name)
{
    uint32_t idx = f->local_count;
    f->local_count++;
    (void)steel_vec_push(&f->local_names, &name);
    return idx;
}

static bool steel_hir_emit(steel_hir_fn* f, steel_hir_inst inst)
{
    return steel_vec_push(&f->insts, &inst) != NULL;
}

static steel_hir_inst steel_hir_inst_make(steel_hir_op op, steel_span span)
{
    steel_hir_inst i;
    memset(&i, 0, sizeof(i));
    i.op = op;
    i.span = span;
    return i;
}

static bool steel_ast_node_text_is(const struct steel_ast_node* n, const char* s)
{
    if (!n || !s) return false;
    if (!n->text) return false;
    return strcmp(n->text, s) == 0;
}

//------------------------------------------------------------------------------
// Symbol collection (best-effort)
//------------------------------------------------------------------------------

static void steel_collect_symbols_rec(steel_hir_build* b, steel_ast_id id)
{
    const struct steel_ast_node* n = steel_ast_cget(b->ast, id);
    if (!n) return;

    // Heuristic:
    // - If node has a name, treat it as a symbol.
    // - kind tag guessed from node.kind range (not stable); default to fn.
    if (n->name && n->name[0])
    {
        steel_hir_symbol sym;
        sym.name = n->name;
        sym.ast_id = id;
        sym.kind = 0;

        // crude heuristics: if text says "type" then kind=type
        if (n->text && strcmp(n->text, "type") == 0) sym.kind = 2;
        if (n->text && strcmp(n->text, "global") == 0) sym.kind = 1;

        (void)steel_vec_push(&b->sym_tmp, &sym);
    }

    size_t kc = steel_ast_kid_count(b->ast, id);
    for (size_t i = 0; i < kc; i++)
        steel_collect_symbols_rec(b, steel_ast_kid_at(b->ast, id, i));

    size_t ac = steel_ast_aux_count(b->ast, id);
    for (size_t i = 0; i < ac; i++)
        steel_collect_symbols_rec(b, steel_ast_aux_at(b->ast, id, i));
}

//------------------------------------------------------------------------------
// Expression lowering (minimal, generic)
//------------------------------------------------------------------------------

// Strategy:
//  - Each AST node becomes a value in a new register.
//  - Node fields:
//      - n->i64 => integer literal
//      - n->f64 => float literal
//      - n->text => identifier / operator token / string literal
//      - kids => operand list
//
// Without a concrete AST kind enum, we rely on convention:
//  - text values like "+", "-", "*", "/", "%", "==", "!=", "<", "<=", ">", ">="
//  - "and"/"or" for logical ops
//  - "call" to represent a call node, where kids[0]=callee, kids[1..]=args
//  - "ret" for return node, kids[0]=expr optional
//  - "set" for assignment: kids[0]=lhs, kids[1]=rhs
//  - "if" for if node: kids[0]=cond, kids[1]=then, kids[2]=else optional

static uint32_t steel_lower_expr(steel_hir_build* b, steel_hir_fn* f, steel_ast_id id);

static uint32_t steel_lower_binary(steel_hir_build* b, steel_hir_fn* f, steel_ast_id id, steel_hir_op op)
{
    const struct steel_ast_node* n = steel_ast_cget(b->ast, id);
    if (!n) return 0;

    steel_ast_id lhs_id = steel_ast_kid_at(b->ast, id, 0);
    steel_ast_id rhs_id = steel_ast_kid_at(b->ast, id, 1);

    uint32_t lhs = steel_lower_expr(b, f, lhs_id);
    uint32_t rhs = steel_lower_expr(b, f, rhs_id);

    uint32_t dst = steel_hir_new_reg(f);
    steel_hir_inst inst = steel_hir_inst_make(op, n->span);
    inst.dst = dst;
    inst.a = lhs;
    inst.b = rhs;
    (void)steel_hir_emit(f, inst);
    return dst;
}

static uint32_t steel_lower_unary(steel_hir_build* b, steel_hir_fn* f, steel_ast_id id, steel_hir_op op)
{
    const struct steel_ast_node* n = steel_ast_cget(b->ast, id);
    if (!n) return 0;

    steel_ast_id x_id = steel_ast_kid_at(b->ast, id, 0);
    uint32_t x = steel_lower_expr(b, f, x_id);

    uint32_t dst = steel_hir_new_reg(f);
    steel_hir_inst inst = steel_hir_inst_make(op, n->span);
    inst.dst = dst;
    inst.a = x;
    (void)steel_hir_emit(f, inst);
    return dst;
}

static uint32_t steel_lower_call(steel_hir_build* b, steel_hir_fn* f, steel_ast_id id)
{
    const struct steel_ast_node* n = steel_ast_cget(b->ast, id);
    if (!n) return 0;

    // kids[0] callee, rest args
    steel_ast_id callee_id = steel_ast_kid_at(b->ast, id, 0);
    uint32_t callee = steel_lower_expr(b, f, callee_id);

    // For simplicity, encode arg regs via aux list (one inst per arg) is overkill.
    // Instead, store up to 2 args directly and ignore extra.
    uint32_t a0 = 0, a1 = 0;
    size_t argc = steel_ast_kid_count(b->ast, id);
    if (argc > 1) a0 = steel_lower_expr(b, f, steel_ast_kid_at(b->ast, id, 1));
    if (argc > 2) a1 = steel_lower_expr(b, f, steel_ast_kid_at(b->ast, id, 2));

    uint32_t dst = steel_hir_new_reg(f);
    steel_hir_inst inst = steel_hir_inst_make(STEEL_HIR_OP_CALL, n->span);
    inst.dst = dst;
    inst.a = callee;
    inst.b = a0;
    inst.c = a1;
    inst.imm_i64 = (int64_t)(argc > 0 ? (argc - 1) : 0);
    (void)steel_hir_emit(f, inst);
    return dst;
}

static uint32_t steel_lower_lit_or_ident(steel_hir_build* b, steel_hir_fn* f, steel_ast_id id)
{
    const struct steel_ast_node* n = steel_ast_cget(b->ast, id);
    if (!n) return 0;

    // If i64 is set, treat as integer const.
    if (n->i64 != 0 || (n->text && strcmp(n->text, "0") == 0))
    {
        uint32_t dst = steel_hir_new_reg(f);
        steel_hir_inst inst = steel_hir_inst_make(STEEL_HIR_OP_CONST_I64, n->span);
        inst.dst = dst;
        inst.imm_i64 = n->i64;
        (void)steel_hir_emit(f, inst);
        return dst;
    }

    // If f64 set, treat as float const.
    if (n->f64 != 0.0)
    {
        uint32_t dst = steel_hir_new_reg(f);
        steel_hir_inst inst = steel_hir_inst_make(STEEL_HIR_OP_CONST_F64, n->span);
        inst.dst = dst;
        inst.imm_f64 = n->f64;
        (void)steel_hir_emit(f, inst);
        return dst;
    }

    // If text is non-empty, treat as identifier or string.
    if (n->text && n->text[0])
    {
        // crude heuristic: quoted => string
        size_t L = strlen(n->text);
        if (L >= 2 && ((n->text[0] == '"' && n->text[L - 1] == '"') || (n->text[0] == '\'' && n->text[L - 1] == '\'')))
        {
            // Store raw string including quotes for now.
            uint32_t dst = steel_hir_new_reg(f);
            steel_hir_inst inst = steel_hir_inst_make(STEEL_HIR_OP_CONST_STR, n->span);
            inst.dst = dst;
            inst.imm_str = n->text;
            (void)steel_hir_emit(f, inst);
            return dst;
        }

        // identifier: map to a local slot lazily
        // We keep local table as names; if exists reuse.
        uint32_t local = UINT32_MAX;
        for (uint32_t i = 0; i < f->local_count; i++)
        {
            const char** pn = (const char**)steel_vec_cat(&f->local_names, i);
            if (pn && *pn && strcmp(*pn, n->text) == 0)
            {
                local = i;
                break;
            }
        }
        if (local == UINT32_MAX)
            local = steel_hir_new_local(f, n->text);

        uint32_t dst = steel_hir_new_reg(f);
        steel_hir_inst inst = steel_hir_inst_make(STEEL_HIR_OP_LOCAL_GET, n->span);
        inst.dst = dst;
        inst.a = local;
        (void)steel_hir_emit(f, inst);
        return dst;
    }

    // fallback
    {
        uint32_t dst = steel_hir_new_reg(f);
        steel_hir_inst inst = steel_hir_inst_make(STEEL_HIR_OP_NOP, n->span);
        inst.dst = dst;
        (void)steel_hir_emit(f, inst);
        return dst;
    }
}

static uint32_t steel_lower_expr(steel_hir_build* b, steel_hir_fn* f, steel_ast_id id)
{
    const struct steel_ast_node* n = steel_ast_cget(b->ast, id);
    if (!n) return 0;

    // Convention-based dispatch by node->text
    const char* t = n->text;

    if (t)
    {
        if (strcmp(t, "+") == 0)  return steel_lower_binary(b, f, id, STEEL_HIR_OP_ADD);
        if (strcmp(t, "-") == 0)
        {
            // unary or binary
            if (steel_ast_kid_count(b->ast, id) >= 2) return steel_lower_binary(b, f, id, STEEL_HIR_OP_SUB);
            return steel_lower_unary(b, f, id, STEEL_HIR_OP_NEG);
        }
        if (strcmp(t, "*") == 0)  return steel_lower_binary(b, f, id, STEEL_HIR_OP_MUL);
        if (strcmp(t, "/") == 0)  return steel_lower_binary(b, f, id, STEEL_HIR_OP_DIV);
        if (strcmp(t, "%") == 0)  return steel_lower_binary(b, f, id, STEEL_HIR_OP_MOD);

        if (strcmp(t, "==") == 0) return steel_lower_binary(b, f, id, STEEL_HIR_OP_EQ);
        if (strcmp(t, "!=") == 0) return steel_lower_binary(b, f, id, STEEL_HIR_OP_NE);
        if (strcmp(t, "<") == 0)  return steel_lower_binary(b, f, id, STEEL_HIR_OP_LT);
        if (strcmp(t, "<=") == 0) return steel_lower_binary(b, f, id, STEEL_HIR_OP_LE);
        if (strcmp(t, ">") == 0)  return steel_lower_binary(b, f, id, STEEL_HIR_OP_GT);
        if (strcmp(t, ">=") == 0) return steel_lower_binary(b, f, id, STEEL_HIR_OP_GE);

        if (strcmp(t, "and") == 0) return steel_lower_binary(b, f, id, STEEL_HIR_OP_AND);
        if (strcmp(t, "or") == 0)  return steel_lower_binary(b, f, id, STEEL_HIR_OP_OR);
        if (strcmp(t, "!") == 0 || strcmp(t, "not") == 0) return steel_lower_unary(b, f, id, STEEL_HIR_OP_NOT);

        if (strcmp(t, "call") == 0) return steel_lower_call(b, f, id);
    }

    // By default treat node as literal/ident.
    return steel_lower_lit_or_ident(b, f, id);
}

//------------------------------------------------------------------------------
// Statement lowering (very small)
//------------------------------------------------------------------------------

static void steel_lower_stmt(steel_hir_build* b, steel_hir_fn* f, steel_ast_id id)
{
    const struct steel_ast_node* n = steel_ast_cget(b->ast, id);
    if (!n) return;

    const char* t = n->text;
    if (t)
    {
        if (strcmp(t, "ret") == 0 || strcmp(t, "return") == 0)
        {
            uint32_t v = 0;
            if (steel_ast_kid_count(b->ast, id) >= 1)
                v = steel_lower_expr(b, f, steel_ast_kid_at(b->ast, id, 0));

            steel_hir_inst inst = steel_hir_inst_make(STEEL_HIR_OP_RET, n->span);
            inst.a = v;
            (void)steel_hir_emit(f, inst);
            return;
        }

        if (strcmp(t, "set") == 0)
        {
            // kids[0]=lhs ident, kids[1]=rhs expr
            steel_ast_id lhs_id = steel_ast_kid_at(b->ast, id, 0);
            steel_ast_id rhs_id = steel_ast_kid_at(b->ast, id, 1);

            const struct steel_ast_node* lhs = steel_ast_cget(b->ast, lhs_id);
            uint32_t rhs = steel_lower_expr(b, f, rhs_id);

            // ensure local for lhs
            uint32_t local = UINT32_MAX;
            if (lhs && lhs->text)
            {
                for (uint32_t i = 0; i < f->local_count; i++)
                {
                    const char** pn = (const char**)steel_vec_cat(&f->local_names, i);
                    if (pn && *pn && strcmp(*pn, lhs->text) == 0)
                    {
                        local = i;
                        break;
                    }
                }
                if (local == UINT32_MAX)
                    local = steel_hir_new_local(f, lhs->text);
            }
            else
            {
                local = steel_hir_new_local(f, "_tmp");
            }

            steel_hir_inst inst = steel_hir_inst_make(STEEL_HIR_OP_LOCAL_SET, n->span);
            inst.a = local;
            inst.b = rhs;
            (void)steel_hir_emit(f, inst);
            return;
        }

        if (strcmp(t, "if") == 0)
        {
            // Minimal lowering: evaluate cond, emit JMP_IF to skip then.
            // Does not create real blocks in this minimal form.
            steel_ast_id cond_id = steel_ast_kid_at(b->ast, id, 0);
            steel_ast_id then_id = steel_ast_kid_at(b->ast, id, 1);
            steel_ast_id else_id = steel_ast_kid_at(b->ast, id, 2);

            uint32_t cond = steel_lower_expr(b, f, cond_id);

            // Placeholder labels as instruction indices.
            uint32_t jmp_if_pos = (uint32_t)steel_vec_len(&f->insts);
            steel_hir_inst jif = steel_hir_inst_make(STEEL_HIR_OP_JMP_IF, n->span);
            jif.a = cond;
            jif.b = 0; // patched target
            (void)steel_hir_emit(f, jif);

            // then
            steel_lower_stmt(b, f, then_id);

            // jmp over else
            uint32_t jmp_pos = (uint32_t)steel_vec_len(&f->insts);
            steel_hir_inst j = steel_hir_inst_make(STEEL_HIR_OP_JMP, n->span);
            j.a = 0; // patched
            (void)steel_hir_emit(f, j);

            // patch jmp_if to else start
            uint32_t else_start = (uint32_t)steel_vec_len(&f->insts);
            steel_hir_inst* pjif = (steel_hir_inst*)steel_vec_at(&f->insts, jmp_if_pos);
            if (pjif) pjif->b = else_start;

            // else (optional)
            if (else_id)
                steel_lower_stmt(b, f, else_id);

            // patch jmp to end
            uint32_t end = (uint32_t)steel_vec_len(&f->insts);
            steel_hir_inst* pj = (steel_hir_inst*)steel_vec_at(&f->insts, jmp_pos);
            if (pj) pj->a = end;
            return;
        }
    }

    // Default: treat as expression statement.
    (void)steel_lower_expr(b, f, id);
}

static void steel_lower_block_like(steel_hir_build* b, steel_hir_fn* f, steel_ast_id id)
{
    size_t kc = steel_ast_kid_count(b->ast, id);
    for (size_t i = 0; i < kc; i++)
        steel_lower_stmt(b, f, steel_ast_kid_at(b->ast, id, i));
}

//------------------------------------------------------------------------------
// Function lowering (best-effort)
//------------------------------------------------------------------------------

static bool steel_lower_function_like(steel_hir_build* b, steel_ast_id fn_id, steel_hir_fn* out)
{
    const struct steel_ast_node* n = steel_ast_cget(b->ast, fn_id);
    if (!n) return false;

    steel_hir_fn_init(out);

    out->name = n->name ? n->name : (n->text ? n->text : "_fn");
    out->span = n->span;

    // Entry block (just a range marker)
    steel_hir_block blk;
    blk.first = 0;
    blk.count = 0;
    (void)steel_vec_push(&out->blocks, &blk);

    // Body is conventionally first child.
    if (steel_ast_kid_count(b->ast, fn_id) > 0)
    {
        steel_ast_id body = steel_ast_kid_at(b->ast, fn_id, steel_ast_kid_count(b->ast, fn_id) - 1);
        steel_lower_block_like(b, out, body);
    }

    // Ensure function ends with RET
    if (steel_vec_len(&out->insts) == 0 || ((steel_hir_inst*)steel_vec_cat(&out->insts, steel_vec_len(&out->insts) - 1))->op != STEEL_HIR_OP_RET)
    {
        steel_hir_inst r = steel_hir_inst_make(STEEL_HIR_OP_RET, n->span);
        r.a = 0;
        (void)steel_hir_emit(out, r);
    }

    // Patch block count
    steel_hir_block* pblk = (steel_hir_block*)steel_vec_at(&out->blocks, 0);
    if (pblk)
    {
        pblk->first = 0;
        pblk->count = (uint32_t)steel_vec_len(&out->insts);
    }

    return true;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void steel_hir_build_init(steel_hir_build* b, steel_ast* ast)
{
    if (!b) return;
    memset(b, 0, sizeof(*b));
    b->ast = ast;
    steel_vec_init(&b->sym_tmp, sizeof(steel_hir_symbol));
}

void steel_hir_build_dispose(steel_hir_build* b)
{
    if (!b) return;
    steel_vec_free(&b->sym_tmp);
    memset(b, 0, sizeof(*b));
}

static void steel_hir_module_i_init(steel_hir_module_i* m)
{
    memset(m, 0, sizeof(*m));
    steel_vec_init(&m->symbols, sizeof(steel_hir_symbol));
    steel_vec_init(&m->fns, sizeof(steel_hir_fn));
}

static void steel_hir_module_i_dispose(steel_hir_module_i* m)
{
    size_t nf = steel_vec_len(&m->fns);
    for (size_t i = 0; i < nf; i++)
    {
        steel_hir_fn* f = (steel_hir_fn*)steel_vec_at(&m->fns, i);
        if (f) steel_hir_fn_dispose(f);
    }
    steel_vec_free(&m->fns);
    steel_vec_free(&m->symbols);
    memset(m, 0, sizeof(*m));
}

bool steel_hir_build_module(steel_hir_build* b, steel_ast_id root, steel_hir_module* out)
{
    if (!b || !b->ast || !out) return false;

    // Internal build into our internal module, then shallow-copy to public.
    steel_hir_module_i m;
    steel_hir_module_i_init(&m);

    // Module name best-effort: node name
    const struct steel_ast_node* rn = steel_ast_cget(b->ast, root);
    m.name = (rn && rn->name) ? rn->name : "root";

    // Collect symbols
    steel_vec_clear(&b->sym_tmp);
    steel_collect_symbols_rec(b, root);

    // Copy unique symbol names into module.
    size_t sc = steel_vec_len(&b->sym_tmp);
    for (size_t i = 0; i < sc; i++)
    {
        steel_hir_symbol* s = (steel_hir_symbol*)steel_vec_at(&b->sym_tmp, i);
        if (!s) continue;

        bool exists = false;
        for (size_t j = 0; j < steel_vec_len(&m.symbols); j++)
        {
            steel_hir_symbol* t = (steel_hir_symbol*)steel_vec_at(&m.symbols, j);
            if (t && t->name && s->name && strcmp(t->name, s->name) == 0)
            {
                exists = true;
                break;
            }
        }
        if (!exists)
            (void)steel_vec_push(&m.symbols, s);
    }

    // Lower functions: best-effort heuristic:
    // - treat any node with name and at least one kid as a function.
    // - for more correctness, hook in AST kind filtering here.
    size_t nodes_n = steel_ast_count(b->ast);
    for (steel_ast_id id = 1; id <= (steel_ast_id)nodes_n; id++)
    {
        const struct steel_ast_node* n = steel_ast_cget(b->ast, id);
        if (!n) continue;

        if (n->name && n->name[0] && steel_ast_kid_count(b->ast, id) > 0)
        {
            // Avoid lowering module root itself.
            if (id == root) continue;

            steel_hir_fn fn;
            if (steel_lower_function_like(b, id, &fn))
                (void)steel_vec_push(&m.fns, &fn);
            else
                steel_hir_fn_dispose(&fn);
        }
    }

    // Export out: assume steel_hir_module is layout-compatible with steel_hir_module_i
    // OR that hir_build.h typedefs steel_hir_module to our internal struct.
    // If not compatible, you must adapt in hir_build.h.
    memcpy(out, &m, sizeof(*out));

    // Do not dispose m because we transferred ownership to out.
    return true;
}

// Optional debug dump
static const char* steel_hir_op_name(steel_hir_op op)
{
    switch (op)
    {
        case STEEL_HIR_OP_NOP: return "nop";
        case STEEL_HIR_OP_CONST_I64: return "const_i64";
        case STEEL_HIR_OP_CONST_F64: return "const_f64";
        case STEEL_HIR_OP_CONST_STR: return "const_str";
        case STEEL_HIR_OP_LOCAL_GET: return "local_get";
        case STEEL_HIR_OP_LOCAL_SET: return "local_set";
        case STEEL_HIR_OP_JMP: return "jmp";
        case STEEL_HIR_OP_JMP_IF: return "jmp_if";
        case STEEL_HIR_OP_RET: return "ret";
        case STEEL_HIR_OP_CALL: return "call";
        case STEEL_HIR_OP_ADD: return "add";
        case STEEL_HIR_OP_SUB: return "sub";
        case STEEL_HIR_OP_MUL: return "mul";
        case STEEL_HIR_OP_DIV: return "div";
        case STEEL_HIR_OP_MOD: return "mod";
        case STEEL_HIR_OP_EQ: return "eq";
        case STEEL_HIR_OP_NE: return "ne";
        case STEEL_HIR_OP_LT: return "lt";
        case STEEL_HIR_OP_LE: return "le";
        case STEEL_HIR_OP_GT: return "gt";
        case STEEL_HIR_OP_GE: return "ge";
        case STEEL_HIR_OP_AND: return "and";
        case STEEL_HIR_OP_OR: return "or";
        case STEEL_HIR_OP_NEG: return "neg";
        case STEEL_HIR_OP_NOT: return "not";
        default: return "?";
    }
}

void steel_hir_dump(FILE* out, const steel_hir_module* m)
{
    if (!out) out = stderr;
    if (!m)
    {
        fputs("(hir=null)\n", out);
        return;
    }

    // Assume module layout compatibility.
    const steel_hir_module_i* mi = (const steel_hir_module_i*)m;

    fprintf(out, "(hir module '%s')\n", mi->name ? mi->name : "?");
    fprintf(out, "symbols=%zu fns=%zu\n", steel_vec_len(&mi->symbols), steel_vec_len(&mi->fns));

    for (size_t i = 0; i < steel_vec_len(&mi->fns); i++)
    {
        const steel_hir_fn* f = (const steel_hir_fn*)steel_vec_cat(&mi->fns, i);
        if (!f) continue;

        fprintf(out, "\nfn %s (regs=%u locals=%u insts=%zu)\n",
                f->name ? f->name : "?", f->reg_count, f->local_count, steel_vec_len(&f->insts));

        for (size_t j = 0; j < steel_vec_len(&f->insts); j++)
        {
            const steel_hir_inst* ins = (const steel_hir_inst*)steel_vec_cat(&f->insts, j);
            if (!ins) continue;

            fprintf(out, "  %04zu: %-10s dst=%u a=%u b=%u c=%u",
                    j, steel_hir_op_name(ins->op), ins->dst, ins->a, ins->b, ins->c);

            if (ins->op == STEEL_HIR_OP_CONST_I64) fprintf(out, " imm=%lld", (long long)ins->imm_i64);
            if (ins->op == STEEL_HIR_OP_CONST_F64) fprintf(out, " imm=%g", ins->imm_f64);
            if (ins->op == STEEL_HIR_OP_CONST_STR) fprintf(out, " imm=%s", ins->imm_str ? ins->imm_str : "\"\"");
            if (ins->op == STEEL_HIR_OP_CALL) fprintf(out, " argc=%lld", (long long)ins->imm_i64);

            fputc('\n', out);
        }
    }
}
