/*
  C wrapper / fallback for memwipe

  API:
    void steel_memwipe(void* p, size_t n);

  Build model:
    - If an asm implementation provides `steel_memwipe` (e.g. asm/aarch64/memwipe.S),
      compile this file with -DSTEEL_HAVE_ASM_MEMWIPE=1 to avoid duplicate symbols.
    - Otherwise, this file provides a portable best-effort implementation.

  Notes:
    - The goal is to reduce the chance the compiler optimizes the wipe away.
    - There is no perfect portable solution across all compilers/ABIs;
      prefer the assembly implementation when available.
*/

#include <stddef.h>
#include <stdint.h>

/* Compiler barrier helper. */
static inline void steel_compiler_barrier(void) {
#if defined(__GNUC__) || defined(__clang__)
  __asm__ __volatile__("" ::: "memory");
#else
  /* Best-effort: no-op. */
#endif
}

static void steel_memwipe_portable_impl(void* p_, size_t n) {
  volatile uint8_t* p = (volatile uint8_t*)p_;

  /* Write zeros using volatile to discourage dead-store elimination. */
  for (size_t i = 0; i < n; i++) {
    p[i] = 0;
  }

  /* Prevent reordering across the wipe in aggressive optimizers. */
  steel_compiler_barrier();
}

/* Always available for internal use (tests / fallback). */
void steel_memwipe_portable(void* p, size_t n) {
  steel_memwipe_portable_impl(p, n);
}

#if !defined(STEEL_HAVE_ASM_MEMWIPE) || (STEEL_HAVE_ASM_MEMWIPE == 0)
void steel_memwipe(void* p, size_t n) {
  steel_memwipe_portable_impl(p, n);
}
#endif
