#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include "ast.h"

typedef struct parser {
    void *tokens;
    uint32_t token_count;
    uint32_t pos;
    void *diag;
} parser_t;

parser_t* parser_create(void *tokens, uint32_t count, void *diag);
void parser_destroy(parser_t *p);

ast_module_t* parser_parse(parser_t *p);

#endif /* COMPILER_PARSER_H */
