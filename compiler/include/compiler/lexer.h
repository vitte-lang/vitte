#ifndef COMPILER_LEXER_H
#define COMPILER_LEXER_H

#include <stdint.h>

typedef enum {
    TOK_EOF,
    TOK_IDENT,
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
    TOK_CHAR,
    TOK_KEYWORD,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_PERCENT,
    TOK_EQ,
    TOK_NE,
    TOK_LT,
    TOK_LE,
    TOK_GT,
    TOK_GE,
    TOK_ASSIGN,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LBRACK,
    TOK_RBRACK,
    TOK_SEMICOLON,
    TOK_COMMA,
    TOK_DOT,
    TOK_COLON,
    TOK_ARROW,
} token_kind_t;

typedef struct token {
    token_kind_t kind;
    const char *lexeme;
    uint32_t len;
    uint32_t line;
    uint32_t col;
} token_t;

typedef struct lexer {
    const char *input;
    uint32_t len;
    uint32_t pos;
    uint32_t line;
    uint32_t col;
} lexer_t;

lexer_t* lexer_create(const char *input, uint32_t len);
void lexer_destroy(lexer_t *lex);

token_t* lexer_tokenize(lexer_t *lex, uint32_t *out_count);

#endif /* COMPILER_LEXER_H */
