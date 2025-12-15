#ifndef STEEL_TOKEN_H
#define STEEL_TOKEN_H

typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_KEYWORD,
    TOKEN_OPERATOR,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_COLON,
    TOKEN_ARROW,
} TokenType;

typedef struct {
    TokenType type;
    const char *value;
    int line;
    int column;
} Token;

const char *token_type_to_string(TokenType type);

#endif
