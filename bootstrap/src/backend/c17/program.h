#ifndef VITTE_BOOTSTRAP_BACKEND_C17_PROGRAM_H
#define VITTE_BOOTSTRAP_BACKEND_C17_PROGRAM_H

#include <stddef.h>

#include "../../api/error.h"
#include "../../ast/ast.h"
#include "options.h"
#include "writer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_c17_program {
    const vitte_ast_t *ast;
    vitte_c17_options_t options;
    size_t module_count;
    vitte_error_t last_error;
} vitte_c17_program_t;

void vitte_c17_program_init(
    vitte_c17_program_t *program,
    const vitte_ast_t *ast,
    const vitte_c17_options_t *options
);

const vitte_error_t *vitte_c17_program_last_error(const vitte_c17_program_t *program);
vitte_status_t vitte_c17_program_emit(vitte_c17_program_t *program, vitte_c17_writer_t *writer);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_BACKEND_C17_PROGRAM_H */
