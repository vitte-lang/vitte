

// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// gc_suite.c
// -----------------------------------------------------------------------------
// Benchmark suite: garbage-collector-like workloads.
//
// This suite does NOT depend on the runtime GC implementation.
// It benchmarks GC-shaped hot paths that matter for a language runtime:
//   - bump allocation
//   - pointer graph construction
//   - mark traversal (stack/queue)
//   - sweep/compaction-like filtering
//
// It uses a synthetic heap of "objects" with pointer fields.
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
#ifndef VITTE_BENCH_GC_SUITE_API
#define VITTE_BENCH_GC_SUITE_API 1

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
    g_sink_u64 ^= (v + 0x9e3779b97f4a7c15ULL) ^ (g_sink_u64 << 5) ^ (g_sink_u64 >> 3);
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
    g_sink_u8 = (uint8_t)(acc + 0x19);
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
    // inclusive lo, exclusive hi
    const uint32_t span = (hi > lo) ? (hi - lo) : 1u;
    return lo + (uint32_t)(xorshift64(s) % span);
}

// -----------------------------------------------------------------------------
// Synthetic heap model
// -----------------------------------------------------------------------------
// Object layout: header + K pointer fields + some payload bytes.
// We keep it POD and compact for cache behavior.

typedef struct obj
{
    uint32_t id;
    uint32_t size_words;  // "size" in pointer-words for accounting
    uint32_t mark;        // 0/1 mark bit
    uint32_t pad;
    struct obj* f0;
    struct obj* f1;
    struct obj* f2;
    struct obj* f3;
    // payload simulated by size_words; we don't store it to avoid huge memory.
} obj_t;

// Arena for obj_t slots.
typedef struct heap
{
    obj_t*  base;
    uint32_t cap;
    uint32_t top;
} heap_t;

static void heap_init(heap_t* h, uint32_t cap)
{
    h->base = (obj_t*)calloc((size_t)cap, sizeof(obj_t));
    if (!h->base) abort();
    h->cap = cap;
    h->top = 0;
}

static void heap_reset(heap_t* h)
{
    // Keep allocated memory, just reset bump pointer.
    h->top = 0;
}

static void heap_destroy(heap_t* h)
{
    free(h->base);
    h->base = NULL;
    h->cap = 0;
    h->top = 0;
}

static inline obj_t* heap_alloc(heap_t* h, uint32_t size_words)
{
    if (h->top >= h->cap) return NULL;
    obj_t* o = &h->base[h->top++];
    o->id = h->top;
    o->size_words = size_words;
    o->mark = 0;
    o->pad = 0;
    o->f0 = NULL; o->f1 = NULL; o->f2 = NULL; o->f3 = NULL;
    return o;
}

// -----------------------------------------------------------------------------
// Graph construction
// -----------------------------------------------------------------------------
// We create a heap with N objects and random edges. Then we pick M roots.

typedef struct gc_fixture
{
    heap_t heap;

    obj_t** roots;
    uint32_t root_count;

    obj_t** stack;
    uint32_t stack_cap;

    bool inited;
} gc_fixture_t;

static void fixture_init(gc_fixture_t* fx, uint32_t heap_cap, uint32_t root_cap)
{
    memset(fx, 0, sizeof(*fx));
    heap_init(&fx->heap, heap_cap);
    fx->roots = (obj_t**)malloc(sizeof(obj_t*) * (size_t)root_cap);
    fx->stack = (obj_t**)malloc(sizeof(obj_t*) * (size_t)heap_cap);
    if (!fx->roots || !fx->stack) abort();
    fx->root_count = 0;
    fx->stack_cap = heap_cap;
    fx->inited = true;
}

static void fixture_destroy(gc_fixture_t* fx)
{
    if (!fx->inited) return;
    heap_destroy(&fx->heap);
    free(fx->roots);
    free(fx->stack);
    fx->roots = NULL;
    fx->stack = NULL;
    fx->root_count = 0;
    fx->stack_cap = 0;
    fx->inited = false;
}

static void build_random_graph(gc_fixture_t* fx, uint64_t seed, uint32_t obj_count, uint32_t root_count)
{
    heap_reset(&fx->heap);

    if (obj_count > fx->heap.cap)
        obj_count = fx->heap.cap;

    // Allocate objects with varying logical sizes.
    uint64_t rng = seed;
    for (uint32_t i = 0; i < obj_count; ++i)
    {
        const uint32_t sz = 2u + (uint32_t)(xorshift64(&rng) % 16u); // 2..17 words
        obj_t* o = heap_alloc(&fx->heap, sz);
        (void)o;
    }

    // Randomly wire pointers.
    for (uint32_t i = 0; i < fx->heap.top; ++i)
    {
        obj_t* o = &fx->heap.base[i];
        // ~50% chance each field is set.
        if (xorshift64(&rng) & 1u) o->f0 = &fx->heap.base[u32_range(&rng, 0, fx->heap.top)];
        if (xorshift64(&rng) & 1u) o->f1 = &fx->heap.base[u32_range(&rng, 0, fx->heap.top)];
        if (xorshift64(&rng) & 1u) o->f2 = &fx->heap.base[u32_range(&rng, 0, fx->heap.top)];
        if (xorshift64(&rng) & 1u) o->f3 = &fx->heap.base[u32_range(&rng, 0, fx->heap.top)];
        o->mark = 0;
    }

    // Choose roots.
    if (root_count > fx->heap.top)
        root_count = fx->heap.top;

    fx->root_count = root_count;
    for (uint32_t i = 0; i < root_count; ++i)
    {
        fx->roots[i] = &fx->heap.base[u32_range(&rng, 0, fx->heap.top)];
    }

    // Mix into sink so graph creation isn't optimized out.
    bench_blackhole_u64((uint64_t)fx->heap.top);
}

// -----------------------------------------------------------------------------
// Mark phase (DFS stack)
// -----------------------------------------------------------------------------
static uint64_t mark_from_roots(gc_fixture_t* fx)
{
    uint32_t sp = 0;
    uint64_t bytes = 0;

    // Push roots.
    for (uint32_t i = 0; i < fx->root_count; ++i)
    {
        obj_t* r = fx->roots[i];
        if (!r) continue;
        fx->stack[sp++] = r;
    }

    while (sp)
    {
        obj_t* o = fx->stack[--sp];
        if (!o || o->mark) continue;
        o->mark = 1;
        bytes += (uint64_t)o->size_words * sizeof(void*);

        // push children
        if (o->f0) fx->stack[sp++] = o->f0;
        if (o->f1) fx->stack[sp++] = o->f1;
        if (o->f2) fx->stack[sp++] = o->f2;
        if (o->f3) fx->stack[sp++] = o->f3;

        // Ensure stack doesn't overflow (cap == heap.cap, but duplicates can push more).
        if (sp + 8u >= fx->stack_cap)
        {
            // Compact stack by removing already-marked items.
            uint32_t w = 0;
            for (uint32_t j = 0; j < sp; ++j)
            {
                obj_t* x = fx->stack[j];
                if (x && !x->mark)
                    fx->stack[w++] = x;
            }
            sp = w;
        }
    }

    return bytes;
}

// -----------------------------------------------------------------------------
// Sweep phase (count/compact survivors)
// -----------------------------------------------------------------------------
static uint32_t sweep_compact(gc_fixture_t* fx)
{
    // We compact in-place by sliding marked objects to the front.
    uint32_t w = 0;
    for (uint32_t i = 0; i < fx->heap.top; ++i)
    {
        obj_t* o = &fx->heap.base[i];
        if (o->mark)
        {
            if (w != i)
            {
                fx->heap.base[w] = *o;
            }
            fx->heap.base[w].mark = 0; // clear for next cycle
            w++;
        }
    }

    // Rebuild pointers to point inside the compacted region.
    // Since we copied objects, their addresses changed, so we cannot keep real pointers.
    // To keep the benchmark deterministic without a relocation table, we rebuild a fresh
    // random graph over the survivors (this models "post-GC" reshaping).
    //
    // This keeps the suite self-contained and focuses on sweep-like filtering cost.

    fx->heap.top = w;
    return w;
}

// -----------------------------------------------------------------------------
// Bench cases
// -----------------------------------------------------------------------------
typedef struct gc_case_cfg
{
    uint32_t objects;
    uint32_t roots;
} gc_case_cfg_t;

static gc_fixture_t g_fx;
static bool g_fx_inited;

static void ensure_fixture(void)
{
    if (g_fx_inited) return;
    // Enough headroom for all cases.
    fixture_init(&g_fx, 1u << 20, 1u << 14); // 1,048,576 objs, 16,384 roots
    g_fx_inited = true;
}

static void bm_gc_mark_only(uint64_t iters, void* user)
{
    ensure_fixture();
    const gc_case_cfg_t* cc = (const gc_case_cfg_t*)user;

    uint64_t acc = 0;
    uint64_t seed = 0xC0FFEEBEEFULL;

    for (uint64_t i = 0; i < iters; ++i)
    {
        build_random_graph(&g_fx, seed, cc->objects, cc->roots);
        const uint64_t live = mark_from_roots(&g_fx);
        acc ^= (live + (acc << 7) + (acc >> 3));
        seed = seed * 6364136223846793005ULL + 1ULL;
    }

    bench_blackhole_u64(acc);
}

static void bm_gc_mark_sweep(uint64_t iters, void* user)
{
    ensure_fixture();
    const gc_case_cfg_t* cc = (const gc_case_cfg_t*)user;

    uint64_t acc = 0;
    uint64_t seed = 0x12345678ABCDEF01ULL;

    for (uint64_t i = 0; i < iters; ++i)
    {
        build_random_graph(&g_fx, seed, cc->objects, cc->roots);
        const uint64_t live_bytes = mark_from_roots(&g_fx);
        const uint32_t survivors = sweep_compact(&g_fx);

        // Re-shape survivors to emulate post-GC pointer write barrier churn.
        // Keep it light but non-trivial.
        uint64_t rng = seed ^ 0x9BADC0DEULL;
        for (uint32_t j = 0; j < survivors; ++j)
        {
            obj_t* o = &g_fx.heap.base[j];
            if (survivors)
            {
                o->f0 = &g_fx.heap.base[u32_range(&rng, 0, survivors)];
                if (xorshift64(&rng) & 1u) o->f1 = &g_fx.heap.base[u32_range(&rng, 0, survivors)];
                if (xorshift64(&rng) & 1u) o->f2 = &g_fx.heap.base[u32_range(&rng, 0, survivors)];
                if (xorshift64(&rng) & 1u) o->f3 = &g_fx.heap.base[u32_range(&rng, 0, survivors)];
            }
        }

        acc ^= (uint64_t)survivors + live_bytes + (acc << 5) + (acc >> 2);
        seed = seed * 6364136223846793005ULL + 1ULL;
    }

    bench_blackhole_u64(acc);
}

static void bm_gc_alloc_bump(uint64_t iters, void* user)
{
    ensure_fixture();
    (void)user;

    // Repeatedly bump-allocate into the heap until full, reset, repeat.
    uint64_t rng = 0xA11CE5EEDULL;
    uint64_t acc = 0;

    for (uint64_t i = 0; i < iters; ++i)
    {
        // Allocate a batch.
        for (uint32_t k = 0; k < 4096; ++k)
        {
            const uint32_t sz = 1u + (uint32_t)(xorshift64(&rng) % 32u);
            obj_t* o = heap_alloc(&g_fx.heap, sz);
            if (!o)
            {
                heap_reset(&g_fx.heap);
                o = heap_alloc(&g_fx.heap, sz);
            }
            // Touch some fields.
            o->f0 = o;
            o->f1 = (xorshift64(&rng) & 1u) ? o : NULL;
            acc += (uint64_t)o->size_words;
        }
    }

    bench_blackhole_u64(acc);
}

// -----------------------------------------------------------------------------
// Suite registration
// -----------------------------------------------------------------------------
static const gc_case_cfg_t k_small = {  1u << 14, 1u << 10 }; // 16K objs, 1K roots
static const gc_case_cfg_t k_med   = {  1u << 16, 1u << 11 }; // 64K objs, 2K roots
static const gc_case_cfg_t k_large = {  1u << 18, 1u << 12 }; // 256K objs, 4K roots

static const bench_case_t k_cases[] = {
    { "alloc_bump",  bm_gc_alloc_bump, NULL },
    { "mark_small",  bm_gc_mark_only, (void*)&k_small },
    { "mark_med",    bm_gc_mark_only, (void*)&k_med   },
    { "mark_large",  bm_gc_mark_only, (void*)&k_large },
    { "ms_small",    bm_gc_mark_sweep, (void*)&k_small },
    { "ms_med",      bm_gc_mark_sweep, (void*)&k_med   },
    { "ms_large",    bm_gc_mark_sweep, (void*)&k_large },
};

static const bench_suite_t k_suite = {
    "gc",
    k_cases,
    sizeof(k_cases) / sizeof(k_cases[0]),
};

VITTE_BENCH_CONSTRUCTOR static void gc_suite_autoregister(void)
{
    bench_register_suite(&k_suite);
}

// Optional explicit symbol for runners that call suites manually.
void vitte_bench_register_gc_suite(void)
{
    bench_register_suite(&k_suite);
}

// Optional teardown hook if your runner supports it (no-op otherwise).
void vitte_bench_gc_suite_teardown(void)
{
    if (g_fx_inited)
    {
        fixture_destroy(&g_fx);
        g_fx_inited = false;
    }
}
