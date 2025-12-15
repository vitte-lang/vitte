#ifndef STEEL_LEXER_H
#define STEEL_LEXER_H

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
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
} TokenType;

typedef struct {
    TokenType type;
    const char *value;
    int line;
    int column;
} Token;

Token *lexer_tokenize(const char *source);
void lexer_free(Token *tokens);

#endif
