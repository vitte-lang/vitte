#pragma once
#include "steel/muf/muf_ast.h"

/* Parse MUF file at path into program. Returns 0 on success. */
int muf_parse_file(const char* path, muf_program* out);
