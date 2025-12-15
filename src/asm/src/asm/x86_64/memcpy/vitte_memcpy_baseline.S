#include "../../common/asm_macros.S"
/* SysV: rdi=dst, rsi=src, rdx=n ; ret rax=dst */
FUNC_BEGIN(vitte_memcpy_baseline)
  mov %rdi, %rax
  test %rdx, %rdx
  je 2f
  mov %rdx, %rcx
  rep movsb
2:
  ret
FUNC_END(vitte_memcpy_baseline)
