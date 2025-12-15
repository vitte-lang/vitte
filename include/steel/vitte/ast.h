#pragma once
#include <stdint.h>
#include "steel/diag/span.h"

typedef struct { const char* name_ptr; uint32_t name_len; steel_span name_span; } ast_ident;
typedef struct { int64_t value; steel_span span; } ast_int_lit;
typedef struct { ast_int_lit ret_value; steel_span span; } ast_return_stmt;

typedef struct { ast_ident name; ast_return_stmt ret; steel_span span; } ast_fn_main;

typedef struct {
  ast_ident module_path;
  int has_module;
  ast_fn_main main_fn;
} ast_program;
