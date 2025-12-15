#include "../../common/asm_macros.S"
/* SysV: rdi=p, rsi=n ; ret eax */
.extern SYM(vitte_utf8_validate_ref)
FUNC_BEGIN(vitte_utf8_validate_asm)
  jmp SYM(vitte_utf8_validate_ref)
FUNC_END(vitte_utf8_validate_asm)
