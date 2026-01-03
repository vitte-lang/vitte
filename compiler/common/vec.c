

// SPDX-License-Identifier: MIT
// vec.c
//
// Generic dynamic array (vector) utilities for vitte.
//
// Design:
//  - Type-erased storage: items are raw bytes.
//  - Caller specifies element size.
//  - Capacity growth: geometric (x2) with overflow checks.
//  - Optional zero-init for new elements.
//  - Simple, deterministic API.
//
// Pairing:
//  - vec.h is expected to define the public structs and function declarations.
//
// Expected minimal shapes (adjust vec.h or this file if your header differs):
//
//   typedef struct steel_vec {
//       void*  data;
//       size_t len;
//       size_t cap;
//       size_t elem_size;
//   } steel_vec;
//
//   typedef struct steel_vec_res {
//       bool ok;
//       int  err; // 0 on success; errno-style codes on failure
//   } steel_vec_res;
//
// And declarations such as:
//   void steel_vec_init(steel_vec* v, size_t elem_size);
//   void steel_vec_free(steel_vec* v);
//   void steel_vec_clear(steel_vec* v);
//   bool steel_vec_reserve(steel_vec* v, size_t cap);
//   bool steel_vec_resize(steel_vec* v, size_t new_len, bool zero_new);
//   void* steel_vec_at(steel_vec* v, size_t i);
//   const void* steel_vec_cat(const steel_vec* v, size_t i);
//   void* steel_vec_push(steel_vec* v, const void* elem);
//   bool steel_vec_pop(steel_vec* v, void* out_elem);
//   bool steel_vec_insert(steel_vec* v, size_t i, const void* elem);
//   bool steel_vec_remove(steel_vec* v, size_t i, void* out_elem);
//   bool steel_vec_shrink_to_fit(steel_vec* v);
//
// This implementation follows those semantics.

#include "vec.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#ifndef STEEL_VEC_MIN_CAP
  #define STEEL_VEC_MIN_CAP 8u
#endif

//------------------------------------------------------------------------------
// Internal helpers
//------------------------------------------------------------------------------

static bool steel_mul_overflow_size(size_t a, size_t b, size_t* out)
{
#if defined(__has_builtin)
  #if __has_builtin(__builtin_mul_overflow)
    return __builtin_mul_overflow(a, b, out);
  #endif
#endif
    if (a == 0 || b == 0)
    {
        *out = 0;
        return false;
    }
    if (SIZE_MAX / a < b) return true;
    *out = a * b;
    return false;
}

static bool steel_add_overflow_size(size_t a, size_t b, size_t* out)
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

static size_t steel_vec_bytes_for(const steel_vec* v, size_t cap)
{
    size_t bytes = 0;
    if (steel_mul_overflow_size(cap, v->elem_size, &bytes)) return 0;
    return bytes;
}

static bool steel_vec_grow_to(steel_vec* v, size_t want_cap)
{
    if (!v) return false;
    if (want_cap <= v->cap) return true;

    // Determine new capacity (power-of-two-ish geometric growth).
    size_t new_cap = (v->cap == 0) ? STEEL_VEC_MIN_CAP : v->cap;
    while (new_cap < want_cap)
    {
        size_t next;
        if (steel_add_overflow_size(new_cap, new_cap, &next))
        {
            errno = ENOMEM;
            return false;
        }
        new_cap = next;
    }

    size_t new_bytes = steel_vec_bytes_for(v, new_cap);
    if (new_bytes == 0 && new_cap != 0)
    {
        errno = ENOMEM;
        return false;
    }

    void* p = realloc(v->data, new_bytes ? new_bytes : 1);
    if (!p)
    {
        errno = ENOMEM;
        return false;
    }

    v->data = p;
    v->cap = new_cap;
    return true;
}

static void steel_vec_move_right(steel_vec* v, size_t from, size_t count)
{
    // Move [from, len) right by count elements.
    // Assumes enough capacity.
    size_t es = v->elem_size;
    uint8_t* base = (uint8_t*)v->data;

    size_t tail = v->len - from;
    memmove(base + (from + count) * es, base + from * es, tail * es);
}

static void steel_vec_move_left(steel_vec* v, size_t from, size_t count)
{
    // Move [from+count, len) left by count elements.
    size_t es = v->elem_size;
    uint8_t* base = (uint8_t*)v->data;

    size_t tail = v->len - (from + count);
    memmove(base + from * es, base + (from + count) * es, tail * es);
}

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

void steel_vec_init(steel_vec* v, size_t elem_size)
{
    if (!v) return;
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
    v->elem_size = (elem_size == 0) ? 1 : elem_size;
}

void steel_vec_free(steel_vec* v)
{
    if (!v) return;
    free(v->data);
    v->data = NULL;
    v->len = 0;
    v->cap = 0;
    // keep elem_size
}

void steel_vec_clear(steel_vec* v)
{
    if (!v) return;
    v->len = 0;
}

bool steel_vec_reserve(steel_vec* v, size_t cap)
{
    if (!v) return false;
    return steel_vec_grow_to(v, cap);
}

bool steel_vec_resize(steel_vec* v, size_t new_len, bool zero_new)
{
    if (!v) return false;

    if (new_len > v->cap)
    {
        if (!steel_vec_grow_to(v, new_len)) return false;
    }

    if (zero_new && new_len > v->len)
    {
        size_t es = v->elem_size;
        uint8_t* base = (uint8_t*)v->data;
        memset(base + v->len * es, 0, (new_len - v->len) * es);
    }

    v->len = new_len;
    return true;
}

void* steel_vec_at(steel_vec* v, size_t i)
{
    if (!v || !v->data) return NULL;
    if (i >= v->len) return NULL;
    return (uint8_t*)v->data + i * v->elem_size;
}

const void* steel_vec_cat(const steel_vec* v, size_t i)
{
    if (!v || !v->data) return NULL;
    if (i >= v->len) return NULL;
    return (const uint8_t*)v->data + i * v->elem_size;
}

void* steel_vec_data(steel_vec* v)
{
    return v ? v->data : NULL;
}

const void* steel_vec_cdata(const steel_vec* v)
{
    return v ? v->data : NULL;
}

size_t steel_vec_len(const steel_vec* v)
{
    return v ? v->len : 0;
}

size_t steel_vec_cap(const steel_vec* v)
{
    return v ? v->cap : 0;
}

size_t steel_vec_elem_size(const steel_vec* v)
{
    return v ? v->elem_size : 0;
}

void* steel_vec_push(steel_vec* v, const void* elem)
{
    if (!v) return NULL;
    if (v->len + 1 > v->cap)
    {
        size_t want;
        if (steel_add_overflow_size(v->len, 1, &want))
        {
            errno = ENOMEM;
            return NULL;
        }
        if (!steel_vec_grow_to(v, want)) return NULL;
    }

    uint8_t* dst = (uint8_t*)v->data + v->len * v->elem_size;
    if (elem)
        memcpy(dst, elem, v->elem_size);
    else
        memset(dst, 0, v->elem_size);

    v->len++;
    return dst;
}

bool steel_vec_pop(steel_vec* v, void* out_elem)
{
    if (!v || v->len == 0) return false;

    size_t i = v->len - 1;
    uint8_t* src = (uint8_t*)v->data + i * v->elem_size;

    if (out_elem)
        memcpy(out_elem, src, v->elem_size);

    v->len = i;
    return true;
}

bool steel_vec_insert(steel_vec* v, size_t i, const void* elem)
{
    if (!v) return false;
    if (i > v->len) return false;

    if (v->len + 1 > v->cap)
    {
        size_t want;
        if (steel_add_overflow_size(v->len, 1, &want))
        {
            errno = ENOMEM;
            return false;
        }
        if (!steel_vec_grow_to(v, want)) return false;
    }

    if (i < v->len)
        steel_vec_move_right(v, i, 1);

    uint8_t* dst = (uint8_t*)v->data + i * v->elem_size;
    if (elem)
        memcpy(dst, elem, v->elem_size);
    else
        memset(dst, 0, v->elem_size);

    v->len++;
    return true;
}

bool steel_vec_remove(steel_vec* v, size_t i, void* out_elem)
{
    if (!v || i >= v->len) return false;

    uint8_t* src = (uint8_t*)v->data + i * v->elem_size;
    if (out_elem)
        memcpy(out_elem, src, v->elem_size);

    if (i + 1 < v->len)
        steel_vec_move_left(v, i, 1);

    v->len--;
    return true;
}

bool steel_vec_shrink_to_fit(steel_vec* v)
{
    if (!v) return false;

    if (v->len == 0)
    {
        free(v->data);
        v->data = NULL;
        v->cap = 0;
        return true;
    }

    size_t bytes = steel_vec_bytes_for(v, v->len);
    if (bytes == 0)
    {
        errno = ENOMEM;
        return false;
    }

    void* p = realloc(v->data, bytes);
    if (!p)
    {
        errno = ENOMEM;
        return false;
    }

    v->data = p;
    v->cap = v->len;
    return true;
}

// Convenience: push N zeroed elements and return pointer to first new element.
void* steel_vec_pushn_zero(steel_vec* v, size_t n)
{
    if (!v) return NULL;
    if (n == 0) return (v->data ? (uint8_t*)v->data + v->len * v->elem_size : NULL);

    size_t new_len;
    if (steel_add_overflow_size(v->len, n, &new_len))
    {
        errno = ENOMEM;
        return NULL;
    }

    size_t old_len = v->len;
    if (!steel_vec_resize(v, new_len, true)) return NULL;

    return (uint8_t*)v->data + old_len * v->elem_size;
}

// Convenience: reserve and append raw bytes.
void* steel_vec_append_bytes(steel_vec* v, const void* data, size_t bytes)
{
    if (!v || v->elem_size != 1) return NULL;
    if (bytes == 0) return (uint8_t*)v->data + v->len;

    size_t new_len;
    if (steel_add_overflow_size(v->len, bytes, &new_len))
    {
        errno = ENOMEM;
        return NULL;
    }

    size_t old_len = v->len;
    if (!steel_vec_resize(v, new_len, false)) return NULL;

    uint8_t* dst = (uint8_t*)v->data + old_len;
    if (data) memcpy(dst, data, bytes);
    else memset(dst, 0, bytes);

    return dst;
}