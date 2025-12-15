#include "../../common/asm_macros.S"
/* Placeholder: route to baseline (replace with AVX2 implementation). */
FUNC_BEGIN(vitte_memcpy_avx2)
  jmp SYM(vitte_memcpy_baseline)
FUNC_END(vitte_memcpy_avx2)
