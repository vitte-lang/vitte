#ifndef VITTE_LEXER_H
#define VITTE_LEXER_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_KW_FN,
    TOKEN_KW_LET,
    TOKEN_KW_CONST,
    TOKEN_KW_IF,
    TOKEN_KW_ELSE,
    TOKEN_KW_WHILE,
    TOKEN_KW_FOR,
    TOKEN_KW_RETURN,
    TOKEN_KW_BREAK,
    TOKEN_KW_CONTINUE,
    TOKEN_KW_STRUCT,
    TOKEN_KW_ENUM,
    TOKEN_KW_IMPL,
    TOKEN_KW_MATCH,
    TOKEN_KW_MUT,
    TOKEN_KW_UNSAFE,
    TOKEN_KW_PUB,
    TOKEN_KW_PRIV,
    TOKEN_KW_USE,
    TOKEN_KW_MOD,
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
    TOKEN_DOUBLE_COLON,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_AMPERSAND,
    TOKEN_PIPE,
    TOKEN_CARET,
    TOKEN_NOT,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LE,
    TOKEN_GE,
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_LOGICAL_AND,
    TOKEN_LOGICAL_OR,
    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_STAR_ASSIGN,
    TOKEN_SLASH_ASSIGN,
    TOKEN_COMMENT,
    TOKEN_ERROR
} token_type_t;

typedef struct {
    token_type_t type;
    const char *lexeme;
    int32_t line;
    int32_t column;
    union {
        int64_t int_value;
        double float_value;
        const char *string_value;
    } value;
} token_t;

typedef struct {
    const char *source;
    size_t length;
    size_t current;
    size_t start;
    int32_t line;
    int32_t column;
    char error_buffer[256];
} lexer_t;

lexer_t* lexer_create(const char *source);
void lexer_free(lexer_t *lexer);
token_t lexer_next_token(lexer_t *lexer);
void lexer_set_error(lexer_t *lexer, const char *error);
const char* lexer_get_error(lexer_t *lexer);
const char* token_type_name(token_type_t type);

#endif
