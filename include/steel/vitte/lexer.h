#pragma once
#include <stdint.h>
#include "steel/vitte/tokens.h"

typedef struct {
  const char* src;
  uint32_t len;
  uint32_t i;
  uint32_t file_id;
} vitte_lexer;

void vitte_lexer_init(vitte_lexer* lx, const char* src, uint32_t len, uint32_t file_id);
vitte_token vitte_lex_next(vitte_lexer* lx);
