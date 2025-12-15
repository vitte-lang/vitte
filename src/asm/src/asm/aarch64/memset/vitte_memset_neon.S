#include "../../common/asm_macros.S"
/* Placeholder: route to baseline (replace with NEON implementation). */
FUNC_BEGIN(vitte_memset_neon)
  b SYM(vitte_memset_baseline)
FUNC_END(vitte_memset_neon)
