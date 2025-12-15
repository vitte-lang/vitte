#include "../../common/asm_macros.S"
/* SysV: rdi=dst, rsi=byte, rdx=n ; ret rax=dst */
FUNC_BEGIN(vitte_memset_baseline)
  mov %rdi, %r8         /* save original dst */
  test %rdx, %rdx
  je 2f
  mov %rdx, %rcx
  mov %sil, %al
  rep stosb
2:
  mov %r8, %rax
  ret
FUNC_END(vitte_memset_baseline)
