#include "../../include/compiler/token.h"
#include <stdlib.h>
#include <string.h>

const char *token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_OPERATOR: return "OPERATOR";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_COMMA: return "COMMA";
        default: return "UNKNOWN";
    }
}
