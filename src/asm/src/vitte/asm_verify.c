#include "vitte/asm.h"
#include <stdint.h>
#include <stdlib.h>

/* Call this from a debug test if you want a runtime equivalence check. */
int vitte_asm_selfcheck(void) {
  /* memcpy */
  for(size_t n=0;n<512;n++) {
    uint8_t a[512], b[512], c[512];
    for(size_t i=0;i<512;i++) a[i] = (uint8_t)(i * 13u + (uint8_t)n);

    vitte_memcpy_ref(b, a, n);
    vitte_memcpy_fast(c, a, n);
    for(size_t i=0;i<n;i++) if(b[i] != c[i]) return 0;
  }

  /* hash */
  for(size_t n=0;n<512;n++) {
    uint8_t a[512];
    for(size_t i=0;i<n;i++) a[i] = (uint8_t)(i * 7u + (uint8_t)n);
    if(vitte_fnv1a64_ref(a, n) != vitte_fnv1a64_fast(a, n)) return 0;
  }

  return 1;
}
