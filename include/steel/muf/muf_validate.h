#pragma once
#include "steel/muf/muf_ast.h"
typedef struct { int ok; const char* error_msg; } muf_validate_result;
muf_validate_result muf_validate(const muf_program* p);
