#include "../../common/asm_macros.S"
.extern SYM(vitte_utf8_validate_ref)
FUNC_BEGIN(vitte_utf8_validate_asm)
  b SYM(vitte_utf8_validate_ref)
FUNC_END(vitte_utf8_validate_asm)
