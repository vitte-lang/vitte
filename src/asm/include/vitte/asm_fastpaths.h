// vitte/src/asm/include/vitte/asm_fastpaths.h
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Low-level assembly fastpaths (per-architecture implementations)
// ============================================================================

// Hash
uint64_t vitte_fnv1a64(const uint8_t* data, size_t len);

// Memory (baseline implementations are always available)
void* vitte_memcpy_baseline(void* dst, const void* src, size_t n);
void* vitte_memset_baseline(void* dst, int c, size_t n);

// x86_64 variants
void* vitte_memcpy_sse2(void* dst, const void* src, size_t n);
void* vitte_memcpy_avx2(void* dst, const void* src, size_t n);
void* vitte_memset_sse2(void* dst, int c, size_t n);
void* vitte_memset_avx2(void* dst, int c, size_t n);

// AArch64 variants
void* vitte_memcpy_neon(void* dst, const void* src, size_t n);
void* vitte_memset_neon(void* dst, int c, size_t n);

// UTF-8 validator stub (scalar baseline shared across arches)
int vitte_utf8_validate_stub(const uint8_t* data, size_t len);

#ifdef __cplusplus
} // extern "C"
#endif

