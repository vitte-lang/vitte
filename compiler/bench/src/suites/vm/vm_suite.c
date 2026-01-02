// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// vm_suite.c
// -----------------------------------------------------------------------------
// Benchmark suite: VM-like hot paths.
//
// This suite is compiler-independent. It defines a small bytecode VM and
// benchmarks:
//   - decode/dispatch loop (switch dispatch)
//   - stack/register traffic
//   - branchy control flow (loops, conditional)
//   - memory load/store to a linear heap
//   - call/ret via a call stack
//   - tiny GC-like pressure via allocations into a bump heap (optional)
//
// Programs are generated deterministically and run for many iterations.
// The goal is stable performance tracking for interpreter work.
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
#ifndef VITTE_BENCH_VM_SUITE_API
#define VITTE_BENCH_VM_SUITE_API 1

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
    g_sink_u8 = (uint8_t)(acc + 0x47);
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
// VM model
// -----------------------------------------------------------------------------

typedef enum op
{
    OP_NOP = 0,

    OP_ICONST,     // A: dst reg, B: imm16
    OP_MOV,        // A: dst reg, B: src reg

    OP_ADD,        // A: dst reg, B: src reg
    OP_SUB,
    OP_MUL,
    OP_XOR,

    OP_LOAD,       // A: dst reg, B: base reg, C: off16
    OP_STORE,      // A: src reg, B: base reg, C: off16

    OP_JMP,        // A: rel16
    OP_JZ,         // A: reg, B: rel16

    OP_CALL,       // A: rel16 (pc-relative)
    OP_RET,        // A: reg (return value)

    OP_ALLOC,      // A: dst reg, B: size16 (bump)

    OP_HALT,
} op_t;

typedef struct ins
{
    uint8_t  op;
    uint8_t  a;
    uint16_t b;
    uint16_t c;
} ins_t;

enum {
    kMaxRegs   = 16,
    kMaxStack  = 1024,
    kMaxCall   = 1024,
};

typedef struct vm
{
    const ins_t* code;
    uint32_t code_len;

    uint64_t regs[kMaxRegs];

    uint8_t* mem;
    uint32_t mem_size;

    uint8_t* heap;
    uint32_t heap_cap;
    uint32_t heap_off;

    uint32_t pc;

    uint32_t call_sp;
    uint32_t call_stack[kMaxCall];

    uint64_t steps;
} vm_t;

static void vm_reset(vm_t* v)
{
    memset(v->regs, 0, sizeof(v->regs));
    v->pc = 0;
    v->call_sp = 0;
    v->steps = 0;
    v->heap_off = 0;
}

static inline uint32_t clamp_u32(uint32_t x, uint32_t lo, uint32_t hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static inline uint32_t mem_addr(vm_t* v, uint64_t base, uint16_t off)
{
    uint64_t a = base + (uint64_t)off;
    if (!v->mem_size) return 0;
    return (uint32_t)(a % (uint64_t)v->mem_size);
}

static inline uint64_t mem_load64(vm_t* v, uint32_t addr)
{
    // unaligned safe load
    uint64_t x = 0;
    const uint32_t a = clamp_u32(addr, 0, v->mem_size ? (v->mem_size - 1) : 0);
    const uint32_t n = (v->mem_size - a < 8) ? (v->mem_size - a) : 8;
    memcpy(&x, v->mem + a, n);
    return x;
}

static inline void mem_store64(vm_t* v, uint32_t addr, uint64_t x)
{
    const uint32_t a = clamp_u32(addr, 0, v->mem_size ? (v->mem_size - 1) : 0);
    const uint32_t n = (v->mem_size - a < 8) ? (v->mem_size - a) : 8;
    memcpy(v->mem + a, &x, n);
}

static inline uint32_t heap_alloc(vm_t* v, uint16_t size)
{
    if (!v->heap_cap || !v->heap) return 0;
    const uint32_t s = (uint32_t)size;
    if (s == 0) return v->heap_off;

    // align to 8
    uint32_t at = (v->heap_off + 7u) & ~7u;
    if (at + s > v->heap_cap)
    {
        // wrap (pressure)
        at = 0;
    }
    v->heap_off = at + s;

    // touch bytes to simulate init
    const uint32_t touch = (s < 32u) ? s : 32u;
    for (uint32_t i = 0; i < touch; ++i)
        v->heap[at + i] = (uint8_t)(v->heap[at + i] + (uint8_t)i);

    return at;
}

static uint64_t vm_run(vm_t* v, uint64_t max_steps)
{
    uint64_t ret = 0;

    while (1)
    {
        if (v->pc >= v->code_len) break;
        const ins_t in = v->code[v->pc++];
        v->steps++;

        switch ((op_t)in.op)
        {
            case OP_NOP:
                break;

            case OP_ICONST:
                v->regs[in.a & (kMaxRegs - 1)] = (uint64_t)(int16_t)in.b;
                break;

            case OP_MOV:
                v->regs[in.a & (kMaxRegs - 1)] = v->regs[in.b & (kMaxRegs - 1)];
                break;

            case OP_ADD:
                v->regs[in.a & (kMaxRegs - 1)] += v->regs[in.b & (kMaxRegs - 1)];
                break;

            case OP_SUB:
                v->regs[in.a & (kMaxRegs - 1)] -= v->regs[in.b & (kMaxRegs - 1)];
                break;

            case OP_MUL:
                v->regs[in.a & (kMaxRegs - 1)] *= (v->regs[in.b & (kMaxRegs - 1)] | 1ull);
                break;

            case OP_XOR:
                v->regs[in.a & (kMaxRegs - 1)] ^= v->regs[in.b & (kMaxRegs - 1)];
                break;

            case OP_LOAD:
            {
                const uint32_t addr = mem_addr(v, v->regs[in.b & (kMaxRegs - 1)], in.c);
                v->regs[in.a & (kMaxRegs - 1)] = mem_load64(v, addr);
            } break;

            case OP_STORE:
            {
                const uint32_t addr = mem_addr(v, v->regs[in.b & (kMaxRegs - 1)], in.c);
                mem_store64(v, addr, v->regs[in.a & (kMaxRegs - 1)]);
            } break;

            case OP_JMP:
            {
                const int16_t rel = (int16_t)in.b;
                v->pc = (uint32_t)((int32_t)v->pc + (int32_t)rel);
            } break;

            case OP_JZ:
            {
                const uint64_t x = v->regs[in.a & (kMaxRegs - 1)];
                if (x == 0)
                {
                    const int16_t rel = (int16_t)in.b;
                    v->pc = (uint32_t)((int32_t)v->pc + (int32_t)rel);
                }
            } break;

            case OP_CALL:
            {
                if (v->call_sp < kMaxCall)
                    v->call_stack[v->call_sp++] = v->pc;

                const int16_t rel = (int16_t)in.b;
                v->pc = (uint32_t)((int32_t)v->pc + (int32_t)rel);
            } break;

            case OP_RET:
            {
                ret = v->regs[in.a & (kMaxRegs - 1)];
                if (!v->call_sp) return ret;
                v->pc = v->call_stack[--v->call_sp];
            } break;

            case OP_ALLOC:
            {
                const uint32_t addr = heap_alloc(v, in.b);
                v->regs[in.a & (kMaxRegs - 1)] = (uint64_t)addr;
            } break;

            case OP_HALT:
                return ret;

            default:
                return ret;
        }

        if (v->steps >= max_steps)
            return ret ^ v->steps;
    }

    return ret ^ v->steps;
}

// -----------------------------------------------------------------------------
// Program generator
// -----------------------------------------------------------------------------

typedef struct prog
{
    ins_t* code;
    uint32_t len;
    uint32_t cap;
} prog_t;

static void prog_init(prog_t* p, uint32_t cap)
{
    p->code = (ins_t*)malloc(sizeof(ins_t) * (size_t)cap);
    if (!p->code) abort();
    p->len = 0;
    p->cap = cap;
}

static void prog_free(prog_t* p)
{
    free(p->code);
    p->code = NULL;
    p->len = 0;
    p->cap = 0;
}

static inline void emit(prog_t* p, uint8_t op, uint8_t a, uint16_t b, uint16_t c)
{
    if (p->len >= p->cap) return;
    p->code[p->len++] = (ins_t){ op, a, b, c };
}

static void gen_program(prog_t* p, uint64_t seed, uint32_t body_len, bool with_alloc)
{
    uint64_t rng = seed;

    // prologue: init some regs
    for (uint32_t r = 0; r < 8; ++r)
        emit(p, OP_ICONST, (uint8_t)r, (uint16_t)(r * 3 + 1), 0);

    // loop counter in r0
    emit(p, OP_ICONST, 0, (uint16_t)(body_len * 8 + 123), 0);

    const uint32_t loop_start = p->len;

    // body
    for (uint32_t i = 0; i < body_len; ++i)
    {
        const uint32_t k = (uint32_t)(xorshift64(&rng) & 15u);
        const uint8_t ra = (uint8_t)(xorshift64(&rng) & 7u);
        const uint8_t rb = (uint8_t)(xorshift64(&rng) & 7u);
        const uint16_t imm = (uint16_t)(xorshift64(&rng) & 0xFFFFu);

        switch (k)
        {
            case 0: emit(p, OP_ADD, ra, rb, 0); break;
            case 1: emit(p, OP_SUB, ra, rb, 0); break;
            case 2: emit(p, OP_MUL, ra, rb, 0); break;
            case 3: emit(p, OP_XOR, ra, rb, 0); break;
            case 4: emit(p, OP_LOAD, ra, rb, imm); break;
            case 5: emit(p, OP_STORE, ra, rb, imm); break;
            case 6:
                if (with_alloc) emit(p, OP_ALLOC, ra, (uint16_t)(8u + (imm & 0x7Fu)), 0);
                else emit(p, OP_NOP, 0, 0, 0);
                break;
            case 7:
                // conditional jump forward small
                emit(p, OP_JZ, ra, (uint16_t)(int16_t)2, 0);
                emit(p, OP_ADD, ra, rb, 0);
                break;
            default:
                emit(p, OP_MOV, ra, rb, 0);
                break;
        }
    }

    // decrement r0 and loop
    emit(p, OP_ICONST, 1, 1, 0);
    emit(p, OP_SUB, 0, 1, 0);

    // if r0 == 0 jump out
    emit(p, OP_JZ, 0, (uint16_t)(int16_t)2, 0);

    // back-edge
    {
        const int32_t rel = (int32_t)loop_start - (int32_t)(p->len + 1);
        emit(p, OP_JMP, 0, (uint16_t)(int16_t)rel, 0);
    }

    // epilogue: compute return in r2
    emit(p, OP_ADD, 2, 3, 0);
    emit(p, OP_RET, 2, 0, 0);
    emit(p, OP_HALT, 0, 0, 0);
}

// -----------------------------------------------------------------------------
// Bench cases
// -----------------------------------------------------------------------------

typedef struct vm_case_cfg
{
    const char* label;
    uint32_t body_len;
    uint32_t mem_size;
    uint32_t heap_size;
    uint32_t seed;
    bool with_alloc;
} vm_case_cfg_t;

static void bm_vm_run(uint64_t iters, void* user)
{
    const vm_case_cfg_t* cc = (const vm_case_cfg_t*)user;

    const uint32_t code_cap = cc->body_len + 128u;
    prog_t pr;
    prog_init(&pr, code_cap);
    gen_program(&pr, 0xC0FFEEULL ^ (uint64_t)cc->seed, cc->body_len, cc->with_alloc);

    uint8_t* mem = (uint8_t*)malloc(cc->mem_size);
    uint8_t* heap = (cc->heap_size ? (uint8_t*)malloc(cc->heap_size) : NULL);
    if (!mem || (cc->heap_size && !heap)) abort();

    // init mem to deterministic pattern
    for (uint32_t i = 0; i < cc->mem_size; ++i)
        mem[i] = (uint8_t)(i * 131u + 7u);
    if (heap)
        memset(heap, 0, cc->heap_size);

    vm_t v;
    memset(&v, 0, sizeof(v));
    v.code = pr.code;
    v.code_len = pr.len;
    v.mem = mem;
    v.mem_size = cc->mem_size;
    v.heap = heap;
    v.heap_cap = cc->heap_size;

    uint64_t acc = 0;

    for (uint64_t i = 0; i < iters; ++i)
    {
        vm_reset(&v);
        // seed registers slightly per-iter
        v.regs[0] = (uint64_t)(cc->seed + (uint32_t)i);
        v.regs[3] = (uint64_t)(0x9E37u * (uint32_t)i + 11u);

        const uint64_t r = vm_run(&v, 100000000ULL);
        acc ^= r + (v.steps * 1315423911ULL) + (acc << 5) + (acc >> 2);

        // touch some bytes to keep buffers live
        bench_blackhole_bytes(mem, cc->mem_size);
        if (heap) bench_blackhole_bytes(heap, cc->heap_size);
    }

    bench_blackhole_u64(acc);

    free(mem);
    free(heap);
    prog_free(&pr);
}

// -----------------------------------------------------------------------------
// Suite registration
// -----------------------------------------------------------------------------

static const vm_case_cfg_t k_small = { "small",  256,  64 * 1024,   0,        0x11111111u, false };
static const vm_case_cfg_t k_med   = { "med",    1024, 256 * 1024,  0,        0x22222222u, false };
static const vm_case_cfg_t k_large = { "large",  4096, 1024 * 1024, 0,        0x33333333u, false };

static const vm_case_cfg_t k_small_alloc = { "small_alloc", 256,  64 * 1024,  256 * 1024, 0x44444444u, true };
static const vm_case_cfg_t k_med_alloc   = { "med_alloc",   1024, 256 * 1024, 1 * 1024 * 1024, 0x55555555u, true };

static const bench_case_t k_cases[] = {
    { "run_small",      bm_vm_run, (void*)&k_small },
    { "run_med",        bm_vm_run, (void*)&k_med },
    { "run_large",      bm_vm_run, (void*)&k_large },

    { "run_small_alloc", bm_vm_run, (void*)&k_small_alloc },
    { "run_med_alloc",   bm_vm_run, (void*)&k_med_alloc },
};

static const bench_suite_t k_suite = {
    "vm",
    k_cases,
    sizeof(k_cases) / sizeof(k_cases[0]),
};

VITTE_BENCH_CONSTRUCTOR static void vm_suite_autoregister(void)
{
    bench_register_suite(&k_suite);
}

// Optional explicit symbol for runners that call suites manually.
void vitte_bench_register_vm_suite(void)
{
    bench_register_suite(&k_suite);
}
