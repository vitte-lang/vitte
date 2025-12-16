#ifndef COMPILER_BACKEND_C_H
#define COMPILER_BACKEND_C_H

#include <stdio.h>

typedef struct backend_c {
    FILE *out;
    int indent_level;
} backend_c_t;

backend_c_t* backend_c_create(FILE *out);
void backend_c_destroy(backend_c_t *be);

int backend_c_emit_ir(backend_c_t *be, void *ir_module);

#endif /* COMPILER_BACKEND_C_H */
