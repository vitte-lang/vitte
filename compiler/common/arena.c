// SPDX-License-Identifier: MIT
// arena.c
//
// A small, dependency-light arena (bump) allocator.
//
// Goals:
//  - Fast allocation, cheap reset/rewind.
//  - Minimal libc usage (malloc/free/memcpy/memset).
//  - Deterministic behaviour, overflow checks.
//
// Typical usage:
//  - arena_init(&a, 64 * 1024);
//  - p = arena_alloc(&a, n);
//  - mark = arena_mark(&a);
//  - ... temporary allocations ...
//  - arena_rewind(&a, mark);
//  - arena_free(&a);

#include "arena.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

//------------------------------------------------------------------------------
// Internal layout
//------------------------------------------------------------------------------

typedef struct arena_block
{
    struct arena_block* next;
    size_t cap;
    size_t used;
    // Flexible array member
    unsigned char data[];
} arena_block;

static inline bool arena_is_pow2(size_t x)
{
    return x && ((x & (x - 1)) == 0);
}

static inline size_t arena_align_up(size_t v, size_t align)
{
    return (v + (align - 1)) & ~(align - 1);
}

static bool arena_add_overflow(size_t a, size_t b, size_t* out)
{
#if defined(__has_builtin)
  #if __has_builtin(__builtin_add_overflow)
    return __builtin_add_overflow(a, b, out);
  #endif
#endif
    if (SIZE_MAX - a < b) return true;
    *out = a + b;
    return false;
}

static arena_block* arena_block_new(size_t payload_cap)
{
    size_t total;
    if (arena_add_overflow(sizeof(arena_block), payload_cap, &total)) return NULL;

    arena_block* b = (arena_block*)malloc(total);
    if (!b) return NULL;

    b->next = NULL;
    b->cap = payload_cap;
    b->used = 0;
    return b;
}

static void arena_blocks_free(arena_block* b)
{
    while (b)
    {
        arena_block* n = b->next;
        free(b);
        b = n;
    }
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

// Note: arena.h is expected to define `arena_t` and `arena_mark_t`.
// This implementation assumes the following minimal shape:
//   - arena_t has: void* head; size_t block_size;
//   - arena_mark_t has: void* block; size_t used;
// If your header differs, adjust arena.h accordingly.

void arena_init(arena_t* a, size_t block_size)
{
    if (!a) return;

    if (block_size == 0) block_size = 64u * 1024u;
    // ensure reasonable minimum
    if (block_size < 1024u) block_size = 1024u;

    a->head = NULL;
    a->block_size = block_size;
}

void arena_free(arena_t* a)
{
    if (!a) return;
    arena_blocks_free((arena_block*)a->head);
    a->head = NULL;
}

void arena_reset(arena_t* a)
{
    // Keep the first block if present, free the rest, and reset usage.
    if (!a) return;

    arena_block* h = (arena_block*)a->head;
    if (!h) return;

    arena_block* rest = h->next;
    h->next = NULL;
    h->used = 0;
    arena_blocks_free(rest);
}

arena_mark_t arena_mark(arena_t* a)
{
    arena_mark_t m;
    m.block = NULL;
    m.used = 0;

    if (!a) return m;

    arena_block* h = (arena_block*)a->head;
    if (!h) return m;

    m.block = h;
    m.used = h->used;
    return m;
}

void arena_rewind(arena_t* a, arena_mark_t m)
{
    if (!a) return;

    // If mark is empty, behave like reset-all.
    if (!m.block)
    {
        arena_free(a);
        return;
    }

    arena_block* target = (arena_block*)m.block;

    // Find the target block in the list (head->next->...)
    arena_block* h = (arena_block*)a->head;
    if (!h) return;

    if (h == target)
    {
        // Free everything after target
        arena_blocks_free(h->next);
        h->next = NULL;
        if (m.used <= h->cap) h->used = m.used;
        else h->used = h->cap;
        return;
    }

    // Walk to find target; if not found, do nothing (defensive).
    arena_block* prev = h;
    arena_block* cur = h->next;
    while (cur)
    {
        if (cur == target)
        {
            // Free blocks after cur
            arena_blocks_free(cur->next);
            cur->next = NULL;
            // Also free blocks between head and target? No: we keep them allocated.
            // But we must detach everything after target only.
            if (m.used <= cur->cap) cur->used = m.used;
            else cur->used = cur->cap;
            return;
        }
        prev = cur;
        cur = cur->next;
    }

    (void)prev;
}

void* arena_alloc_align(arena_t* a, size_t size, size_t align)
{
    if (!a) return NULL;
    if (size == 0) size = 1;

    if (align == 0) align = sizeof(void*);
    if (!arena_is_pow2(align))
    {
        // Force to next power-of-two-ish fallback: use pointer alignment.
        align = sizeof(void*);
    }

    arena_block* h = (arena_block*)a->head;
    if (!h)
    {
        // First block
        size_t need;
        if (arena_add_overflow(size, align, &need)) return NULL;
        size_t cap = (a->block_size > need) ? a->block_size : need;
        h = arena_block_new(cap);
        if (!h) return NULL;
        a->head = (void*)h;
    }

    // Try current head; if no room, allocate new head block.
    for (;;)
    {
        size_t off = arena_align_up(h->used, align);

        size_t end;
        if (arena_add_overflow(off, size, &end)) return NULL;

        if (end <= h->cap)
        {
            void* p = (void*)(h->data + off);
            h->used = end;
            return p;
        }

        // Need a new block: allocate enough for this allocation.
        size_t need;
        if (arena_add_overflow(size, align, &need)) return NULL;
        size_t cap = (a->block_size > need) ? a->block_size : need;

        arena_block* nb = arena_block_new(cap);
        if (!nb) return NULL;

        // Push front (becomes new head)
        nb->next = h;
        a->head = (void*)nb;
        h = nb;
    }
}

void* arena_alloc(arena_t* a, size_t size)
{
    return arena_alloc_align(a, size, sizeof(void*));
}

void* arena_calloc(arena_t* a, size_t count, size_t elem_size)
{
    size_t total;
    if (count == 0 || elem_size == 0) total = 0;
    else
    {
#if defined(__has_builtin)
  #if __has_builtin(__builtin_mul_overflow)
        if (__builtin_mul_overflow(count, elem_size, &total)) return NULL;
  #else
        if (SIZE_MAX / count < elem_size) return NULL;
        total = count * elem_size;
  #endif
#else
        if (SIZE_MAX / count < elem_size) return NULL;
        total = count * elem_size;
#endif
    }

    void* p = arena_alloc(a, total);
    if (!p) return NULL;
    if (total) memset(p, 0, total);
    return p;
}

char* arena_strdup(arena_t* a, const char* s)
{
    if (!s) return NULL;
    size_t n = strlen(s);
    char* d = (char*)arena_alloc(a, n + 1);
    if (!d) return NULL;
    memcpy(d, s, n);
    d[n] = 0;
    return d;
}

char* arena_strndup(arena_t* a, const char* s, size_t n)
{
    if (!s) return NULL;
    size_t sl = strlen(s);
    if (n > sl) n = sl;
    char* d = (char*)arena_alloc(a, n + 1);
    if (!d) return NULL;
    memcpy(d, s, n);
    d[n] = 0;
    return d;
}
