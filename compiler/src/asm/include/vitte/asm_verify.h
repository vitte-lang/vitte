// vitte/src/asm/include/vitte/asm_verify.h
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ASM fastpath verification helpers
//
// These helpers are optional and can be wired into unit tests / CI to compare
// assembly implementations against the scalar baselines.
// ============================================================================

typedef enum vitte_asm_verify_status_t {
  VITTE_ASM_VERIFY_OK = 0,
  VITTE_ASM_VERIFY_E_MEMCPY_MISMATCH = 1,
  VITTE_ASM_VERIFY_E_MEMSET_MISMATCH = 2,
  VITTE_ASM_VERIFY_E_FNV_MISMATCH = 3,
  VITTE_ASM_VERIFY_E_UTF8_MISMATCH = 4,
  VITTE_ASM_VERIFY_E_INVAL = 10
} vitte_asm_verify_status_t;

// Runs all fastpath comparisons (memcpy/memset/hash/utf8). Higher iterations
// increase coverage; the routine is deterministic.
vitte_asm_verify_status_t vitte_asm_verify_all(size_t iterations);

#ifdef __cplusplus
} // extern "C"
#endif
