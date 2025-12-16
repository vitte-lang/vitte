#ifndef COMPILER_CODEGEN_H
#define COMPILER_CODEGEN_H

typedef struct codegen {
    void *ir_module;
    void *target_info;
    int optimization_level;
} codegen_t;

codegen_t* codegen_create(void *ir_module, void *target_info);
void codegen_destroy(codegen_t *cg);

void* codegen_emit(codegen_t *cg);

#endif /* COMPILER_CODEGEN_H */
