#ifndef VITTE_BOOTSTRAP_DRIVER_H
#define VITTE_BOOTSTRAP_DRIVER_H

#include <stdbool.h>
#include <stddef.h>

#include "../api/context.h"
#include "../api/error.h"
#include "../ast/ast.h"
#include "../config/config.h"
#include "../diagnostic/diagnostic.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_DRIVER_DEFAULT_OUTPUT_BUFFER_SIZE ((size_t)256u * 1024u)
#define VITTE_DRIVER_MAX_PATH_LENGTH ((size_t)4096u)

typedef enum vitte_driver_emit_kind {
    VITTE_DRIVER_EMIT_CHECK = 0,
    VITTE_DRIVER_EMIT_AST,
    VITTE_DRIVER_EMIT_C,
    VITTE_DRIVER_EMIT_OBJECT,
    VITTE_DRIVER_EMIT_BINARY
} vitte_driver_emit_kind_t;

typedef enum vitte_driver_stage {
    VITTE_DRIVER_STAGE_INIT = 0,
    VITTE_DRIVER_STAGE_LOAD_CONFIG,
    VITTE_DRIVER_STAGE_LOAD_SOURCE,
    VITTE_DRIVER_STAGE_LEX,
    VITTE_DRIVER_STAGE_PARSE,
    VITTE_DRIVER_STAGE_BUILD_AST,
    VITTE_DRIVER_STAGE_VALIDATE_AST,
    VITTE_DRIVER_STAGE_CONSTANTS,
    VITTE_DRIVER_STAGE_SEMANTIC,
    VITTE_DRIVER_STAGE_BACKEND,
    VITTE_DRIVER_STAGE_CODEGEN_C,
    VITTE_DRIVER_STAGE_COMPILE_LINK,
    VITTE_DRIVER_STAGE_DIAGNOSTICS,
    VITTE_DRIVER_STAGE_CLEANUP,
    VITTE_DRIVER_STAGE_COUNT
} vitte_driver_stage_t;

typedef struct vitte_driver_options {
    const char *input_path;
    const char *output_path;
    const char *module_name;
    const char *root_path;
    const char *sysroot_path;
    const char *target_triple;
    const char *c_compiler;
    vitte_driver_emit_kind_t emit_kind;
    size_t optimization_level;
    size_t max_source_bytes;
    size_t max_ast_depth;
    size_t max_diagnostics;
    bool warnings_as_errors;
    bool color_diagnostics;
    bool dump_ast;
    bool verbose;
    bool keep_intermediate_c;
} vitte_driver_options_t;

typedef enum vitte_driver_input_kind {
    VITTE_DRIVER_INPUT_NONE = 0,
    VITTE_DRIVER_INPUT_FILE,
    VITTE_DRIVER_INPUT_BUFFER
} vitte_driver_input_kind_t;

typedef struct vitte_driver_input {
    vitte_driver_input_kind_t kind;
    const char *source_name;
    const char *path;
    const char *buffer;
    size_t size;
    bool owns_buffer;
} vitte_driver_input_t;

typedef struct vitte_driver_output {
    vitte_driver_emit_kind_t kind;
    const char *path;
    const char *c_path;
    const char *buffer;
    size_t bytes_written;
    size_t lines_written;
    size_t functions_emitted;
} vitte_driver_output_t;

typedef struct vitte_driver_result {
    vitte_status_t status;
    vitte_driver_stage_t failed_stage;
    vitte_driver_output_t output;
    size_t warning_count;
    size_t error_count;
    size_t diagnostic_count;
    size_t stages_completed;
    char generated_c_path[VITTE_DRIVER_MAX_PATH_LENGTH];
    char output_buffer[VITTE_DRIVER_DEFAULT_OUTPUT_BUFFER_SIZE];
    vitte_error_t last_error;
} vitte_driver_result_t;

typedef struct vitte_driver_pipeline {
    vitte_status_t stages[VITTE_DRIVER_STAGE_COUNT];
    size_t completed_count;
    vitte_driver_stage_t failed_stage;
} vitte_driver_pipeline_t;

typedef struct vitte_driver {
    bool initialized;
    vitte_context_t *context;
    vitte_config_t config;
    vitte_diagnostic_options_t diagnostic_options;
    vitte_diagnostic_t diagnostic_storage[128];
    vitte_diagnostic_bag_t diagnostics;
    vitte_driver_pipeline_t pipeline;
    vitte_error_t last_error;
} vitte_driver_t;

typedef vitte_driver_t vitte_driver_session_t;

void vitte_driver_options_init(vitte_driver_options_t *options);
bool vitte_driver_emit_kind_is_valid(vitte_driver_emit_kind_t kind);
const char *vitte_driver_emit_kind_name(vitte_driver_emit_kind_t kind);
const char *vitte_driver_stage_name(vitte_driver_stage_t stage);

void vitte_driver_input_init(vitte_driver_input_t *input);
vitte_status_t vitte_driver_input_from_buffer(
    vitte_driver_input_t *input,
    const char *source_name,
    const char *buffer,
    size_t size
);
vitte_status_t vitte_driver_input_from_file(
    vitte_driver_input_t *input,
    const char *path,
    size_t max_bytes
);
void vitte_driver_input_destroy(vitte_driver_input_t *input);

void vitte_driver_output_init(vitte_driver_output_t *output);
void vitte_driver_result_init(vitte_driver_result_t *result);
void vitte_driver_result_reset(vitte_driver_result_t *result);

vitte_status_t vitte_driver_init(
    vitte_driver_t *driver,
    vitte_context_t *context,
    const vitte_driver_options_t *options
);
void vitte_driver_shutdown(vitte_driver_t *driver);
bool vitte_driver_is_initialized(const vitte_driver_t *driver);
const vitte_error_t *vitte_driver_last_error(const vitte_driver_t *driver);
const vitte_diagnostic_bag_t *vitte_driver_diagnostics(const vitte_driver_t *driver);

vitte_status_t vitte_driver_run(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input,
    vitte_driver_result_t *result
);
vitte_status_t vitte_driver_check(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input,
    vitte_driver_result_t *result
);
vitte_status_t vitte_driver_emit_c(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input,
    const char *output_path,
    vitte_driver_result_t *result
);
vitte_status_t vitte_driver_build(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input,
    const char *output_path,
    vitte_driver_result_t *result
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_DRIVER_H */
