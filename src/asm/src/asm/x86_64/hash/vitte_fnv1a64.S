#include "../../common/asm_macros.S"
/* SysV: rdi=data, rsi=len ; ret rax */
FUNC_BEGIN(vitte_fnv1a64_asm)
  movabs $1469598103934665603, %rax
  test %rsi, %rsi
  je 2f
1:
  movzbq (%rdi), %rcx
  xor %rcx, %rax
  movabs $1099511628211, %rcx
  imul %rcx, %rax
  inc %rdi
  dec %rsi
  jne 1b
2:
  ret
FUNC_END(vitte_fnv1a64_asm)
