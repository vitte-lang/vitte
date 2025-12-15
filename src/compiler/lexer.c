/* Simple lexer implementation for Vitte (minimal, suitable for tests)
 * Produces a NULL-terminated array of Token where the last token has type TOKEN_EOF.
 */
#include "../../include/compiler/lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *strdup_safe(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (!p) return NULL;
    memcpy(p, s, n);
    return p;
}

Token *lexer_tokenize(const char *source) {
    if (!source) return NULL;
    size_t cap = 64;
    Token *tokens = malloc(sizeof(Token) * cap);
    size_t count = 0;

    const char *p = source;
    int line = 1, column = 1;

    while (*p) {
        if (isspace((unsigned char)*p)) {
            if (*p == '\n') { line++; column = 1; } else column++;
            p++;
            continue;
        }

        if (isalpha((unsigned char)*p) || *p == '_') {
            const char *start = p;
            while (isalnum((unsigned char)*p) || *p == '_') p++;
            size_t len = p - start;
            char *val = malloc(len + 1);
            memcpy(val, start, len);
            val[len] = '\0';
            if (count + 1 >= cap) { cap *= 2; tokens = realloc(tokens, sizeof(Token) * cap); }
            tokens[count++] = (Token){ .type = TOKEN_IDENTIFIER, .value = val, .line = line, .column = column };
            column += (int)len;
            continue;
        }

        if (isdigit((unsigned char)*p)) {
            const char *start = p;
            while (isdigit((unsigned char)*p)) p++;
            size_t len = p - start;
            char *val = malloc(len + 1);
            memcpy(val, start, len);
            val[len] = '\0';
            if (count + 1 >= cap) { cap *= 2; tokens = realloc(tokens, sizeof(Token) * cap); }
            tokens[count++] = (Token){ .type = TOKEN_NUMBER, .value = val, .line = line, .column = column };
            column += (int)len;
            continue;
        }

        if (*p == '"') {
            p++; const char *start = p;
            while (*p && *p != '"') p++;
            size_t len = p - start;
            char *val = malloc(len + 1);
            memcpy(val, start, len);
            val[len] = '\0';
            if (*p == '"') p++;
            if (count + 1 >= cap) { cap *= 2; tokens = realloc(tokens, sizeof(Token) * cap); }
            tokens[count++] = (Token){ .type = TOKEN_STRING, .value = val, .line = line, .column = column };
            column += (int)(len + 2);
            continue;
        }

        /* Single-character tokens */
        TokenType t = TOKEN_OPERATOR;
        switch (*p) {
            case '(' : t = TOKEN_LPAREN; break;
            case ')' : t = TOKEN_RPAREN; break;
            case '{' : t = TOKEN_LBRACE; break;
            case '}' : t = TOKEN_RBRACE; break;
            case ';' : t = TOKEN_SEMICOLON; break;
            case ',' : t = TOKEN_COMMA; break;
            default: t = TOKEN_OPERATOR; break;
        }
        char buf[2] = { *p, '\0' };
        if (count + 1 >= cap) { cap *= 2; tokens = realloc(tokens, sizeof(Token) * cap); }
        tokens[count++] = (Token){ .type = t, .value = strdup_safe(buf), .line = line, .column = column };
        p++; column++;
    }

    /* EOF token */
    if (count + 1 >= cap) { cap += 1; tokens = realloc(tokens, sizeof(Token) * cap); }
    tokens[count++] = (Token){ .type = TOKEN_EOF, .value = NULL, .line = line, .column = column };

    /* shrink to fit */
    tokens = realloc(tokens, sizeof(Token) * count);
    return tokens;
}

void lexer_free(Token *tokens) {
    if (!tokens) return;
    for (size_t i = 0; ; i++) {
        Token t = tokens[i];
        if (t.value) free((void*)t.value);
        if (t.type == TOKEN_EOF) break;
    }
    free(tokens);
}
