#include "codegen.h"

#include <string.h>

#include "../backend/c17/backend.h"

static void vitte_codegen_set_error(
    vitte_codegen_t *codegen,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (codegen != NULL) {
        vitte_error_set_details(&codegen->last_error, status, code, message, details);
    }
}

void vitte_codegen_options_init(vitte_codegen_options_t *options) {
    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->backend = VITTE_CODEGEN_BACKEND_C17;
    options->input_kind = VITTE_CODEGEN_INPUT_AST;
    options->output_kind = VITTE_CODEGEN_OUTPUT_BUFFER;
    options->source_name = "<memory>";
    options->indent_width = 4u;
    options->emit_includes = true;
}

static bool vitte_codegen_backend_is_valid(vitte_codegen_backend_t backend) {
    return backend == VITTE_CODEGEN_BACKEND_C17;
}

static bool vitte_codegen_input_kind_is_valid(vitte_codegen_input_kind_t kind) {
    return kind == VITTE_CODEGEN_INPUT_AST ||
        kind == VITTE_CODEGEN_INPUT_HIR ||
        kind == VITTE_CODEGEN_INPUT_IR;
}

static bool vitte_codegen_output_kind_is_valid(vitte_codegen_output_kind_t kind) {
    return kind == VITTE_CODEGEN_OUTPUT_BUFFER ||
        kind == VITTE_CODEGEN_OUTPUT_FILE;
}

vitte_status_t vitte_codegen_options_validate(const vitte_codegen_options_t *options, vitte_error_t *error) {
    if (options == NULL) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CODEGEN_E_ARGUMENT", "missing codegen options", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_codegen_backend_is_valid(options->backend)) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CODEGEN_E_BACKEND", "unknown codegen backend", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_codegen_input_kind_is_valid(options->input_kind)) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CODEGEN_E_ARGUMENT", "unknown codegen input kind", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_codegen_output_kind_is_valid(options->output_kind)) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CODEGEN_E_OUTPUT", "unknown codegen output kind", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (options->indent_width > 16u) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CODEGEN_E_ARGUMENT", "invalid codegen indentation width", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (options->output_kind == VITTE_CODEGEN_OUTPUT_BUFFER &&
        (options->buffer == NULL || options->buffer_capacity == 0u)) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CODEGEN_E_OUTPUT", "missing codegen output buffer", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (options->output_kind == VITTE_CODEGEN_OUTPUT_FILE &&
        (options->output_path == NULL || options->output_path[0] == '\0')) {
        vitte_error_set_details(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CODEGEN_E_OUTPUT", "missing codegen output path", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (error != NULL) {
        vitte_error_reset(error);
    }
    return VITTE_STATUS_OK;
}

void vitte_codegen_result_init(vitte_codegen_result_t *result) {
    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->status = VITTE_STATUS_OK;
    result->backend = VITTE_CODEGEN_BACKEND_C17;
    result->input_kind = VITTE_CODEGEN_INPUT_AST;
    result->output_kind = VITTE_CODEGEN_OUTPUT_BUFFER;
}

vitte_status_t vitte_codegen_init(vitte_codegen_t *codegen, const vitte_codegen_options_t *options) {
    vitte_codegen_options_t defaults;
    const vitte_codegen_options_t *effective_options = options;
    vitte_status_t status;

    if (codegen == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(codegen, 0, sizeof(*codegen));
    vitte_error_init(&codegen->last_error);
    if (effective_options == NULL) {
        vitte_codegen_options_init(&defaults);
        effective_options = &defaults;
    }

    status = vitte_codegen_options_validate(effective_options, &codegen->last_error);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    codegen->options = *effective_options;
    codegen->initialized = true;
    return VITTE_STATUS_OK;
}

void vitte_codegen_destroy(vitte_codegen_t *codegen) {
    if (codegen == NULL) {
        return;
    }

    memset(codegen, 0, sizeof(*codegen));
}

bool vitte_codegen_is_initialized(const vitte_codegen_t *codegen) {
    return codegen != NULL && codegen->initialized;
}

const vitte_error_t *vitte_codegen_last_error(const vitte_codegen_t *codegen) {
    return codegen != NULL ? &codegen->last_error : vitte_error_last();
}

void vitte_codegen_clear_error(vitte_codegen_t *codegen) {
    if (codegen != NULL) {
        vitte_error_reset(&codegen->last_error);
    }
}

const char *vitte_codegen_backend_name(vitte_codegen_backend_t backend) {
    switch (backend) {
        case VITTE_CODEGEN_BACKEND_C17:
            return "c17";
        default:
            return "unknown";
    }
}

const char *vitte_codegen_input_kind_name(vitte_codegen_input_kind_t kind) {
    switch (kind) {
        case VITTE_CODEGEN_INPUT_AST:
            return "ast";
        case VITTE_CODEGEN_INPUT_HIR:
            return "hir";
        case VITTE_CODEGEN_INPUT_IR:
            return "ir";
        default:
            return "unknown";
    }
}

const char *vitte_codegen_output_kind_name(vitte_codegen_output_kind_t kind) {
    switch (kind) {
        case VITTE_CODEGEN_OUTPUT_BUFFER:
            return "buffer";
        case VITTE_CODEGEN_OUTPUT_FILE:
            return "file";
        default:
            return "unknown";
    }
}

static vitte_status_t vitte_codegen_validate_ast(vitte_codegen_t *codegen, const vitte_ast_t *ast) {
    if (ast == NULL) {
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CODEGEN_E_ARGUMENT", "missing AST input for codegen", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_ast_is_initialized(ast) || ast->root == NULL) {
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CODEGEN_E_STATE", "codegen requires initialized AST with root module", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_codegen_validate_hir(vitte_codegen_t *codegen, const vitte_hir_t *hir) {
    if (hir == NULL) {
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CODEGEN_E_ARGUMENT", "missing HIR input for codegen", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_hir_is_initialized(hir) || hir->root == NULL) {
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CODEGEN_E_STATE", "codegen requires initialized HIR with root module", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_codegen_validate_ir(vitte_codegen_t *codegen, const vitte_ir_t *ir) {
    if (ir == NULL) {
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CODEGEN_E_ARGUMENT", "missing IR input for codegen", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_ir_is_initialized(ir) || ir->module == NULL) {
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CODEGEN_E_STATE", "codegen requires initialized IR with module", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return VITTE_STATUS_OK;
}

static void vitte_codegen_make_c17_options(const vitte_codegen_options_t *options, vitte_c17_options_t *c17_options) {
    vitte_c17_options_init(c17_options);
    c17_options->source_name = options->source_name;
    c17_options->output_path = options->output_path;
    c17_options->indent_width = options->indent_width;
    c17_options->emit_includes = options->emit_includes;
    c17_options->emit_debug_comments = options->emit_debug_comments;
    c17_options->emit_main_wrapper = false;
}

static void vitte_codegen_copy_c17_result(
    vitte_codegen_result_t *result,
    const vitte_codegen_options_t *options,
    const vitte_c17_emit_result_t *c17_result
) {
    if (result == NULL) {
        return;
    }

    vitte_codegen_result_init(result);
    result->status = c17_result != NULL ? c17_result->status : VITTE_STATUS_OK;
    result->backend = options->backend;
    result->input_kind = options->input_kind;
    result->output_kind = options->output_kind;
    result->bytes_written = c17_result != NULL ? c17_result->bytes_written : 0u;
    result->lines_written = c17_result != NULL ? c17_result->lines_written : 0u;
    result->units_emitted = 1u;
    result->functions_emitted = c17_result != NULL ? c17_result->functions_emitted : 0u;
    result->output_path = options->output_path;
    result->error_count = result->status == VITTE_STATUS_OK ? 0u : 1u;
}

static vitte_status_t vitte_codegen_emit_ast_c17(
    vitte_codegen_t *codegen,
    const vitte_ast_t *ast,
    char *buffer,
    size_t buffer_capacity,
    const char *output_path,
    vitte_codegen_output_kind_t output_kind,
    vitte_codegen_result_t *result
) {
    vitte_c17_options_t c17_options;
    vitte_c17_backend_t backend;
    vitte_c17_emit_result_t c17_result;
    vitte_codegen_options_t effective_options;
    vitte_status_t status;

    status = vitte_codegen_validate_ast(codegen, ast);
    if (status != VITTE_STATUS_OK) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = status;
            result->error_count = 1u;
        }
        return status;
    }

    effective_options = codegen->options;
    effective_options.output_kind = output_kind;
    effective_options.buffer = buffer;
    effective_options.buffer_capacity = buffer_capacity;
    effective_options.output_path = output_path;
    status = vitte_codegen_options_validate(&effective_options, &codegen->last_error);
    if (status != VITTE_STATUS_OK) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = status;
            result->error_count = 1u;
        }
        return status;
    }

    vitte_codegen_make_c17_options(&effective_options, &c17_options);
    status = vitte_c17_backend_init(&backend, &c17_options);
    if (status != VITTE_STATUS_OK) {
        vitte_codegen_set_error(codegen, status, "VITTE_CODEGEN_E_BACKEND", "failed to initialize C17 backend", NULL);
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = status;
            result->error_count = 1u;
        }
        return status;
    }

    if (output_kind == VITTE_CODEGEN_OUTPUT_BUFFER) {
        status = vitte_c17_backend_emit_ast_to_buffer(&backend, ast, buffer, buffer_capacity, &c17_result);
    } else {
        status = vitte_c17_backend_emit_ast_to_file(&backend, ast, output_path, &c17_result);
    }

    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&codegen->last_error, vitte_c17_backend_last_error(&backend));
    } else {
        vitte_error_reset(&codegen->last_error);
    }
    vitte_codegen_copy_c17_result(result, &effective_options, &c17_result);
    vitte_c17_backend_destroy(&backend);
    return status;
}

static vitte_status_t vitte_codegen_emit_ir_c17(
    vitte_codegen_t *codegen,
    const vitte_ir_t *ir,
    char *buffer,
    size_t buffer_capacity,
    const char *output_path,
    vitte_codegen_output_kind_t output_kind,
    vitte_codegen_result_t *result
) {
    vitte_c17_options_t c17_options;
    vitte_c17_backend_t backend;
    vitte_c17_emit_result_t c17_result;
    vitte_codegen_options_t effective_options;
    vitte_status_t status;

    status = vitte_codegen_validate_ir(codegen, ir);
    if (status != VITTE_STATUS_OK) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = status;
            result->input_kind = VITTE_CODEGEN_INPUT_IR;
            result->error_count = 1u;
        }
        return status;
    }

    effective_options = codegen->options;
    effective_options.input_kind = VITTE_CODEGEN_INPUT_IR;
    effective_options.output_kind = output_kind;
    effective_options.buffer = buffer;
    effective_options.buffer_capacity = buffer_capacity;
    effective_options.output_path = output_path;
    status = vitte_codegen_options_validate(&effective_options, &codegen->last_error);
    if (status != VITTE_STATUS_OK) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = status;
            result->input_kind = VITTE_CODEGEN_INPUT_IR;
            result->error_count = 1u;
        }
        return status;
    }

    vitte_codegen_make_c17_options(&effective_options, &c17_options);
    status = vitte_c17_backend_init(&backend, &c17_options);
    if (status != VITTE_STATUS_OK) {
        vitte_codegen_set_error(codegen, status, "VITTE_CODEGEN_E_BACKEND", "failed to initialize C17 backend", NULL);
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = status;
            result->input_kind = VITTE_CODEGEN_INPUT_IR;
            result->error_count = 1u;
        }
        return status;
    }

    if (output_kind == VITTE_CODEGEN_OUTPUT_BUFFER) {
        status = vitte_c17_backend_emit_ir_to_buffer(&backend, ir, buffer, buffer_capacity, &c17_result);
    } else {
        status = vitte_c17_backend_emit_ir_to_file(&backend, ir, output_path, &c17_result);
    }

    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&codegen->last_error, vitte_c17_backend_last_error(&backend));
    } else {
        vitte_error_reset(&codegen->last_error);
    }
    vitte_codegen_copy_c17_result(result, &effective_options, &c17_result);
    vitte_c17_backend_destroy(&backend);
    return status;
}

vitte_status_t vitte_codegen_emit_ast_to_buffer(
    vitte_codegen_t *codegen,
    const vitte_ast_t *ast,
    char *buffer,
    size_t buffer_capacity,
    vitte_codegen_result_t *result
) {
    if (!vitte_codegen_is_initialized(codegen)) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = VITTE_STATUS_ERROR_INVALID_STATE;
            result->error_count = 1u;
        }
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CODEGEN_E_STATE", "codegen is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return vitte_codegen_emit_ast_c17(codegen, ast, buffer, buffer_capacity, NULL, VITTE_CODEGEN_OUTPUT_BUFFER, result);
}

vitte_status_t vitte_codegen_emit_ast_to_file(
    vitte_codegen_t *codegen,
    const vitte_ast_t *ast,
    const char *output_path,
    vitte_codegen_result_t *result
) {
    if (!vitte_codegen_is_initialized(codegen)) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = VITTE_STATUS_ERROR_INVALID_STATE;
            result->error_count = 1u;
        }
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CODEGEN_E_STATE", "codegen is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return vitte_codegen_emit_ast_c17(codegen, ast, NULL, 0u, output_path, VITTE_CODEGEN_OUTPUT_FILE, result);
}

vitte_status_t vitte_codegen_emit_hir_to_buffer(
    vitte_codegen_t *codegen,
    const vitte_hir_t *hir,
    char *buffer,
    size_t buffer_capacity,
    vitte_codegen_result_t *result
) {
    vitte_ir_t ir;
    vitte_status_t status;

    if (!vitte_codegen_is_initialized(codegen)) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = VITTE_STATUS_ERROR_INVALID_STATE;
            result->input_kind = VITTE_CODEGEN_INPUT_HIR;
            result->error_count = 1u;
        }
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CODEGEN_E_STATE", "codegen is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    status = vitte_codegen_validate_hir(codegen, hir);
    if (status != VITTE_STATUS_OK) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = status;
            result->input_kind = VITTE_CODEGEN_INPUT_HIR;
            result->error_count = 1u;
        }
        return status;
    }
    status = vitte_ir_init_owned(&ir, NULL);
    if (status != VITTE_STATUS_OK) {
        vitte_codegen_set_error(codegen, status, "VITTE_CODEGEN_E_IR", "failed to initialize temporary IR for HIR codegen", NULL);
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = status;
            result->input_kind = VITTE_CODEGEN_INPUT_HIR;
            result->error_count = 1u;
        }
        return status;
    }
    status = vitte_ir_lower_hir(&ir, hir);
    if (status == VITTE_STATUS_OK) {
        status = vitte_ir_validate(&ir);
    }
    if (status == VITTE_STATUS_OK) {
        status = vitte_codegen_emit_ir_c17(codegen, &ir, buffer, buffer_capacity, NULL, VITTE_CODEGEN_OUTPUT_BUFFER, result);
        if (result != NULL) {
            result->input_kind = VITTE_CODEGEN_INPUT_HIR;
        }
    } else {
        vitte_error_copy(&codegen->last_error, vitte_ir_last_error(&ir));
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = status;
            result->input_kind = VITTE_CODEGEN_INPUT_HIR;
            result->error_count = 1u;
        }
    }
    vitte_ir_destroy(&ir);
    return status;
}

vitte_status_t vitte_codegen_emit_hir_to_file(
    vitte_codegen_t *codegen,
    const vitte_hir_t *hir,
    const char *output_path,
    vitte_codegen_result_t *result
) {
    vitte_ir_t ir;
    vitte_status_t status;

    if (!vitte_codegen_is_initialized(codegen)) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = VITTE_STATUS_ERROR_INVALID_STATE;
            result->input_kind = VITTE_CODEGEN_INPUT_HIR;
            result->error_count = 1u;
        }
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CODEGEN_E_STATE", "codegen is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    status = vitte_codegen_validate_hir(codegen, hir);
    if (status != VITTE_STATUS_OK) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = status;
            result->input_kind = VITTE_CODEGEN_INPUT_HIR;
            result->error_count = 1u;
        }
        return status;
    }
    status = vitte_ir_init_owned(&ir, NULL);
    if (status != VITTE_STATUS_OK) {
        vitte_codegen_set_error(codegen, status, "VITTE_CODEGEN_E_IR", "failed to initialize temporary IR for HIR codegen", NULL);
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = status;
            result->input_kind = VITTE_CODEGEN_INPUT_HIR;
            result->error_count = 1u;
        }
        return status;
    }
    status = vitte_ir_lower_hir(&ir, hir);
    if (status == VITTE_STATUS_OK) {
        status = vitte_ir_validate(&ir);
    }
    if (status == VITTE_STATUS_OK) {
        status = vitte_codegen_emit_ir_c17(codegen, &ir, NULL, 0u, output_path, VITTE_CODEGEN_OUTPUT_FILE, result);
        if (result != NULL) {
            result->input_kind = VITTE_CODEGEN_INPUT_HIR;
        }
    } else {
        vitte_error_copy(&codegen->last_error, vitte_ir_last_error(&ir));
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = status;
            result->input_kind = VITTE_CODEGEN_INPUT_HIR;
            result->error_count = 1u;
        }
    }
    vitte_ir_destroy(&ir);
    return status;
}

vitte_status_t vitte_codegen_emit_ir_to_buffer(
    vitte_codegen_t *codegen,
    const vitte_ir_t *ir,
    char *buffer,
    size_t buffer_capacity,
    vitte_codegen_result_t *result
) {
    if (!vitte_codegen_is_initialized(codegen)) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = VITTE_STATUS_ERROR_INVALID_STATE;
            result->input_kind = VITTE_CODEGEN_INPUT_IR;
            result->error_count = 1u;
        }
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CODEGEN_E_STATE", "codegen is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return vitte_codegen_emit_ir_c17(codegen, ir, buffer, buffer_capacity, NULL, VITTE_CODEGEN_OUTPUT_BUFFER, result);
}

vitte_status_t vitte_codegen_emit_ir_to_file(
    vitte_codegen_t *codegen,
    const vitte_ir_t *ir,
    const char *output_path,
    vitte_codegen_result_t *result
) {
    if (!vitte_codegen_is_initialized(codegen)) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = VITTE_STATUS_ERROR_INVALID_STATE;
            result->input_kind = VITTE_CODEGEN_INPUT_IR;
            result->error_count = 1u;
        }
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CODEGEN_E_STATE", "codegen is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    return vitte_codegen_emit_ir_c17(codegen, ir, NULL, 0u, output_path, VITTE_CODEGEN_OUTPUT_FILE, result);
}

vitte_status_t vitte_codegen_emit(
    vitte_codegen_t *codegen,
    const void *input,
    vitte_codegen_result_t *result
) {
    if (!vitte_codegen_is_initialized(codegen)) {
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = VITTE_STATUS_ERROR_INVALID_STATE;
            result->error_count = 1u;
        }
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_CODEGEN_E_STATE", "codegen is not initialized", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    if (codegen->options.input_kind != VITTE_CODEGEN_INPUT_AST) {
        if (codegen->options.input_kind == VITTE_CODEGEN_INPUT_HIR) {
            if (codegen->options.output_kind == VITTE_CODEGEN_OUTPUT_BUFFER) {
                return vitte_codegen_emit_hir_to_buffer(
                    codegen,
                    (const vitte_hir_t *)input,
                    codegen->options.buffer,
                    codegen->options.buffer_capacity,
                    result
                );
            }
            return vitte_codegen_emit_hir_to_file(
                codegen,
                (const vitte_hir_t *)input,
                codegen->options.output_path,
                result
            );
        }
        if (codegen->options.input_kind == VITTE_CODEGEN_INPUT_IR) {
            if (codegen->options.output_kind == VITTE_CODEGEN_OUTPUT_BUFFER) {
                return vitte_codegen_emit_ir_to_buffer(
                    codegen,
                    (const vitte_ir_t *)input,
                    codegen->options.buffer,
                    codegen->options.buffer_capacity,
                    result
                );
            }
            return vitte_codegen_emit_ir_to_file(
                codegen,
                (const vitte_ir_t *)input,
                codegen->options.output_path,
                result
            );
        }
        if (result != NULL) {
            vitte_codegen_result_init(result);
            result->status = VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            result->error_count = 1u;
        }
        vitte_codegen_set_error(codegen, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_CODEGEN_E_ARGUMENT", "unknown codegen input kind", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (codegen->options.output_kind == VITTE_CODEGEN_OUTPUT_BUFFER) {
        return vitte_codegen_emit_ast_to_buffer(
            codegen,
            (const vitte_ast_t *)input,
            codegen->options.buffer,
            codegen->options.buffer_capacity,
            result
        );
    }
    return vitte_codegen_emit_ast_to_file(
        codegen,
        (const vitte_ast_t *)input,
        codegen->options.output_path,
        result
    );
}
