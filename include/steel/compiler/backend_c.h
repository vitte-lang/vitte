#pragma once
#include "steel/vitte/ast.h"

/* Emit a single C translation unit for the given AST program. */
int backend_c_emit_program(const ast_program* prog, const char* out_c_path);
