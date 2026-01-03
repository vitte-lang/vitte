// SPDX-License-Identifier: MIT
// rt_alloc.c
//
// Runtime allocator (max).
//
// Goals:
//  - Centralize allocation strategy for the runtime/VM.
//  - Provide predictable OOM behavior (hook + diagnostics).
//  - Optional debug guards (header + canary) to catch double-free/overrun.
//  - Minimal dependencies: uses libc malloc/free by default.
//
// Integration:
//  - If `rt_alloc.h` exists, this file includes it.
//  - Otherwise, this file provides a fallback API that can later be moved
//    into `rt_alloc.h`.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
  #include <stdatomic.h>
  #define RT_HAVE_ATOMICS 1
#else
  #define RT_HAVE_ATOMICS 0
#endif

#if defined(__has_include)
  #if __has_include("rt_alloc.h")
    #include "rt_alloc.h"
    #define RT_HAVE_RT_ALLOC_H 1
  #elif __has_include("../runtime/rt_alloc.h")
    #include "../runtime/rt_alloc.h"
    #define RT_HAVE_RT_ALLOC_H 1
  #endif
#endif

#ifndef RT_HAVE_RT_ALLOC_H

//------------------------------------------------------------------------------
// Fallback API (align later with rt_alloc.h)
//------------------------------------------------------------------------------

typedef void (*rt_oom_fn)(void* user, size_t requested);

typedef struct rt_alloc_stats
{
    uint64_t alloc_count;
    uint64_t free_count;
    uint64_t realloc_count;

    uint64_t bytes_live;      // best-effort
    uint64_t bytes_peak;      // best-effort
    uint64_t bytes_total;     // sum of requested sizes
} rt_alloc_stats;

typedef struct rt_allocator
{
    rt_oom_fn oom;
    void* oom_user;

    rt_alloc_stats stats;

    // last error text (thread-local)
} rt_allocator;

void rt_alloc_init(rt_allocator* a);
void rt_alloc_dispose(rt_allocator* a);

void rt_alloc_set_oom_handler(rt_allocator* a, rt_oom_fn fn, void* user);

void* rt_malloc(rt_allocator* a, size_t n);
void* rt_calloc(rt_allocator* a, size_t count, size_t size);
void* rt_realloc(rt_allocator* a, void* p, size_t n);
void  rt_free(rt_allocator* a, void* p);

void* rt_memalign(rt_allocator* a, size_t alignment, size_t n);
void  rt_aligned_free(rt_allocator* a, void* p);

char* rt_strdup(rt_allocator* a, const char* s);

rt_alloc_stats rt_alloc_get_stats(const rt_allocator* a);

const char* rt_alloc_last_error(void);

#endif // !RT_HAVE_RT_ALLOC_H

//------------------------------------------------------------------------------
// Debug guards
//------------------------------------------------------------------------------

// Enable with -DRT_ALLOC_DEBUG=1
#ifndef RT_ALLOC_DEBUG
  #define RT_ALLOC_DEBUG 0
#endif

#if RT_ALLOC_DEBUG
  #define RT_GUARD_CANARY 0xC0FFEE11u
  typedef struct rt_dbg_hdr
  {
      uint32_t canary;
      uint32_t flags;
      size_t   size;
  } rt_dbg_hdr;

  enum { RT_DBG_F_FREED = 1u << 0 };

  static void* rt_dbg_wrap_(void* raw, size_t n)
  {
      if (!raw) return NULL;
      rt_dbg_hdr* h = (rt_dbg_hdr*)raw;
      h->canary = RT_GUARD_CANARY;
      h->flags = 0;
      h->size = n;
      uint8_t* user = (uint8_t*)(h + 1);
      // trailer canary
      uint32_t* tail = (uint32_t*)(user + n);
      *tail = RT_GUARD_CANARY;
      return user;
  }

  static rt_dbg_hdr* rt_dbg_hdr_(void* user)
  {
      if (!user) return NULL;
      return ((rt_dbg_hdr*)user) - 1;
  }

  static bool rt_dbg_check_(void* user)
  {
      rt_dbg_hdr* h = rt_dbg_hdr_(user);
      if (!h) return false;
      if (h->canary != RT_GUARD_CANARY) return false;
      uint8_t* u = (uint8_t*)user;
      uint32_t* tail = (uint32_t*)(u + h->size);
      if (*tail != RT_GUARD_CANARY) return false;
      if (h->flags & RT_DBG_F_FREED) return false;
      return true;
  }

  static void rt_dbg_mark_freed_(void* user)
  {
      rt_dbg_hdr* h = rt_dbg_hdr_(user);
      if (h) h->flags |= RT_DBG_F_FREED;
  }

  static size_t rt_dbg_total_(size_t n)
  {
      // header + payload + tail canary
      return sizeof(rt_dbg_hdr) + n + sizeof(uint32_t);
  }
#endif

//------------------------------------------------------------------------------
// Error handling
//------------------------------------------------------------------------------

static _Thread_local char g_rt_alloc_err_[256];

static void rt_set_msg_(const char* msg)
{
    if (!msg) msg = "";
    size_t n = strlen(msg);
    if (n >= sizeof(g_rt_alloc_err_)) n = sizeof(g_rt_alloc_err_) - 1;
    memcpy(g_rt_alloc_err_, msg, n);
    g_rt_alloc_err_[n] = 0;
}

static void rt_set_errno_(const char* prefix)
{
    const char* e = strerror(errno);
    if (!prefix) prefix = "";
    if (!e) e = "";

    if (prefix[0])
        snprintf(g_rt_alloc_err_, sizeof(g_rt_alloc_err_), "%s: %s", prefix, e);
    else
        snprintf(g_rt_alloc_err_, sizeof(g_rt_alloc_err_), "%s", e);
}

const char* rt_alloc_last_error(void)
{
    return g_rt_alloc_err_;
}

//------------------------------------------------------------------------------
// Atomics helpers
//------------------------------------------------------------------------------

#if RT_HAVE_ATOMICS

static inline void rt_atomic_add_u64_(atomic_uint_least64_t* p, uint64_t v)
{
    (void)atomic_fetch_add_explicit(p, (uint_least64_t)v, memory_order_relaxed);
}

static inline uint64_t rt_atomic_load_u64_(const atomic_uint_least64_t* p)
{
    return (uint64_t)atomic_load_explicit(p, memory_order_relaxed);
}

static inline void rt_atomic_store_u64_(atomic_uint_least64_t* p, uint64_t v)
{
    atomic_store_explicit(p, (uint_least64_t)v, memory_order_relaxed);
}

#else

static inline void rt_atomic_add_u64_(uint64_t* p, uint64_t v) { *p += v; }
static inline uint64_t rt_atomic_load_u64_(const uint64_t* p) { return *p; }
static inline void rt_atomic_store_u64_(uint64_t* p, uint64_t v) { *p = v; }

#endif

//------------------------------------------------------------------------------
// Global allocator state (per-allocator instance data is stored in rt_allocator)
//------------------------------------------------------------------------------

typedef struct rt_alloc_state
{
#if RT_HAVE_ATOMICS
    atomic_uint_least64_t alloc_count;
    atomic_uint_least64_t free_count;
    atomic_uint_least64_t realloc_count;

    atomic_uint_least64_t bytes_live;
    atomic_uint_least64_t bytes_peak;
    atomic_uint_least64_t bytes_total;
#else
    uint64_t alloc_count;
    uint64_t free_count;
    uint64_t realloc_count;

    uint64_t bytes_live;
    uint64_t bytes_peak;
    uint64_t bytes_total;
#endif
} rt_alloc_state;

static rt_alloc_state g_state_; // shared across all allocators (good enough for toolchain)

static void state_add_live_(uint64_t delta)
{
#if RT_HAVE_ATOMICS
    uint64_t live = (uint64_t)atomic_fetch_add_explicit(&g_state_.bytes_live, (uint_least64_t)delta, memory_order_relaxed) + delta;
    // peak update (best-effort)
    uint64_t peak = rt_atomic_load_u64_(&g_state_.bytes_peak);
    while (live > peak)
    {
        if (atomic_compare_exchange_weak_explicit(&g_state_.bytes_peak,
                                                 (uint_least64_t*)&peak,
                                                 (uint_least64_t)live,
                                                 memory_order_relaxed,
                                                 memory_order_relaxed))
            break;
        // peak updated in 'peak' by compare_exchange_weak
    }
#else
    g_state_.bytes_live += delta;
    if (g_state_.bytes_live > g_state_.bytes_peak)
        g_state_.bytes_peak = g_state_.bytes_live;
#endif
}

static void state_sub_live_(uint64_t delta)
{
#if RT_HAVE_ATOMICS
    (void)atomic_fetch_sub_explicit(&g_state_.bytes_live, (uint_least64_t)delta, memory_order_relaxed);
#else
    if (g_state_.bytes_live >= delta) g_state_.bytes_live -= delta;
#endif
}

//------------------------------------------------------------------------------
// OOM
//------------------------------------------------------------------------------

static void default_oom_(void* user, size_t requested)
{
    (void)user;
    // Keep it deterministic: emit to stderr then abort.
    fprintf(stderr, "rt_alloc: out of memory (requested=%zu)\n", requested);
    abort();
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void rt_alloc_init(rt_allocator* a)
{
    if (!a) return;
    memset(a, 0, sizeof(*a));
    a->oom = default_oom_;
    a->oom_user = NULL;
}

void rt_alloc_dispose(rt_allocator* a)
{
    if (!a) return;
    // No per-allocator heap objects.
    memset(a, 0, sizeof(*a));
}

void rt_alloc_set_oom_handler(rt_allocator* a, rt_oom_fn fn, void* user)
{
    if (!a) return;
    a->oom = fn ? fn : default_oom_;
    a->oom_user = user;
}

static void* rt_malloc_raw_(size_t n)
{
    // malloc(0) is implementation-defined; normalize to 1.
    if (n == 0) n = 1;
    return malloc(n);
}

static void* rt_realloc_raw_(void* p, size_t n)
{
    if (n == 0) n = 1;
    return realloc(p, n);
}

void* rt_malloc(rt_allocator* a, size_t n)
{
    if (!a)
    {
        rt_set_msg_("invalid allocator");
        return NULL;
    }

#if RT_ALLOC_DEBUG
    size_t total = rt_dbg_total_(n);
    void* raw = rt_malloc_raw_(total);
#else
    void* raw = rt_malloc_raw_(n);
#endif

    if (!raw)
    {
        rt_set_errno_("malloc");
        a->oom(a->oom_user, n);
        return NULL;
    }

#if RT_HAVE_ATOMICS
    rt_atomic_add_u64_(&g_state_.alloc_count, 1);
    rt_atomic_add_u64_(&g_state_.bytes_total, (uint64_t)n);
#else
    g_state_.alloc_count++;
    g_state_.bytes_total += (uint64_t)n;
#endif

    state_add_live_((uint64_t)n);

#if RT_ALLOC_DEBUG
    return rt_dbg_wrap_(raw, n);
#else
    return raw;
#endif
}

void* rt_calloc(rt_allocator* a, size_t count, size_t size)
{
    if (!a)
    {
        rt_set_msg_("invalid allocator");
        return NULL;
    }

    // overflow check
    if (count != 0 && size > (SIZE_MAX / count))
    {
        rt_set_msg_("calloc overflow");
        a->oom(a->oom_user, (size_t)-1);
        return NULL;
    }

    size_t n = count * size;

#if RT_ALLOC_DEBUG
    size_t total = rt_dbg_total_(n);
    void* raw = rt_malloc_raw_(total);
    if (raw) memset(raw, 0, total);
#else
    void* raw = calloc(count, size);
#endif

    if (!raw)
    {
        rt_set_errno_("calloc");
        a->oom(a->oom_user, n);
        return NULL;
    }

#if RT_HAVE_ATOMICS
    rt_atomic_add_u64_(&g_state_.alloc_count, 1);
    rt_atomic_add_u64_(&g_state_.bytes_total, (uint64_t)n);
#else
    g_state_.alloc_count++;
    g_state_.bytes_total += (uint64_t)n;
#endif

    state_add_live_((uint64_t)n);

#if RT_ALLOC_DEBUG
    return rt_dbg_wrap_(raw, n);
#else
    return raw;
#endif
}

void* rt_realloc(rt_allocator* a, void* p, size_t n)
{
    if (!a)
    {
        rt_set_msg_("invalid allocator");
        return NULL;
    }

    if (!p)
        return rt_malloc(a, n);

#if RT_ALLOC_DEBUG
    if (!rt_dbg_check_(p))
    {
        rt_set_msg_("rt_realloc: invalid pointer (debug guard)" );
        abort();
    }

    rt_dbg_hdr* h = rt_dbg_hdr_(p);
    size_t old = h->size;

    size_t total = rt_dbg_total_(n);
    void* raw_old = (void*)h;
    void* raw_new = rt_realloc_raw_(raw_old, total);
    if (!raw_new)
    {
        rt_set_errno_("realloc");
        a->oom(a->oom_user, n);
        return NULL;
    }

    // update header/trailer
    void* user = rt_dbg_wrap_(raw_new, n);

    // adjust stats
    if (n > old) state_add_live_((uint64_t)(n - old));
    else state_sub_live_((uint64_t)(old - n));

#else
    // Without debug we cannot know old size; bytes_live becomes best-effort.
    void* raw_new = rt_realloc_raw_(p, n);
    if (!raw_new)
    {
        rt_set_errno_("realloc");
        a->oom(a->oom_user, n);
        return NULL;
    }

    // best-effort: treat as +n live (overcounts) if you never enable debug.
    state_add_live_((uint64_t)n);
    void* user = raw_new;
#endif

#if RT_HAVE_ATOMICS
    rt_atomic_add_u64_(&g_state_.realloc_count, 1);
    rt_atomic_add_u64_(&g_state_.bytes_total, (uint64_t)n);
#else
    g_state_.realloc_count++;
    g_state_.bytes_total += (uint64_t)n;
#endif

    return user;
}

void rt_free(rt_allocator* a, void* p)
{
    (void)a;
    if (!p) return;

#if RT_ALLOC_DEBUG
    if (!rt_dbg_check_(p))
    {
        rt_set_msg_("rt_free: invalid pointer (debug guard)");
        abort();
    }

    rt_dbg_hdr* h = rt_dbg_hdr_(p);
    size_t n = h->size;
    rt_dbg_mark_freed_(p);

    state_sub_live_((uint64_t)n);

    free((void*)h);
#else
    // no size knowledge
    free(p);
#endif

#if RT_HAVE_ATOMICS
    rt_atomic_add_u64_(&g_state_.free_count, 1);
#else
    g_state_.free_count++;
#endif
}

void* rt_memalign(rt_allocator* a, size_t alignment, size_t n)
{
    if (!a)
    {
        rt_set_msg_("invalid allocator");
        return NULL;
    }

    if (alignment < sizeof(void*))
        alignment = sizeof(void*);

    // alignment must be power of two for posix_memalign
    if ((alignment & (alignment - 1)) != 0)
    {
        rt_set_msg_("alignment must be power of two");
        return NULL;
    }

    void* p = NULL;

#if RT_ALLOC_DEBUG
    // In debug, we still want canaries; easiest: allocate extra then align manually.
    // Keep it simple and predictable: use posix_memalign for the raw block and
    // place header at the beginning of the aligned block.
    size_t total = rt_dbg_total_(n);
    int rc = posix_memalign(&p, alignment, total);
    if (rc != 0)
    {
        errno = rc;
        rt_set_errno_("posix_memalign");
        a->oom(a->oom_user, n);
        return NULL;
    }

    // zero init not required; wrap
    void* user = rt_dbg_wrap_(p, n);
#else
    int rc = posix_memalign(&p, alignment, (n == 0 ? 1 : n));
    if (rc != 0)
    {
        errno = rc;
        rt_set_errno_("posix_memalign");
        a->oom(a->oom_user, n);
        return NULL;
    }
    void* user = p;
#endif

#if RT_HAVE_ATOMICS
    rt_atomic_add_u64_(&g_state_.alloc_count, 1);
    rt_atomic_add_u64_(&g_state_.bytes_total, (uint64_t)n);
#else
    g_state_.alloc_count++;
    g_state_.bytes_total += (uint64_t)n;
#endif

    state_add_live_((uint64_t)n);
    return user;
}

void rt_aligned_free(rt_allocator* a, void* p)
{
    (void)a;
    // same as free for posix_memalign
    rt_free(a, p);
}

char* rt_strdup(rt_allocator* a, const char* s)
{
    if (!s) s = "";
    size_t n = strlen(s);
    char* p = (char*)rt_malloc(a, n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = 0;
    return p;
}

rt_alloc_stats rt_alloc_get_stats(const rt_allocator* a)
{
    (void)a;
    rt_alloc_stats st;
    memset(&st, 0, sizeof(st));

#if RT_HAVE_ATOMICS
    st.alloc_count = rt_atomic_load_u64_(&g_state_.alloc_count);
    st.free_count = rt_atomic_load_u64_(&g_state_.free_count);
    st.realloc_count = rt_atomic_load_u64_(&g_state_.realloc_count);
    st.bytes_live = rt_atomic_load_u64_(&g_state_.bytes_live);
    st.bytes_peak = rt_atomic_load_u64_(&g_state_.bytes_peak);
    st.bytes_total = rt_atomic_load_u64_(&g_state_.bytes_total);
#else
    st.alloc_count = g_state_.alloc_count;
    st.free_count = g_state_.free_count;
    st.realloc_count = g_state_.realloc_count;
    st.bytes_live = g_state_.bytes_live;
    st.bytes_peak = g_state_.bytes_peak;
    st.bytes_total = g_state_.bytes_total;
#endif

    return st;
}
