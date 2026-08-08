#ifndef VITTE_BOOTSTRAP_API_H
#define VITTE_BOOTSTRAP_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "context.h"
#include "error.h"
#include "version.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum vitte_output_kind {
    VITTE_OUTPUT_CHECK = 0,
    VITTE_OUTPUT_C17,
    VITTE_OUTPUT_EXECUTABLE
} vitte_output_kind_t;

typedef struct vitte_compile_options {
    const char *input_path;
    const char *output_path;
    const char *c_compiler;
    vitte_output_kind_t output_kind;
    bool keep_intermediate_c;
} vitte_compile_options_t;

typedef struct vitte_compile_result {
    vitte_status_t status;
    const char *input_path;
    const char *output_path;
    size_t warning_count;
    size_t error_count;
} vitte_compile_result_t;

vitte_status_t vitte_api_init(const vitte_api_config_t *config);
void vitte_api_shutdown(void);
bool vitte_api_is_initialized(void);

vitte_status_t vitte_api_context_create(
    const vitte_api_config_t *config,
    vitte_context_t **context
);

void vitte_compile_options_init(vitte_compile_options_t *options);
void vitte_compile_result_init(vitte_compile_result_t *result);

vitte_status_t vitte_api_check_file(
    vitte_context_t *context,
    const char *input_path,
    vitte_compile_result_t *result
);

vitte_status_t vitte_api_emit_c_file(
    vitte_context_t *context,
    const char *input_path,
    const char *output_path,
    vitte_compile_result_t *result
);

vitte_status_t vitte_api_build_file(
    vitte_context_t *context,
    const vitte_compile_options_t *options,
    vitte_compile_result_t *result
);

const vitte_error_t *vitte_api_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_API_H */
