#include "driver.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../codegen/codegen.h"
#include "../filesystem/filesystem.h"
#include "../hir/hir.h"
#include "../import/import.h"
#include "../ir/ir.h"
#include "../module/module.h"
#include "../parser/parser.h"
#include "../sema/sema.h"

#define VITTE_DRIVER_MAX_IMPORTED_UNITS ((size_t)256u)

typedef struct vitte_driver_import_unit {
    char module_name[VITTE_IMPORT_MAX_MODULE_NAME];
    char resolved_path[VITTE_FS_MAX_PATH];
    vitte_ast_t ast;
    vitte_module_t module;
} vitte_driver_import_unit_t;

typedef struct vitte_driver_flatten_binding {
    const char *visible_name;
    const char *owner_module_name;
    const vitte_ast_decl_t *source_decl;
    struct vitte_driver_flatten_binding *next;
} vitte_driver_flatten_binding_t;

typedef struct vitte_driver_flatten_state {
    vitte_driver_flatten_binding_t *bindings;
} vitte_driver_flatten_state_t;

static const vitte_driver_import_unit_t *vitte_driver_find_import_unit_by_path(
    vitte_driver_import_unit_t *const *units,
    size_t unit_count,
    const char *resolved_path
);

static void vitte_driver_set_error(
    vitte_driver_t *driver,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (driver != NULL) {
        vitte_error_set_details(&driver->last_error, status, code, message, details);
    }
}

static void vitte_driver_result_set_error(
    vitte_driver_result_t *result,
    vitte_status_t status,
    vitte_driver_stage_t stage,
    const char *code,
    const char *message,
    const char *details
) {
    if (result == NULL) {
        return;
    }
    result->status = status;
    result->failed_stage = stage;
    vitte_error_set_details(&result->last_error, status, code, message, details);
}

static vitte_status_t vitte_driver_add_diag(
    vitte_driver_t *driver,
    vitte_diagnostic_severity_t severity,
    const char *code,
    const char *message,
    const char *details
) {
    if (driver == NULL || !vitte_diagnostic_bag_is_initialized(&driver->diagnostics)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return vitte_diagnostic_add(&driver->diagnostics, severity, code, message, details, NULL);
}

static void vitte_driver_pipeline_reset(vitte_driver_pipeline_t *pipeline) {
    size_t index;

    if (pipeline == NULL) {
        return;
    }
    for (index = 0u; index < VITTE_DRIVER_STAGE_COUNT; index++) {
        pipeline->stages[index] = VITTE_STATUS_OK;
    }
    pipeline->completed_count = 0u;
    pipeline->failed_stage = VITTE_DRIVER_STAGE_COUNT;
}

static void vitte_driver_pipeline_mark(
    vitte_driver_pipeline_t *pipeline,
    vitte_driver_stage_t stage,
    vitte_status_t status
) {
    if (pipeline == NULL || stage >= VITTE_DRIVER_STAGE_COUNT) {
        return;
    }
    pipeline->stages[stage] = status;
    if (status == VITTE_STATUS_OK) {
        pipeline->completed_count++;
    } else if (pipeline->failed_stage == VITTE_DRIVER_STAGE_COUNT) {
        pipeline->failed_stage = stage;
    }
}

void vitte_driver_options_init(vitte_driver_options_t *options) {
    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->target_triple = "host";
    options->c_compiler = "cc";
    options->emit_kind = VITTE_DRIVER_EMIT_CHECK;
    options->max_source_bytes = VITTE_CONFIG_DEFAULT_MAX_SOURCE_BYTES;
    options->max_ast_depth = VITTE_CONFIG_DEFAULT_MAX_AST_DEPTH;
    options->max_diagnostics = VITTE_CONFIG_DEFAULT_MAX_DIAGNOSTICS;
}

bool vitte_driver_emit_kind_is_valid(vitte_driver_emit_kind_t kind) {
    return kind == VITTE_DRIVER_EMIT_CHECK ||
        kind == VITTE_DRIVER_EMIT_AST ||
        kind == VITTE_DRIVER_EMIT_C ||
        kind == VITTE_DRIVER_EMIT_OBJECT ||
        kind == VITTE_DRIVER_EMIT_BINARY;
}

const char *vitte_driver_emit_kind_name(vitte_driver_emit_kind_t kind) {
    switch (kind) {
        case VITTE_DRIVER_EMIT_CHECK:
            return "check";
        case VITTE_DRIVER_EMIT_AST:
            return "ast";
        case VITTE_DRIVER_EMIT_C:
            return "c17";
        case VITTE_DRIVER_EMIT_OBJECT:
            return "object";
        case VITTE_DRIVER_EMIT_BINARY:
            return "binary";
        default:
            return "unknown";
    }
}

const char *vitte_driver_stage_name(vitte_driver_stage_t stage) {
    switch (stage) {
        case VITTE_DRIVER_STAGE_INIT:
            return "init";
        case VITTE_DRIVER_STAGE_LOAD_CONFIG:
            return "load-config";
        case VITTE_DRIVER_STAGE_LOAD_SOURCE:
            return "load-source";
        case VITTE_DRIVER_STAGE_LEX:
            return "lex";
        case VITTE_DRIVER_STAGE_PARSE:
            return "parse";
        case VITTE_DRIVER_STAGE_BUILD_AST:
            return "build-ast";
        case VITTE_DRIVER_STAGE_VALIDATE_AST:
            return "validate-ast";
        case VITTE_DRIVER_STAGE_CONSTANTS:
            return "constants";
        case VITTE_DRIVER_STAGE_SEMANTIC:
            return "semantic";
        case VITTE_DRIVER_STAGE_BACKEND:
            return "backend";
        case VITTE_DRIVER_STAGE_CODEGEN_C:
            return "codegen-c";
        case VITTE_DRIVER_STAGE_COMPILE_LINK:
            return "compile-link";
        case VITTE_DRIVER_STAGE_DIAGNOSTICS:
            return "diagnostics";
        case VITTE_DRIVER_STAGE_CLEANUP:
            return "cleanup";
        case VITTE_DRIVER_STAGE_COUNT:
        default:
            return "unknown";
    }
}

void vitte_driver_input_init(vitte_driver_input_t *input) {
    if (input == NULL) {
        return;
    }
    memset(input, 0, sizeof(*input));
}

vitte_status_t vitte_driver_input_from_buffer(
    vitte_driver_input_t *input,
    const char *source_name,
    const char *buffer,
    size_t size
) {
    if (input == NULL || buffer == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0u) {
        size = strlen(buffer);
    }

    vitte_driver_input_init(input);
    input->kind = VITTE_DRIVER_INPUT_BUFFER;
    input->source_name = source_name != NULL ? source_name : "<memory>";
    input->buffer = buffer;
    input->size = size;
    input->owns_buffer = false;
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_driver_file_size(FILE *stream, size_t *size) {
    long end;

    if (stream == NULL || size == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (fseek(stream, 0L, SEEK_END) != 0) {
        return VITTE_STATUS_ERROR_IO;
    }
    end = ftell(stream);
    if (end < 0L) {
        return VITTE_STATUS_ERROR_IO;
    }
    if (fseek(stream, 0L, SEEK_SET) != 0) {
        return VITTE_STATUS_ERROR_IO;
    }
    *size = (size_t)end;
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_driver_input_from_file(
    vitte_driver_input_t *input,
    const char *path,
    size_t max_bytes
) {
    FILE *stream;
    char *buffer;
    size_t size;
    size_t read_count;
    vitte_status_t status;

    if (input == NULL || path == NULL || path[0] == '\0') {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    stream = fopen(path, "rb");
    if (stream == NULL) {
        return VITTE_STATUS_ERROR_IO;
    }

    status = vitte_driver_file_size(stream, &size);
    if (status != VITTE_STATUS_OK) {
        (void)fclose(stream);
        return status;
    }
    if (max_bytes != 0u && size > max_bytes) {
        (void)fclose(stream);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (size == (size_t)-1) {
        (void)fclose(stream);
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    buffer = (char *)malloc(size + 1u);
    if (buffer == NULL) {
        (void)fclose(stream);
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    read_count = size > 0u ? fread(buffer, 1u, size, stream) : 0u;
    if (read_count != size || ferror(stream) != 0) {
        free(buffer);
        (void)fclose(stream);
        return VITTE_STATUS_ERROR_IO;
    }
    buffer[size] = '\0';
    if (fclose(stream) != 0) {
        free(buffer);
        return VITTE_STATUS_ERROR_IO;
    }

    vitte_driver_input_init(input);
    input->kind = VITTE_DRIVER_INPUT_FILE;
    input->source_name = path;
    input->path = path;
    input->buffer = buffer;
    input->size = size;
    input->owns_buffer = true;
    return VITTE_STATUS_OK;
}

void vitte_driver_input_destroy(vitte_driver_input_t *input) {
    if (input == NULL) {
        return;
    }
    if (input->owns_buffer) {
        free((void *)input->buffer);
    }
    vitte_driver_input_init(input);
}

void vitte_driver_output_init(vitte_driver_output_t *output) {
    if (output == NULL) {
        return;
    }
    memset(output, 0, sizeof(*output));
    output->kind = VITTE_DRIVER_EMIT_CHECK;
}

void vitte_driver_result_init(vitte_driver_result_t *result) {
    if (result == NULL) {
        return;
    }
    memset(result, 0, sizeof(*result));
    result->status = VITTE_STATUS_OK;
    result->failed_stage = VITTE_DRIVER_STAGE_COUNT;
    result->output.kind = VITTE_DRIVER_EMIT_CHECK;
    vitte_error_init(&result->last_error);
}

void vitte_driver_result_reset(vitte_driver_result_t *result) {
    vitte_driver_result_init(result);
}

static vitte_config_build_mode_t vitte_driver_config_mode(vitte_driver_emit_kind_t kind) {
    if (kind == VITTE_DRIVER_EMIT_C) {
        return VITTE_CONFIG_BUILD_MODE_EMIT_C;
    }
    if (kind == VITTE_DRIVER_EMIT_BINARY || kind == VITTE_DRIVER_EMIT_OBJECT) {
        return VITTE_CONFIG_BUILD_MODE_BUILD;
    }
    return VITTE_CONFIG_BUILD_MODE_CHECK;
}

static vitte_status_t vitte_driver_apply_options(
    vitte_driver_t *driver,
    const vitte_driver_options_t *options
) {
    vitte_driver_options_t defaults;
    const vitte_driver_options_t *effective_options = options;

    if (driver == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (effective_options == NULL) {
        vitte_driver_options_init(&defaults);
        effective_options = &defaults;
    }
    if (!vitte_driver_emit_kind_is_valid(effective_options->emit_kind)) {
        vitte_driver_set_error(driver, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_DRIVER_E_OPTIONS", "invalid driver emit kind", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (effective_options->optimization_level > 3u) {
        vitte_driver_set_error(driver, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_DRIVER_E_OPTIONS", "invalid optimization level", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_config_init(&driver->config);
    driver->config.paths.input_path = effective_options->input_path;
    driver->config.paths.output_path = effective_options->output_path;
    driver->config.paths.root_path = effective_options->root_path;
    driver->config.paths.sysroot_path = effective_options->sysroot_path;
    driver->config.target = VITTE_CONFIG_TARGET_HOST;
    driver->config.build_mode = vitte_driver_config_mode(effective_options->emit_kind);
    driver->config.codegen.c_compiler = effective_options->c_compiler != NULL ? effective_options->c_compiler : "cc";
    driver->config.codegen.keep_intermediate_c = effective_options->keep_intermediate_c;
    driver->config.limits.max_source_bytes = effective_options->max_source_bytes != 0u ?
        effective_options->max_source_bytes :
        VITTE_CONFIG_DEFAULT_MAX_SOURCE_BYTES;
    driver->config.limits.max_ast_depth = effective_options->max_ast_depth != 0u ?
        effective_options->max_ast_depth :
        VITTE_CONFIG_DEFAULT_MAX_AST_DEPTH;
    driver->config.limits.max_diagnostics = effective_options->max_diagnostics != 0u ?
        effective_options->max_diagnostics :
        VITTE_CONFIG_DEFAULT_MAX_DIAGNOSTICS;
    driver->config.verbose = effective_options->verbose;
    driver->config.warnings_as_errors = effective_options->warnings_as_errors;

    vitte_diagnostic_options_init(&driver->diagnostic_options);
    driver->diagnostic_options.max_diagnostics = driver->config.limits.max_diagnostics;
    if (driver->diagnostic_options.max_diagnostics > 128u) {
        driver->diagnostic_options.max_diagnostics = 128u;
    }
    driver->diagnostic_options.warnings_as_errors = effective_options->warnings_as_errors;
    driver->diagnostic_options.color_enabled = effective_options->color_diagnostics;
    driver->diagnostic_options.show_codes = true;
    driver->diagnostic_options.show_details = true;

    return VITTE_STATUS_OK;
}

vitte_status_t vitte_driver_init(
    vitte_driver_t *driver,
    vitte_context_t *context,
    const vitte_driver_options_t *options
) {
    vitte_status_t status;

    if (driver == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(driver, 0, sizeof(*driver));
    vitte_error_init(&driver->last_error);
    vitte_driver_pipeline_reset(&driver->pipeline);
    driver->context = context;

    status = vitte_driver_apply_options(driver, options);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    status = vitte_diagnostic_bag_init(
        &driver->diagnostics,
        driver->diagnostic_storage,
        sizeof(driver->diagnostic_storage) / sizeof(driver->diagnostic_storage[0]),
        &driver->diagnostic_options
    );
    if (status != VITTE_STATUS_OK) {
        vitte_driver_set_error(driver, status, "VITTE_DRIVER_E_DIAGNOSTIC", "failed to initialize diagnostics", NULL);
        return status;
    }

    driver->initialized = true;
    return VITTE_STATUS_OK;
}

void vitte_driver_shutdown(vitte_driver_t *driver) {
    if (driver == NULL) {
        return;
    }
    memset(driver, 0, sizeof(*driver));
}

bool vitte_driver_is_initialized(const vitte_driver_t *driver) {
    return driver != NULL && driver->initialized;
}

const vitte_error_t *vitte_driver_last_error(const vitte_driver_t *driver) {
    return driver != NULL ? &driver->last_error : vitte_error_last();
}

const vitte_diagnostic_bag_t *vitte_driver_diagnostics(const vitte_driver_t *driver) {
    return driver != NULL ? &driver->diagnostics : NULL;
}

static bool vitte_driver_input_is_valid(const vitte_driver_input_t *input) {
    return input != NULL &&
        (input->kind == VITTE_DRIVER_INPUT_FILE || input->kind == VITTE_DRIVER_INPUT_BUFFER) &&
        input->buffer != NULL;
}

static bool vitte_driver_source_is_blank(const char *source, size_t size) {
    size_t index;

    if (source == NULL || size == 0u) {
        return true;
    }
    for (index = 0u; index < size; index++) {
        if (!isspace((unsigned char)source[index])) {
            return false;
        }
    }
    return true;
}

static vitte_status_t vitte_driver_configure_import_resolver(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input,
    vitte_import_resolver_t *resolver
) {
    vitte_import_options_t options;
    vitte_fs_path_t parent_path;
    vitte_status_t status;

    if (driver == NULL || input == NULL || resolver == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_import_options_init(&options);
    options.read_source = false;
    options.use_cache = true;
    options.max_depth = driver->config.limits.max_include_depth;
    options.max_source_bytes = driver->config.limits.max_source_bytes;

    status = vitte_import_resolver_init(resolver, &options);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    if (driver->config.paths.root_path != NULL && driver->config.paths.root_path[0] != '\0') {
        status = vitte_import_resolver_add_search_path(resolver, driver->config.paths.root_path);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    if (driver->config.paths.sysroot_path != NULL && driver->config.paths.sysroot_path[0] != '\0') {
        status = vitte_import_resolver_add_search_path(resolver, driver->config.paths.sysroot_path);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    if (input->path != NULL && input->path[0] != '\0' &&
        vitte_fs_parent_path(input->path, &parent_path) == VITTE_STATUS_OK) {
        status = vitte_import_resolver_add_search_path(resolver, parent_path.text);
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_driver_parse_ast(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input,
    vitte_ast_t *ast,
    vitte_module_t *module
) {
    vitte_parser_t parser;
    vitte_parser_options_t parser_options;
    vitte_parser_result_t parser_result;
    vitte_status_t status;

    if (driver == NULL || input == NULL || ast == NULL || module == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_parser_options_init(&parser_options);
    parser_options.max_depth = driver->config.limits.max_ast_depth;
    parser_options.recover_errors = true;
    vitte_parser_result_init(&parser_result);
    status = vitte_module_init(module, NULL);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    if (input->path != NULL && input->path[0] != '\0') {
        status = vitte_module_set_source_path(module, input->path);
        if (status != VITTE_STATUS_OK) {
            vitte_error_copy(&driver->last_error, vitte_module_last_error(module));
            return status;
        }
    }
    status = vitte_module_attach_source(
        module,
        input->source_name != NULL ? input->source_name : "<memory>",
        (char *)input->buffer,
        input->size,
        false
    );
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&driver->last_error, vitte_module_last_error(module));
        return status;
    }
    status = vitte_parser_init_module(&parser, module, ast, &parser_options, &driver->diagnostics);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&driver->last_error, vitte_parser_last_error(&parser));
        return status;
    }

    status = vitte_parser_parse_module(&parser, &parser_result);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&driver->last_error, &parser_result.last_error);
        vitte_parser_destroy(&parser);
        return status;
    }
    vitte_parser_destroy(&parser);
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_driver_validate_input(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input
) {
    if (!vitte_driver_input_is_valid(input)) {
        vitte_driver_add_diag(driver, VITTE_DIAGNOSTIC_FATAL, "VITTE_DRIVER_E_INPUT", "missing source input", NULL);
        vitte_driver_set_error(driver, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_DRIVER_E_INPUT", "missing source input", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (driver->config.limits.max_source_bytes != 0u && input->size > driver->config.limits.max_source_bytes) {
        vitte_driver_add_diag(driver, VITTE_DIAGNOSTIC_FATAL, "VITTE_DRIVER_E_INPUT", "source exceeds configured maximum size", input->source_name);
        vitte_driver_set_error(driver, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_DRIVER_E_INPUT", "source exceeds configured maximum size", input->source_name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (vitte_driver_source_is_blank(input->buffer, input->size)) {
        vitte_driver_add_diag(driver, VITTE_DIAGNOSTIC_FATAL, "VITTE_DRIVER_E_EMPTY_SOURCE", "source file is empty", input->source_name);
        vitte_driver_set_error(driver, VITTE_STATUS_ERROR_PARSE, "VITTE_DRIVER_E_EMPTY_SOURCE", "source file is empty", input->source_name);
        return VITTE_STATUS_ERROR_PARSE;
    }
    return VITTE_STATUS_OK;
}

static void vitte_driver_update_counts(vitte_driver_t *driver, vitte_driver_result_t *result) {
    const vitte_diagnostic_counts_t *counts;

    if (driver == NULL || result == NULL) {
        return;
    }
    counts = vitte_diagnostic_bag_counts(&driver->diagnostics);
    if (counts != NULL) {
        result->warning_count = counts->warning_count;
        result->error_count = counts->error_count + counts->fatal_count;
        result->diagnostic_count = vitte_diagnostic_bag_total_count(&driver->diagnostics);
    }
    result->stages_completed = driver->pipeline.completed_count;
}

static vitte_status_t vitte_driver_prepare_config(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input,
    vitte_driver_emit_kind_t kind,
    const char *output_path
) {
    if (driver == NULL || input == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    driver->config.paths.input_path = input->path != NULL ? input->path : input->source_name;
    driver->config.paths.output_path = output_path;
    driver->config.build_mode = vitte_driver_config_mode(kind);
    return vitte_config_validate(&driver->config);
}

static vitte_status_t vitte_driver_emit_c_impl(
    vitte_driver_t *driver,
    vitte_codegen_input_kind_t input_kind,
    const void *input,
    const char *output_path,
    vitte_driver_result_t *result
) {
    vitte_codegen_options_t options;
    vitte_codegen_t codegen;
    vitte_codegen_result_t codegen_result;
    vitte_status_t status;

    vitte_config_to_codegen_options(
        &driver->config,
        &options,
        result != NULL ? result->output_buffer : NULL,
        result != NULL ? sizeof(result->output_buffer) : 0u
    );
    if (output_path != NULL) {
        options.output_kind = VITTE_CODEGEN_OUTPUT_FILE;
        options.output_path = output_path;
    } else {
        options.output_kind = VITTE_CODEGEN_OUTPUT_BUFFER;
    }
    options.input_kind = input_kind;

    status = vitte_codegen_init(&codegen, &options);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_add_diag(driver, VITTE_DIAGNOSTIC_FATAL, "VITTE_DRIVER_E_CODEGEN", "failed to initialize codegen", NULL);
        vitte_error_copy(&driver->last_error, vitte_codegen_last_error(&codegen));
        return status;
    }

    status = vitte_codegen_emit(&codegen, input, &codegen_result);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_add_diag(driver, VITTE_DIAGNOSTIC_FATAL, "VITTE_DRIVER_E_CODEGEN", "C17 emission failed", NULL);
        vitte_error_copy(&driver->last_error, vitte_codegen_last_error(&codegen));
        vitte_codegen_destroy(&codegen);
        return status;
    }

    if (result != NULL) {
        result->output.kind = VITTE_DRIVER_EMIT_C;
        result->output.path = output_path;
        result->output.c_path = output_path;
        result->output.bytes_written = codegen_result.bytes_written;
        result->output.lines_written = codegen_result.lines_written;
        result->output.functions_emitted = codegen_result.functions_emitted;
        if (output_path == NULL) {
            result->output.buffer = result->output_buffer;
        }
    }
    vitte_codegen_destroy(&codegen);
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_driver_parse_imported_ast(
    vitte_driver_t *driver,
    const char *path,
    vitte_ast_t *ast
) {
    vitte_driver_input_t input;
    vitte_module_t module;
    vitte_status_t status;

    if (driver == NULL || path == NULL || ast == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_driver_input_init(&input);
    status = vitte_driver_input_from_file(&input, path, driver->config.limits.max_source_bytes);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_set_error(driver, status, "VITTE_DRIVER_E_IMPORT", "failed to read imported module", path);
        return status;
    }
    status = vitte_ast_init_owned(ast, NULL);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_input_destroy(&input);
        return status;
    }
    status = vitte_driver_parse_ast(driver, &input, ast, &module);
    if (status == VITTE_STATUS_OK) {
        status = vitte_ast_validate(ast);
        if (status != VITTE_STATUS_OK) {
            vitte_error_copy(&driver->last_error, vitte_ast_last_error(ast));
        }
    }
    if (vitte_module_is_initialized(&module)) {
        vitte_module_destroy(&module);
    }
    vitte_driver_input_destroy(&input);
    if (status != VITTE_STATUS_OK) {
        vitte_ast_destroy(ast);
    }
    return status;
}

static vitte_status_t vitte_driver_run_sema(
    vitte_driver_t *driver,
    const vitte_ast_t *ast,
    const vitte_module_t *module,
    const vitte_ast_t *imported_asts,
    size_t imported_ast_count
) {
    vitte_sema_t sema;
    vitte_sema_options_t options;
    vitte_sema_result_t result;
    vitte_status_t status;
    size_t index;

    if (driver == NULL || ast == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_sema_options_init(&options);
    options.max_depth = driver->config.limits.max_ast_depth;
    options.enable_constant_folding = true;
    vitte_sema_result_init(&result);
    status = vitte_sema_init(&sema, &options, &driver->diagnostics);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&driver->last_error, vitte_sema_last_error(&sema));
        return status;
    }
    if (module != NULL) {
        for (index = 0u; index < imported_ast_count && index < module->import_count; index++) {
            status = vitte_sema_add_import_module(&sema, module->imports[index].module_name, &imported_asts[index]);
            if (status != VITTE_STATUS_OK) {
                vitte_error_copy(&driver->last_error, vitte_sema_last_error(&sema));
                vitte_sema_destroy(&sema);
                return status;
            }
        }
    }

    status = vitte_sema_analyze(&sema, ast, &result);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&driver->last_error, &result.last_error);
    } else {
        vitte_error_reset(&driver->last_error);
    }
    vitte_sema_destroy(&sema);
    return status;
}

static vitte_status_t vitte_driver_run_import_unit_sema(
    vitte_driver_t *driver,
    const vitte_driver_import_unit_t *unit,
    vitte_driver_import_unit_t *const *imported_units,
    size_t imported_unit_count
) {
    vitte_sema_t sema;
    vitte_sema_options_t options;
    vitte_sema_result_t result;
    vitte_status_t status;
    size_t index;

    if (driver == NULL || unit == NULL || imported_units == NULL ||
        !vitte_ast_is_initialized(&unit->ast) || unit->ast.root == NULL ||
        !vitte_module_is_initialized(&unit->module)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_sema_options_init(&options);
    options.max_depth = driver->config.limits.max_ast_depth;
    options.enable_constant_folding = true;
    vitte_sema_result_init(&result);
    status = vitte_sema_init(&sema, &options, &driver->diagnostics);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&driver->last_error, vitte_sema_last_error(&sema));
        return status;
    }

    for (index = 0u; index < unit->module.import_count; index++) {
        const vitte_module_import_t *dependency = &unit->module.imports[index];
        const vitte_driver_import_unit_t *dependency_unit;

        if (!dependency->resolved || dependency->resolved_path[0] == '\0') {
            continue;
        }
        dependency_unit = vitte_driver_find_import_unit_by_path(
            imported_units,
            imported_unit_count,
            dependency->resolved_path
        );
        if (dependency_unit == NULL || !vitte_ast_is_initialized(&dependency_unit->ast) ||
            dependency_unit->ast.root == NULL) {
            vitte_driver_set_error(
                driver,
                VITTE_STATUS_ERROR_INVALID_STATE,
                "VITTE_DRIVER_E_IMPORT",
                "missing imported dependency AST for semantic analysis",
                dependency->resolved_path
            );
            vitte_sema_destroy(&sema);
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
        status = vitte_sema_add_import_module(&sema, dependency->module_name, &dependency_unit->ast);
        if (status != VITTE_STATUS_OK) {
            vitte_error_copy(&driver->last_error, vitte_sema_last_error(&sema));
            vitte_sema_destroy(&sema);
            return status;
        }
    }

    status = vitte_sema_analyze(&sema, &unit->ast, &result);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&driver->last_error, &result.last_error);
    } else {
        vitte_error_reset(&driver->last_error);
    }
    vitte_sema_destroy(&sema);
    return status;
}

static vitte_status_t vitte_driver_run_import_graph_sema(
    vitte_driver_t *driver,
    vitte_driver_import_unit_t *const *imported_units,
    size_t imported_unit_count
) {
    size_t index;

    if (driver == NULL || imported_units == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    for (index = 0u; index < imported_unit_count; index++) {
        if (imported_units[index] == NULL) {
            continue;
        }
        if (vitte_driver_run_import_unit_sema(
                driver,
                imported_units[index],
                imported_units,
                imported_unit_count
            ) != VITTE_STATUS_OK) {
            return vitte_driver_last_error(driver)->status;
        }
    }
    return VITTE_STATUS_OK;
}

static const char *vitte_driver_last_path_segment(const char *text) {
    const char *last_dot;

    if (text == NULL) {
        return NULL;
    }
    last_dot = strrchr(text, '.');
    return last_dot != NULL ? last_dot + 1 : text;
}

static bool vitte_driver_copy_text(char *buffer, size_t capacity, const char *text) {
    size_t length;

    if (buffer == NULL || capacity == 0u || text == NULL) {
        return false;
    }
    length = strlen(text);
    if (length + 1u > capacity) {
        return false;
    }
    (void)memcpy(buffer, text, length);
    buffer[length] = '\0';
    return true;
}

static char *vitte_driver_ast_join_name(
    vitte_ast_t *ast,
    const char *left,
    const char *separator,
    const char *right
) {
    size_t left_length;
    size_t separator_length;
    size_t right_length;
    char *joined;

    if (ast == NULL || left == NULL || separator == NULL || right == NULL) {
        return NULL;
    }
    left_length = strlen(left);
    separator_length = strlen(separator);
    right_length = strlen(right);
    joined = (char *)vitte_arena_alloc(ast->arena, left_length + separator_length + right_length + 1u, 1u);
    if (joined == NULL) {
        vitte_error_set_details(&ast->last_error, VITTE_STATUS_ERROR_OUT_OF_MEMORY, "VITTE_DRIVER_E_OOM", "failed to allocate joined import name", right);
        return NULL;
    }
    (void)memcpy(joined, left, left_length);
    (void)memcpy(joined + left_length, separator, separator_length);
    (void)memcpy(joined + left_length + separator_length, right, right_length);
    joined[left_length + separator_length + right_length] = '\0';
    return joined;
}

static const vitte_ast_decl_t *vitte_driver_find_module_decl(
    const vitte_ast_module_t *module_root,
    const char *name
) {
    const vitte_ast_node_t *decl;

    if (module_root == NULL || module_root->kind != VITTE_AST_NODE_MODULE || name == NULL) {
        return NULL;
    }
    for (decl = module_root->as.module.declarations.first; decl != NULL; decl = decl->next) {
        if (decl->kind == VITTE_AST_NODE_PROC_DECL &&
            decl->as.proc_decl.name != NULL &&
            strcmp(decl->as.proc_decl.name, name) == 0) {
            return decl;
        }
        if (decl->kind == VITTE_AST_NODE_CONST_DECL &&
            decl->as.const_decl.name != NULL &&
            strcmp(decl->as.const_decl.name, name) == 0) {
            return decl;
        }
    }
    return NULL;
}

static bool vitte_driver_decl_is_exported(const vitte_ast_decl_t *decl) {
    if (decl == NULL) {
        return false;
    }
    if (decl->kind == VITTE_AST_NODE_PROC_DECL) {
        return decl->as.proc_decl.exported;
    }
    if (decl->kind == VITTE_AST_NODE_CONST_DECL) {
        return decl->as.const_decl.exported;
    }
    return false;
}

static vitte_ast_decl_t *vitte_driver_clone_decl_shallow(
    vitte_ast_t *ast,
    const vitte_ast_decl_t *source_decl,
    const char *name
) {
    vitte_ast_decl_t *decl;

    if (ast == NULL || source_decl == NULL || name == NULL) {
        return NULL;
    }
    decl = vitte_ast_alloc_node(ast, source_decl->kind, source_decl->span);
    if (decl == NULL) {
        return NULL;
    }
    if (source_decl->kind == VITTE_AST_NODE_PROC_DECL) {
        decl->as.proc_decl.name = name;
        decl->as.proc_decl.parameters = source_decl->as.proc_decl.parameters;
        decl->as.proc_decl.return_type = source_decl->as.proc_decl.return_type;
        decl->as.proc_decl.body = source_decl->as.proc_decl.body;
        return decl;
    }
    if (source_decl->kind == VITTE_AST_NODE_CONST_DECL) {
        decl->as.const_decl.name = name;
        decl->as.const_decl.type = source_decl->as.const_decl.type;
        decl->as.const_decl.value = source_decl->as.const_decl.value;
        return decl;
    }
    vitte_error_set_details(&ast->last_error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_DRIVER_E_IMPORT", "unsupported imported declaration kind", vitte_ast_node_kind_name(source_decl->kind));
    return NULL;
}

static bool vitte_driver_decl_matches_source(
    const vitte_ast_decl_t *existing_decl,
    const vitte_ast_decl_t *source_decl
) {
    if (existing_decl == NULL || source_decl == NULL || existing_decl->kind != source_decl->kind) {
        return false;
    }
    if (existing_decl->kind == VITTE_AST_NODE_PROC_DECL) {
        return existing_decl->as.proc_decl.parameters.first == source_decl->as.proc_decl.parameters.first &&
            existing_decl->as.proc_decl.parameters.last == source_decl->as.proc_decl.parameters.last &&
            existing_decl->as.proc_decl.parameters.count == source_decl->as.proc_decl.parameters.count &&
            existing_decl->as.proc_decl.return_type == source_decl->as.proc_decl.return_type &&
            existing_decl->as.proc_decl.body == source_decl->as.proc_decl.body;
    }
    if (existing_decl->kind == VITTE_AST_NODE_CONST_DECL) {
        return existing_decl->as.const_decl.type == source_decl->as.const_decl.type &&
            existing_decl->as.const_decl.value == source_decl->as.const_decl.value;
    }
    return false;
}

static const vitte_driver_flatten_binding_t *vitte_driver_flatten_find_binding(
    const vitte_driver_flatten_state_t *state,
    const char *visible_name
) {
    const vitte_driver_flatten_binding_t *binding;

    if (state == NULL || visible_name == NULL) {
        return NULL;
    }
    for (binding = state->bindings; binding != NULL; binding = binding->next) {
        if (binding->visible_name != NULL && strcmp(binding->visible_name, visible_name) == 0) {
            return binding;
        }
    }
    return NULL;
}

static vitte_status_t vitte_driver_flatten_register_binding(
    vitte_ast_t *ast,
    vitte_driver_flatten_state_t *state,
    const char *visible_name,
    const char *owner_module_name,
    const vitte_ast_decl_t *source_decl
) {
    vitte_driver_flatten_binding_t *binding;

    if (ast == NULL || state == NULL || visible_name == NULL || owner_module_name == NULL || source_decl == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    binding = (vitte_driver_flatten_binding_t *)vitte_arena_alloc_zeroed(
        ast->arena,
        sizeof(*binding),
        _Alignof(vitte_driver_flatten_binding_t)
    );
    if (binding == NULL) {
        vitte_error_set_details(
            &ast->last_error,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_DRIVER_E_OOM",
            "failed to allocate backend flatten binding metadata",
            visible_name
        );
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }
    binding->visible_name = visible_name;
    binding->owner_module_name = owner_module_name;
    binding->source_decl = source_decl;
    binding->next = state->bindings;
    state->bindings = binding;
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_driver_append_import_decl(
    vitte_driver_t *driver,
    vitte_ast_t *ast,
    vitte_ast_module_t *module_root,
    vitte_driver_flatten_state_t *state,
    const vitte_ast_decl_t *source_decl,
    const char *name,
    const char *owner_module_name
) {
    vitte_ast_decl_t *clone;
    const vitte_ast_decl_t *existing_decl;
    const vitte_driver_flatten_binding_t *binding;
    vitte_status_t status;

    if (driver == NULL || ast == NULL || module_root == NULL || state == NULL || source_decl == NULL ||
        name == NULL || owner_module_name == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    binding = vitte_driver_flatten_find_binding(state, name);
    if (binding != NULL) {
        if (binding->source_decl == source_decl ||
            vitte_driver_decl_matches_source(binding->source_decl, source_decl)) {
            return VITTE_STATUS_OK;
        }
        vitte_error_set_details(
            &ast->last_error,
            VITTE_STATUS_ERROR_INVALID_STATE,
            "VITTE_DRIVER_E_IMPORT_COLLISION_IMPORTED",
            "imported declarations from different modules collide during backend lowering",
            name
        );
        vitte_driver_set_error(
            driver,
            VITTE_STATUS_ERROR_INVALID_STATE,
            "VITTE_DRIVER_E_IMPORT_COLLISION_IMPORTED",
            "imported declarations from different modules collide during backend lowering",
            name
        );
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    existing_decl = vitte_driver_find_module_decl(module_root, name);
    if (existing_decl != NULL) {
        vitte_error_set_details(
            &ast->last_error,
            VITTE_STATUS_ERROR_INVALID_STATE,
            "VITTE_DRIVER_E_IMPORT_COLLISION_LOCAL",
            "local declaration conflicts with imported declaration during backend lowering",
            name
        );
        vitte_driver_set_error(
            driver,
            VITTE_STATUS_ERROR_INVALID_STATE,
            "VITTE_DRIVER_E_IMPORT_COLLISION_LOCAL",
            "local declaration conflicts with imported declaration during backend lowering",
            name
        );
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    clone = vitte_driver_clone_decl_shallow(ast, source_decl, name);
    if (clone == NULL) {
        return vitte_ast_last_error(ast)->status;
    }
    if (!vitte_ast_module_add_decl(module_root, clone)) {
        vitte_error_set_details(&ast->last_error, VITTE_STATUS_ERROR_INTERNAL, "VITTE_DRIVER_E_IMPORT", "failed to append imported declaration", name);
        return VITTE_STATUS_ERROR_INTERNAL;
    }
    status = vitte_driver_flatten_register_binding(ast, state, name, owner_module_name, source_decl);
    if (status != VITTE_STATUS_OK) {
        return status;
    }
    return VITTE_STATUS_OK;
}

static bool vitte_driver_import_dependency_name(
    const vitte_ast_decl_t *import_decl,
    char *buffer,
    size_t buffer_capacity
) {
    const char *path;
    size_t length;

    if (import_decl == NULL || import_decl->kind != VITTE_AST_NODE_IMPORT_DECL ||
        buffer == NULL || buffer_capacity == 0u) {
        return false;
    }
    path = import_decl->as.import_decl.path;
    if (path == NULL || path[0] == '\0') {
        return false;
    }
    length = strlen(path);
    if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_SYMBOL) {
        const char *last_dot = strrchr(path, '.');
        if (last_dot != NULL) {
            length = (size_t)(last_dot - path);
        }
    }
    if (length + 1u > buffer_capacity) {
        return false;
    }
    (void)memcpy(buffer, path, length);
    buffer[length] = '\0';
    return true;
}

static const vitte_module_import_t *vitte_driver_find_module_import(
    const vitte_module_t *module,
    const char *module_name
) {
    size_t index;

    if (module == NULL || module_name == NULL) {
        return NULL;
    }
    for (index = 0u; index < module->import_count; index++) {
        if (strcmp(module->imports[index].module_name, module_name) == 0) {
            return &module->imports[index];
        }
    }
    return NULL;
}

static void vitte_driver_destroy_import_unit(vitte_driver_import_unit_t *unit) {
    if (unit == NULL) {
        return;
    }
    if (vitte_module_is_initialized(&unit->module)) {
        vitte_module_destroy(&unit->module);
    }
    if (vitte_ast_is_initialized(&unit->ast)) {
        vitte_ast_destroy(&unit->ast);
    }
    free(unit);
}

static void vitte_driver_destroy_import_units(
    vitte_driver_import_unit_t **units,
    size_t *count
) {
    size_t index;

    if (units == NULL || count == NULL) {
        return;
    }
    for (index = 0u; index < *count; index++) {
        vitte_driver_destroy_import_unit(units[index]);
        units[index] = NULL;
    }
    *count = 0u;
}

static const vitte_driver_import_unit_t *vitte_driver_find_import_unit_by_path(
    vitte_driver_import_unit_t *const *units,
    size_t unit_count,
    const char *resolved_path
) {
    size_t index;

    if (units == NULL || resolved_path == NULL || resolved_path[0] == '\0') {
        return NULL;
    }
    for (index = 0u; index < unit_count; index++) {
        if (units[index] != NULL &&
            units[index]->resolved_path[0] != '\0' &&
            strcmp(units[index]->resolved_path, resolved_path) == 0) {
            return units[index];
        }
    }
    return NULL;
}

static vitte_status_t vitte_driver_parse_imported_unit(
    vitte_driver_t *driver,
    const char *module_name,
    const char *path,
    vitte_driver_import_unit_t *unit
) {
    vitte_driver_input_t input;
    vitte_status_t status;

    if (driver == NULL || module_name == NULL || path == NULL || unit == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_driver_copy_text(unit->module_name, sizeof(unit->module_name), module_name) ||
        !vitte_driver_copy_text(unit->resolved_path, sizeof(unit->resolved_path), path)) {
        vitte_driver_set_error(driver, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_DRIVER_E_IMPORT", "import metadata is too long", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_driver_input_init(&input);
    status = vitte_driver_input_from_file(&input, path, driver->config.limits.max_source_bytes);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_set_error(driver, status, "VITTE_DRIVER_E_IMPORT", "failed to read imported module", path);
        return status;
    }
    status = vitte_ast_init_owned(&unit->ast, NULL);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_input_destroy(&input);
        return status;
    }
    status = vitte_driver_parse_ast(driver, &input, &unit->ast, &unit->module);
    if (status == VITTE_STATUS_OK) {
        status = vitte_ast_validate(&unit->ast);
        if (status != VITTE_STATUS_OK) {
            vitte_error_copy(&driver->last_error, vitte_ast_last_error(&unit->ast));
        }
    }
    vitte_driver_input_destroy(&input);
    return status;
}

static vitte_status_t vitte_driver_collect_import_unit(
    vitte_driver_t *driver,
    vitte_import_resolver_t *resolver,
    const char *module_name,
    const char *resolved_path,
    vitte_driver_import_unit_t **units,
    size_t *unit_count
) {
    vitte_driver_import_unit_t *unit;
    vitte_status_t status;
    size_t index;

    if (driver == NULL || resolver == NULL || module_name == NULL || resolved_path == NULL ||
        units == NULL || unit_count == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (vitte_driver_find_import_unit_by_path(units, *unit_count, resolved_path) != NULL) {
        return VITTE_STATUS_OK;
    }
    if (*unit_count >= VITTE_DRIVER_MAX_IMPORTED_UNITS) {
        vitte_driver_set_error(
            driver,
            VITTE_STATUS_ERROR_INVALID_STATE,
            "VITTE_DRIVER_E_IMPORT",
            "transitive import set exceeds bootstrap backend limit",
            resolved_path
        );
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    unit = (vitte_driver_import_unit_t *)calloc(1u, sizeof(*unit));
    if (unit == NULL) {
        vitte_driver_set_error(driver, VITTE_STATUS_ERROR_OUT_OF_MEMORY, "VITTE_DRIVER_E_IMPORT", "failed to allocate transitive import unit", resolved_path);
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }
    units[*unit_count] = unit;
    (*unit_count)++;

    status = vitte_driver_parse_imported_unit(driver, module_name, resolved_path, unit);
    if (status != VITTE_STATUS_OK) {
        units[*unit_count - 1u] = NULL;
        (*unit_count)--;
        vitte_driver_destroy_import_unit(unit);
        return status;
    }
    if (unit->module.import_count > 0u) {
        status = vitte_module_resolve_imports(&unit->module, resolver);
        if (status != VITTE_STATUS_OK) {
            vitte_error_copy(&driver->last_error, vitte_module_last_error(&unit->module));
            units[*unit_count - 1u] = NULL;
            (*unit_count)--;
            vitte_driver_destroy_import_unit(unit);
            return status;
        }
    }

    for (index = 0u; index < unit->module.import_count; index++) {
        const vitte_module_import_t *entry = &unit->module.imports[index];

        if (!entry->resolved || entry->resolved_path[0] == '\0') {
            continue;
        }
        status = vitte_driver_collect_import_unit(
            driver,
            resolver,
            entry->module_name,
            entry->resolved_path,
            units,
            unit_count
        );
        if (status != VITTE_STATUS_OK) {
            return status;
        }
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_driver_flatten_module_imports(
    vitte_driver_t *driver,
    vitte_ast_t *ast,
    vitte_ast_module_t *module_root,
    vitte_driver_flatten_state_t *state,
    const vitte_ast_module_t *source_root,
    const vitte_module_t *source_module,
    vitte_driver_import_unit_t *const *imported_units,
    size_t imported_unit_count
) {
    const vitte_ast_node_t *import_decl;

    if (driver == NULL || ast == NULL || module_root == NULL || state == NULL || source_root == NULL ||
        source_module == NULL || imported_units == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    for (import_decl = source_root->as.module.imports.first; import_decl != NULL; import_decl = import_decl->next) {
        char dependency_name[VITTE_IMPORT_MAX_MODULE_NAME];
        const vitte_module_import_t *dependency;
        const vitte_driver_import_unit_t *imported_unit;
        const vitte_ast_module_t *imported_root;

        if (!vitte_driver_import_dependency_name(import_decl, dependency_name, sizeof(dependency_name))) {
            vitte_driver_set_error(driver, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_DRIVER_E_IMPORT", "invalid import dependency name", import_decl->as.import_decl.path);
            return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
        }
        dependency = vitte_driver_find_module_import(source_module, dependency_name);
        if (dependency == NULL || !dependency->resolved || dependency->resolved_path[0] == '\0') {
            vitte_driver_set_error(driver, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_DRIVER_E_IMPORT", "missing resolved module metadata for lowering", dependency_name);
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
        imported_unit = vitte_driver_find_import_unit_by_path(imported_units, imported_unit_count, dependency->resolved_path);
        if (imported_unit == NULL || !vitte_ast_is_initialized(&imported_unit->ast) || imported_unit->ast.root == NULL) {
            vitte_driver_set_error(driver, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_DRIVER_E_IMPORT", "missing imported AST for lowering", dependency->resolved_path);
            return VITTE_STATUS_ERROR_INVALID_STATE;
        }
        imported_root = imported_unit->ast.root;

        if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_MODULE) {
            const char *prefix = import_decl->as.import_decl.alias != NULL ?
                import_decl->as.import_decl.alias :
                vitte_driver_last_path_segment(dependency_name);
            const vitte_ast_node_t *decl;

            for (decl = imported_root->as.module.declarations.first; decl != NULL; decl = decl->next) {
                const char *decl_name = decl->kind == VITTE_AST_NODE_PROC_DECL ?
                    decl->as.proc_decl.name :
                    decl->kind == VITTE_AST_NODE_CONST_DECL ? decl->as.const_decl.name : NULL;
                char *visible_name;
                vitte_status_t status;

                if (decl_name == NULL || !vitte_driver_decl_is_exported(decl)) {
                    continue;
                }
                visible_name = vitte_driver_ast_join_name(ast, prefix, "::", decl_name);
                if (visible_name == NULL) {
                    vitte_driver_set_error(driver, vitte_ast_last_error(ast)->status, "VITTE_DRIVER_E_IMPORT", "failed to allocate qualified import name", decl_name);
                    return vitte_ast_last_error(ast)->status;
                }
                status = vitte_driver_append_import_decl(
                    driver,
                    ast,
                    module_root,
                    state,
                    decl,
                    visible_name,
                    imported_unit->module_name
                );
                if (status != VITTE_STATUS_OK) {
                    const vitte_error_t *error = vitte_driver_last_error(driver);
                    vitte_driver_set_error(
                        driver,
                        status,
                        error != NULL && error->code != NULL ? error->code : "VITTE_DRIVER_E_IMPORT",
                        error != NULL && error->message != NULL ? error->message : "failed to append qualified imported declaration",
                        error != NULL ? error->details : visible_name
                    );
                    return status;
                }
            }
        } else if (import_decl->as.import_decl.import_kind == VITTE_AST_IMPORT_SYMBOL) {
            const char *leaf_name = vitte_driver_last_path_segment(import_decl->as.import_decl.path);
            const char *visible_name = import_decl->as.import_decl.alias != NULL ?
                import_decl->as.import_decl.alias :
                leaf_name;
            const vitte_ast_decl_t *decl = vitte_driver_find_module_decl(imported_root, leaf_name);
            vitte_status_t status;

            if (decl == NULL) {
                vitte_driver_set_error(driver, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_DRIVER_E_IMPORT", "missing imported symbol declaration", import_decl->as.import_decl.path);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            if (!vitte_driver_decl_is_exported(decl)) {
                vitte_driver_set_error(driver, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_DRIVER_E_IMPORT_PRIVATE", "imported symbol is not exported by module", import_decl->as.import_decl.path);
                return VITTE_STATUS_ERROR_INVALID_STATE;
            }
            status = vitte_driver_append_import_decl(
                driver,
                ast,
                module_root,
                state,
                decl,
                visible_name,
                imported_unit->module_name
            );
            if (status != VITTE_STATUS_OK) {
                const vitte_error_t *error = vitte_driver_last_error(driver);
                vitte_driver_set_error(
                    driver,
                    status,
                    error != NULL && error->code != NULL ? error->code : "VITTE_DRIVER_E_IMPORT",
                    error != NULL && error->message != NULL ? error->message : "failed to append imported symbol declaration",
                    error != NULL ? error->details : visible_name
                );
                return status;
            }
        }
    }

    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_driver_flatten_imported_modules(
    vitte_driver_t *driver,
    vitte_ast_t *ast,
    const vitte_module_t *module,
    vitte_driver_import_unit_t *const *imported_units,
    size_t imported_unit_count
) {
    vitte_ast_module_t *module_root;
    vitte_driver_flatten_state_t state;
    size_t index;

    if (driver == NULL || ast == NULL || module == NULL || imported_units == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    module_root = ast->root;
    if (module_root == NULL || module_root->kind != VITTE_AST_NODE_MODULE) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    memset(&state, 0, sizeof(state));

    for (index = 0u; index < imported_unit_count; index++) {
        const vitte_driver_import_unit_t *unit = imported_units[index];
        const vitte_ast_module_t *imported_root = unit != NULL && vitte_ast_is_initialized(&unit->ast) ? unit->ast.root : NULL;
        const vitte_ast_node_t *decl;

        if (imported_root == NULL) {
            continue;
        }
        for (decl = imported_root->as.module.declarations.first; decl != NULL; decl = decl->next) {
            const char *decl_name = decl->kind == VITTE_AST_NODE_PROC_DECL ?
                decl->as.proc_decl.name :
                decl->kind == VITTE_AST_NODE_CONST_DECL ? decl->as.const_decl.name : NULL;
            vitte_status_t status;

            if (decl_name == NULL || !vitte_driver_decl_is_exported(decl)) {
                continue;
            }
            status = vitte_driver_append_import_decl(
                driver,
                ast,
                module_root,
                &state,
                decl,
                decl_name,
                unit->module_name
            );
            if (status != VITTE_STATUS_OK) {
                const vitte_error_t *error = vitte_driver_last_error(driver);
                vitte_driver_set_error(
                    driver,
                    status,
                    error != NULL && error->code != NULL ? error->code : "VITTE_DRIVER_E_IMPORT",
                    error != NULL && error->message != NULL ? error->message : "failed to flatten imported declaration",
                    error != NULL ? error->details : decl_name
                );
                return status;
            }
        }
    }

    if (vitte_driver_flatten_module_imports(
            driver,
            ast,
            module_root,
            &state,
            module_root,
            module,
            imported_units,
            imported_unit_count
        ) != VITTE_STATUS_OK) {
        return vitte_driver_last_error(driver)->status;
    }

    for (index = 0u; index < imported_unit_count; index++) {
        const vitte_driver_import_unit_t *unit = imported_units[index];

        if (unit == NULL || !vitte_ast_is_initialized(&unit->ast) || unit->ast.root == NULL) {
            continue;
        }
        if (vitte_driver_flatten_module_imports(
                driver,
                ast,
                module_root,
                &state,
                unit->ast.root,
                &unit->module,
                imported_units,
                imported_unit_count
            ) != VITTE_STATUS_OK) {
            return vitte_driver_last_error(driver)->status;
        }
    }

    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_driver_run_backend(
    vitte_driver_t *driver,
    const vitte_ast_t *ast,
    vitte_hir_t *hir,
    vitte_ir_t *ir
) {
    vitte_status_t status;

    if (driver == NULL || ast == NULL || hir == NULL || ir == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_hir_init_owned(hir, NULL);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&driver->last_error, vitte_hir_last_error(hir));
        return status;
    }

    status = vitte_hir_lower_ast(hir, ast);
    if (status == VITTE_STATUS_OK) {
        status = vitte_hir_validate(hir);
    }
    if (status == VITTE_STATUS_OK) {
        status = vitte_ir_init_owned(ir, NULL);
        if (status != VITTE_STATUS_OK) {
            vitte_error_copy(&driver->last_error, vitte_ir_last_error(ir));
            vitte_hir_destroy(hir);
            return status;
        }
        status = vitte_ir_lower_hir(ir, hir);
        if (status == VITTE_STATUS_OK) {
            status = vitte_ir_validate(ir);
        }
        if (status != VITTE_STATUS_OK) {
            vitte_error_copy(&driver->last_error, vitte_ir_last_error(ir));
        }
    }
    if (status != VITTE_STATUS_OK) {
        if (vitte_error_is_ok(&driver->last_error)) {
            vitte_error_copy(&driver->last_error, vitte_hir_last_error(hir));
        }
        if (vitte_ir_is_initialized(ir)) {
            vitte_ir_destroy(ir);
        }
        if (vitte_hir_is_initialized(hir)) {
            vitte_hir_destroy(hir);
        }
    } else {
        vitte_error_reset(&driver->last_error);
    }

    return status;
}

static bool vitte_driver_append_text(char *buffer, size_t capacity, const char *text) {
    size_t used;
    size_t length;

    if (buffer == NULL || text == NULL) {
        return false;
    }
    used = strlen(buffer);
    length = strlen(text);
    if (used + length >= capacity) {
        return false;
    }
    (void)memcpy(buffer + used, text, length + 1u);
    return true;
}

static bool vitte_driver_shell_quote(char *buffer, size_t capacity, const char *value) {
    const char *cursor;

    if (!vitte_driver_append_text(buffer, capacity, "'")) {
        return false;
    }
    if (value != NULL) {
        for (cursor = value; *cursor != '\0'; cursor++) {
            char byte[2];
            if (*cursor == '\'') {
                if (!vitte_driver_append_text(buffer, capacity, "'\\''")) {
                    return false;
                }
            } else {
                byte[0] = *cursor;
                byte[1] = '\0';
                if (!vitte_driver_append_text(buffer, capacity, byte)) {
                    return false;
                }
            }
        }
    }
    return vitte_driver_append_text(buffer, capacity, "'");
}

static vitte_status_t vitte_driver_make_c_path(
    const char *output_path,
    vitte_driver_result_t *result
) {
    if (result == NULL || output_path == NULL || output_path[0] == '\0') {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (strlen(output_path) + 3u >= sizeof(result->generated_c_path)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    (void)snprintf(result->generated_c_path, sizeof(result->generated_c_path), "%s.c", output_path);
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_driver_compile_c(
    vitte_driver_t *driver,
    const char *c_path,
    const char *output_path
) {
    char command[8192];
    int exit_code;

    if (driver == NULL || c_path == NULL || output_path == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    command[0] = '\0';
    if (!vitte_driver_shell_quote(command, sizeof(command), driver->config.codegen.c_compiler) ||
        !vitte_driver_append_text(command, sizeof(command), " -std=c17 -Wall -Wextra -Werror -pedantic ") ||
        !vitte_driver_shell_quote(command, sizeof(command), c_path) ||
        !vitte_driver_append_text(command, sizeof(command), " -o ") ||
        !vitte_driver_shell_quote(command, sizeof(command), output_path)) {
        vitte_driver_set_error(driver, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_DRIVER_E_COMMAND", "C compiler command is too long", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    exit_code = system(command);
    if (exit_code != 0) {
        vitte_driver_add_diag(driver, VITTE_DIAGNOSTIC_FATAL, "VITTE_DRIVER_E_LINK", "C compiler failed", output_path);
        vitte_driver_set_error(driver, VITTE_STATUS_ERROR_BACKEND, "VITTE_DRIVER_E_LINK", "C compiler failed", output_path);
        return VITTE_STATUS_ERROR_BACKEND;
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_driver_run_impl(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input,
    vitte_driver_emit_kind_t kind,
    const char *output_path,
    vitte_driver_result_t *result
) {
    vitte_ast_t ast;
    vitte_ast_t imported_asts[VITTE_MODULE_MAX_IMPORTS];
    vitte_driver_import_unit_t *imported_units[VITTE_DRIVER_MAX_IMPORTED_UNITS];
    vitte_hir_t hir;
    vitte_ir_t ir;
    vitte_module_t module;
    vitte_import_resolver_t resolver;
    vitte_status_t status;
    size_t imported_ast_count = 0u;
    size_t imported_unit_count = 0u;
    size_t imported_index;
    bool ast_initialized = false;
    bool hir_initialized = false;
    bool ir_initialized = false;
    bool module_initialized = false;
    bool resolver_initialized = false;

    if (result != NULL) {
        vitte_driver_result_reset(result);
        result->output.kind = kind;
        result->output.path = output_path;
    }
    if (!vitte_driver_is_initialized(driver)) {
        vitte_driver_result_set_error(result, VITTE_STATUS_ERROR_INVALID_STATE, VITTE_DRIVER_STAGE_INIT, "VITTE_DRIVER_E_STATE", "driver is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    vitte_error_reset(&driver->last_error);
    vitte_diagnostic_bag_reset(&driver->diagnostics);
    vitte_driver_pipeline_reset(&driver->pipeline);
    vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_INIT, VITTE_STATUS_OK);

    status = vitte_driver_validate_input(driver, input);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_LOAD_SOURCE, status);
        vitte_driver_result_set_error(result, status, VITTE_DRIVER_STAGE_LOAD_SOURCE, "VITTE_DRIVER_E_INPUT", "invalid source input", input != NULL ? input->source_name : NULL);
        vitte_driver_update_counts(driver, result);
        return status;
    }
    vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_LOAD_SOURCE, VITTE_STATUS_OK);

    status = vitte_driver_prepare_config(driver, input, kind, output_path);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_LOAD_CONFIG, status);
        vitte_driver_add_diag(driver, VITTE_DIAGNOSTIC_FATAL, "VITTE_DRIVER_E_CONFIG", "invalid driver configuration", vitte_config_last_error(&driver->config)->details);
        vitte_error_copy(&driver->last_error, vitte_config_last_error(&driver->config));
        vitte_driver_result_set_error(result, status, VITTE_DRIVER_STAGE_LOAD_CONFIG, "VITTE_DRIVER_E_CONFIG", "invalid driver configuration", NULL);
        vitte_driver_update_counts(driver, result);
        return status;
    }
    vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_LOAD_CONFIG, VITTE_STATUS_OK);
    status = vitte_ast_init_owned(&ast, NULL);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_BUILD_AST, status);
        vitte_driver_add_diag(driver, VITTE_DIAGNOSTIC_FATAL, "VITTE_DRIVER_E_AST", "failed to initialize AST", NULL);
        vitte_driver_result_set_error(result, status, VITTE_DRIVER_STAGE_BUILD_AST, "VITTE_DRIVER_E_AST", "failed to initialize AST", NULL);
        vitte_driver_update_counts(driver, result);
        return status;
    }
    ast_initialized = true;
    memset(imported_asts, 0, sizeof(imported_asts));
    memset(imported_units, 0, sizeof(imported_units));

    status = vitte_driver_parse_ast(driver, input, &ast, &module);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_LEX, status);
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_PARSE, status);
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_BUILD_AST, status);
        vitte_driver_result_set_error(result, status, VITTE_DRIVER_STAGE_PARSE, "VITTE_DRIVER_E_PARSE", "failed to parse source", NULL);
        if (vitte_module_is_initialized(&module)) {
            vitte_module_destroy(&module);
        }
        vitte_ast_destroy(&ast);
        vitte_driver_update_counts(driver, result);
        return status;
    }
    module_initialized = true;
    vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_LEX, VITTE_STATUS_OK);
    vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_PARSE, VITTE_STATUS_OK);
    vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_BUILD_AST, VITTE_STATUS_OK);

    if (module.import_count > 0u) {
        status = vitte_driver_configure_import_resolver(driver, input, &resolver);
        if (status != VITTE_STATUS_OK) {
            vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_SEMANTIC, status);
            vitte_driver_add_diag(driver, VITTE_DIAGNOSTIC_FATAL, "VITTE_DRIVER_E_IMPORT", "failed to initialize import resolver", input->source_name);
            vitte_driver_set_error(driver, status, "VITTE_DRIVER_E_IMPORT", "failed to initialize import resolver", input->source_name);
            vitte_driver_result_set_error(result, status, VITTE_DRIVER_STAGE_SEMANTIC, "VITTE_DRIVER_E_IMPORT", "failed to initialize import resolver", NULL);
            vitte_module_destroy(&module);
            vitte_ast_destroy(&ast);
            vitte_driver_update_counts(driver, result);
            return status;
        }
        resolver_initialized = true;
        status = vitte_module_resolve_imports(&module, &resolver);
        if (status != VITTE_STATUS_OK) {
            const vitte_error_t *error = vitte_module_last_error(&module);
            vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_SEMANTIC, status);
            vitte_driver_add_diag(
                driver,
                VITTE_DIAGNOSTIC_FATAL,
                error != NULL && error->code != NULL ? error->code : "VITTE_DRIVER_E_IMPORT",
                error != NULL && error->message != NULL ? error->message : "failed to resolve module imports",
                error != NULL ? error->details : input->source_name
            );
            if (error != NULL) {
                vitte_error_copy(&driver->last_error, error);
            }
            vitte_driver_result_set_error(result, status, VITTE_DRIVER_STAGE_SEMANTIC, "VITTE_DRIVER_E_IMPORT", "failed to resolve module imports", NULL);
            vitte_import_resolver_destroy(&resolver);
            vitte_module_destroy(&module);
            vitte_ast_destroy(&ast);
            vitte_driver_update_counts(driver, result);
            return status;
        }
    }

    status = vitte_ast_validate(&ast);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_VALIDATE_AST, status);
        vitte_driver_add_diag(driver, VITTE_DIAGNOSTIC_FATAL, "VITTE_DRIVER_E_AST", "AST validation failed", vitte_ast_last_error(&ast)->details);
        vitte_driver_result_set_error(result, status, VITTE_DRIVER_STAGE_VALIDATE_AST, "VITTE_DRIVER_E_AST", "AST validation failed", NULL);
        if (resolver_initialized) {
            vitte_import_resolver_destroy(&resolver);
        }
        vitte_module_destroy(&module);
        vitte_ast_destroy(&ast);
        vitte_driver_update_counts(driver, result);
        return status;
    }
    vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_VALIDATE_AST, VITTE_STATUS_OK);

    for (imported_index = 0u; imported_index < module.import_count; imported_index++) {
        if (!module.imports[imported_index].resolved || module.imports[imported_index].resolved_path[0] == '\0') {
            continue;
        }
        status = vitte_driver_parse_imported_ast(
            driver,
            module.imports[imported_index].resolved_path,
            &imported_asts[imported_ast_count]
        );
        if (status != VITTE_STATUS_OK) {
            vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_SEMANTIC, status);
            vitte_driver_add_diag(
                driver,
                VITTE_DIAGNOSTIC_FATAL,
                "VITTE_DRIVER_E_IMPORT",
                "failed to parse imported module",
                module.imports[imported_index].resolved_path
            );
            if (resolver_initialized) {
                vitte_import_resolver_destroy(&resolver);
            }
            vitte_module_destroy(&module);
            while (imported_ast_count > 0u) {
                imported_ast_count--;
                vitte_ast_destroy(&imported_asts[imported_ast_count]);
            }
            vitte_driver_destroy_import_units(imported_units, &imported_unit_count);
            vitte_ast_destroy(&ast);
            vitte_driver_update_counts(driver, result);
            return status;
        }
        imported_ast_count++;
        if (resolver_initialized) {
            status = vitte_driver_collect_import_unit(
                driver,
                &resolver,
                module.imports[imported_index].module_name,
                module.imports[imported_index].resolved_path,
                imported_units,
                &imported_unit_count
            );
            if (status != VITTE_STATUS_OK) {
                vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_SEMANTIC, status);
                vitte_driver_add_diag(
                    driver,
                    VITTE_DIAGNOSTIC_FATAL,
                    "VITTE_DRIVER_E_IMPORT",
                    "failed to collect imported module graph",
                    module.imports[imported_index].resolved_path
                );
                if (resolver_initialized) {
                    vitte_import_resolver_destroy(&resolver);
                }
                vitte_module_destroy(&module);
                while (imported_ast_count > 0u) {
                    imported_ast_count--;
                    vitte_ast_destroy(&imported_asts[imported_ast_count]);
                }
                vitte_driver_destroy_import_units(imported_units, &imported_unit_count);
                vitte_ast_destroy(&ast);
                vitte_driver_update_counts(driver, result);
                return status;
            }
        }
    }

    status = vitte_driver_run_import_graph_sema(driver, imported_units, imported_unit_count);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_CONSTANTS, status);
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_SEMANTIC, status);
        vitte_driver_add_diag(
            driver,
            VITTE_DIAGNOSTIC_FATAL,
            "VITTE_DRIVER_E_IMPORT",
            "semantic analysis failed in imported module graph",
            vitte_driver_last_error(driver)->details
        );
        vitte_driver_result_set_error(
            result,
            status,
            VITTE_DRIVER_STAGE_SEMANTIC,
            "VITTE_DRIVER_E_IMPORT",
            "semantic analysis failed in imported module graph",
            NULL
        );
        if (resolver_initialized) {
            vitte_import_resolver_destroy(&resolver);
        }
        vitte_module_destroy(&module);
        while (imported_ast_count > 0u) {
            imported_ast_count--;
            vitte_ast_destroy(&imported_asts[imported_ast_count]);
        }
        vitte_driver_destroy_import_units(imported_units, &imported_unit_count);
        vitte_ast_destroy(&ast);
        vitte_driver_update_counts(driver, result);
        return status;
    }

    status = vitte_driver_run_sema(driver, &ast, &module, imported_asts, imported_ast_count);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_CONSTANTS, status);
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_SEMANTIC, status);
        vitte_driver_result_set_error(result, status, VITTE_DRIVER_STAGE_SEMANTIC, "VITTE_DRIVER_E_SEMA", "semantic analysis failed", NULL);
        if (resolver_initialized) {
            vitte_import_resolver_destroy(&resolver);
        }
        vitte_module_destroy(&module);
        while (imported_ast_count > 0u) {
            imported_ast_count--;
            vitte_ast_destroy(&imported_asts[imported_ast_count]);
        }
        vitte_driver_destroy_import_units(imported_units, &imported_unit_count);
        vitte_ast_destroy(&ast);
        vitte_driver_update_counts(driver, result);
        return status;
    }
    vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_CONSTANTS, VITTE_STATUS_OK);
    vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_SEMANTIC, VITTE_STATUS_OK);

    if (kind != VITTE_DRIVER_EMIT_AST) {
        status = vitte_driver_flatten_imported_modules(driver, &ast, &module, imported_units, imported_unit_count);
        if (status != VITTE_STATUS_OK) {
            const vitte_error_t *error = vitte_driver_last_error(driver);
            vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_BACKEND, status);
            vitte_driver_add_diag(
                driver,
                VITTE_DIAGNOSTIC_FATAL,
                error != NULL && error->code != NULL ? error->code : "VITTE_DRIVER_E_IMPORT",
                error != NULL && error->message != NULL ? error->message : "failed to prepare imported declarations for backend lowering",
                error != NULL ? error->details : NULL
            );
            vitte_driver_result_set_error(
                result,
                status,
                VITTE_DRIVER_STAGE_BACKEND,
                error != NULL && error->code != NULL ? error->code : "VITTE_DRIVER_E_IMPORT",
                error != NULL && error->message != NULL ? error->message : "failed to prepare imported declarations for backend lowering",
                error != NULL ? error->details : NULL
            );
            if (resolver_initialized) {
                vitte_import_resolver_destroy(&resolver);
            }
            vitte_module_destroy(&module);
            while (imported_ast_count > 0u) {
                imported_ast_count--;
                vitte_ast_destroy(&imported_asts[imported_ast_count]);
            }
            vitte_driver_destroy_import_units(imported_units, &imported_unit_count);
            vitte_ast_destroy(&ast);
            vitte_driver_update_counts(driver, result);
            return status;
        }
        status = vitte_driver_run_backend(driver, &ast, &hir, &ir);
        if (status != VITTE_STATUS_OK) {
            vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_BACKEND, status);
            vitte_driver_add_diag(driver, VITTE_DIAGNOSTIC_FATAL, "VITTE_DRIVER_E_BACKEND", "backend lowering failed", vitte_driver_last_error(driver)->details);
            vitte_driver_result_set_error(result, status, VITTE_DRIVER_STAGE_BACKEND, "VITTE_DRIVER_E_BACKEND", "backend lowering failed", NULL);
            if (resolver_initialized) {
                vitte_import_resolver_destroy(&resolver);
            }
            vitte_module_destroy(&module);
            while (imported_ast_count > 0u) {
                imported_ast_count--;
                vitte_ast_destroy(&imported_asts[imported_ast_count]);
            }
            vitte_driver_destroy_import_units(imported_units, &imported_unit_count);
            vitte_ast_destroy(&ast);
            vitte_driver_update_counts(driver, result);
            return status;
        }
        hir_initialized = true;
        ir_initialized = true;
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_BACKEND, VITTE_STATUS_OK);
    }

    if (kind == VITTE_DRIVER_EMIT_AST) {
        if (result != NULL) {
            result->output.kind = VITTE_DRIVER_EMIT_AST;
            result->output.functions_emitted = 1u;
        }
    } else if (kind == VITTE_DRIVER_EMIT_C || kind == VITTE_DRIVER_EMIT_BINARY || kind == VITTE_DRIVER_EMIT_OBJECT) {
        const char *c_output_path = output_path;
        if (kind == VITTE_DRIVER_EMIT_BINARY || kind == VITTE_DRIVER_EMIT_OBJECT) {
            status = vitte_driver_make_c_path(output_path, result);
            if (status != VITTE_STATUS_OK) {
                vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_CODEGEN_C, status);
                vitte_driver_set_error(driver, status, "VITTE_DRIVER_E_OUTPUT", "invalid build output path", output_path);
                vitte_driver_result_set_error(result, status, VITTE_DRIVER_STAGE_CODEGEN_C, "VITTE_DRIVER_E_OUTPUT", "invalid build output path", output_path);
                if (resolver_initialized) {
                    vitte_import_resolver_destroy(&resolver);
                }
                vitte_module_destroy(&module);
                while (imported_ast_count > 0u) {
                    imported_ast_count--;
                    vitte_ast_destroy(&imported_asts[imported_ast_count]);
                }
                vitte_driver_destroy_import_units(imported_units, &imported_unit_count);
                vitte_ast_destroy(&ast);
                vitte_driver_update_counts(driver, result);
                return status;
            }
            c_output_path = result != NULL ? result->generated_c_path : NULL;
        }
        status = vitte_driver_emit_c_impl(driver, VITTE_CODEGEN_INPUT_IR, &ir, c_output_path, result);
        if (status != VITTE_STATUS_OK) {
            vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_CODEGEN_C, status);
            vitte_driver_result_set_error(result, status, VITTE_DRIVER_STAGE_CODEGEN_C, "VITTE_DRIVER_E_CODEGEN", "failed to emit C17", NULL);
            if (ir_initialized) {
                vitte_ir_destroy(&ir);
            }
            if (hir_initialized) {
                vitte_hir_destroy(&hir);
            }
            if (resolver_initialized) {
                vitte_import_resolver_destroy(&resolver);
            }
            vitte_module_destroy(&module);
            while (imported_ast_count > 0u) {
                imported_ast_count--;
                vitte_ast_destroy(&imported_asts[imported_ast_count]);
            }
            vitte_driver_destroy_import_units(imported_units, &imported_unit_count);
            vitte_ast_destroy(&ast);
            vitte_driver_update_counts(driver, result);
            return status;
        }
        vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_CODEGEN_C, VITTE_STATUS_OK);

        if (kind == VITTE_DRIVER_EMIT_BINARY || kind == VITTE_DRIVER_EMIT_OBJECT) {
            status = vitte_driver_compile_c(driver, c_output_path, output_path);
            if (status != VITTE_STATUS_OK) {
                vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_COMPILE_LINK, status);
                vitte_driver_result_set_error(result, status, VITTE_DRIVER_STAGE_COMPILE_LINK, "VITTE_DRIVER_E_LINK", "failed to compile generated C", output_path);
                if (ir_initialized) {
                    vitte_ir_destroy(&ir);
                }
                if (hir_initialized) {
                    vitte_hir_destroy(&hir);
                }
                if (resolver_initialized) {
                    vitte_import_resolver_destroy(&resolver);
                }
                vitte_module_destroy(&module);
                while (imported_ast_count > 0u) {
                    imported_ast_count--;
                    vitte_ast_destroy(&imported_asts[imported_ast_count]);
                }
                vitte_driver_destroy_import_units(imported_units, &imported_unit_count);
                vitte_ast_destroy(&ast);
                vitte_driver_update_counts(driver, result);
                return status;
            }
            vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_COMPILE_LINK, VITTE_STATUS_OK);
            if (result != NULL) {
                result->output.kind = kind;
                result->output.path = output_path;
                result->output.c_path = c_output_path;
            }
        }
    }

    vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_DIAGNOSTICS, VITTE_STATUS_OK);
    vitte_driver_pipeline_mark(&driver->pipeline, VITTE_DRIVER_STAGE_CLEANUP, VITTE_STATUS_OK);

    if (ast_initialized) {
        vitte_ast_destroy(&ast);
    }
    if (resolver_initialized) {
        vitte_import_resolver_destroy(&resolver);
    }
    if (module_initialized) {
        vitte_module_destroy(&module);
    }
    while (imported_ast_count > 0u) {
        imported_ast_count--;
        vitte_ast_destroy(&imported_asts[imported_ast_count]);
    }
    vitte_driver_destroy_import_units(imported_units, &imported_unit_count);
    if (ir_initialized) {
        vitte_ir_destroy(&ir);
    }
    if (hir_initialized) {
        vitte_hir_destroy(&hir);
    }
    if (result != NULL) {
        result->status = vitte_diagnostic_status(&driver->diagnostics);
        if (result->status == VITTE_STATUS_OK) {
            vitte_error_reset(&result->last_error);
        }
    }
    vitte_error_reset(&driver->last_error);
    vitte_driver_update_counts(driver, result);
    return result != NULL ? result->status : vitte_diagnostic_status(&driver->diagnostics);
}

vitte_status_t vitte_driver_run(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input,
    vitte_driver_result_t *result
) {
    vitte_driver_emit_kind_t kind = VITTE_DRIVER_EMIT_CHECK;

    if (driver != NULL) {
        kind = driver->config.build_mode == VITTE_CONFIG_BUILD_MODE_EMIT_C ?
            VITTE_DRIVER_EMIT_C :
            driver->config.build_mode == VITTE_CONFIG_BUILD_MODE_BUILD ?
                VITTE_DRIVER_EMIT_BINARY :
                VITTE_DRIVER_EMIT_CHECK;
    }
    return vitte_driver_run_impl(driver, input, kind, driver != NULL ? driver->config.paths.output_path : NULL, result);
}

vitte_status_t vitte_driver_check(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input,
    vitte_driver_result_t *result
) {
    return vitte_driver_run_impl(driver, input, VITTE_DRIVER_EMIT_CHECK, NULL, result);
}

vitte_status_t vitte_driver_emit_c(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input,
    const char *output_path,
    vitte_driver_result_t *result
) {
    return vitte_driver_run_impl(driver, input, VITTE_DRIVER_EMIT_C, output_path, result);
}

vitte_status_t vitte_driver_build(
    vitte_driver_t *driver,
    const vitte_driver_input_t *input,
    const char *output_path,
    vitte_driver_result_t *result
) {
    if (output_path == NULL || output_path[0] == '\0') {
        vitte_driver_result_set_error(result, VITTE_STATUS_ERROR_INVALID_ARGUMENT, VITTE_DRIVER_STAGE_COMPILE_LINK, "VITTE_DRIVER_E_OUTPUT", "missing build output path", NULL);
        vitte_driver_set_error(driver, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_DRIVER_E_OUTPUT", "missing build output path", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    return vitte_driver_run_impl(driver, input, VITTE_DRIVER_EMIT_BINARY, output_path, result);
}
