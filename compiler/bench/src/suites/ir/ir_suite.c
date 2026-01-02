

// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// ir_suite.c
// -----------------------------------------------------------------------------
// Benchmark suite: IR construction + analysis / transformation shaped workloads.
//
// This suite is compiler-independent: it defines a small synthetic IR and runs
// representative hot paths:
//   - build IR from deterministic pseudo-random stream
//   - linear IR walk (hashing / simulated dataflow)
//   - CFG traversal (BFS)
//   - DCE-like liveness propagation (value graph)
//   - serialization / pretty-print (string building)
//
// The purpose is to stress memory bandwidth, pointer chasing, and typical IR
// algorithms without linking to the real compiler.
// -----------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Bench API (adapt here if your harness differs)
// -----------------------------------------------------------------------------
#ifndef VITTE_BENCH_IR_SUITE_API
#define VITTE_BENCH_IR_SUITE_API 1

typedef void (*bench_fn)(uint64_t iters, void* user);

typedef struct bench_case
{
    const char* name;
    bench_fn fn;
    void* user;
} bench_case_t;

typedef struct bench_suite
{
    const char* name;
    const bench_case_t* cases;
    size_t case_count;
} bench_suite_t;

// Provided by the bench runner.
void bench_register_suite(const bench_suite_t* suite);

#endif

#if defined(__GNUC__) || defined(__clang__)
#define VITTE_BENCH_CONSTRUCTOR __attribute__((constructor))
#else
#define VITTE_BENCH_CONSTRUCTOR
#endif

// -----------------------------------------------------------------------------
// Blackhole
// -----------------------------------------------------------------------------
static volatile uint64_t g_sink_u64;
static volatile uint8_t  g_sink_u8;

static inline void bench_blackhole_u64(uint64_t v)
{
    g_sink_u64 ^= (v + 0x9e3779b97f4a7c15ULL) ^ (g_sink_u64 << 7) ^ (g_sink_u64 >> 3);
}

static inline void bench_blackhole_bytes(const void* p, size_t n)
{
    const uint8_t* b = (const uint8_t*)p;
    uint8_t acc = (uint8_t)g_sink_u8;
    if (n)
    {
        acc ^= b[0];
        acc ^= b[n >> 1];
        acc ^= b[n - 1];
    }
    g_sink_u8 = (uint8_t)(acc + 0x2b);
}

// -----------------------------------------------------------------------------
// Deterministic RNG
// -----------------------------------------------------------------------------
static inline uint64_t xorshift64(uint64_t* s)
{
    uint64_t x = *s;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *s = x;
    return x;
}

static inline uint32_t u32_range(uint64_t* s, uint32_t lo, uint32_t hi)
{
    const uint32_t span = (hi > lo) ? (hi - lo) : 1u;
    return lo + (uint32_t)(xorshift64(s) % span);
}

static inline uint64_t fnv1a64(const void* data, size_t n)
{
    const uint8_t* p = (const uint8_t*)data;
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < n; ++i)
    {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

// -----------------------------------------------------------------------------
// Tiny string builder
// -----------------------------------------------------------------------------
typedef struct strbuf
{
    char*  p;
    size_t cap;
    size_t len;
} strbuf_t;

static void sb_init(strbuf_t* sb, char* dst, size_t cap)
{
    sb->p = dst;
    sb->cap = cap;
    sb->len = 0;
    if (cap) sb->p[0] = '\0';
}

static inline void sb_putc(strbuf_t* sb, char c)
{
    if (sb->len + 1 >= sb->cap) return;
    sb->p[sb->len++] = c;
    sb->p[sb->len] = '\0';
}

static inline void sb_puts(strbuf_t* sb, const char* s)
{
    if (!s) return;
    const size_t n = strlen(s);
    const size_t rem = (sb->cap > sb->len) ? (sb->cap - sb->len) : 0;
    if (rem <= 1) return;
    const size_t w = (n < rem - 1) ? n : (rem - 1);
    memcpy(sb->p + sb->len, s, w);
    sb->len += w;
    sb->p[sb->len] = '\0';
}

static inline void sb_u32(strbuf_t* sb, uint32_t v)
{
    char tmp[16];
    snprintf(tmp, sizeof(tmp), "%u", (unsigned)v);
    sb_puts(sb, tmp);
}

// -----------------------------------------------------------------------------
// Synthetic IR model
// -----------------------------------------------------------------------------
// Value ids are 1..value_count-1. Value 0 means "none".
// Each instruction optionally defines a destination value (dst != 0).
//
// Opcodes are selected to create a mix of:
//   - pure ops (add/mul/cmp)
//   - memory-ish ops (load/store)
//   - control flow (br/jmp/ret)
//   - call-ish side effect

typedef enum ir_op
{
    IR_NOP = 0,

    IR_CONST,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_AND,
    IR_OR,
    IR_XOR,
    IR_SHL,
    IR_SHR,
    IR_CMP,

    IR_PHI,
    IR_LOAD,
    IR_STORE,
    IR_CALL,

    IR_JMP,
    IR_BR,
    IR_RET,
} ir_op_t;

typedef struct ir_inst
{
    ir_op_t op;
    uint16_t flags;
    uint32_t dst;     // value id, 0 if none
    uint32_t a;       // operand value id
    uint32_t b;       // operand value id
    uint32_t imm;     // immediate / target / extra
} ir_inst_t;

typedef struct ir_block
{
    uint32_t first_inst;
    uint32_t inst_count;

    uint32_t succ0;
    uint32_t succ1;
} ir_block_t;

typedef struct ir_func
{
    ir_block_t* blocks;
    uint32_t block_count;

    ir_inst_t* insts;
    uint32_t inst_count;

    uint32_t value_count;
} ir_func_t;

static inline bool ir_op_has_side_effect(ir_op_t op)
{
    return (op == IR_STORE) || (op == IR_CALL);
}

static inline bool ir_op_is_terminator(ir_op_t op)
{
    return (op == IR_JMP) || (op == IR_BR) || (op == IR_RET);
}

static const char* ir_op_name(ir_op_t op)
{
    switch (op)
    {
        case IR_NOP: return "nop";
        case IR_CONST: return "const";
        case IR_ADD: return "add";
        case IR_SUB: return "sub";
        case IR_MUL: return "mul";
        case IR_DIV: return "div";
        case IR_AND: return "and";
        case IR_OR: return "or";
        case IR_XOR: return "xor";
        case IR_SHL: return "shl";
        case IR_SHR: return "shr";
        case IR_CMP: return "cmp";
        case IR_PHI: return "phi";
        case IR_LOAD: return "load";
        case IR_STORE: return "store";
        case IR_CALL: return "call";
        case IR_JMP: return "jmp";
        case IR_BR: return "br";
        case IR_RET: return "ret";
        default: return "?";
    }
}

// -----------------------------------------------------------------------------
// Fixture (pre-allocated buffers)
// -----------------------------------------------------------------------------

typedef struct ir_fixture
{
    // capacity
    uint32_t cap_blocks;
    uint32_t cap_insts;
    uint32_t cap_values;

    // storage
    ir_block_t* blocks;
    ir_inst_t* insts;

    // analysis scratch
    uint32_t* def_inst;     // value -> inst index or UINT32_MAX
    uint8_t*  live;         // value -> 0/1
    uint32_t* stack;        // value work stack

    // cfg scratch
    uint8_t*  vis_blocks;
    uint32_t* q_blocks;

    bool inited;
} ir_fixture_t;

static ir_fixture_t g_fx;
static bool g_fx_inited;

static void fixture_init(ir_fixture_t* fx, uint32_t cap_blocks, uint32_t cap_insts, uint32_t cap_values)
{
    memset(fx, 0, sizeof(*fx));

    fx->cap_blocks = cap_blocks;
    fx->cap_insts = cap_insts;
    fx->cap_values = cap_values;

    fx->blocks = (ir_block_t*)malloc(sizeof(ir_block_t) * (size_t)cap_blocks);
    fx->insts  = (ir_inst_t*)malloc(sizeof(ir_inst_t) * (size_t)cap_insts);

    fx->def_inst = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)cap_values);
    fx->live     = (uint8_t*) malloc(sizeof(uint8_t)  * (size_t)cap_values);
    fx->stack    = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)cap_values);

    fx->vis_blocks = (uint8_t*) malloc(sizeof(uint8_t)  * (size_t)cap_blocks);
    fx->q_blocks   = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)cap_blocks);

    if (!fx->blocks || !fx->insts || !fx->def_inst || !fx->live || !fx->stack || !fx->vis_blocks || !fx->q_blocks)
        abort();

    fx->inited = true;
}

static void fixture_destroy(ir_fixture_t* fx)
{
    if (!fx->inited) return;

    free(fx->blocks);
    free(fx->insts);
    free(fx->def_inst);
    free(fx->live);
    free(fx->stack);
    free(fx->vis_blocks);
    free(fx->q_blocks);

    memset(fx, 0, sizeof(*fx));
}

static void ensure_fixture(void)
{
    if (g_fx_inited) return;

    // Sizing to cover all cases below.
    // Values are roughly <= insts + some slack.
    fixture_init(&g_fx,
                 8192,          // blocks
                 1u << 20,       // insts: 1,048,576
                 (1u << 20) + 8, // values
                 );

    g_fx_inited = true;
}

// -----------------------------------------------------------------------------
// IR builder
// -----------------------------------------------------------------------------

typedef struct ir_build_cfg
{
    uint32_t blocks;
    uint32_t insts_per_block;
    uint32_t seed;
} ir_build_cfg_t;

static inline ir_op_t pick_pure_op(uint64_t* rng)
{
    // A mix of ALU-ish ops.
    static const ir_op_t ops[] = {
        IR_ADD, IR_SUB, IR_MUL, IR_DIV,
        IR_AND, IR_OR, IR_XOR,
        IR_SHL, IR_SHR,
        IR_CMP,
    };
    return ops[(size_t)(xorshift64(rng) % (sizeof(ops) / sizeof(ops[0])))] ;
}

static void ir_build_func(ir_func_t* f, ir_fixture_t* fx, const ir_build_cfg_t* cfg)
{
    const uint32_t bc = cfg->blocks;
    const uint32_t ipb = cfg->insts_per_block;
    const uint32_t ic = bc * ipb;

    if (bc > fx->cap_blocks || ic > fx->cap_insts)
        abort();

    f->blocks = fx->blocks;
    f->block_count = bc;
    f->insts = fx->insts;
    f->inst_count = ic;

    // Values: one dst per non-terminator + constants; keep headroom.
    f->value_count = 1;

    // Zero only the used region.
    memset(f->blocks, 0, sizeof(ir_block_t) * (size_t)bc);
    memset(f->insts,  0, sizeof(ir_inst_t)  * (size_t)ic);

    uint64_t rng = 0x9BADC0DE12345678ULL ^ (uint64_t)cfg->seed;

    uint32_t inst_index = 0;

    for (uint32_t bi = 0; bi < bc; ++bi)
    {
        ir_block_t* b = &f->blocks[bi];
        b->first_inst = inst_index;
        b->inst_count = ipb;

        // Wire successors: mostly linear with occasional back-edge.
        if (bi + 1u < bc)
        {
            b->succ0 = bi + 1u;
            b->succ1 = bi + 1u;
        }
        else
        {
            b->succ0 = 0;
            b->succ1 = 0;
        }

        if ((xorshift64(&rng) & 63u) == 0u && bi > 2)
        {
            // rare back-edge
            const uint32_t back = u32_range(&rng, 0, bi);
            b->succ0 = back;
        }

        // Per-block: seed a couple of constants.
        const uint32_t local_consts = 1u + (uint32_t)(xorshift64(&rng) & 3u);
        uint32_t const_vals[8];
        const uint32_t cv_count = (local_consts < 8u) ? local_consts : 8u;

        for (uint32_t k = 0; k < cv_count; ++k)
        {
            ir_inst_t* ins = &f->insts[inst_index++];
            ins->op = IR_CONST;
            ins->dst = f->value_count++;
            ins->imm = (uint32_t)(xorshift64(&rng) & 0xFFFFu);
            const_vals[k] = ins->dst;
        }

        // Fill the rest except leave 1 terminator slot at end.
        const uint32_t body_slots = (ipb > 1) ? (ipb - 1) : 0;
        uint32_t filled = cv_count;

        while (filled < body_slots)
        {
            ir_inst_t* ins = &f->insts[inst_index++];

            // Mix in memory and calls sometimes.
            const uint32_t r = (uint32_t)(xorshift64(&rng) & 31u);
            if (r == 0)
                ins->op = IR_CALL;
            else if (r == 1)
                ins->op = IR_STORE;
            else if (r == 2)
                ins->op = IR_LOAD;
            else if (r == 3)
                ins->op = IR_PHI;
            else
                ins->op = pick_pure_op(&rng);

            // dst for ops that define a value.
            if (ins->op != IR_STORE)
                ins->dst = f->value_count++;
            else
                ins->dst = 0;

            // pick operands.
            const uint32_t maxv = (f->value_count > 1) ? (f->value_count - 1) : 1;
            const uint32_t pick = u32_range(&rng, 0, 4);

            if (pick == 0 && cv_count)
                ins->a = const_vals[xorshift64(&rng) % cv_count];
            else
                ins->a = u32_range(&rng, 1, maxv + 1u);

            if (pick == 1 && cv_count)
                ins->b = const_vals[xorshift64(&rng) % cv_count];
            else
                ins->b = u32_range(&rng, 1, maxv + 1u);

            ins->imm = (uint32_t)(xorshift64(&rng) & 0xFFFFu);

            filled++;
        }

        // Terminator
        {
            ir_inst_t* t = &f->insts[inst_index++];
            const uint32_t tr = (uint32_t)(xorshift64(&rng) & 7u);
            if (bi + 1u >= bc)
            {
                t->op = IR_RET;
                t->dst = 0;
                // return something
                if (f->value_count > 1)
                    t->a = u32_range(&rng, 1, f->value_count);
            }
            else if (tr == 0)
            {
                t->op = IR_JMP;
                t->imm = b->succ0;
            }
            else
            {
                t->op = IR_BR;
                // condition
                if (f->value_count > 1)
                    t->a = u32_range(&rng, 1, f->value_count);
                // targets
                t->imm = b->succ0;
                t->b   = b->succ1;
            }
        }

        // A last sanity: ensure block inst_count matches.
        // Because we emitted constants within the same block, the block's inst_count
        // still counts them (we always emit exactly ipb insts per block).
    }

    // cap values
    if (f->value_count > fx->cap_values)
        abort();
}

static uint64_t ir_hash_func(const ir_func_t* f)
{
    // Hash blocks + insts as a stability check.
    uint64_t h = 1469598103934665603ULL;

    h ^= fnv1a64(f->blocks, sizeof(ir_block_t) * (size_t)f->block_count);
    h *= 1099511628211ULL;

    h ^= fnv1a64(f->insts, sizeof(ir_inst_t) * (size_t)f->inst_count);
    h *= 1099511628211ULL;

    h ^= (uint64_t)f->value_count;
    h *= 1099511628211ULL;

    return h;
}

// -----------------------------------------------------------------------------
// Passes
// -----------------------------------------------------------------------------
static uint64_t ir_walk_hash(const ir_func_t* f)
{
    // Linear scan that touches fields to stress cache.
    uint64_t h = 0xCBF29CE484222325ULL;
    for (uint32_t i = 0; i < f->inst_count; ++i)
    {
        const ir_inst_t* in = &f->insts[i];
        const uint64_t mix = ((uint64_t)in->op) ^ ((uint64_t)in->dst << 17) ^ ((uint64_t)in->a << 7) ^ ((uint64_t)in->b << 3) ^ (uint64_t)in->imm;
        h ^= (mix + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    }
    return h;
}

static uint32_t ir_cfg_bfs(const ir_func_t* f, ir_fixture_t* fx)
{
    memset(fx->vis_blocks, 0, (size_t)f->block_count);

    uint32_t qh = 0, qt = 0;
    fx->q_blocks[qt++] = 0;
    fx->vis_blocks[0] = 1;

    uint32_t visited = 0;

    while (qh < qt)
    {
        const uint32_t bidx = fx->q_blocks[qh++];
        visited++;

        const ir_block_t* b = &f->blocks[bidx];
        const uint32_t s0 = b->succ0;
        const uint32_t s1 = b->succ1;

        if (s0 < f->block_count && !fx->vis_blocks[s0])
        {
            fx->vis_blocks[s0] = 1;
            fx->q_blocks[qt++] = s0;
        }
        if (s1 < f->block_count && !fx->vis_blocks[s1])
        {
            fx->vis_blocks[s1] = 1;
            fx->q_blocks[qt++] = s1;
        }
    }

    return visited;
}

static uint32_t ir_dce_like(const ir_func_t* f, ir_fixture_t* fx)
{
    // Build def map: value -> defining inst (or UINT32_MAX).
    for (uint32_t i = 0; i < f->value_count; ++i)
        fx->def_inst[i] = UINT32_MAX;

    for (uint32_t i = 0; i < f->inst_count; ++i)
    {
        const ir_inst_t* in = &f->insts[i];
        if (in->dst && in->dst < f->value_count)
            fx->def_inst[in->dst] = i;
    }

    memset(fx->live, 0, (size_t)f->value_count);

    // Seed worklist with values required by side effects and terminators.
    uint32_t sp = 0;
    for (uint32_t i = 0; i < f->inst_count; ++i)
    {
        const ir_inst_t* in = &f->insts[i];
        if (ir_op_has_side_effect(in->op) || ir_op_is_terminator(in->op))
        {
            if (in->a && in->a < f->value_count) fx->stack[sp++] = in->a;
            if (in->b && in->b < f->value_count) fx->stack[sp++] = in->b;
        }
    }

    // Propagate: live value => its defining operands become live.
    while (sp)
    {
        const uint32_t v = fx->stack[--sp];
        if (!v || v >= f->value_count) continue;
        if (fx->live[v]) continue;
        fx->live[v] = 1;

        const uint32_t di = fx->def_inst[v];
        if (di == UINT32_MAX) continue;
        const ir_inst_t* def = &f->insts[di];

        if (def->a && def->a < f->value_count) fx->stack[sp++] = def->a;
        if (def->b && def->b < f->value_count) fx->stack[sp++] = def->b;

        // avoid overflow on pathological graphs
        if (sp + 8u >= f->value_count)
        {
            // compact stack by removing already-live
            uint32_t w = 0;
            for (uint32_t j = 0; j < sp; ++j)
            {
                const uint32_t x = fx->stack[j];
                if (x && x < f->value_count && !fx->live[x])
                    fx->stack[w++] = x;
            }
            sp = w;
        }
    }

    // Count survivors.
    uint32_t keep = 0;
    for (uint32_t i = 0; i < f->inst_count; ++i)
    {
        const ir_inst_t* in = &f->insts[i];
        if (ir_op_has_side_effect(in->op) || ir_op_is_terminator(in->op))
        {
            keep++;
            continue;
        }
        if (in->dst && in->dst < f->value_count && fx->live[in->dst])
            keep++;
    }

    return keep;
}

static size_t ir_serialize(const ir_func_t* f, char* out, size_t cap)
{
    strbuf_t sb;
    sb_init(&sb, out, cap);

    sb_puts(&sb, "fn ir_bench()\n");

    for (uint32_t bi = 0; bi < f->block_count; ++bi)
    {
        const ir_block_t* b = &f->blocks[bi];
        sb_puts(&sb, "  bb");
        sb_u32(&sb, bi);
        sb_puts(&sb, ":\n");

        const uint32_t start = b->first_inst;
        const uint32_t end = start + b->inst_count;
        for (uint32_t i = start; i < end && i < f->inst_count; ++i)
        {
            const ir_inst_t* in = &f->insts[i];
            sb_puts(&sb, "    ");
            if (in->dst)
            {
                sb_putc(&sb, '%');
                sb_u32(&sb, in->dst);
                sb_puts(&sb, " = ");
            }
            sb_puts(&sb, ir_op_name(in->op));

            // operands (best-effort)
            if (in->a)
            {
                sb_putc(&sb, ' ');
                sb_putc(&sb, '%');
                sb_u32(&sb, in->a);
            }
            if (in->b)
            {
                sb_puts(&sb, ", %");
                sb_u32(&sb, in->b);
            }

            if (in->op == IR_CONST)
            {
                sb_puts(&sb, " " );
                sb_u32(&sb, in->imm);
            }
            else if (in->op == IR_JMP || in->op == IR_BR)
            {
                sb_puts(&sb, " -> bb");
                sb_u32(&sb, in->imm);
                if (in->op == IR_BR)
                {
                    sb_puts(&sb, ", bb");
                    sb_u32(&sb, in->b);
                }
            }

            sb_putc(&sb, '\n');
        }
    }

    sb_puts(&sb, ".end\n");
    return sb.len;
}

// -----------------------------------------------------------------------------
// Bench cases
// -----------------------------------------------------------------------------

typedef struct ir_case_cfg
{
    const char* label;
    ir_build_cfg_t build;
} ir_case_cfg_t;

static void bm_ir_build(uint64_t iters, void* user)
{
    ensure_fixture();
    const ir_case_cfg_t* cc = (const ir_case_cfg_t*)user;

    ir_func_t f;
    uint64_t acc = 0;

    for (uint64_t i = 0; i < iters; ++i)
    {
        ir_build_cfg_t bc = cc->build;
        bc.seed ^= (uint32_t)i;

        ir_build_func(&f, &g_fx, &bc);
        const uint64_t h = ir_hash_func(&f);
        acc ^= (h + (acc << 6) + (acc >> 2));
    }

    bench_blackhole_u64(acc);
}

static void bm_ir_walk(uint64_t iters, void* user)
{
    ensure_fixture();
    const ir_case_cfg_t* cc = (const ir_case_cfg_t*)user;

    ir_func_t f;
    ir_build_func(&f, &g_fx, &cc->build);

    uint64_t acc = 0;
    for (uint64_t i = 0; i < iters; ++i)
    {
        const uint64_t h = ir_walk_hash(&f);
        acc ^= (h + (acc << 7) + (acc >> 3));
    }

    bench_blackhole_u64(acc);
}

static void bm_ir_cfg(uint64_t iters, void* user)
{
    ensure_fixture();
    const ir_case_cfg_t* cc = (const ir_case_cfg_t*)user;

    ir_func_t f;
    ir_build_func(&f, &g_fx, &cc->build);

    uint64_t acc = 0;
    for (uint64_t i = 0; i < iters; ++i)
    {
        const uint32_t v = ir_cfg_bfs(&f, &g_fx);
        acc += (uint64_t)v * 1315423911ULL;
    }

    bench_blackhole_u64(acc);
}

static void bm_ir_dce(uint64_t iters, void* user)
{
    ensure_fixture();
    const ir_case_cfg_t* cc = (const ir_case_cfg_t*)user;

    ir_func_t f;
    ir_build_func(&f, &g_fx, &cc->build);

    uint64_t acc = 0;
    for (uint64_t i = 0; i < iters; ++i)
    {
        const uint32_t keep = ir_dce_like(&f, &g_fx);
        acc ^= (uint64_t)keep + (acc << 5) + (acc >> 2);
    }

    bench_blackhole_u64(acc);
}

static void bm_ir_serialize(uint64_t iters, void* user)
{
    ensure_fixture();
    const ir_case_cfg_t* cc = (const ir_case_cfg_t*)user;

    ir_func_t f;
    ir_build_func(&f, &g_fx, &cc->build);

    // Allocate a buffer sized to fit; keep fixed to avoid malloc noise in loop.
    const size_t cap = (size_t)f.inst_count * 64u + (size_t)f.block_count * 32u + 256u;
    char* buf = (char*)malloc(cap);
    if (!buf) abort();

    uint64_t acc = 0;
    for (uint64_t i = 0; i < iters; ++i)
    {
        const size_t n = ir_serialize(&f, buf, cap);
        const uint64_t h = fnv1a64(buf, n);
        acc ^= (h + (uint64_t)n + (acc << 6) + (acc >> 2));
        bench_blackhole_bytes(buf, n);
    }

    bench_blackhole_u64(acc);
    free(buf);
}

// -----------------------------------------------------------------------------
// Suite registration
// -----------------------------------------------------------------------------

static const ir_case_cfg_t k_small = { "small", { 128, 64,  0x11111111u } };  // 8k inst
static const ir_case_cfg_t k_med   = { "med",   { 512, 64,  0x22222222u } };  // 32k inst
static const ir_case_cfg_t k_large = { "large", { 2048, 64, 0x33333333u } };  // 131k inst

static const bench_case_t k_cases[] = {
    { "build_small",    bm_ir_build,     (void*)&k_small },
    { "build_med",      bm_ir_build,     (void*)&k_med   },
    { "build_large",    bm_ir_build,     (void*)&k_large },

    { "walk_small",     bm_ir_walk,      (void*)&k_small },
    { "walk_med",       bm_ir_walk,      (void*)&k_med   },

    { "cfg_large",      bm_ir_cfg,       (void*)&k_large },
    { "dce_med",        bm_ir_dce,       (void*)&k_med   },

    { "serialize_small", bm_ir_serialize, (void*)&k_small },
    { "serialize_med",   bm_ir_serialize, (void*)&k_med   },
};

static const bench_suite_t k_suite = {
    "ir",
    k_cases,
    sizeof(k_cases) / sizeof(k_cases[0]),
};

VITTE_BENCH_CONSTRUCTOR static void ir_suite_autoregister(void)
{
    bench_register_suite(&k_suite);
}

// Optional explicit symbol for runners that call suites manually.
void vitte_bench_register_ir_suite(void)
{
    bench_register_suite(&k_suite);
}

// Optional teardown hook if your runner supports it (no-op otherwise).
void vitte_bench_ir_suite_teardown(void)
{
    if (g_fx_inited)
    {
        fixture_destroy(&g_fx);
        g_fx_inited = false;
    }
}