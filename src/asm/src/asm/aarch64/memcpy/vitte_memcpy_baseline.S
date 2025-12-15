#include "../../common/asm_macros.S"
/* AAPCS64: x0=dst, x1=src, x2=n ; ret x0 */
FUNC_BEGIN(vitte_memcpy_baseline)
  mov x3, x0
  cbz x2, 2f
1:
  ldrb w4, [x1], #1
  strb w4, [x0], #1
  subs x2, x2, #1
  b.ne 1b
2:
  mov x0, x3
  ret
FUNC_END(vitte_memcpy_baseline)
