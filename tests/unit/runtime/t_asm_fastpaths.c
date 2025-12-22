#include <assert.h>
#include <stdio.h>

#include "vitte/asm_verify.h"

void test_asm_fastpaths_verify(void) {
  // Keep it small for unit tests; the routine is deterministic.
  vitte_asm_verify_status_t st = vitte_asm_verify_all(256);
  assert(st == VITTE_ASM_VERIFY_OK);
  printf("test_asm_fastpaths_verify passed\n");
}

