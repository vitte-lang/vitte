#include "../../common/asm_macros.S"
/* AAPCS64: x0=dst, x1=byte, x2=n ; ret x0 */
FUNC_BEGIN(vitte_memset_baseline)
  mov x3, x0
  cbz x2, 2f
  and w1, w1, #0xFF
1:
  strb w1, [x0], #1
  subs x2, x2, #1
  b.ne 1b
2:
  mov x0, x3
  ret
FUNC_END(vitte_memset_baseline)
