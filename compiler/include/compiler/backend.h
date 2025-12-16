#ifndef COMPILER_BACKEND_H
#define COMPILER_BACKEND_H

#include "backend_c.h"
#include "codegen.h"

typedef struct backend {
    backend_c_t *c_emitter;
    codegen_t *codegen;
    void *target_info;
} backend_t;

backend_t* backend_create(void *target_info);
void backend_destroy(backend_t *be);

int backend_emit(backend_t *be, void *ir_module, const char *output_file);

#endif /* COMPILER_BACKEND_H */
