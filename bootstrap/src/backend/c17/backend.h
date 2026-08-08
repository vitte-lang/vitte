#ifndef VITTE_BOOTSTRAP_BACKEND_C17_BACKEND_H
#define VITTE_BOOTSTRAP_BACKEND_C17_BACKEND_H

#include <stdbool.h>
#include <stddef.h>

#include "../../api/error.h"
#include "../../ast/ast.h"
#include "options.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_c17_emit_result {
    vitte_status_t status;
    size_t bytes_written;
    size_t lines_written;
    size_t functions_emitted;
} vitte_c17_emit_result_t;

typedef struct vitte_c17_backend {
    bool initialized;
    vitte_c17_options_t options;
    vitte_error_t last_error;
} vitte_c17_backend_t;

vitte_status_t vitte_c17_backend_init(
    vitte_c17_backend_t *backend,
    const vitte_c17_options_t *options
);

void vitte_c17_backend_destroy(vitte_c17_backend_t *backend);
bool vitte_c17_backend_is_initialized(const vitte_c17_backend_t *backend);
const vitte_error_t *vitte_c17_backend_last_error(const vitte_c17_backend_t *backend);
void vitte_c17_emit_result_init(vitte_c17_emit_result_t *result);

vitte_status_t vitte_c17_backend_emit_ast_to_buffer(
    vitte_c17_backend_t *backend,
    const vitte_ast_t *ast,
    char *buffer,
    size_t buffer_capacity,
    vitte_c17_emit_result_t *result
);

vitte_status_t vitte_c17_backend_emit_ast_to_file(
    vitte_c17_backend_t *backend,
    const vitte_ast_t *ast,
    const char *output_path,
    vitte_c17_emit_result_t *result
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_BACKEND_C17_BACKEND_H */
