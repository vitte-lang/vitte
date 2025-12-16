#ifndef COMPILER_DRIVER_H
#define COMPILER_DRIVER_H

#include <stdint.h>

typedef enum {
    EMIT_AST,
    EMIT_HIR,
    EMIT_IR,
    EMIT_C,
} emit_kind_t;

typedef struct compile_opts {
    emit_kind_t emit_kind;
    int verbose;
    int json_diagnostics;
    const char *output_file;
} compile_opts_t;

typedef struct compile_result {
    int success;
    void *output;
    uint32_t output_size;
} compile_result_t;

compile_result_t* compiler_compile(const char *input_file, compile_opts_t *opts);
void compile_result_free(compile_result_t *res);

#endif /* COMPILER_DRIVER_H */
