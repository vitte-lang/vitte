#include "../../common/asm_macros.S"
FUNC_BEGIN(vitte_memset_avx2)
  jmp SYM(vitte_memset_baseline)
FUNC_END(vitte_memset_avx2)
