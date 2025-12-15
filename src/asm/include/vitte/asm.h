#pragma once
#include <stddef.h>
#include <stdint.h>

/* Stable API used by the rest of the codebase. */
void*    vitte_memcpy_fast(void* dst, const void* src, size_t n);
void*    vitte_memset_fast(void* dst, int byte, size_t n);
uint64_t vitte_fnv1a64_fast(const void* data, size_t n);

/* Optional: UTF-8 validation fastpath. Implement ref in C and keep asm as a stub for now. */
int      vitte_utf8_validate_fast(const uint8_t* p, size_t n);

/* Reference (C) implementations (always correct; used for verification/fallback). */
void*    vitte_memcpy_ref(void* dst, const void* src, size_t n);
void*    vitte_memset_ref(void* dst, int byte, size_t n);
uint64_t vitte_fnv1a64_ref(const void* data, size_t n);

/* Optional: provide this in C if you enable utf8 asm stubs. */
int      vitte_utf8_validate_ref(const uint8_t* p, size_t n);
