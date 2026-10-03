#include "tree_sitter/parser.h"

enum TokenType {
  TRAILING_DECIMAL_FLOAT,
};

static bool advance_decimal_digit_sequence(TSLexer *lexer) {
  if (lexer->lookahead < '0' || lexer->lookahead > '9') {
    return false;
  }

  lexer->advance(lexer, false);
  while (lexer->lookahead >= '0' && lexer->lookahead <= '9') {
    lexer->advance(lexer, false);
  }
  while (lexer->lookahead == '_') {
    lexer->advance(lexer, false);
    if (lexer->lookahead < '0' || lexer->lookahead > '9') {
      return false;
    }
    lexer->advance(lexer, false);
    while (lexer->lookahead >= '0' && lexer->lookahead <= '9') {
      lexer->advance(lexer, false);
    }
  }

  return true;
}

void *tree_sitter_vitte_external_scanner_create(void) {
  return NULL;
}

void tree_sitter_vitte_external_scanner_destroy(void *payload) {
  (void)payload;
}

unsigned tree_sitter_vitte_external_scanner_serialize(void *payload, char *buffer) {
  (void)payload;
  (void)buffer;
  return 0;
}

void tree_sitter_vitte_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  (void)payload;
  (void)buffer;
  (void)length;
}

bool tree_sitter_vitte_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  (void)payload;

  if (!valid_symbols[TRAILING_DECIMAL_FLOAT]) {
    return false;
  }

  while (lexer->lookahead == ' ' || lexer->lookahead == '\t' ||
         lexer->lookahead == '\n' || lexer->lookahead == '\r') {
    lexer->advance(lexer, true);
  }
  lexer->mark_end(lexer);

  if (!advance_decimal_digit_sequence(lexer) || lexer->lookahead != '.') {
    return false;
  }

  lexer->advance(lexer, false);
  if (lexer->lookahead == '.') {
    return false;
  }

  if (lexer->lookahead >= '0' && lexer->lookahead <= '9') {
    return false;
  }

  if (lexer->lookahead == 'e' || lexer->lookahead == 'E') {
    lexer->advance(lexer, false);
    if (lexer->lookahead == '+' || lexer->lookahead == '-') {
      lexer->advance(lexer, false);
    }
    if (!advance_decimal_digit_sequence(lexer)) {
      return false;
    }
  }

  lexer->mark_end(lexer);
  lexer->result_symbol = TRAILING_DECIMAL_FLOAT;
  return true;
}
