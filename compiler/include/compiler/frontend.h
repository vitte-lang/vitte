#ifndef COMPILER_FRONTEND_H
#define COMPILER_FRONTEND_H

#include "lexer.h"
#include "parser.h"

typedef struct frontend {
    lexer_t *lexer;
    parser_t *parser;
    void *diag_ctx;
} frontend_t;

frontend_t* frontend_create(void *diag_ctx);
void frontend_destroy(frontend_t *fe);

void* frontend_compile(frontend_t *fe, const char *source);

#endif /* COMPILER_FRONTEND_H */
