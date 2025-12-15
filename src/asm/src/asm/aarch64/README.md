// vitte/src/asm/include/vitte/asm_impl.h
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Internal ASM symbols (per-arch impls)
//
// But:
//   - Exposed as C-callable functions
//   - Used by vitte runtime / std / tools as fastpaths
//   - Keep names stable; dispatch can be done elsewhere (cpu caps, etc.)
// ============================================================================

// ---------------------------
// Hash
// ---------------------------

// AArch64: FNV-1a 64-bit
uint64_t vitte_fnv1a64(const uint8_t* data, size_t len);

// ---------------------------
// Memory
// ---------------------------

// Baseline memcpy (AArch64)
void* vitte_memcpy_baseline(void* dst, const void* src, size_t n);

// NEON memcpy (AArch64)
void* vitte_memcpy_neon(void* dst, const void* src, size_t n);

// Baseline memset (AArch64)
void* vitte_memset_baseline(void* dst, int c, size_t n);

// NEON memset (AArch64)
void* vitte_memset_neon(void* dst, int c, size_t n);

// ---------------------------
// UTF-8
// ---------------------------

// UTF-8 validator stub (AArch64), returns 1 if valid else 0.
int vitte_utf8_validate_stub(const uint8_t* data, size_t len);

#ifdef __cplusplus
} // extern "C"
#endif
