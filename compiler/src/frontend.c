#include "compiler/frontend.h"
#include <stdlib.h>

frontend_t* frontend_create(void *diag_ctx) {
    frontend_t *fe = (frontend_t *)malloc(sizeof(frontend_t));
    if (!fe) return NULL;
    
    fe->lexer = NULL;
    fe->parser = NULL;
    fe->diag_ctx = diag_ctx;
    
    return fe;
}

void frontend_destroy(frontend_t *fe) {
    if (!fe) return;
    if (fe->lexer) lexer_destroy(fe->lexer);
    if (fe->parser) parser_destroy(fe->parser);
    free(fe);
}

void* frontend_compile(frontend_t *fe, const char *source) {
    if (!fe || !source) return NULL;
    
    // TODO: Implement frontend compilation pipeline
    // 1. Tokenize with lexer
    // 2. Parse tokens with parser
    // 3. Return AST module
    
    return NULL;
}
