#include "api.h"

#include <string.h>

#include "../driver/driver.h"

static bool g_vitte_api_initialized;
static vitte_context_t g_vitte_api_context;

static void vitte_api_fill_driver_options(
    vitte_context_t *context,
    const char *input_path,
    const char *output_path,
    const char *c_compiler,
    vitte_driver_emit_kind_t emit_kind,
    bool keep_intermediate_c,
    vitte_driver_options_t *options
) {
    vitte_driver_options_init(options);
    options->input_path = input_path;
    options->output_path = output_path;
    options->root_path = vitte_context_root_path(context);
    options->sysroot_path = vitte_context_sysroot_path(context);
    options->c_compiler = c_compiler != NULL ? c_compiler : "cc";
    options->emit_kind = emit_kind;
    options->keep_intermediate_c = keep_intermediate_c;
}

static void vitte_api_fill_compile_result(
    vitte_compile_result_t *result,
    const char *input_path,
    const char *output_path,
    const vitte_driver_result_t *driver_result,
    vitte_status_t status
) {
    if (result == NULL) {
        return;
    }

    vitte_compile_result_init(result);
    result->status = status;
    result->input_path = input_path;
    result->output_path = output_path;
    if (driver_result != NULL) {
        result->warning_count = driver_result->warning_count;
        result->error_count = driver_result->error_count;
    } else if (status != VITTE_STATUS_OK) {
        result->error_count = 1u;
    }
}

static vitte_status_t vitte_api_run_driver(
    vitte_context_t *context,
    const char *input_path,
    const char *output_path,
    const char *c_compiler,
    vitte_driver_emit_kind_t emit_kind,
    bool keep_intermediate_c,
    vitte_compile_result_t *result
) {
    vitte_driver_options_t driver_options;
    vitte_driver_t driver;
    vitte_driver_input_t input;
    vitte_driver_result_t driver_result;
    vitte_status_t status;

    if (context == NULL || !vitte_context_is_initialized(context) || input_path == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_api_fill_driver_options(
        context,
        input_path,
        output_path,
        c_compiler,
        emit_kind,
        keep_intermediate_c,
        &driver_options
    );
    status = vitte_driver_init(&driver, context, &driver_options);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&context->last_error, vitte_driver_last_error(&driver));
        vitte_api_fill_compile_result(result, input_path, output_path, NULL, status);
        return status;
    }

    vitte_driver_input_init(&input);
    status = vitte_driver_input_from_file(&input, input_path, 0u);
    if (status != VITTE_STATUS_OK) {
        vitte_driver_shutdown(&driver);
        vitte_context_set_error(context, status, "VITTE_API_E_INPUT", "failed to read input file", input_path);
        vitte_api_fill_compile_result(result, input_path, output_path, NULL, status);
        return status;
    }

    vitte_driver_result_init(&driver_result);
    if (emit_kind == VITTE_DRIVER_EMIT_CHECK) {
        status = vitte_driver_check(&driver, &input, &driver_result);
    } else if (emit_kind == VITTE_DRIVER_EMIT_C) {
        status = vitte_driver_emit_c(&driver, &input, output_path, &driver_result);
    } else {
        status = vitte_driver_build(&driver, &input, output_path, &driver_result);
    }

    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&context->last_error, vitte_driver_last_error(&driver));
    } else {
        vitte_context_reset_error(context);
    }
    vitte_api_fill_compile_result(result, input_path, output_path, &driver_result, status);

    vitte_driver_input_destroy(&input);
    vitte_driver_shutdown(&driver);
    return status;
}

vitte_status_t vitte_api_init(const vitte_api_config_t *config) {
    vitte_status_t status;

    if (g_vitte_api_initialized) {
        return VITTE_STATUS_OK;
    }

    status = vitte_context_init(&g_vitte_api_context, config);
    if (status != VITTE_STATUS_OK) {
        return status;
    }

    g_vitte_api_initialized = true;
    return VITTE_STATUS_OK;
}

void vitte_api_shutdown(void) {
    if (!g_vitte_api_initialized) {
        return;
    }

    vitte_context_destroy(&g_vitte_api_context);
    g_vitte_api_initialized = false;
}

bool vitte_api_is_initialized(void) {
    return g_vitte_api_initialized;
}

vitte_status_t vitte_api_context_create(
    const vitte_api_config_t *config,
    vitte_context_t **context
) {
    return vitte_context_create(config, context);
}

void vitte_compile_options_init(vitte_compile_options_t *options) {
    if (options == NULL) {
        return;
    }

    memset(options, 0, sizeof(*options));
    options->c_compiler = "cc";
    options->output_kind = VITTE_OUTPUT_CHECK;
    options->keep_intermediate_c = true;
}

void vitte_compile_result_init(vitte_compile_result_t *result) {
    if (result == NULL) {
        return;
    }

    memset(result, 0, sizeof(*result));
    result->status = VITTE_STATUS_OK;
}

vitte_status_t vitte_api_check_file(
    vitte_context_t *context,
    const char *input_path,
    vitte_compile_result_t *result
) {
    return vitte_api_run_driver(
        context,
        input_path,
        NULL,
        "cc",
        VITTE_DRIVER_EMIT_CHECK,
        false,
        result
    );
}

vitte_status_t vitte_api_emit_c_file(
    vitte_context_t *context,
    const char *input_path,
    const char *output_path,
    vitte_compile_result_t *result
) {
    if (output_path == NULL) {
        if (result != NULL) {
            vitte_compile_result_init(result);
            result->input_path = input_path;
            result->status = VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            result->error_count = 1u;
        }
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    return vitte_api_run_driver(
        context,
        input_path,
        output_path,
        "cc",
        VITTE_DRIVER_EMIT_C,
        false,
        result
    );
}

vitte_status_t vitte_api_build_file(
    vitte_context_t *context,
    const vitte_compile_options_t *options,
    vitte_compile_result_t *result
) {
    vitte_compile_options_t defaults;

    if (options == NULL) {
        vitte_compile_options_init(&defaults);
        options = &defaults;
    }
    if (context == NULL || !vitte_context_is_initialized(context) || options->input_path == NULL) {
        if (result != NULL) {
            vitte_compile_result_init(result);
            result->input_path = options != NULL ? options->input_path : NULL;
            result->output_path = options != NULL ? options->output_path : NULL;
            result->status = VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            result->error_count = 1u;
        }
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (options->output_kind == VITTE_OUTPUT_CHECK) {
        return vitte_api_check_file(context, options->input_path, result);
    }
    if (options->output_kind == VITTE_OUTPUT_C17) {
        return vitte_api_emit_c_file(context, options->input_path, options->output_path, result);
    }
    if (options->output_kind != VITTE_OUTPUT_EXECUTABLE || options->output_path == NULL) {
        if (result != NULL) {
            vitte_compile_result_init(result);
            result->input_path = options->input_path;
            result->output_path = options->output_path;
            result->status = VITTE_STATUS_ERROR_INVALID_ARGUMENT;
            result->error_count = 1u;
        }
        vitte_context_set_error(context, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_API_E_OUTPUT", "missing or unsupported output kind/path", options->output_path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    return vitte_api_run_driver(
        context,
        options->input_path,
        options->output_path,
        options->c_compiler,
        VITTE_DRIVER_EMIT_BINARY,
        options->keep_intermediate_c,
        result
    );
}

const vitte_error_t *vitte_api_last_error(void) {
    if (g_vitte_api_initialized) {
        return vitte_context_last_error(&g_vitte_api_context);
    }

    return vitte_error_last();
}
