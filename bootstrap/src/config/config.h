#ifndef VITTE_BOOTSTRAP_CONFIG_H
#define VITTE_BOOTSTRAP_CONFIG_H

#include <stdbool.h>
#include <stddef.h>

#include "../api/context.h"
#include "../api/error.h"
#include "../codegen/codegen.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_CONFIG_DEFAULT_MAX_SOURCE_BYTES ((size_t)64u * 1024u * 1024u)
#define VITTE_CONFIG_DEFAULT_MAX_AST_DEPTH ((size_t)256u)
#define VITTE_CONFIG_DEFAULT_MAX_DIAGNOSTICS ((size_t)100u)
#define VITTE_CONFIG_DEFAULT_MAX_INCLUDE_DEPTH ((size_t)64u)
#define VITTE_CONFIG_DEFAULT_MAX_PATH_LENGTH ((size_t)4096u)

typedef enum vitte_config_target {
    VITTE_CONFIG_TARGET_HOST = 0,
    VITTE_CONFIG_TARGET_C17
} vitte_config_target_t;

typedef enum vitte_config_backend {
    VITTE_CONFIG_BACKEND_C17 = 0
} vitte_config_backend_t;

typedef enum vitte_config_build_mode {
    VITTE_CONFIG_BUILD_MODE_CHECK = 0,
    VITTE_CONFIG_BUILD_MODE_EMIT_C,
    VITTE_CONFIG_BUILD_MODE_BUILD,
    VITTE_CONFIG_BUILD_MODE_RUN
} vitte_config_build_mode_t;

typedef enum vitte_config_diagnostic_mode {
    VITTE_CONFIG_DIAGNOSTIC_PLAIN = 0,
    VITTE_CONFIG_DIAGNOSTIC_COLOR_AUTO,
    VITTE_CONFIG_DIAGNOSTIC_COLOR_NEVER,
    VITTE_CONFIG_DIAGNOSTIC_COLOR_ALWAYS
} vitte_config_diagnostic_mode_t;

typedef struct vitte_config_paths {
    const char *root_path;
    const char *sysroot_path;
    const char *input_path;
    const char *output_path;
    const char *cache_path;
    const char *temp_path;
} vitte_config_paths_t;

typedef struct vitte_config_codegen {
    vitte_config_backend_t backend;
    const char *c_compiler;
    size_t indent_width;
    bool emit_includes;
    bool emit_debug_comments;
    bool keep_intermediate_c;
} vitte_config_codegen_t;

typedef struct vitte_config_limits {
    size_t max_source_bytes;
    size_t max_ast_depth;
    size_t max_diagnostics;
    size_t max_include_depth;
    size_t max_path_length;
} vitte_config_limits_t;

typedef struct vitte_config {
    bool initialized;
    vitte_config_target_t target;
    vitte_config_build_mode_t build_mode;
    vitte_config_diagnostic_mode_t diagnostic_mode;
    vitte_config_paths_t paths;
    vitte_config_codegen_t codegen;
    vitte_config_limits_t limits;
    bool verbose;
    bool warnings_as_errors;
    vitte_error_t last_error;
} vitte_config_t;

void vitte_config_init(vitte_config_t *config);
void vitte_config_reset(vitte_config_t *config);
bool vitte_config_is_initialized(const vitte_config_t *config);
const vitte_error_t *vitte_config_last_error(const vitte_config_t *config);
void vitte_config_clear_error(vitte_config_t *config);

vitte_status_t vitte_config_validate(vitte_config_t *config);

const char *vitte_config_target_name(vitte_config_target_t target);
const char *vitte_config_backend_name(vitte_config_backend_t backend);
const char *vitte_config_build_mode_name(vitte_config_build_mode_t mode);
const char *vitte_config_diagnostic_mode_name(vitte_config_diagnostic_mode_t mode);

vitte_status_t vitte_config_set_input_path(vitte_config_t *config, const char *path);
vitte_status_t vitte_config_set_output_path(vitte_config_t *config, const char *path);
vitte_status_t vitte_config_set_root_path(vitte_config_t *config, const char *path);
vitte_status_t vitte_config_set_sysroot_path(vitte_config_t *config, const char *path);
vitte_status_t vitte_config_set_cache_path(vitte_config_t *config, const char *path);
vitte_status_t vitte_config_set_temp_path(vitte_config_t *config, const char *path);
vitte_status_t vitte_config_set_c_compiler(vitte_config_t *config, const char *cc);
vitte_status_t vitte_config_set_build_mode(vitte_config_t *config, vitte_config_build_mode_t mode);
vitte_status_t vitte_config_set_target(vitte_config_t *config, vitte_config_target_t target);
vitte_status_t vitte_config_set_backend(vitte_config_t *config, vitte_config_backend_t backend);
vitte_status_t vitte_config_set_diagnostic_mode(vitte_config_t *config, vitte_config_diagnostic_mode_t mode);

void vitte_config_to_api_config(const vitte_config_t *config, vitte_api_config_t *api_config);
void vitte_config_to_codegen_options(
    const vitte_config_t *config,
    vitte_codegen_options_t *options,
    char *buffer,
    size_t buffer_capacity
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_CONFIG_H */
