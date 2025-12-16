#ifndef COMPILER_NATIVE_CODEGEN_H
#define COMPILER_NATIVE_CODEGEN_H

#include <stdint.h>
#include <stdio.h>

/* Native assembly code generation (alternative to C backend) */

typedef enum {
    NATIVE_X86_64,
    NATIVE_AARCH64,
} native_target_t;

typedef struct native_codegen {
    FILE *out;
    native_target_t target;
    int indent_level;
    uint32_t label_count;
    uint32_t temp_var_count;
} native_codegen_t;

/* Create native code generator */
native_codegen_t* native_codegen_create(FILE *out, native_target_t target);
void native_codegen_destroy(native_codegen_t *gen);

/* Emit native code */
int native_codegen_emit_function(native_codegen_t *gen, void *ir_func);
int native_codegen_emit_preamble(native_codegen_t *gen);
int native_codegen_emit_epilogue(native_codegen_t *gen);

/* Helper functions */
const char* native_get_register(native_target_t target, int idx);
uint32_t native_get_label(native_codegen_t *gen);

#endif /* COMPILER_NATIVE_CODEGEN_H */
