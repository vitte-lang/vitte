#ifndef VITTEC_INCLUDE_VITTEC_FRONT_TOKEN_H
    #define VITTEC_INCLUDE_VITTEC_FRONT_TOKEN_H

    #include <stdint.h>

    #include "vittec/diag/span.h"
#include "vittec/support/str.h"

typedef enum vittec_token_kind {
  TK_EOF = 0,

  TK_IDENT,
  TK_INT,
  TK_FLOAT,
  TK_STRING,

  TK_DOT,
  TK_COMMA,
  TK_COLON,
  TK_SEMI,
  TK_LPAREN,
  TK_RPAREN,
  TK_LBRACK,
  TK_RBRACK,
  TK_ARROW,
  TK_EQ,

  TK_KW_MODULE,
  TK_KW_IMPORT,
  TK_KW_EXPORT,
  TK_KW_STRUCT,
  TK_KW_ENUM,
  TK_KW_TYPE,
  TK_KW_FN,
  TK_KW_LET,
  TK_KW_CONST,
  TK_KW_IF,
  TK_KW_ELIF,
  TK_KW_ELSE,
  TK_KW_WHILE,
  TK_KW_FOR,
  TK_KW_MATCH,
  TK_KW_BREAK,
  TK_KW_CONTINUE,
  TK_KW_RET,
  TK_KW_SAY,
  TK_KW_DO,
  TK_KW_SET,
  TK_KW_LOOP,
  TK_KW_WHEN,
  TK_KW_END  /* .end keyword tokenised as END */
} vittec_token_kind_t;

typedef struct vittec_token {
  vittec_token_kind_t kind;
  vittec_span_t span;
  vittec_sv_t text; /* view into source */
} vittec_token_t;

    #endif /* VITTEC_INCLUDE_VITTEC_FRONT_TOKEN_H */
