#ifndef VITTE_BOOTSTRAP_CODEGEN_H
#define VITTE_BOOTSTRAP_CODEGEN_H

#include <stdbool.h>
#include <stddef.h>

#include "../api/error.h"
#include "../ast/ast.h"
#include "../hir/hir.h"
#include "../ir/ir.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum vitte_codegen_backend {
    VITTE_CODEGEN_BACKEND_C17 = 0
} vitte_codegen_backend_t;

typedef enum vitte_codegen_input_kind {
    VITTE_CODEGEN_INPUT_AST = 0,
    VITTE_CODEGEN_INPUT_HIR,
    VITTE_CODEGEN_INPUT_IR
} vitte_codegen_input_kind_t;

typedef enum vitte_codegen_output_kind {
    VITTE_CODEGEN_OUTPUT_BUFFER = 0,
    VITTE_CODEGEN_OUTPUT_FILE
} vitte_codegen_output_kind_t;

typedef struct vitte_codegen_options {
    vitte_codegen_backend_t backend;
    vitte_codegen_input_kind_t input_kind;
    vitte_codegen_output_kind_t output_kind;
    const char *source_name;
    const char *output_path;
    char *buffer;
    size_t buffer_capacity;
    size_t indent_width;
    bool emit_includes;
    bool emit_debug_comments;
} vitte_codegen_options_t;

typedef struct vitte_codegen_result {
    vitte_status_t status;
    vitte_codegen_backend_t backend;
    vitte_codegen_input_kind_t input_kind;
    vitte_codegen_output_kind_t output_kind;
    size_t bytes_written;
    size_t lines_written;
    size_t units_emitted;
    size_t functions_emitted;
    const char *output_path;
    size_t error_count;
} vitte_codegen_result_t;

typedef struct vitte_codegen {
    bool initialized;
    vitte_codegen_options_t options;
    vitte_error_t last_error;
} vitte_codegen_t;

void vitte_codegen_options_init(vitte_codegen_options_t *options);
vitte_status_t vitte_codegen_options_validate(const vitte_codegen_options_t *options, vitte_error_t *error);
void vitte_codegen_result_init(vitte_codegen_result_t *result);

vitte_status_t vitte_codegen_init(vitte_codegen_t *codegen, const vitte_codegen_options_t *options);
void vitte_codegen_destroy(vitte_codegen_t *codegen);
bool vitte_codegen_is_initialized(const vitte_codegen_t *codegen);
const vitte_error_t *vitte_codegen_last_error(const vitte_codegen_t *codegen);
void vitte_codegen_clear_error(vitte_codegen_t *codegen);

const char *vitte_codegen_backend_name(vitte_codegen_backend_t backend);
const char *vitte_codegen_input_kind_name(vitte_codegen_input_kind_t kind);
const char *vitte_codegen_output_kind_name(vitte_codegen_output_kind_t kind);

vitte_status_t vitte_codegen_emit_ast_to_buffer(
    vitte_codegen_t *codegen,
    const vitte_ast_t *ast,
    char *buffer,
    size_t buffer_capacity,
    vitte_codegen_result_t *result
);

vitte_status_t vitte_codegen_emit_ast_to_file(
    vitte_codegen_t *codegen,
    const vitte_ast_t *ast,
    const char *output_path,
    vitte_codegen_result_t *result
);

vitte_status_t vitte_codegen_emit_hir_to_buffer(
    vitte_codegen_t *codegen,
    const vitte_hir_t *hir,
    char *buffer,
    size_t buffer_capacity,
    vitte_codegen_result_t *result
);

vitte_status_t vitte_codegen_emit_hir_to_file(
    vitte_codegen_t *codegen,
    const vitte_hir_t *hir,
    const char *output_path,
    vitte_codegen_result_t *result
);

vitte_status_t vitte_codegen_emit_ir_to_buffer(
    vitte_codegen_t *codegen,
    const vitte_ir_t *ir,
    char *buffer,
    size_t buffer_capacity,
    vitte_codegen_result_t *result
);

vitte_status_t vitte_codegen_emit_ir_to_file(
    vitte_codegen_t *codegen,
    const vitte_ir_t *ir,
    const char *output_path,
    vitte_codegen_result_t *result
);

vitte_status_t vitte_codegen_emit(
    vitte_codegen_t *codegen,
    const void *input,
    vitte_codegen_result_t *result
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_CODEGEN_H */
