/*
  bench/arena.h

  Small monotonic arena allocator for the Vitte benchmark harness.

  Why
  - Bench code often needs many short-lived allocations (parsing logs, JSON, etc.)
  - Using malloc/free introduces noise and nondeterminism.
  - A monotonic arena provides:
    - predictable performance
    - deterministic lifetime management (reset/rewind)

  Model
  - The arena owns a contiguous backing buffer provided by the caller.
  - Allocations bump a cursor forward; frees are not supported.
  - You can:
    - reset the entire arena (fast)
    - mark/rewind to a previous cursor
    - use scratch scopes

  Threading
  - Not thread-safe. One arena per thread or external synchronization.

  Pairing
  - This header is self-contained; the implementation is header-only.
  - Requires bench/detail/compat.h.
*/

#pragma once
#ifndef VITTE_BENCH_ARENA_H
#define VITTE_BENCH_ARENA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/detail/compat.h"

/* -------------------------------------------------------------------------- */
/* Types                                                                       */
/* -------------------------------------------------------------------------- */

typedef struct vitte_arena {
    uint8_t* base;
    size_t   cap;
    size_t   off;

    /* High-water mark (max off observed). */
    size_t   hi;

    /* Optional name for diagnostics. */
    const char* name;

    /* Optional OOM hook (may abort). */
    void (*oom)(struct vitte_arena* a, size_t request, size_t align);
} vitte_arena;

/* A saved cursor that can be rewound to. */
typedef struct vitte_arena_mark {
    size_t off;
} vitte_arena_mark;

/* Scratch scope (RAII-style in C via begin/end). */
typedef struct vitte_arena_scratch {
    vitte_arena* a;
    vitte_arena_mark mark;
} vitte_arena_scratch;

/* -------------------------------------------------------------------------- */
/* Helpers                                                                     */
/* -------------------------------------------------------------------------- */

static inline size_t vitte__align_up(size_t x, size_t align)
{
    /* align must be power of two. */
    return (x + (align - 1u)) & ~(align - 1u);
}

static inline bool vitte__is_pow2(size_t x)
{
    return x != 0u && (x & (x - 1u)) == 0u;
}

/* -------------------------------------------------------------------------- */
/* Init / reset                                                                */
/* -------------------------------------------------------------------------- */

/*
  Initialize arena with caller-provided memory.

  - `mem` must be valid for `cap` bytes.
  - `cap` may be 0 (arena always OOM).
*/
static inline void vitte_arena_init(vitte_arena* a, void* mem, size_t cap)
{
    a->base = (uint8_t*)mem;
    a->cap  = cap;
    a->off  = 0;
    a->hi   = 0;
    a->name = NULL;
    a->oom  = NULL;
}

/* Optional: set a name for diagnostics. */
static inline void vitte_arena_set_name(vitte_arena* a, const char* name)
{
    a->name = name;
}

/* Optional: set an out-of-memory hook. */
static inline void vitte_arena_set_oom(vitte_arena* a, void (*oom)(vitte_arena* a, size_t request, size_t align))
{
    a->oom = oom;
}

/* Reset arena to empty (keeps capacity/buffer). */
static inline void vitte_arena_reset(vitte_arena* a)
{
    a->off = 0;
}

/* Bytes currently allocated (cursor). */
static inline size_t vitte_arena_used(const vitte_arena* a)
{
    return a->off;
}

/* Total capacity in bytes. */
static inline size_t vitte_arena_capacity(const vitte_arena* a)
{
    return a->cap;
}

/* Remaining bytes. */
static inline size_t vitte_arena_remaining(const vitte_arena* a)
{
    return (a->off <= a->cap) ? (a->cap - a->off) : 0;
}

/* High-water mark in bytes. */
static inline size_t vitte_arena_high_water(const vitte_arena* a)
{
    return a->hi;
}

/* -------------------------------------------------------------------------- */
/* Mark / rewind                                                               */
/* -------------------------------------------------------------------------- */

static inline vitte_arena_mark vitte_arena_mark(vitte_arena* a)
{
    vitte_arena_mark m;
    m.off = a->off;
    return m;
}

static inline void vitte_arena_rewind(vitte_arena* a, vitte_arena_mark m)
{
    if (m.off <= a->off) {
        a->off = m.off;
    }
}

/* -------------------------------------------------------------------------- */
/* Allocation                                                                  */
/* -------------------------------------------------------------------------- */

/*
  Allocate `n` bytes aligned to `align`.

  Contract
  - `align` must be a power of two; typical values: 1,2,4,8,16.
  - Returns NULL on OOM (unless oom hook aborts).
  - Does not zero memory.
*/
static inline void* vitte_arena_alloc_aligned(vitte_arena* a, size_t n, size_t align)
{
    if (!vitte__is_pow2(align)) {
        /* defensive: align=1 as fallback */
        align = 1;
    }

    size_t start = vitte__align_up(a->off, align);

    /* overflow checks */
    if (start > a->cap) {
        if (a->oom) a->oom(a, n, align);
        return NULL;
    }

    if (n > (a->cap - start)) {
        if (a->oom) a->oom(a, n, align);
        return NULL;
    }

    size_t end = start + n;
    a->off = end;
    if (a->off > a->hi) {
        a->hi = a->off;
    }

    return (void*)(a->base + start);
}

/* Allocate `n` bytes with natural pointer alignment. */
static inline void* vitte_arena_alloc(vitte_arena* a, size_t n)
{
    return vitte_arena_alloc_aligned(a, n, sizeof(void*));
}

/* Allocate and zero-initialize. */
static inline void* vitte_arena_calloc_aligned(vitte_arena* a, size_t n, size_t align)
{
    uint8_t* p = (uint8_t*)vitte_arena_alloc_aligned(a, n, align);
    if (!p) {
        return NULL;
    }
    for (size_t i = 0; i < n; i++) {
        p[i] = 0;
    }
    return (void*)p;
}

static inline void* vitte_arena_calloc(vitte_arena* a, size_t n)
{
    return vitte_arena_calloc_aligned(a, n, sizeof(void*));
}

/*
  Typed allocation helpers.

  Example:
    my_t* p = VITTE_ARENA_NEW(&arena, my_t);
    my_t* arr = VITTE_ARENA_NEW_ARRAY(&arena, my_t, 128);
*/
#define VITTE_ARENA_NEW(a, T) ((T*)vitte_arena_calloc_aligned((a), sizeof(T), _Alignof(T)))
#define VITTE_ARENA_NEW_ARRAY(a, T, count) ((T*)vitte_arena_calloc_aligned((a), sizeof(T) * (size_t)(count), _Alignof(T)))

/* -------------------------------------------------------------------------- */
/* Scratch scopes                                                              */
/* -------------------------------------------------------------------------- */

static inline vitte_arena_scratch vitte_arena_scratch_begin(vitte_arena* a)
{
    vitte_arena_scratch s;
    s.a = a;
    s.mark = vitte_arena_mark(a);
    return s;
}

static inline void vitte_arena_scratch_end(vitte_arena_scratch s)
{
    if (s.a) {
        vitte_arena_rewind(s.a, s.mark);
    }
}

/* -------------------------------------------------------------------------- */
/* Diagnostics                                                                 */
/* -------------------------------------------------------------------------- */

/*
  Default OOM hook that panics.

  You can wire it with:
    vitte_arena_set_oom(&a, vitte_arena_oom_panic);
*/
static inline void vitte_arena_oom_panic(vitte_arena* a, size_t request, size_t align)
{
    const char* nm = a && a->name ? a->name : "arena";
    vitte_bench_panic(
        "%s: OOM: request=%llu align=%llu used=%llu cap=%llu",
        nm,
        (unsigned long long)request,
        (unsigned long long)align,
        (unsigned long long)(a ? a->off : 0u),
        (unsigned long long)(a ? a->cap : 0u)
    );
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_ARENA_H */
