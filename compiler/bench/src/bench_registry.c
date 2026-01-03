// SPDX-License-Identifier: MIT
// bench_registry.c - benchmark case registry (C17)
//
// This file provides a small, dependency-light registry used by the bench runner
// to discover and enumerate available benchmarks.
//
// Design goals:
//  - No global constructors; explicit init + register.
//  - Registry stores stable copies of strings (owned by registry).
//  - Simple API: init/destroy/add/entries/find/sort.
//
// If your project already has a public header, wire these declarations into it
// and include that header here.

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//------------------------------------------------------------------------------
// Forward declarations (keep these minimal; your bench runner can provide the
// actual definition of bench_ctx).
//------------------------------------------------------------------------------

typedef struct bench_ctx bench_ctx;

typedef void (*bench_fn)(bench_ctx* ctx);

//------------------------------------------------------------------------------
// Registry types
//------------------------------------------------------------------------------

typedef struct bench_entry {
    char*    name;   // owned (NUL-terminated)
    char*    group;  // owned (optional, can be NULL)
    bench_fn fn;     // benchmark callback
} bench_entry;

typedef struct bench_registry {
    bench_entry* entries;
    size_t       len;
    size_t       cap;
} bench_registry;

//------------------------------------------------------------------------------
// Local helpers
//------------------------------------------------------------------------------

static void* bench_xrealloc(void* p, size_t n) {
    void* q = realloc(p, n);
    if (!q && n != 0) {
        fprintf(stderr, "bench_registry: OOM realloc(%zu)\n", n);
        abort();
    }
    return q;
}

static char* bench_xstrdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char*  p = (char*)malloc(n);
    if (!p) {
        fprintf(stderr, "bench_registry: OOM strdup(%zu)\n", n);
        abort();
    }
    memcpy(p, s, n);
    return p;
}

static int bench_entry_cmp_name(const void* a, const void* b) {
    const bench_entry* ea = (const bench_entry*)a;
    const bench_entry* eb = (const bench_entry*)b;
    const char*        na = ea->name ? ea->name : "";
    const char*        nb = eb->name ? eb->name : "";
    return strcmp(na, nb);
}

//------------------------------------------------------------------------------
// Public-ish API (define these in a header if you want external use)
//------------------------------------------------------------------------------

void bench_registry_init(bench_registry* r) {
    assert(r);
    memset(r, 0, sizeof(*r));
}

void bench_registry_destroy(bench_registry* r) {
    if (!r) return;
    for (size_t i = 0; i < r->len; ++i) {
        free(r->entries[i].name);
        free(r->entries[i].group);
        r->entries[i].name  = NULL;
        r->entries[i].group = NULL;
        r->entries[i].fn    = NULL;
    }
    free(r->entries);
    r->entries = NULL;
    r->len = 0;
    r->cap = 0;
}

// Returns 0 on success, negative on error.
int bench_registry_add(bench_registry* r, const char* name, bench_fn fn, const char* group) {
    if (!r || !name || !*name || !fn) {
        return -EINVAL;
    }

    // Prevent duplicates by name (first wins).
    for (size_t i = 0; i < r->len; ++i) {
        if (r->entries[i].name && strcmp(r->entries[i].name, name) == 0) {
            return -EEXIST;
        }
    }

    if (r->len == r->cap) {
        size_t new_cap = (r->cap == 0) ? 16 : (r->cap * 2);
        r->entries = (bench_entry*)bench_xrealloc(r->entries, new_cap * sizeof(bench_entry));
        // zero new tail to keep valgrind clean
        for (size_t i = r->cap; i < new_cap; ++i) {
            r->entries[i].name  = NULL;
            r->entries[i].group = NULL;
            r->entries[i].fn    = NULL;
        }
        r->cap = new_cap;
    }

    bench_entry* e = &r->entries[r->len++];
    e->name  = bench_xstrdup(name);
    e->group = bench_xstrdup(group);
    e->fn    = fn;
    return 0;
}

void bench_registry_sort_by_name(bench_registry* r) {
    if (!r || r->len <= 1) return;
    qsort(r->entries, r->len, sizeof(r->entries[0]), bench_entry_cmp_name);
}

const bench_entry* bench_registry_entries(const bench_registry* r, size_t* out_count) {
    if (out_count) *out_count = r ? r->len : 0;
    return r ? r->entries : NULL;
}

const bench_entry* bench_registry_find(const bench_registry* r, const char* name) {
    if (!r || !name) return NULL;
    for (size_t i = 0; i < r->len; ++i) {
        if (r->entries[i].name && strcmp(r->entries[i].name, name) == 0) {
            return &r->entries[i];
        }
    }
    return NULL;
}

//------------------------------------------------------------------------------
// Built-in registrations
//------------------------------------------------------------------------------
//
// Wire your benchmark functions here. The extern names below are intentionally
// simple; rename them to match your actual files (e.g. bm_add.c).
//
// Convention used:
//   - name  : "micro/<bench>" or "suite/<bench>"
//   - group : "micro" / "suite" etc.
//

// NOTE: rename these externs to match your real symbols.
extern void bm_add(bench_ctx*);
extern void bm_hash(bench_ctx*);
extern void bm_memcpy(bench_ctx*);
extern void bm_json_parse(bench_ctx*);

// Registers all built-in benchmarks.
// Returns 0 on success; negative errno-style on error.
int bench_registry_register_builtins(bench_registry* r) {
    if (!r) return -EINVAL;

    int rc = 0;

    // Micro benchmarks
    rc = bench_registry_add(r, "micro/add",        bm_add,        "micro");
    if (rc && rc != -EEXIST) return rc;

    rc = bench_registry_add(r, "micro/hash",       bm_hash,       "micro");
    if (rc && rc != -EEXIST) return rc;

    rc = bench_registry_add(r, "micro/memcpy",     bm_memcpy,     "micro");
    if (rc && rc != -EEXIST) return rc;

    rc = bench_registry_add(r, "micro/json_parse", bm_json_parse, "micro");
    if (rc && rc != -EEXIST) return rc;

    bench_registry_sort_by_name(r);
    return 0;
}

//------------------------------------------------------------------------------
// Optional singleton (handy for small bench executables)
//------------------------------------------------------------------------------

static bench_registry g_bench_registry;
static bool          g_bench_registry_inited = false;

bench_registry* bench_registry_global(void) {
    if (!g_bench_registry_inited) {
        bench_registry_init(&g_bench_registry);
        (void)bench_registry_register_builtins(&g_bench_registry);
        g_bench_registry_inited = true;
    }
    return &g_bench_registry;
}

void bench_registry_global_shutdown(void) {
    if (g_bench_registry_inited) {
        bench_registry_destroy(&g_bench_registry);
        g_bench_registry_inited = false;
    }
}
