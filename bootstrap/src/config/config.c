#include "config.h"

#include <string.h>

static void vitte_config_set_error(
    vitte_config_t *config,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (config != NULL) {
        vitte_error_set_details(&config->last_error, status, code, message, details);
    }
}

static bool vitte_config_target_is_valid(vitte_config_target_t target) {
    return target == VITTE_CONFIG_TARGET_HOST || target == VITTE_CONFIG_TARGET_C17;
}

static bool vitte_config_backend_is_valid(vitte_config_backend_t backend) {
    return backend == VITTE_CONFIG_BACKEND_C17;
}

static bool vitte_config_build_mode_is_valid(vitte_config_build_mode_t mode) {
    return mode == VITTE_CONFIG_BUILD_MODE_CHECK ||
        mode == VITTE_CONFIG_BUILD_MODE_EMIT_C ||
        mode == VITTE_CONFIG_BUILD_MODE_BUILD ||
        mode == VITTE_CONFIG_BUILD_MODE_RUN;
}

static bool vitte_config_diagnostic_mode_is_valid(vitte_config_diagnostic_mode_t mode) {
    return mode == VITTE_CONFIG_DIAGNOSTIC_PLAIN ||
        mode == VITTE_CONFIG_DIAGNOSTIC_COLOR_AUTO ||
        mode == VITTE_CONFIG_DIAGNOSTIC_COLOR_NEVER ||
        mode == VITTE_CONFIG_DIAGNOSTIC_COLOR_ALWAYS;
}

void vitte_config_init(vitte_config_t *config) {
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(*config));
    config->initialized = true;
    config->target = VITTE_CONFIG_TARGET_HOST;
    config->build_mode = VITTE_CONFIG_BUILD_MODE_CHECK;
    config->diagnostic_mode = VITTE_CONFIG_DIAGNOSTIC_COLOR_AUTO;
    config->codegen.backend = VITTE_CONFIG_BACKEND_C17;
    config->codegen.c_compiler = "cc";
    config->codegen.indent_width = 4u;
    config->codegen.emit_includes = true;
    config->limits.max_source_bytes = VITTE_CONFIG_DEFAULT_MAX_SOURCE_BYTES;
    config->limits.max_ast_depth = VITTE_CONFIG_DEFAULT_MAX_AST_DEPTH;
    config->limits.max_diagnostics = VITTE_CONFIG_DEFAULT_MAX_DIAGNOSTICS;
    config->limits.max_include_depth = VITTE_CONFIG_DEFAULT_MAX_INCLUDE_DEPTH;
    config->limits.max_path_length = VITTE_CONFIG_DEFAULT_MAX_PATH_LENGTH;
    vitte_error_init(&config->last_error);
}

void vitte_config_reset(vitte_config_t *config) {
    vitte_config_init(config);
}

bool vitte_config_is_initialized(const vitte_config_t *config) {
    return config != NULL && config->initialized;
}

const vitte_error_t *vitte_config_last_error(const vitte_config_t *config) {
    return config != NULL ? &config->last_error : vitte_error_last();
}

void vitte_config_clear_error(vitte_config_t *config) {
    if (config != NULL) {
        vitte_error_reset(&config->last_error);
    }
}

static bool vitte_config_path_is_valid(const vitte_config_t *config, const char *path) {
    size_t length;

    if (path == NULL) {
        return true;
    }
    if (path[0] == '\0') {
        return false;
    }
    length = strlen(path);
    return config == NULL || config->limits.max_path_length == 0u || length <= config->limits.max_path_length;
}

static vitte_status_t vitte_config_require_path(vitte_config_t *config, const char *path, const char *name) {
    if (path == NULL || path[0] == '\0') {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_PATH", "missing required config path", name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_config_path_is_valid(config, path)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_PATH", "invalid config path", name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_config_validate(vitte_config_t *config) {
    vitte_status_t status;

    if (!vitte_config_is_initialized(config)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CONFIG_E_STATE", "config is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (!vitte_config_target_is_valid(config->target)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_TARGET", "invalid config target", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_config_backend_is_valid(config->codegen.backend)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_CONFIG_E_BACKEND", "unsupported config backend", NULL);
        return VITTE_STATUS_ERROR_UNSUPPORTED;
    }
    if (!vitte_config_build_mode_is_valid(config->build_mode)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_MODE", "invalid config build mode", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_config_diagnostic_mode_is_valid(config->diagnostic_mode)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_MODE", "invalid config diagnostic mode", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (config->codegen.indent_width > 16u) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_LIMIT", "invalid config indentation width", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (config->limits.max_source_bytes == 0u ||
        config->limits.max_ast_depth == 0u ||
        config->limits.max_diagnostics == 0u ||
        config->limits.max_include_depth == 0u ||
        config->limits.max_path_length == 0u) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_LIMIT", "config limits must be non-zero", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_config_path_is_valid(config, config->paths.root_path) ||
        !vitte_config_path_is_valid(config, config->paths.sysroot_path) ||
        !vitte_config_path_is_valid(config, config->paths.input_path) ||
        !vitte_config_path_is_valid(config, config->paths.output_path) ||
        !vitte_config_path_is_valid(config, config->paths.cache_path) ||
        !vitte_config_path_is_valid(config, config->paths.temp_path)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_PATH", "invalid config path", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_config_require_path(config, config->paths.input_path, "input_path");
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    if ((config->build_mode == VITTE_CONFIG_BUILD_MODE_BUILD ||
            config->build_mode == VITTE_CONFIG_BUILD_MODE_RUN) &&
        (config->codegen.c_compiler == NULL || config->codegen.c_compiler[0] == '\0')) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_PATH", "missing C compiler for build mode", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_error_reset(&config->last_error);
    return VITTE_STATUS_OK;
}

const char *vitte_config_target_name(vitte_config_target_t target) {
    switch (target) {
        case VITTE_CONFIG_TARGET_HOST:
            return "host";
        case VITTE_CONFIG_TARGET_C17:
            return "c17";
        default:
            return "unknown";
    }
}

const char *vitte_config_backend_name(vitte_config_backend_t backend) {
    switch (backend) {
        case VITTE_CONFIG_BACKEND_C17:
            return "c17";
        default:
            return "unknown";
    }
}

const char *vitte_config_build_mode_name(vitte_config_build_mode_t mode) {
    switch (mode) {
        case VITTE_CONFIG_BUILD_MODE_CHECK:
            return "check";
        case VITTE_CONFIG_BUILD_MODE_EMIT_C:
            return "emit-c";
        case VITTE_CONFIG_BUILD_MODE_BUILD:
            return "build";
        case VITTE_CONFIG_BUILD_MODE_RUN:
            return "run";
        default:
            return "unknown";
    }
}

const char *vitte_config_diagnostic_mode_name(vitte_config_diagnostic_mode_t mode) {
    switch (mode) {
        case VITTE_CONFIG_DIAGNOSTIC_PLAIN:
            return "plain";
        case VITTE_CONFIG_DIAGNOSTIC_COLOR_AUTO:
            return "color-auto";
        case VITTE_CONFIG_DIAGNOSTIC_COLOR_NEVER:
            return "color-never";
        case VITTE_CONFIG_DIAGNOSTIC_COLOR_ALWAYS:
            return "color-always";
        default:
            return "unknown";
    }
}

static vitte_status_t vitte_config_set_path(const char **slot, vitte_config_t *config, const char *path, const char *name) {
    if (!vitte_config_is_initialized(config) || slot == NULL) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CONFIG_E_STATE", "config is not initialized", name);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (!vitte_config_path_is_valid(config, path)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_PATH", "invalid config path", name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    *slot = path;
    vitte_error_reset(&config->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_config_set_input_path(vitte_config_t *config, const char *path) {
    if (config == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return vitte_config_set_path(&config->paths.input_path, config, path, "input_path");
}

vitte_status_t vitte_config_set_output_path(vitte_config_t *config, const char *path) {
    if (config == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return vitte_config_set_path(&config->paths.output_path, config, path, "output_path");
}

vitte_status_t vitte_config_set_root_path(vitte_config_t *config, const char *path) {
    if (config == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return vitte_config_set_path(&config->paths.root_path, config, path, "root_path");
}

vitte_status_t vitte_config_set_sysroot_path(vitte_config_t *config, const char *path) {
    if (config == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return vitte_config_set_path(&config->paths.sysroot_path, config, path, "sysroot_path");
}

vitte_status_t vitte_config_set_cache_path(vitte_config_t *config, const char *path) {
    if (config == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return vitte_config_set_path(&config->paths.cache_path, config, path, "cache_path");
}

vitte_status_t vitte_config_set_temp_path(vitte_config_t *config, const char *path) {
    if (config == NULL) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return vitte_config_set_path(&config->paths.temp_path, config, path, "temp_path");
}

vitte_status_t vitte_config_set_c_compiler(vitte_config_t *config, const char *cc) {
    if (!vitte_config_is_initialized(config)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CONFIG_E_STATE", "config is not initialized", "c_compiler");
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (cc == NULL || cc[0] == '\0') {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_PATH", "invalid C compiler", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    config->codegen.c_compiler = cc;
    vitte_error_reset(&config->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_config_set_build_mode(vitte_config_t *config, vitte_config_build_mode_t mode) {
    if (!vitte_config_is_initialized(config)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CONFIG_E_STATE", "config is not initialized", "build_mode");
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (!vitte_config_build_mode_is_valid(mode)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_MODE", "invalid config build mode", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    config->build_mode = mode;
    vitte_error_reset(&config->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_config_set_target(vitte_config_t *config, vitte_config_target_t target) {
    if (!vitte_config_is_initialized(config)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CONFIG_E_STATE", "config is not initialized", "target");
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (!vitte_config_target_is_valid(target)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_TARGET", "invalid config target", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    config->target = target;
    vitte_error_reset(&config->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_config_set_backend(vitte_config_t *config, vitte_config_backend_t backend) {
    if (!vitte_config_is_initialized(config)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CONFIG_E_STATE", "config is not initialized", "backend");
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (!vitte_config_backend_is_valid(backend)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_UNSUPPORTED, "VITTE_CONFIG_E_BACKEND", "unsupported config backend", NULL);
        return VITTE_STATUS_ERROR_UNSUPPORTED;
    }
    config->codegen.backend = backend;
    vitte_error_reset(&config->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_config_set_diagnostic_mode(vitte_config_t *config, vitte_config_diagnostic_mode_t mode) {
    if (!vitte_config_is_initialized(config)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CONFIG_E_STATE", "config is not initialized", "diagnostic_mode");
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (!vitte_config_diagnostic_mode_is_valid(mode)) {
        vitte_config_set_error(config, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CONFIG_E_MODE", "invalid config diagnostic mode", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    config->diagnostic_mode = mode;
    vitte_error_reset(&config->last_error);
    return VITTE_STATUS_OK;
}

void vitte_config_to_api_config(const vitte_config_t *config, vitte_api_config_t *api_config) {
    if (api_config == NULL) {
        return;
    }

    vitte_api_config_init(api_config);
    if (config == NULL) {
        return;
    }
    api_config->root_path = config->paths.root_path;
    api_config->sysroot_path = config->paths.sysroot_path;
    api_config->deterministic = true;
}

void vitte_config_to_codegen_options(
    const vitte_config_t *config,
    vitte_codegen_options_t *options,
    char *buffer,
    size_t buffer_capacity
) {
    if (options == NULL) {
        return;
    }

    vitte_codegen_options_init(options);
    if (config == NULL) {
        options->buffer = buffer;
        options->buffer_capacity = buffer_capacity;
        return;
    }

    options->backend = VITTE_CODEGEN_BACKEND_C17;
    options->input_kind = VITTE_CODEGEN_INPUT_AST;
    options->output_kind = config->paths.output_path != NULL ? VITTE_CODEGEN_OUTPUT_FILE : VITTE_CODEGEN_OUTPUT_BUFFER;
    options->source_name = config->paths.input_path;
    options->output_path = config->paths.output_path;
    options->buffer = buffer;
    options->buffer_capacity = buffer_capacity;
    options->indent_width = config->codegen.indent_width;
    options->emit_includes = config->codegen.emit_includes;
    options->emit_debug_comments = config->codegen.emit_debug_comments;
}
