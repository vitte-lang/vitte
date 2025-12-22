// vitte/include/vitte/runtime.h (snprintf-like to_string; define VITTE_RUNTIME_ENABLE_STDIO for FILE*)
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "vitte/cpu.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Vitte runtime - stable facade for fastpaths (C API)
// ============================================================================

void* vitte_memcpy(void* dst, const void* src, size_t n);
void* vitte_memset(void* dst, int c, size_t n);
int vitte_utf8_validate(const uint8_t* data, size_t len);
uint64_t vitte_hash_fnv1a64(const uint8_t* data, size_t len);

const void* vitte_asm_selected_memcpy(void);
const void* vitte_asm_selected_memset(void);
const void* vitte_asm_selected_utf8_validate(void);

typedef struct vitte_asm_selected_caps_t {
  uint32_t abi;          // set to 1
  vitte_cpu_arch_t arch; // VITTE_CPU_ARCH_*
  uint32_t features_lo;  // arch-specific feature bits (see vitte/cpu.h)
  uint32_t features_hi;
} vitte_asm_selected_caps_t;

vitte_asm_selected_caps_t vitte_asm_selected_caps(void);

typedef enum vitte_asm_impl_id_t {
  VITTE_ASM_IMPL_UNKNOWN = 0,
  VITTE_ASM_IMPL_BASELINE = 1,
  VITTE_ASM_IMPL_SSE2 = 2,
  VITTE_ASM_IMPL_AVX2 = 3,
  VITTE_ASM_IMPL_NEON = 4
} vitte_asm_impl_id_t;

enum {
  VITTE_ASM_SELECTED_F_MEMCPY_ASM = 1u << 0,
  VITTE_ASM_SELECTED_F_MEMSET_ASM = 1u << 1,
  VITTE_ASM_SELECTED_F_UTF8_ASM = 1u << 2
};

typedef struct vitte_asm_selected_info_t {
  uint32_t abi;          // set to 1
  vitte_cpu_arch_t arch; // VITTE_CPU_ARCH_*
  uint32_t features_lo;
  uint32_t features_hi;
  uint32_t flags; // VITTE_ASM_SELECTED_F_*
  vitte_asm_impl_id_t memcpy_impl;
  vitte_asm_impl_id_t memset_impl;
  vitte_asm_impl_id_t utf8_impl;
} vitte_asm_selected_info_t;

vitte_asm_selected_info_t vitte_asm_selected_info(void);

// Debug/telemetry helper (stdio-free).
// Writes a short human-readable report into `dst` (NUL-terminated if cap>0).
// Returns the number of chars that would have been written (excluding NUL),
// like snprintf.
size_t vitte_asm_dump_selected_to_string(char* dst, size_t cap);

// Optional stdio helper (kept behind a build flag so runtime.h stays light).
#if defined(VITTE_RUNTIME_ENABLE_STDIO)
#  include <stdio.h>
// Returns 0 if `file` is NULL.
int vitte_asm_dump_selected(FILE* file);
#endif

#ifdef __cplusplus
} // extern "C"
#endif
