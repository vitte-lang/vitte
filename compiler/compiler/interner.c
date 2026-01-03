// SPDX-License-Identifier: MIT
// interner.c
//
// String interner used by the compiler.
//
// Features:
//  - Stores unique copies of byte strings and returns stable pointers.
//  - Open-addressing hash table (linear probing), power-of-two capacity.
//  - FNV-1a hash (64-bit) with stored length.
//  - Interning from NUL-terminated or (ptr,len).
//  - Optional stats/debug.
//
// Ownership:
//  - Interned strings are allocated from an internal arena of chunks.
//  - All memory is freed when the interner is disposed.
//
// Pairing:
//  - interner.h should declare:
//      typedef struct steel_interner steel_interner;
//      void  steel_interner_init(steel_interner* it);
//      void  steel_interner_dispose(steel_interner* it);
//      const char* steel_interner_intern(steel_interner* it, const char* s);
//      const char* steel_interner_intern_n(steel_interner* it, const char* s, size_t n);
//      size_t steel_interner_count(const steel_interner* it);
//      size_t steel_interner_capacity(const steel_interner* it);
//
// If your header uses different names/types, provide wrappers there.

#include "interner.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef STEEL_INTERNER_CHUNK_MIN
  #define STEEL_INTERNER_CHUNK_MIN (64u * 1024u)
#endif

#ifndef STEEL_INTERNER_INITIAL_CAP
  #define STEEL_INTERNER_INITIAL_CAP 1024u
#endif

#ifndef STEEL_INTERNER_LOAD_NUM
  #define STEEL_INTERNER_LOAD_NUM 7u
#endif
#ifndef STEEL_INTERNER_LOAD_DEN
  #define STEEL_INTERNER_LOAD_DEN 10u
#endif

//------------------------------------------------------------------------------
// Arena chunks
//------------------------------------------------------------------------------

typedef struct steel_intern_chunk
{
    struct steel_intern_chunk* next;
    size_t cap;
    size_t used;
    // uint8_t data[]
} steel_intern_chunk;

static size_t steel_align_up(size_t x, size_t a)
{
    return (x + (a - 1)) & ~(a - 1);
}

static void* steel_chunk_alloc(steel_intern_chunk** head, size_t size, size_t align)
{
    if (size == 0) size = 1;
    if (align < 8) align = 8;

    size = steel_align_up(size, align);

    steel_intern_chunk* c = *head;
    if (!c || steel_align_up(c->used, align) + size > c->cap)
    {
        size_t want = STEEL_INTERNER_CHUNK_MIN;
        if (want < size + 64) want = size + 64;

        size_t total = sizeof(steel_intern_chunk) + want;
        steel_intern_chunk* nc = (steel_intern_chunk*)malloc(total);
        if (!nc) return NULL;

        nc->next = *head;
        nc->cap = want;
        nc->used = 0;
        *head = nc;
        c = nc;
    }

    size_t off = steel_align_up(c->used, align);
    uint8_t* base = (uint8_t*)(c + 1);
    void* p = base + off;
    c->used = off + size;
    return p;
}

static void steel_chunks_free_all(steel_intern_chunk** head)
{
    steel_intern_chunk* c = *head;
    while (c)
    {
        steel_intern_chunk* n = c->next;
        free(c);
        c = n;
    }
    *head = NULL;
}

static char* steel_chunk_strdup_n(steel_intern_chunk** head, const char* s, size_t n)
{
    char* p = (char*)steel_chunk_alloc(head, n + 1, 1);
    if (!p) return NULL;
    if (n) memcpy(p, s, n);
    p[n] = 0;
    return p;
}

//------------------------------------------------------------------------------
// Hashing
//------------------------------------------------------------------------------

static uint64_t steel_fnv1a64(const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < len; i++)
    {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ull;
    }
    return h ? h : 1ull; // avoid 0 sentinel
}

static bool steel_is_pow2(size_t x)
{
    return x && ((x & (x - 1)) == 0);
}

//------------------------------------------------------------------------------
// Table
//------------------------------------------------------------------------------

typedef struct steel_intern_entry
{
    uint64_t hash;
    const char* str;
    size_t len;
} steel_intern_entry;

struct steel_interner
{
    steel_intern_entry* entries;
    size_t cap;
    size_t len;

    steel_intern_chunk* chunks;
};

static bool steel_table_rehash(steel_interner* it, size_t new_cap)
{
    steel_intern_entry* ne = (steel_intern_entry*)calloc(new_cap, sizeof(*ne));
    if (!ne) return false;

    for (size_t i = 0; i < it->cap; i++)
    {
        steel_intern_entry e = it->entries[i];
        if (!e.str) continue;

        size_t mask = new_cap - 1;
        size_t idx = (size_t)e.hash & mask;
        while (ne[idx].str)
            idx = (idx + 1) & mask;
        ne[idx] = e;
    }

    free(it->entries);
    it->entries = ne;
    it->cap = new_cap;
    return true;
}

static bool steel_table_ensure_cap(steel_interner* it)
{
    if (it->cap == 0)
        return steel_table_rehash(it, STEEL_INTERNER_INITIAL_CAP);

    if (!steel_is_pow2(it->cap))
        return steel_table_rehash(it, STEEL_INTERNER_INITIAL_CAP);

    if ((it->len + 1) * STEEL_INTERNER_LOAD_DEN < it->cap * STEEL_INTERNER_LOAD_NUM)
        return true;

    return steel_table_rehash(it, it->cap * 2);
}

static const char* steel_table_put(steel_interner* it, const char* s, size_t n)
{
    if (!s) { s = ""; n = 0; }
    if (!steel_table_ensure_cap(it))
        return NULL;

    uint64_t h = steel_fnv1a64(s, n);
    size_t mask = it->cap - 1;
    size_t idx = (size_t)h & mask;

    while (it->entries[idx].str)
    {
        steel_intern_entry* e = &it->entries[idx];
        if (e->hash == h && e->len == n && memcmp(e->str, s, n) == 0)
            return e->str;
        idx = (idx + 1) & mask;
    }

    char* copy = steel_chunk_strdup_n(&it->chunks, s, n);
    if (!copy) return NULL;

    it->entries[idx].hash = h;
    it->entries[idx].str = copy;
    it->entries[idx].len = n;
    it->len++;
    return copy;
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void steel_interner_init(steel_interner* it)
{
    if (!it) return;
    memset(it, 0, sizeof(*it));
}

void steel_interner_dispose(steel_interner* it)
{
    if (!it) return;

    free(it->entries);
    it->entries = NULL;
    it->cap = 0;
    it->len = 0;

    steel_chunks_free_all(&it->chunks);

    memset(it, 0, sizeof(*it));
}

const char* steel_interner_intern(steel_interner* it, const char* s)
{
    if (!it) return NULL;
    if (!s) s = "";
    return steel_table_put(it, s, strlen(s));
}

const char* steel_interner_intern_n(steel_interner* it, const char* s, size_t n)
{
    if (!it) return NULL;
    if (!s) { s = ""; n = 0; }
    return steel_table_put(it, s, n);
}

size_t steel_interner_count(const steel_interner* it)
{
    return it ? it->len : 0;
}

size_t steel_interner_capacity(const steel_interner* it)
{
    return it ? it->cap : 0;
}

