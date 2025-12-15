#ifndef VITTEC_INCLUDE_VITTEC_FRONT_LEXER_H
    #define VITTEC_INCLUDE_VITTEC_FRONT_LEXER_H

    #include <stdint.h>

    #include "vittec/front/token.h"
#include "vittec/diag/diagnostic.h"

typedef struct vittec_lexer {
  const char* src;
  uint32_t len;
  uint32_t i;
  uint32_t file_id;
  vittec_diag_sink_t* diags;
} vittec_lexer_t;

void vittec_lexer_init(vittec_lexer_t* lx, const char* src, uint32_t len, uint32_t file_id, vittec_diag_sink_t* diags);
vittec_token_t vittec_lex_next(vittec_lexer_t* lx);

    #endif /* VITTEC_INCLUDE_VITTEC_FRONT_LEXER_H */
