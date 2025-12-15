#include "../../include/compiler/parser.h"
#include <stdlib.h>
#include <string.h>

static Token *toks;
static size_t idx;

static Token *peek(void) { return &toks[idx]; }
static Token *advance(void) { return &toks[idx++]; }

ASTNode *parse_primary(void) {
    Token *t = peek();
    if (t->type == TOKEN_NUMBER) {
        long v = atol(t->value);
        advance();
        return ast_new_number(v);
    }
    if (t->type == TOKEN_IDENTIFIER) {
        char *s = strdup(t->value);
        advance();
        ASTNode *n = ast_new_ident(s);
        free(s);
        return n;
    }
    if (t->type == TOKEN_LPAREN) {
        advance();
        ASTNode *e = NULL;
        // simple expression parse: primary (op primary)?
        Token *p = peek();
        e = parse_primary();
        if (peek()->type == TOKEN_OPERATOR) {
            char op = peek()->value ? peek()->value[0] : '\0';
            advance();
            ASTNode *r = parse_primary();
            ASTNode *bin = ast_new_binary(op, e, r);
            if (peek()->type == TOKEN_RPAREN) advance();
            return bin;
        }
        if (peek()->type == TOKEN_RPAREN) advance();
        return e;
    }
    return NULL;
}

ASTNode *parser_parse(Token *tokens) {
    if (!tokens) return NULL;
    toks = tokens; idx = 0;
    return parse_primary();
}

void parser_free_ast(ASTNode *ast) { ast_free(ast); }
