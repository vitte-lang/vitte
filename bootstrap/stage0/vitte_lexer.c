#include "vitte_lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

lexer_t* lexer_create(const char *source) {
    lexer_t *lexer = (lexer_t *)malloc(sizeof(lexer_t));
    if (!lexer) return NULL;
    
    lexer->source = source;
    lexer->length = strlen(source);
    lexer->current = 0;
    lexer->start = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->error_buffer[0] = '\0';
    
    return lexer;
}

void lexer_free(lexer_t *lexer) {
    if (lexer) free(lexer);
}

static int32_t peek(lexer_t *lexer) {
    if (lexer->current >= lexer->length) return EOF;
    return lexer->source[lexer->current];
}

static int32_t peek_next(lexer_t *lexer) {
    if (lexer->current + 1 >= lexer->length) return EOF;
    return lexer->source[lexer->current + 1];
}

static int32_t advance(lexer_t *lexer) {
    if (lexer->current >= lexer->length) return EOF;
    int32_t c = lexer->source[lexer->current++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static void skip_whitespace(lexer_t *lexer) {
    while (isspace(peek(lexer))) {
        advance(lexer);
    }
}

static void skip_line_comment(lexer_t *lexer) {
    while (peek(lexer) != '\n' && peek(lexer) != EOF) {
        advance(lexer);
    }
}

static void skip_block_comment(lexer_t *lexer) {
    while (peek(lexer) != EOF) {
        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
            advance(lexer);
            advance(lexer);
            break;
        }
        advance(lexer);
    }
}

static token_type_t check_keyword(const char *text, size_t length) {
    if (length == 2 && strncmp(text, "fn", 2) == 0) return TOKEN_KW_FN;
    if (length == 3 && strncmp(text, "let", 3) == 0) return TOKEN_KW_LET;
    if (length == 5 && strncmp(text, "const", 5) == 0) return TOKEN_KW_CONST;
    if (length == 2 && strncmp(text, "if", 2) == 0) return TOKEN_KW_IF;
    if (length == 4 && strncmp(text, "else", 4) == 0) return TOKEN_KW_ELSE;
    if (length == 5 && strncmp(text, "while", 5) == 0) return TOKEN_KW_WHILE;
    if (length == 3 && strncmp(text, "for", 3) == 0) return TOKEN_KW_FOR;
    if (length == 6 && strncmp(text, "return", 6) == 0) return TOKEN_KW_RETURN;
    if (length == 5 && strncmp(text, "break", 5) == 0) return TOKEN_KW_BREAK;
    if (length == 8 && strncmp(text, "continue", 8) == 0) return TOKEN_KW_CONTINUE;
    if (length == 6 && strncmp(text, "struct", 6) == 0) return TOKEN_KW_STRUCT;
    if (length == 4 && strncmp(text, "enum", 4) == 0) return TOKEN_KW_ENUM;
    if (length == 4 && strncmp(text, "impl", 4) == 0) return TOKEN_KW_IMPL;
    if (length == 5 && strncmp(text, "match", 5) == 0) return TOKEN_KW_MATCH;
    if (length == 3 && strncmp(text, "mut", 3) == 0) return TOKEN_KW_MUT;
    if (length == 6 && strncmp(text, "unsafe", 6) == 0) return TOKEN_KW_UNSAFE;
    if (length == 3 && strncmp(text, "pub", 3) == 0) return TOKEN_KW_PUB;
    if (length == 4 && strncmp(text, "priv", 4) == 0) return TOKEN_KW_PRIV;
    if (length == 3 && strncmp(text, "use", 3) == 0) return TOKEN_KW_USE;
    if (length == 3 && strncmp(text, "mod", 3) == 0) return TOKEN_KW_MOD;
    return TOKEN_IDENTIFIER;
}

static token_t read_identifier(lexer_t *lexer) {
    token_t token;
    token.line = lexer->line;
    token.column = lexer->column;
    
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }
    
    size_t length = lexer->current - lexer->start;
    token.lexeme = lexer->source + lexer->start;
    token.type = check_keyword(token.lexeme, length);
    
    return token;
}

static token_t read_number(lexer_t *lexer) {
    token_t token;
    token.line = lexer->line;
    token.column = lexer->column;
    token.type = TOKEN_NUMBER;
    
    while (isdigit(peek(lexer))) {
        advance(lexer);
    }
    
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        advance(lexer);
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }
    
    size_t length = lexer->current - lexer->start;
    token.lexeme = lexer->source + lexer->start;
    token.value.int_value = strtoll(token.lexeme, NULL, 10);
    
    return token;
}

static token_t read_string(lexer_t *lexer) {
    token_t token;
    token.line = lexer->line;
    token.column = lexer->column;
    token.type = TOKEN_STRING;
    
    char quote = advance(lexer);
    
    while (peek(lexer) != quote && peek(lexer) != EOF) {
        if (peek(lexer) == '\\') {
            advance(lexer);
        }
        advance(lexer);
    }
    
    if (peek(lexer) == quote) {
        advance(lexer);
    }
    
    token.lexeme = lexer->source + lexer->start;
    return token;
}

token_t lexer_next_token(lexer_t *lexer) {
    skip_whitespace(lexer);
    
    lexer->start = lexer->current;
    
    token_t token;
    token.line = lexer->line;
    token.column = lexer->column;
    token.lexeme = NULL;
    
    int32_t c = peek(lexer);
    
    if (c == EOF) {
        token.type = TOKEN_EOF;
        return token;
    }
    
    if (c == '/' && peek_next(lexer) == '/') {
        skip_line_comment(lexer);
        return lexer_next_token(lexer);
    }
    
    if (c == '/' && peek_next(lexer) == '*') {
        advance(lexer);
        advance(lexer);
        skip_block_comment(lexer);
        return lexer_next_token(lexer);
    }
    
    if (isalpha(c) || c == '_') {
        return read_identifier(lexer);
    }
    
    if (isdigit(c)) {
        return read_number(lexer);
    }
    
    if (c == '"' || c == '\'') {
        return read_string(lexer);
    }
    
    advance(lexer);
    
    switch (c) {
        case '(': token.type = TOKEN_LPAREN; break;
        case ')': token.type = TOKEN_RPAREN; break;
        case '{': token.type = TOKEN_LBRACE; break;
        case '}': token.type = TOKEN_RBRACE; break;
        case '[': token.type = TOKEN_LBRACKET; break;
        case ']': token.type = TOKEN_RBRACKET; break;
        case ';': token.type = TOKEN_SEMICOLON; break;
        case ',': token.type = TOKEN_COMMA; break;
        case '.': token.type = TOKEN_DOT; break;
        case ':':
            if (peek(lexer) == ':') {
                advance(lexer);
                token.type = TOKEN_DOUBLE_COLON;
            } else {
                token.type = TOKEN_COLON;
            }
            break;
        case '=':
            if (peek(lexer) == '=') {
                advance(lexer);
                token.type = TOKEN_EQ;
            } else if (peek(lexer) == '>') {
                advance(lexer);
                token.type = TOKEN_ARROW;
            } else {
                token.type = TOKEN_ASSIGN;
            }
            break;
        case '+':
            if (peek(lexer) == '=') {
                advance(lexer);
                token.type = TOKEN_PLUS_ASSIGN;
            } else {
                token.type = TOKEN_PLUS;
            }
            break;
        case '-': token.type = TOKEN_MINUS; break;
        case '*':
            if (peek(lexer) == '=') {
                advance(lexer);
                token.type = TOKEN_STAR_ASSIGN;
            } else {
                token.type = TOKEN_STAR;
            }
            break;
        case '/':
            if (peek(lexer) == '=') {
                advance(lexer);
                token.type = TOKEN_SLASH_ASSIGN;
            } else {
                token.type = TOKEN_SLASH;
            }
            break;
        case '%': token.type = TOKEN_PERCENT; break;
        case '&':
            if (peek(lexer) == '&') {
                advance(lexer);
                token.type = TOKEN_LOGICAL_AND;
            } else {
                token.type = TOKEN_AMPERSAND;
            }
            break;
        case '|':
            if (peek(lexer) == '|') {
                advance(lexer);
                token.type = TOKEN_LOGICAL_OR;
            } else {
                token.type = TOKEN_PIPE;
            }
            break;
        case '^': token.type = TOKEN_CARET; break;
        case '!':
            if (peek(lexer) == '=') {
                advance(lexer);
                token.type = TOKEN_NE;
            } else {
                token.type = TOKEN_NOT;
            }
            break;
        case '<':
            if (peek(lexer) == '=') {
                advance(lexer);
                token.type = TOKEN_LE;
            } else {
                token.type = TOKEN_LT;
            }
            break;
        case '>':
            if (peek(lexer) == '=') {
                advance(lexer);
                token.type = TOKEN_GE;
            } else {
                token.type = TOKEN_GT;
            }
            break;
        default:
            token.type = TOKEN_ERROR;
            lexer_set_error(lexer, "Unexpected character");
    }
    
    token.lexeme = lexer->source + lexer->start;
    return token;
}

void lexer_set_error(lexer_t *lexer, const char *error) {
    snprintf(lexer->error_buffer, sizeof(lexer->error_buffer), 
             "[Lexer] Line %d, Col %d: %s", lexer->line, lexer->column, error);
}

const char* lexer_get_error(lexer_t *lexer) {
    return lexer->error_buffer[0] != '\0' ? lexer->error_buffer : NULL;
}

const char* token_type_name(token_type_t type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_KW_FN: return "KW_FN";
        case TOKEN_KW_LET: return "KW_LET";
        case TOKEN_KW_IF: return "KW_IF";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        default: return "UNKNOWN";
    }
}
