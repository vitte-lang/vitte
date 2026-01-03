// SPDX-License-Identifier: MIT
// hashmap.c
//
// Minimal open-addressing hash map for string keys.
//
// Design:
//  - Robin-hood style linear probing (bounded by load factor).
//  - Keys are immutable null-terminated strings; values are `void*`.
//  - Deletions use tombstones.
//  - Hash: 64-bit FNV-1a.
//
// This file pairs with hashmap.h.
// hashmap.h is expected to declare the functions implemented here and define:
//   typedef struct steel_hashmap steel_hashmap;
//   typedef struct steel_hashmap_iter steel_hashmap_iter;
//   and public API signatures.
//
// If your header differs, adjust accordingly.

#include "hashmap.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef STEEL_HM_MIN_CAP
  #define STEEL_HM_MIN_CAP 16u
#endif

#ifndef STEEL_HM_MAX_LOAD_NUM
  #define STEEL_HM_MAX_LOAD_NUM 7u
#endif
#ifndef STEEL_HM_MAX_LOAD_DEN
  #define STEEL_HM_MAX_LOAD_DEN 10u
#endif

//------------------------------------------------------------------------------
// Internal types
//------------------------------------------------------------------------------

typedef enum steel_hm_state
{
    STEEL_HM_EMPTY = 0,
    STEEL_HM_FULL  = 1,
    STEEL_HM_TOMB  = 2,
} steel_hm_state;

typedef struct steel_hm_entry
{
    const char* key;   // owned by caller unless API says otherwise
    void* value;
    uint64_t hash;
    uint8_t state;
} steel_hm_entry;

struct steel_hashmap
{
    steel_hm_entry* entries;
    size_t cap;     // power of two
    size_t len;     // number of FULL entries
    size_t tombs;   // number of TOMB entries
};

struct steel_hashmap_iter
{
    const steel_hashmap* map;
    size_t idx;
};

//------------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------------

static void* steel_xmalloc(size_t n)
{
    void* p = malloc(n);
    if (!p)
    {
        fprintf(stderr, "vitte: out of memory\n");
        abort();
    }
    return p;
}

static void* steel_xrealloc(void* p, size_t n)
{
    void* q = realloc(p, n);
    if (!q)
    {
        fprintf(stderr, "vitte: out of memory\n");
        abort();
    }
    return q;
}

static bool steel_is_pow2(size_t x)
{
    return x && ((x & (x - 1)) == 0);
}

static size_t steel_round_pow2(size_t x)
{
    if (x < 2) return 2;
    // Round up to next power of two.
    size_t p = 1;
    while (p < x) p <<= 1;
    return p;
}

static uint64_t steel_fnv1a64(const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < len; i++)
    {
        h ^= (uint64_t)p[i];
        h *= 1099511628211ull;
    }
    return h;
}

static uint64_t steel_hash_str(const char* s)
{
    return steel_fnv1a64(s, strlen(s));
}

static bool steel_key_eq(const char* a, const char* b)
{
    return strcmp(a, b) == 0;
}

static size_t steel_hm_mask(const steel_hashmap* m)
{
    return m->cap - 1;
}

static bool steel_hm_should_grow(const steel_hashmap* m)
{
    // Grow when (len + tombs) / cap > max_load
    size_t used = m->len + m->tombs;
    return (used * STEEL_HM_MAX_LOAD_DEN) > (m->cap * STEEL_HM_MAX_LOAD_NUM);
}

static void steel_hm_entries_clear(steel_hm_entry* e, size_t cap)
{
    memset(e, 0, cap * sizeof(*e));
}

//------------------------------------------------------------------------------
// Core insert/find
//------------------------------------------------------------------------------

typedef struct steel_hm_find_result
{
    bool found;
    size_t idx;
} steel_hm_find_result;

static steel_hm_find_result steel_hm_find_slot(const steel_hashmap* m, const char* key, uint64_t hash)
{
    // Linear probing with tombstones.
    // Returns either the existing key slot, or the best insertion slot.
    size_t mask = steel_hm_mask(m);
    size_t i = (size_t)hash & mask;

    size_t first_tomb = (size_t)-1;

    for (;;)
    {
        const steel_hm_entry* e = &m->entries[i];

        if (e->state == STEEL_HM_EMPTY)
        {
            // If we saw a tombstone earlier, insert there.
            if (first_tomb != (size_t)-1)
                return (steel_hm_find_result){ .found = false, .idx = first_tomb };
            return (steel_hm_find_result){ .found = false, .idx = i };
        }

        if (e->state == STEEL_HM_TOMB)
        {
            if (first_tomb == (size_t)-1) first_tomb = i;
        }
        else if (e->hash == hash && e->key && steel_key_eq(e->key, key))
        {
            return (steel_hm_find_result){ .found = true, .idx = i };
        }

        i = (i + 1) & mask;
    }
}

static void steel_hm_rehash(steel_hashmap* m, size_t new_cap)
{
    new_cap = steel_round_pow2(new_cap);
    if (new_cap < STEEL_HM_MIN_CAP) new_cap = STEEL_HM_MIN_CAP;

    steel_hm_entry* old = m->entries;
    size_t old_cap = m->cap;

    m->entries = (steel_hm_entry*)steel_xmalloc(new_cap * sizeof(steel_hm_entry));
    m->cap = new_cap;
    m->len = 0;
    m->tombs = 0;
    steel_hm_entries_clear(m->entries, m->cap);

    if (old)
    {
        for (size_t i = 0; i < old_cap; i++)
        {
            steel_hm_entry* e = &old[i];
            if (e->state != STEEL_HM_FULL) continue;

            steel_hm_find_result r = steel_hm_find_slot(m, e->key, e->hash);
            steel_hm_entry* dst = &m->entries[r.idx];
            dst->key = e->key;
            dst->value = e->value;
            dst->hash = e->hash;
            dst->state = STEEL_HM_FULL;
            m->len++;
        }
        free(old);
    }
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void steel_hashmap_init(steel_hashmap* m, size_t initial_cap)
{
    if (!m) return;
    if (initial_cap == 0) initial_cap = STEEL_HM_MIN_CAP;
    initial_cap = steel_round_pow2(initial_cap);
    if (initial_cap < STEEL_HM_MIN_CAP) initial_cap = STEEL_HM_MIN_CAP;

    m->entries = NULL;
    m->cap = 0;
    m->len = 0;
    m->tombs = 0;

    steel_hm_rehash(m, initial_cap);
}

void steel_hashmap_free(steel_hashmap* m)
{
    if (!m) return;
    free(m->entries);
    m->entries = NULL;
    m->cap = 0;
    m->len = 0;
    m->tombs = 0;
}

void steel_hashmap_clear(steel_hashmap* m)
{
    if (!m || !m->entries) return;
    steel_hm_entries_clear(m->entries, m->cap);
    m->len = 0;
    m->tombs = 0;
}

size_t steel_hashmap_len(const steel_hashmap* m)
{
    return m ? m->len : 0;
}

bool steel_hashmap_get(const steel_hashmap* m, const char* key, void** out_value)
{
    if (out_value) *out_value = NULL;
    if (!m || !m->entries || !key) return false;

    uint64_t h = steel_hash_str(key);
    steel_hm_find_result r = steel_hm_find_slot(m, key, h);
    if (!r.found) return false;

    if (out_value) *out_value = m->entries[r.idx].value;
    return true;
}

bool steel_hashmap_has(const steel_hashmap* m, const char* key)
{
    return steel_hashmap_get(m, key, NULL);
}

bool steel_hashmap_put(steel_hashmap* m, const char* key, void* value)
{
    if (!m || !key) return false;

    if (!m->entries || m->cap == 0)
        steel_hm_rehash(m, STEEL_HM_MIN_CAP);

    if (steel_hm_should_grow(m))
        steel_hm_rehash(m, m->cap * 2);

    uint64_t h = steel_hash_str(key);
    steel_hm_find_result r = steel_hm_find_slot(m, key, h);

    steel_hm_entry* e = &m->entries[r.idx];
    if (r.found)
    {
        e->value = value;
        return true;
    }

    if (e->state == STEEL_HM_TOMB) m->tombs--;

    e->key = key;
    e->value = value;
    e->hash = h;
    e->state = STEEL_HM_FULL;
    m->len++;
    return true;
}

bool steel_hashmap_remove(steel_hashmap* m, const char* key, void** out_old)
{
    if (out_old) *out_old = NULL;
    if (!m || !m->entries || !key) return false;

    uint64_t h = steel_hash_str(key);
    steel_hm_find_result r = steel_hm_find_slot(m, key, h);
    if (!r.found) return false;

    steel_hm_entry* e = &m->entries[r.idx];
    if (out_old) *out_old = e->value;

    e->key = NULL;
    e->value = NULL;
    e->hash = 0;
    e->state = STEEL_HM_TOMB;

    m->len--;
    m->tombs++;

    // If too many tombstones, rehash in-place.
    if (m->tombs * 2 > m->cap)
        steel_hm_rehash(m, m->cap);

    return true;
}

// Iteration

steel_hashmap_iter steel_hashmap_iter_begin(const steel_hashmap* m)
{
    steel_hashmap_iter it;
    it.map = m;
    it.idx = 0;
    return it;
}

bool steel_hashmap_iter_next(steel_hashmap_iter* it, const char** out_key, void** out_value)
{
    if (out_key) *out_key = NULL;
    if (out_value) *out_value = NULL;

    if (!it || !it->map || !it->map->entries) return false;

    const steel_hashmap* m = it->map;
    while (it->idx < m->cap)
    {
        size_t i = it->idx++;
        const steel_hm_entry* e = &m->entries[i];
        if (e->state == STEEL_HM_FULL)
        {
            if (out_key) *out_key = e->key;
            if (out_value) *out_value = e->value;
            return true;
        }
    }

    return false;
}
