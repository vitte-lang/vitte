#pragma once
#include "steel/vitte/lexer.h"
#include "steel/vitte/ast.h"

typedef struct {
  vitte_lexer lx;
  vitte_token cur;
  const char* src;
  uint32_t len;
  uint32_t file_id;
  int had_error;
} vitte_parser;

void vitte_parser_init(vitte_parser* p, const char* src, uint32_t len, uint32_t file_id);
int vitte_parse_program(vitte_parser* p, ast_program* out);
