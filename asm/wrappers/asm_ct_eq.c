/*
  C wrapper / fallback for constant-time equality

  API:
    int steel_ct_eq(const void* a, const void* b, size_t n);

  Build model:
    - If an asm implementation provides `steel_ct_eq` (e.g. asm/aarch64/ct_eq.S),
      compile this file with -DSTEEL_HAVE_ASM_CT_EQ=1 to avoid duplicate symbols.
    - Otherwise, this file provides a portable constant-time implementation.

  Notes:
    - Constant-time w.r.t. buffer contents (no data-dependent branches).
    - Runtime depends on n (public).
*/

#include <stddef.h>
#include <stdint.h>

static int steel_ct_eq_portable_impl(const void* a_, const void* b_, size_t n) {
  const uint8_t* a = (const uint8_t*)a_;
  const uint8_t* b = (const uint8_t*)b_;

  /* XOR-accumulate all differences; do not early-exit. */
  uint32_t acc = 0;
  for (size_t i = 0; i < n; i++) {
    acc |= (uint32_t)(a[i] ^ b[i]);
  }

  /* Convert acc==0 to 1 else 0 without branches depending on contents. */
  /* acc | -acc has top bit set iff acc != 0. */
  uint32_t nz = (acc | (uint32_t)(0u - acc)) >> 31;
  return (int)(1u ^ nz);
}

/* Always available for internal use (tests / fallback). */
int steel_ct_eq_portable(const void* a, const void* b, size_t n) {
  return steel_ct_eq_portable_impl(a, b, n);
}

#if !defined(STEEL_HAVE_ASM_CT_EQ) || (STEEL_HAVE_ASM_CT_EQ == 0)
int steel_ct_eq(const void* a, const void* b, size_t n) {
  return steel_ct_eq_portable_impl(a, b, n);
}
#endif
