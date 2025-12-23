// vitte/include/vitte/asm_verify.h
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// ASM fastpath verification helpers
// ============================================================================

typedef enum vitte_asm_verify_status_t {
  VITTE_ASM_VERIFY_OK = 0,
  VITTE_ASM_VERIFY_E_MEMCPY_MISMATCH = 1,
  VITTE_ASM_VERIFY_E_MEMSET_MISMATCH = 2,
  VITTE_ASM_VERIFY_E_FNV_MISMATCH = 3,
  VITTE_ASM_VERIFY_E_UTF8_MISMATCH = 4,
  VITTE_ASM_VERIFY_E_INVAL = 10
} vitte_asm_verify_status_t;

vitte_asm_verify_status_t vitte_asm_verify_all(size_t iterations);

#ifdef __cplusplus
} // extern "C"
#endif

