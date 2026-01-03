// SPDX-License-Identifier: MIT
// ir_build.c
//
// IR builder (HIR -> IR) for vitte.
//
// This file lowers the (high-level) HIR into a more explicit, backend-friendly IR.
// The IR here is designed to be straightforward for multiple backends (VM, C, etc.):
//  - Explicit basic blocks
//  - Explicit terminators (jmp, br, ret)
//  - SSA-like virtual registers (still untyped here)
//  - Calls, locals, constants
//
// Important:
//  - This implementation is self-contained and does not require full type info.
//  - It is "max" in the sense that it includes robust data structures, builders,
//    validation, and dump tooling, while remaining generic.
//
// Pairing:
//  - ir_build.h should declare:
//      typedef struct steel_ir_build steel_ir_build;
//      typedef struct steel_ir_module steel_ir_module;
//      void steel_ir_build_init(steel_ir_build* b);
//      void steel_ir_build_dispose(steel_ir_build* b);
//      bool steel_ir_build_from_hir(steel_ir_build* b, const steel_hir_module* hir, steel_ir_module* out);
//      void steel_ir_dump(FILE* out, const steel_ir_module* m);
//      bool steel_ir_validate(const steel_ir_module* m);
//
//  - If your project already defines IR structs, adapt by typedefing steel_ir_module
//    to the internal representation below or by copying only the algorithms.
//

#include "ir_build.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "../common/vec.h"

//------------------------------------------------------------------------------
// Internal IR representation
//------------------------------------------------------------------------------

typedef uint32_t steel_ir_id;

typedef enum steel_ir_op
{
    STEEL_IR_OP_NOP = 0,

    // constants
    STEEL_IR_OP_CONST_I64,
    STEEL_IR_OP_CONST_F64,
    STEEL_IR_OP_CONST_STR,

    // locals
    STEEL_IR_OP_LOCAL_GET,
    STEEL_IR_OP_LOCAL_SET,

    // arithmetic / logic (untyped)
    STEEL_IR_OP_ADD,
    STEEL_IR_OP_SUB,
    STEEL_IR_OP_MUL,
    STEEL_IR_OP_DIV,
    STEEL_IR_OP_MOD,

    STEEL_IR_OP_EQ,
    STEEL_IR_OP_NE,
    STEEL_IR_OP_LT,
    STEEL_IR_OP_LE,
    STEEL_IR_OP_GT,
    STEEL_IR_OP_GE,

    STEEL_IR_OP_AND,
    STEEL_IR_OP_OR,

    STEEL_IR_OP_NEG,
    STEEL_IR_OP_NOT,

    // calls
    STEEL_IR_OP_CALL,

} steel_ir_op;

typedef enum steel_ir_term_kind
{
    STEEL_IR_TERM_NONE = 0,
    STEEL_IR_TERM_JMP,
    STEEL_IR_TERM_BR,
    STEEL_IR_TERM_RET,
} steel_ir_term_kind;

typedef struct steel_ir_inst
{
    steel_ir_op op;
    uint32_t dst; // vreg
    uint32_t a;
    uint32_t b;
    uint32_t c;

    int64_t imm_i64;
    double  imm_f64;
    const char* imm_str;

    // optional source span if you have one; keep opaque via u64.
    uint64_t span_lo;
    uint64_t span_hi;
} steel_ir_inst;

typedef struct steel_ir_term
{
    steel_ir_term_kind kind;
    uint32_t a;
    uint32_t b;
    uint32_t c;

    // For BR: a=cond vreg, b=true_bb, c=false_bb
    // For JMP: a=target_bb
    // For RET: a=value vreg (0 if void)

} steel_ir_term;

typedef struct steel_ir_block
{
    uint32_t id; // stable block id
    steel_vec insts; // steel_ir_inst
    steel_ir_term term;
} steel_ir_block;

typedef struct steel_ir_fn
{
    const char* name;

    uint32_t reg_count;
    uint32_t local_count;

    steel_vec blocks; // steel_ir_block

    // local names (optional)
    steel_vec local_names; // const char*

} steel_ir_fn;

typedef struct steel_ir_module_i
{
    const char* name;
    steel_vec fns; // steel_ir_fn
} steel_ir_module_i;

//------------------------------------------------------------------------------
// Builder state
//------------------------------------------------------------------------------

struct steel_ir_build
{
    // scratch
    steel_vec tmp_u32; // uint32_t
};

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

static void steel_ir_block_init(steel_ir_block* b, uint32_t id)
{
    memset(b, 0, sizeof(*b));
    b->id = id;
    steel_vec_init(&b->insts, sizeof(steel_ir_inst));
    b->term.kind = STEEL_IR_TERM_NONE;
}

static void steel_ir_block_dispose(steel_ir_block* b)
{
    steel_vec_free(&b->insts);
    memset(b, 0, sizeof(*b));
}

static void steel_ir_fn_init(steel_ir_fn* f)
{
    memset(f, 0, sizeof(*f));
    steel_vec_init(&f->blocks, sizeof(steel_ir_block));
    steel_vec_init(&f->local_names, sizeof(const char*));
}

static void steel_ir_fn_dispose(steel_ir_fn* f)
{
    size_t nb = steel_vec_len(&f->blocks);
    for (size_t i = 0; i < nb; i++)
    {
        steel_ir_block* b = (steel_ir_block*)steel_vec_at(&f->blocks, i);
        if (b) steel_ir_block_dispose(b);
    }
    steel_vec_free(&f->blocks);
    steel_vec_free(&f->local_names);
    memset(f, 0, sizeof(*f));
}

static void steel_ir_module_i_init(steel_ir_module_i* m)
{
    memset(m, 0, sizeof(*m));
    steel_vec_init(&m->fns, sizeof(steel_ir_fn));
}

static void steel_ir_module_i_dispose(steel_ir_module_i* m)
{
    size_t nf = steel_vec_len(&m->fns);
    for (size_t i = 0; i < nf; i++)
    {
        steel_ir_fn* f = (steel_ir_fn*)steel_vec_at(&m->fns, i);
        if (f) steel_ir_fn_dispose(f);
    }
    steel_vec_free(&m->fns);
    memset(m, 0, sizeof(*m));
}

static uint32_t steel_ir_new_reg(steel_ir_fn* f)
{
    f->reg_count++;
    return f->reg_count;
}

static uint32_t steel_ir_new_local(steel_ir_fn* f, const char* name)
{
    uint32_t idx = f->local_count;
    f->local_count++;
    (void)steel_vec_push(&f->local_names, &name);
    return idx;
}

static steel_ir_block* steel_ir_add_block(steel_ir_fn* f)
{
    uint32_t id = (uint32_t)steel_vec_len(&f->blocks);
    steel_ir_block b;
    steel_ir_block_init(&b, id);
    return (steel_ir_block*)steel_vec_push(&f->blocks, &b);
}

static bool steel_ir_emit(steel_ir_block* bb, steel_ir_inst ins)
{
    return steel_vec_push(&bb->insts, &ins) != NULL;
}

static steel_ir_inst steel_ir_inst_make(steel_ir_op op)
{
    steel_ir_inst i;
    memset(&i, 0, sizeof(i));
    i.op = op;
    return i;
}

static void steel_ir_set_term_jmp(steel_ir_block* bb, uint32_t target)
{
    bb->term.kind = STEEL_IR_TERM_JMP;
    bb->term.a = target;
}

static void steel_ir_set_term_br(steel_ir_block* bb, uint32_t cond, uint32_t tbb, uint32_t fbb)
{
    bb->term.kind = STEEL_IR_TERM_BR;
    bb->term.a = cond;
    bb->term.b = tbb;
    bb->term.c = fbb;
}

static void steel_ir_set_term_ret(steel_ir_block* bb, uint32_t v)
{
    bb->term.kind = STEEL_IR_TERM_RET;
    bb->term.a = v;
}

//------------------------------------------------------------------------------
// HIR compatibility (minimal)
//------------------------------------------------------------------------------

// We only assume the HIR module contains fns with instruction streams similar
// to the one produced by hir_build.c we generated earlier.

typedef enum steel_hir_op
{
    STEEL_HIR_OP_NOP = 0,
    STEEL_HIR_OP_CONST_I64,
    STEEL_HIR_OP_CONST_F64,
    STEEL_HIR_OP_CONST_STR,
    STEEL_HIR_OP_LOCAL_GET,
    STEEL_HIR_OP_LOCAL_SET,
    STEEL_HIR_OP_JMP,
    STEEL_HIR_OP_JMP_IF,
    STEEL_HIR_OP_RET,
    STEEL_HIR_OP_CALL,
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
    STEEL_HIR_OP_NEG,
    STEEL_HIR_OP_NOT,
} steel_hir_op;

typedef struct steel_hir_inst
{
    steel_hir_op op;
    uint32_t     dst;
    uint32_t     a;
    uint32_t     b;
    uint32_t     c;
    int64_t      imm_i64;
    double       imm_f64;
    const char*  imm_str;
    uint64_t     span_lo;
    uint64_t     span_hi;
} steel_hir_inst;

typedef struct steel_hir_fn
{
    const char* name;
    uint32_t reg_count;
    uint32_t local_count;
    steel_vec blocks; // unused here
    steel_vec insts;  // steel_hir_inst
    steel_vec local_names; // const char*
} steel_hir_fn;

typedef struct steel_hir_module_i
{
    const char* name;
    steel_vec symbols; // unused
    steel_vec fns; // steel_hir_fn
} steel_hir_module_i;

static steel_ir_op steel_map_hir_op(steel_hir_op op)
{
    switch (op)
    {
        case STEEL_HIR_OP_CONST_I64: return STEEL_IR_OP_CONST_I64;
        case STEEL_HIR_OP_CONST_F64: return STEEL_IR_OP_CONST_F64;
        case STEEL_HIR_OP_CONST_STR: return STEEL_IR_OP_CONST_STR;
        case STEEL_HIR_OP_LOCAL_GET: return STEEL_IR_OP_LOCAL_GET;
        case STEEL_HIR_OP_LOCAL_SET: return STEEL_IR_OP_LOCAL_SET;
        case STEEL_HIR_OP_CALL: return STEEL_IR_OP_CALL;

        case STEEL_HIR_OP_ADD: return STEEL_IR_OP_ADD;
        case STEEL_HIR_OP_SUB: return STEEL_IR_OP_SUB;
        case STEEL_HIR_OP_MUL: return STEEL_IR_OP_MUL;
        case STEEL_HIR_OP_DIV: return STEEL_IR_OP_DIV;
        case STEEL_HIR_OP_MOD: return STEEL_IR_OP_MOD;

        case STEEL_HIR_OP_EQ: return STEEL_IR_OP_EQ;
        case STEEL_HIR_OP_NE: return STEEL_IR_OP_NE;
        case STEEL_HIR_OP_LT: return STEEL_IR_OP_LT;
        case STEEL_HIR_OP_LE: return STEEL_IR_OP_LE;
        case STEEL_HIR_OP_GT: return STEEL_IR_OP_GT;
        case STEEL_HIR_OP_GE: return STEEL_IR_OP_GE;

        case STEEL_HIR_OP_AND: return STEEL_IR_OP_AND;
        case STEEL_HIR_OP_OR: return STEEL_IR_OP_OR;
        case STEEL_HIR_OP_NEG: return STEEL_IR_OP_NEG;
        case STEEL_HIR_OP_NOT: return STEEL_IR_OP_NOT;

        default: return STEEL_IR_OP_NOP;
    }
}

//------------------------------------------------------------------------------
// Lowering
//------------------------------------------------------------------------------

static bool steel_lower_hir_fn_to_ir(const steel_hir_fn* hf, steel_ir_fn* out)
{
    steel_ir_fn_init(out);

    out->name = hf->name ? hf->name : "_fn";
    out->reg_count = 0;
    out->local_count = 0;

    // Copy locals (optional)
    size_t ln = steel_vec_len(&hf->local_names);
    for (size_t i = 0; i < ln; i++)
    {
        const char** pn = (const char**)steel_vec_cat(&hf->local_names, i);
        const char* name = (pn && *pn) ? *pn : NULL;
        (void)steel_ir_new_local(out, name);
    }

    // Create one block initially
    steel_ir_block* bb = steel_ir_add_block(out);
    if (!bb) return false;

    // Basic blockization heuristic:
    // - We split blocks at HIR JMP/JMP_IF/RET targets (which are instruction indices).
    // - To keep this generic and robust, we compute a set of leader indices.

    // Collect leader indices
    steel_vec leaders; // uint32_t
    steel_vec_init(&leaders, sizeof(uint32_t));

    uint32_t zero = 0;
    (void)steel_vec_push(&leaders, &zero);

    size_t hi_n = steel_vec_len(&hf->insts);
    for (size_t i = 0; i < hi_n; i++)
    {
        const steel_hir_inst* in = (const steel_hir_inst*)steel_vec_cat(&hf->insts, i);
        if (!in) continue;

        if (in->op == STEEL_HIR_OP_JMP)
        {
            uint32_t tgt = in->a;
            (void)steel_vec_push(&leaders, &tgt);
            uint32_t next = (uint32_t)(i + 1);
            if (next < (uint32_t)hi_n) (void)steel_vec_push(&leaders, &next);
        }
        else if (in->op == STEEL_HIR_OP_JMP_IF)
        {
            uint32_t tgt = in->b;
            (void)steel_vec_push(&leaders, &tgt);
            uint32_t next = (uint32_t)(i + 1);
            if (next < (uint32_t)hi_n) (void)steel_vec_push(&leaders, &next);
        }
        else if (in->op == STEEL_HIR_OP_RET)
        {
            uint32_t next = (uint32_t)(i + 1);
            if (next < (uint32_t)hi_n) (void)steel_vec_push(&leaders, &next);
        }
    }

    // Sort and unique leaders (simple O(n^2) since typical sizes are small)
    // If your project has a sort util, replace.
    size_t L = steel_vec_len(&leaders);
    for (size_t i = 0; i < L; i++)
    {
        uint32_t* a = (uint32_t*)steel_vec_at(&leaders, i);
        for (size_t j = i + 1; j < L; j++)
        {
            uint32_t* b = (uint32_t*)steel_vec_at(&leaders, j);
            if (a && b && *b < *a)
            {
                uint32_t tmp = *a;
                *a = *b;
                *b = tmp;
            }
        }
    }

    // unique
    steel_vec uniq; // uint32_t
    steel_vec_init(&uniq, sizeof(uint32_t));
    uint32_t last = UINT32_MAX;
    for (size_t i = 0; i < steel_vec_len(&leaders); i++)
    {
        const uint32_t* v = (const uint32_t*)steel_vec_cat(&leaders, i);
        if (!v) continue;
        if (*v == last) continue;
        last = *v;
        (void)steel_vec_push(&uniq, v);
    }
    steel_vec_free(&leaders);

    // Build mapping inst_index -> block_id
    // We store block start indices in uniq.
    size_t nb = steel_vec_len(&uniq);
    if (nb == 0)
    {
        steel_vec_free(&uniq);
        return true;
    }

    // Create additional blocks
    for (size_t i = 1; i < nb; i++)
        (void)steel_ir_add_block(out);

    // For each block, lower instructions in its range
    for (size_t bi = 0; bi < nb; bi++)
    {
        const uint32_t* startp = (const uint32_t*)steel_vec_cat(&uniq, bi);
        uint32_t start = startp ? *startp : 0;
        uint32_t end = (uint32_t)hi_n;
        if (bi + 1 < nb)
        {
            const uint32_t* endp = (const uint32_t*)steel_vec_cat(&uniq, bi + 1);
            end = endp ? *endp : end;
        }

        steel_ir_block* bb2 = (steel_ir_block*)steel_vec_at(&out->blocks, bi);
        if (!bb2) continue;

        // Lower insts in [start,end)
        for (uint32_t ii = start; ii < end; ii++)
        {
            const steel_hir_inst* hin = (const steel_hir_inst*)steel_vec_cat(&hf->insts, ii);
            if (!hin) continue;

            if (hin->op == STEEL_HIR_OP_JMP)
            {
                // terminator
                // target is instruction index -> convert to block id
                uint32_t tgt_idx = hin->a;
                // find block id by scanning uniq
                uint32_t tgt_bb = 0;
                for (size_t k = 0; k < nb; k++)
                {
                    const uint32_t* sp = (const uint32_t*)steel_vec_cat(&uniq, k);
                    if (sp && *sp == tgt_idx) { tgt_bb = (uint32_t)k; break; }
                }
                steel_ir_set_term_jmp(bb2, tgt_bb);
                break;
            }
            if (hin->op == STEEL_HIR_OP_JMP_IF)
            {
                // cond in a, target in b, fallthrough => next block
                uint32_t tgt_idx = hin->b;
                uint32_t tbb = 0;
                for (size_t k = 0; k < nb; k++)
                {
                    const uint32_t* sp = (const uint32_t*)steel_vec_cat(&uniq, k);
                    if (sp && *sp == tgt_idx) { tbb = (uint32_t)k; break; }
                }

                uint32_t fbb = (bi + 1 < nb) ? (uint32_t)(bi + 1) : (uint32_t)bi;
                steel_ir_set_term_br(bb2, hin->a, tbb, fbb);
                break;
            }
            if (hin->op == STEEL_HIR_OP_RET)
            {
                steel_ir_set_term_ret(bb2, hin->a);
                break;
            }

            steel_ir_inst ins = steel_ir_inst_make(steel_map_hir_op(hin->op));
            ins.dst = hin->dst;
            ins.a = hin->a;
            ins.b = hin->b;
            ins.c = hin->c;
            ins.imm_i64 = hin->imm_i64;
            ins.imm_f64 = hin->imm_f64;
            ins.imm_str = hin->imm_str;
            ins.span_lo = hin->span_lo;
            ins.span_hi = hin->span_hi;

            (void)steel_ir_emit(bb2, ins);
            if (ins.dst > out->reg_count) out->reg_count = ins.dst;
        }

        // If no terminator, fallthrough to next block or ret void
        if (bb2->term.kind == STEEL_IR_TERM_NONE)
        {
            if (bi + 1 < nb)
                steel_ir_set_term_jmp(bb2, (uint32_t)(bi + 1));
            else
                steel_ir_set_term_ret(bb2, 0);
        }
    }

    steel_vec_free(&uniq);

    return true;
}

//------------------------------------------------------------------------------
// Validation
//------------------------------------------------------------------------------

static bool steel_ir_validate_fn(const steel_ir_fn* f)
{
    if (!f) return false;
    size_t nb = steel_vec_len(&f->blocks);
    if (nb == 0) return false;

    for (size_t i = 0; i < nb; i++)
    {
        const steel_ir_block* bb = (const steel_ir_block*)steel_vec_cat(&f->blocks, i);
        if (!bb) return false;

        if (bb->term.kind == STEEL_IR_TERM_NONE)
            return false;

        if (bb->term.kind == STEEL_IR_TERM_JMP)
        {
            if (bb->term.a >= nb) return false;
        }
        else if (bb->term.kind == STEEL_IR_TERM_BR)
        {
            if (bb->term.b >= nb) return false;
            if (bb->term.c >= nb) return false;
        }
        // RET ok
    }

    return true;
}

bool steel_ir_validate(const steel_ir_module* m)
{
    if (!m) return false;
    const steel_ir_module_i* mi = (const steel_ir_module_i*)m;

    size_t nf = steel_vec_len(&mi->fns);
    for (size_t i = 0; i < nf; i++)
    {
        const steel_ir_fn* f = (const steel_ir_fn*)steel_vec_cat(&mi->fns, i);
        if (!steel_ir_validate_fn(f))
            return false;
    }

    return true;
}

//------------------------------------------------------------------------------
// Dump
//------------------------------------------------------------------------------

static const char* steel_ir_op_name(steel_ir_op op)
{
    switch (op)
    {
        case STEEL_IR_OP_NOP: return "nop";
        case STEEL_IR_OP_CONST_I64: return "const_i64";
        case STEEL_IR_OP_CONST_F64: return "const_f64";
        case STEEL_IR_OP_CONST_STR: return "const_str";
        case STEEL_IR_OP_LOCAL_GET: return "local_get";
        case STEEL_IR_OP_LOCAL_SET: return "local_set";
        case STEEL_IR_OP_ADD: return "add";
        case STEEL_IR_OP_SUB: return "sub";
        case STEEL_IR_OP_MUL: return "mul";
        case STEEL_IR_OP_DIV: return "div";
        case STEEL_IR_OP_MOD: return "mod";
        case STEEL_IR_OP_EQ: return "eq";
        case STEEL_IR_OP_NE: return "ne";
        case STEEL_IR_OP_LT: return "lt";
        case STEEL_IR_OP_LE: return "le";
        case STEEL_IR_OP_GT: return "gt";
        case STEEL_IR_OP_GE: return "ge";
        case STEEL_IR_OP_AND: return "and";
        case STEEL_IR_OP_OR: return "or";
        case STEEL_IR_OP_NEG: return "neg";
        case STEEL_IR_OP_NOT: return "not";
        case STEEL_IR_OP_CALL: return "call";
        default: return "?";
    }
}

static void steel_ir_dump_term(FILE* out, const steel_ir_term* t)
{
    switch (t->kind)
    {
        case STEEL_IR_TERM_JMP:
            fprintf(out, "    term: jmp bb%u\n", t->a);
            break;
        case STEEL_IR_TERM_BR:
            fprintf(out, "    term: br r%u bb%u bb%u\n", t->a, t->b, t->c);
            break;
        case STEEL_IR_TERM_RET:
            fprintf(out, "    term: ret r%u\n", t->a);
            break;
        default:
            fprintf(out, "    term: (none)\n");
            break;
    }
}

void steel_ir_dump(FILE* out, const steel_ir_module* m)
{
    if (!out) out = stderr;
    if (!m)
    {
        fputs("(ir=null)\n", out);
        return;
    }

    const steel_ir_module_i* mi = (const steel_ir_module_i*)m;
    fprintf(out, "(ir module '%s') fns=%zu\n", mi->name ? mi->name : "?", steel_vec_len(&mi->fns));

    for (size_t fi = 0; fi < steel_vec_len(&mi->fns); fi++)
    {
        const steel_ir_fn* f = (const steel_ir_fn*)steel_vec_cat(&mi->fns, fi);
        if (!f) continue;

        fprintf(out, "\nfn %s (regs=%u locals=%u blocks=%zu)\n",
                f->name ? f->name : "?", f->reg_count, f->local_count, steel_vec_len(&f->blocks));

        for (size_t bi = 0; bi < steel_vec_len(&f->blocks); bi++)
        {
            const steel_ir_block* bb = (const steel_ir_block*)steel_vec_cat(&f->blocks, bi);
            if (!bb) continue;

            fprintf(out, "  bb%u:\n", bb->id);

            for (size_t ii = 0; ii < steel_vec_len(&bb->insts); ii++)
            {
                const steel_ir_inst* ins = (const steel_ir_inst*)steel_vec_cat(&bb->insts, ii);
                if (!ins) continue;

                fprintf(out, "    %04zu: %-10s dst=%u a=%u b=%u c=%u",
                        ii, steel_ir_op_name(ins->op), ins->dst, ins->a, ins->b, ins->c);

                if (ins->op == STEEL_IR_OP_CONST_I64) fprintf(out, " imm=%lld", (long long)ins->imm_i64);
                if (ins->op == STEEL_IR_OP_CONST_F64) fprintf(out, " imm=%g", ins->imm_f64);
                if (ins->op == STEEL_IR_OP_CONST_STR) fprintf(out, " imm=%s", ins->imm_str ? ins->imm_str : "\"\"");
                if (ins->op == STEEL_IR_OP_CALL) fprintf(out, " argc=%lld", (long long)ins->imm_i64);

                fputc('\n', out);
            }

            steel_ir_dump_term(out, &bb->term);
        }
    }
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void steel_ir_build_init(steel_ir_build* b)
{
    if (!b) return;
    memset(b, 0, sizeof(*b));
    steel_vec_init(&b->tmp_u32, sizeof(uint32_t));
}

void steel_ir_build_dispose(steel_ir_build* b)
{
    if (!b) return;
    steel_vec_free(&b->tmp_u32);
    memset(b, 0, sizeof(*b));
}

bool steel_ir_build_from_hir(steel_ir_build* b, const steel_hir_module* hir, steel_ir_module* out)
{
    (void)b;
    if (!hir || !out) return false;

    const steel_hir_module_i* hm = (const steel_hir_module_i*)hir;

    steel_ir_module_i m;
    steel_ir_module_i_init(&m);
    m.name = hm->name ? hm->name : "root";

    size_t nf = steel_vec_len(&hm->fns);
    for (size_t i = 0; i < nf; i++)
    {
        const steel_hir_fn* hf = (const steel_hir_fn*)steel_vec_cat(&hm->fns, i);
        if (!hf) continue;

        steel_ir_fn f;
        if (!steel_lower_hir_fn_to_ir(hf, &f))
        {
            steel_ir_fn_dispose(&f);
            steel_ir_module_i_dispose(&m);
            return false;
        }

        (void)steel_vec_push(&m.fns, &f);
    }

    memcpy(out, &m, sizeof(*out));
    return true;
}
