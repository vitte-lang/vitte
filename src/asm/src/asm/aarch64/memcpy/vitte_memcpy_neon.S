#include "../../common/asm_macros.S"
/* Placeholder: route to baseline (replace with NEON implementation). */
FUNC_BEGIN(vitte_memcpy_neon)
  b SYM(vitte_memcpy_baseline)
FUNC_END(vitte_memcpy_neon)
