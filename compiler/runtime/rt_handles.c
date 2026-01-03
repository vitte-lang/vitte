// SPDX-License-Identifier: MIT
// rt_handles.c
//
// Runtime handle table (max).
//
// Purpose:
//  - Provide stable, opaque handles to runtime-owned objects (pointers).
//  - Safe against stale handles (generation counter).
//  - Efficient allocate/free via freelist.
//  - Optional visitor for GC/root enumeration.
//
// Design:
//  - Handle is a 64-bit token: [gen:32 | index+1:32]
//    index is 0-based slot index; 0 token means invalid.
//
// Integration:
//  - If `rt_handles.h` exists, include it.
//  - Otherwise provide a fallback API that can later be moved to `rt_handles.h`.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if defined(__has_include)
  #if __has_include("rt_handles.h")
    #include "rt_handles.h"
    #define RT_HAVE_RT_HANDLES_H 1
  #elif __has_include("../runtime/rt_handles.h")
    #include "../runtime/rt_handles.h"
    #define RT_HAVE_RT_HANDLES_H 1
  #endif
#endif

#ifndef RT_HAVE_RT_HANDLES_H

//------------------------------------------------------------------------------
// Fallback API (align later with rt_handles.h)
//------------------------------------------------------------------------------

// Forward decl allocator (from rt_alloc)
typedef struct rt_allocator rt_allocator;

// Opaque handle token.
typedef uint64_t rt_handle;

typedef struct rt_handle_slot
{
    void*    ptr;
    uint32_t gen;
    uint32_t next_free; // freelist index, UINT32_MAX for none
    uint8_t  in_use;
    uint8_t  _pad[3];
} rt_handle_slot;

typedef struct rt_handle_table
{
    rt_allocator* alloc; // not owned
    rt_handle_slot* slots;
    uint32_t cap;
    uint32_t live;
    uint32_t free_head; // index into slots, UINT32_MAX if none
} rt_handle_table;

typedef void (*rt_handle_visit_fn)(void* user, rt_handle h, void* ptr);

void rt_handles_init(rt_handle_table* t, rt_allocator* a, uint32_t initial_cap);
void rt_handles_dispose(rt_handle_table* t);

rt_handle rt_handle_make(uint32_t index, uint32_t gen);
bool      rt_handle_decode(rt_handle h, uint32_t* out_index, uint32_t* out_gen);

rt_handle rt_handles_alloc(rt_handle_table* t, void* ptr);
bool      rt_handles_free(rt_handle_table* t, rt_handle h);

bool      rt_handles_is_valid(const rt_handle_table* t, rt_handle h);
void*     rt_handles_get(const rt_handle_table* t, rt_handle h);
bool      rt_handles_set(rt_handle_table* t, rt_handle h, void* ptr);

uint32_t  rt_handles_count(const rt_handle_table* t);

void      rt_handles_visit(const rt_handle_table* t, rt_handle_visit_fn fn, void* user);

const char* rt_handles_last_error(void);

#endif // !RT_HAVE_RT_HANDLES_H

//------------------------------------------------------------------------------
// Dependencies (allocator)
//------------------------------------------------------------------------------

#if !defined(RT_HAVE_RT_ALLOC_H)
// If rt_alloc.h wasn't included, declare the few allocator funcs we need.
// This matches the fallback API in rt_alloc.c.
typedef struct rt_alloc_stats rt_alloc_stats;
void* rt_malloc(rt_allocator* a, size_t n);
void* rt_realloc(rt_allocator* a, void* p, size_t n);
void  rt_free(rt_allocator* a, void* p);
#endif

//------------------------------------------------------------------------------
// Error
//------------------------------------------------------------------------------

static _Thread_local char g_handles_err_[256];

static void set_msg_(const char* msg)
{
    if (!msg) msg = "";
    size_t n = strlen(msg);
    if (n >= sizeof(g_handles_err_)) n = sizeof(g_handles_err_) - 1;
    memcpy(g_handles_err_, msg, n);
    g_handles_err_[n] = 0;
}

static void set_fmt_(const char* fmt, uint64_t a, uint64_t b)
{
    if (!fmt) fmt = "";
    (void)snprintf(g_handles_err_, sizeof(g_handles_err_), fmt, (unsigned long long)a, (unsigned long long)b);
}

const char* rt_handles_last_error(void)
{
    return g_handles_err_;
}

//------------------------------------------------------------------------------
// Handle encoding
//------------------------------------------------------------------------------

// 0 token is invalid.
rt_handle rt_handle_make(uint32_t index, uint32_t gen)
{
    uint64_t idx1 = (uint64_t)index + 1ull;
    uint64_t g = (uint64_t)gen;
    return (g << 32) | (idx1 & 0xFFFFFFFFull);
}

bool rt_handle_decode(rt_handle h, uint32_t* out_index, uint32_t* out_gen)
{
    if (h == 0)
        return false;

    uint32_t idx1 = (uint32_t)(h & 0xFFFFFFFFull);
    uint32_t gen  = (uint32_t)((h >> 32) & 0xFFFFFFFFull);

    if (idx1 == 0)
        return false;

    if (out_index) *out_index = idx1 - 1;
    if (out_gen) *out_gen = gen;
    return true;
}

//------------------------------------------------------------------------------
// Table internals
//------------------------------------------------------------------------------

static void table_reset_(rt_handle_table* t)
{
    t->slots = NULL;
    t->cap = 0;
    t->live = 0;
    t->free_head = UINT32_MAX;
}

static bool ensure_cap_(rt_handle_table* t, uint32_t want)
{
    if (t->cap >= want)
        return true;

    uint32_t new_cap = (t->cap == 0) ? 16u : t->cap;
    while (new_cap < want)
    {
        // grow *2 with clamp
        uint32_t next = new_cap * 2u;
        if (next < new_cap) { set_msg_("capacity overflow"); return false; }
        new_cap = next;
    }

    size_t new_bytes = (size_t)new_cap * sizeof(rt_handle_slot);

    rt_handle_slot* ns = (rt_handle_slot*)rt_realloc(t->alloc, t->slots, new_bytes);
    if (!ns)
    {
        set_msg_("out of memory");
        return false;
    }

    // init new region
    for (uint32_t i = t->cap; i < new_cap; i++)
    {
        ns[i].ptr = NULL;
        ns[i].gen = 1u; // start at 1; 0 can be treated as uninitialized
        ns[i].in_use = 0;
        ns[i].next_free = (i + 1u < new_cap) ? (i + 1u) : UINT32_MAX;
    }

    // attach new region to freelist (prepend for O(1))
    uint32_t old_free = t->free_head;
    t->free_head = (t->cap < new_cap) ? t->cap : UINT32_MAX;

    // connect tail of new free chain to old free head
    if (t->free_head != UINT32_MAX)
    {
        uint32_t tail = new_cap - 1u;
        ns[tail].next_free = old_free;
    }

    t->slots = ns;
    t->cap = new_cap;

    return true;
}

static bool slot_matches_(const rt_handle_table* t, uint32_t idx, uint32_t gen)
{
    if (!t || !t->slots) return false;
    if (idx >= t->cap) return false;

    const rt_handle_slot* s = &t->slots[idx];
    if (!s->in_use) return false;
    if (s->gen != gen) return false;

    return true;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void rt_handles_init(rt_handle_table* t, rt_allocator* a, uint32_t initial_cap)
{
    if (!t) return;
    memset(t, 0, sizeof(*t));
    t->alloc = a;
    table_reset_(t);

    if (initial_cap == 0)
        initial_cap = 64u;

    (void)ensure_cap_(t, initial_cap);
}

void rt_handles_dispose(rt_handle_table* t)
{
    if (!t) return;
    if (t->slots)
        rt_free(t->alloc, t->slots);
    table_reset_(t);
    t->alloc = NULL;
}

rt_handle rt_handles_alloc(rt_handle_table* t, void* ptr)
{
    if (!t || !t->alloc)
    {
        set_msg_("invalid table/allocator");
        return 0;
    }

    if (t->free_head == UINT32_MAX)
    {
        // grow
        uint32_t want = (t->cap == 0) ? 16u : (t->cap + 1u);
        if (!ensure_cap_(t, want))
            return 0;
    }

    uint32_t idx = t->free_head;
    if (idx == UINT32_MAX || idx >= t->cap)
    {
        set_msg_("corrupt freelist");
        return 0;
    }

    rt_handle_slot* s = &t->slots[idx];
    t->free_head = s->next_free;

    s->ptr = ptr;
    s->in_use = 1;
    s->next_free = UINT32_MAX;

    // gen already set; keep it as-is.
    t->live++;

    return rt_handle_make(idx, s->gen);
}

bool rt_handles_free(rt_handle_table* t, rt_handle h)
{
    if (!t)
    {
        set_msg_("invalid table");
        return false;
    }

    uint32_t idx = 0, gen = 0;
    if (!rt_handle_decode(h, &idx, &gen))
    {
        set_msg_("invalid handle");
        return false;
    }

    if (!slot_matches_(t, idx, gen))
    {
        set_fmt_("stale/invalid handle (idx=%llu gen=%llu)", (uint64_t)idx, (uint64_t)gen);
        return false;
    }

    rt_handle_slot* s = &t->slots[idx];

    // free
    s->ptr = NULL;
    s->in_use = 0;

    // bump generation (avoid 0)
    s->gen += 1u;
    if (s->gen == 0u) s->gen = 1u;

    // push to freelist
    s->next_free = t->free_head;
    t->free_head = idx;

    if (t->live > 0) t->live--;

    return true;
}

bool rt_handles_is_valid(const rt_handle_table* t, rt_handle h)
{
    uint32_t idx = 0, gen = 0;
    if (!rt_handle_decode(h, &idx, &gen))
        return false;
    return slot_matches_(t, idx, gen);
}

void* rt_handles_get(const rt_handle_table* t, rt_handle h)
{
    uint32_t idx = 0, gen = 0;
    if (!rt_handle_decode(h, &idx, &gen))
        return NULL;

    if (!slot_matches_(t, idx, gen))
        return NULL;

    return t->slots[idx].ptr;
}

bool rt_handles_set(rt_handle_table* t, rt_handle h, void* ptr)
{
    uint32_t idx = 0, gen = 0;
    if (!rt_handle_decode(h, &idx, &gen))
    {
        set_msg_("invalid handle");
        return false;
    }

    if (!slot_matches_(t, idx, gen))
    {
        set_msg_("stale handle");
        return false;
    }

    t->slots[idx].ptr = ptr;
    return true;
}

uint32_t rt_handles_count(const rt_handle_table* t)
{
    if (!t) return 0;
    return t->live;
}

void rt_handles_visit(const rt_handle_table* t, rt_handle_visit_fn fn, void* user)
{
    if (!t || !t->slots || !fn)
        return;

    for (uint32_t i = 0; i < t->cap; i++)
    {
        const rt_handle_slot* s = &t->slots[i];
        if (!s->in_use)
            continue;

        rt_handle h = rt_handle_make(i, s->gen);
        fn(user, h, s->ptr);
    }
}

