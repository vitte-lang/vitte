#pragma once
#include <stdint.h>
#include "steel/diag/span.h"

typedef enum {
  TK_EOF = 0,
  TK_IDENT,
  TK_INT,
  TK_KW_MODULE,
  TK_KW_FN,
  TK_KW_RETURN,
  TK_COLON,
  TK_ARROW,
  TK_DOT,
  TK_LPAREN,
  TK_RPAREN,
  TK_SEMI,
} vitte_tok_kind;

typedef struct {
  vitte_tok_kind kind;
  steel_span span;
  const char* lexeme;
  uint32_t len;
  int64_t int_val;
} vitte_token;
