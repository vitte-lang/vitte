#include "backend.h"

#include <stdio.h>
#include <string.h>

#include "program.h"
#include "writer.h"

static void vitte_c17_backend_set_error(
    vitte_c17_backend_t *backend,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (backend != NULL) {
        vitte_error_set_details(&backend->last_error, status, code, message, details);
    }
}

vitte_status_t vitte_c17_backend_init(
    vitte_c17_backend_t *backend,
    const vitte_c17_options_t *options
) {
    vitte_c17_options_t defaults;
    const vitte_c17_options_t *effective_options = options;
    vitte_status_t status;

    if (backend == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(backend, 0, sizeof(*backend));
    vitte_error_init(&backend->last_error);
    if (effective_options == NULL) {
        vitte_c17_options_init(&defaults);
        effective_options = &defaults;
    }

    status = vitte_c17_options_validate(effective_options, &backend->last_error);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    backend->options = *effective_options;
    backend->initialized = true;
    return VITTE_STATUS_OK;
}

void vitte_c17_backend_destroy(vitte_c17_backend_t *backend) {
    if (backend == NULL) {
        return;
    }

    memset(backend, 0, sizeof(*backend));
}

bool vitte_c17_backend_is_initialized(const vitte_c17_backend_t *backend) {
    return backend != NULL && backend->initialized;
}

const vitte_error_t *vitte_c17_backend_last_error(const vitte_c17_backend_t *backend) {
    return backend != NULL ? &backend->last_error : vitte_error_last();
}

void vitte_c17_emit_result_init(vitte_c17_emit_result_t *result) {
    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->status = VITTE_STATUS_OK;
}

static size_t vitte_c17_count_ir_functions(const vitte_ir_t *ir) {
    return ir != NULL && ir->module != NULL ? ir->module->function_count : 0u;
}

static vitte_status_t vitte_c17_backend_emit_with_writer(
    vitte_c17_backend_t *backend,
    const vitte_ir_t *ir,
    vitte_c17_writer_t *writer,
    vitte_c17_emit_result_t *result
) {
    vitte_c17_program_t program;
    vitte_status_t status;

    if (result != NULL) {
        vitte_c17_emit_result_init(result);
    }
    if (!vitte_c17_backend_is_initialized(backend) || ir == NULL || writer == NULL) {
        vitte_c17_backend_set_error(backend, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_BACKEND", "missing initialized C17 backend, input, or writer", NULL);
        if (result != NULL) {
            result->status = VITTE_STATUS_ERROR_INVALID_ARGUMENT;
        }
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_c17_program_init_ir(&program, ir, &backend->options);
    status = vitte_c17_program_emit(&program, writer);
    if (status != VITTE_STATUS_OK) {
        if (vitte_error_is_set(vitte_c17_writer_last_error(writer))) {
            vitte_error_copy(&backend->last_error, vitte_c17_writer_last_error(writer));
        } else {
            vitte_error_copy(&backend->last_error, vitte_c17_program_last_error(&program));
        }
        if (result != NULL) {
            result->status = status;
        }
        return status;
    }

    status = vitte_c17_writer_flush(writer);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&backend->last_error, vitte_c17_writer_last_error(writer));
        if (result != NULL) {
            result->status = status;
        }
        return status;
    }

    if (result != NULL) {
        result->status = VITTE_STATUS_OK;
        result->bytes_written = writer->byte_count;
        result->lines_written = writer->line_count;
        result->functions_emitted = vitte_c17_count_ir_functions(ir);
    }
    vitte_error_reset(&backend->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_c17_backend_emit_ir_to_buffer(
    vitte_c17_backend_t *backend,
    const vitte_ir_t *ir,
    char *buffer,
    size_t buffer_capacity,
    vitte_c17_emit_result_t *result
) {
    vitte_c17_writer_t writer;
    vitte_status_t status;

    if (buffer == NULL || buffer_capacity == 0u) {
        vitte_c17_backend_set_error(backend, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_BUFFER", "missing C17 output buffer", NULL);
        if (result != NULL) {
            result->status = VITTE_STATUS_ERROR_INVALID_ARGUMENT;
        }
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    status = vitte_c17_writer_init_buffer(&writer, buffer, buffer_capacity, backend != NULL ? &backend->options : NULL);
    if (status != VITTE_STATUS_OK) {
        vitte_c17_backend_set_error(backend, status, "VITTE_C17_E_WRITER", "failed to initialize C17 buffer writer", NULL);
        if (result != NULL) {
            result->status = status;
        }
        return status;
    }

    return vitte_c17_backend_emit_with_writer(backend, ir, &writer, result);
}

vitte_status_t vitte_c17_backend_emit_ir_to_file(
    vitte_c17_backend_t *backend,
    const vitte_ir_t *ir,
    const char *output_path,
    vitte_c17_emit_result_t *result
) {
    FILE *stream;
    vitte_c17_writer_t writer;
    vitte_status_t status;

    if (output_path == NULL) {
        vitte_c17_backend_set_error(backend, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_C17_E_FILE", "missing C17 output path", NULL);
        if (result != NULL) {
            result->status = VITTE_STATUS_ERROR_INVALID_ARGUMENT;
        }
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    stream = fopen(output_path, "wb");
    if (stream == NULL) {
        vitte_c17_backend_set_error(backend, VITTE_STATUS_ERROR_IO, "VITTE_C17_E_FILE", "failed to open C17 output file", output_path);
        if (result != NULL) {
            result->status = VITTE_STATUS_ERROR_IO;
        }
        return VITTE_STATUS_ERROR_IO;
    }

    status = vitte_c17_writer_init_file(&writer, stream, backend != NULL ? &backend->options : NULL);
    if (status == VITTE_STATUS_OK) {
        status = vitte_c17_backend_emit_with_writer(backend, ir, &writer, result);
    }
    if (fclose(stream) != 0 && status == VITTE_STATUS_OK) {
        vitte_c17_backend_set_error(backend, VITTE_STATUS_ERROR_IO, "VITTE_C17_E_FILE", "failed to close C17 output file", output_path);
        status = VITTE_STATUS_ERROR_IO;
        if (result != NULL) {
            result->status = status;
        }
    }

    return status;
}
