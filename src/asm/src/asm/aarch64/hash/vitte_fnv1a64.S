#include "../../common/asm_macros.S"
/* AAPCS64: x0=data, x1=len ; ret x0 */
FUNC_BEGIN(vitte_fnv1a64_asm)
  ldr x2, =1469598103934665603
  cbz x1, 2f
1:
  ldrb w3, [x0], #1
  eor x2, x2, x3
  ldr x4, =1099511628211
  mul x2, x2, x4
  subs x1, x1, #1
  b.ne 1b
2:
  mov x0, x2
  ret
FUNC_END(vitte_fnv1a64_asm)
