#include "module.h"

#include <string.h>

static void vitte_module_set_error(
    vitte_module_t *module,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (module != NULL) {
        vitte_error_set_details(&module->last_error, status, code, message, details);
        module->state = VITTE_MODULE_STATE_FAILED;
        module->stats.error_count++;
    }
}

static bool vitte_module_copy_text(char *destination, size_t capacity, const char *source) {
    size_t length;

    if (destination == NULL || capacity == 0u || source == NULL) {
        return false;
    }
    length = strlen(source);
    if (length >= capacity) {
        return false;
    }
    (void)memcpy(destination, source, length + 1u);
    return true;
}

static void vitte_module_import_reset(vitte_module_import_t *entry) {
    if (entry == NULL) {
        return;
    }
    if (entry->source_owned && entry->source_buffer != NULL) {
        vitte_fs_free(entry->source_buffer);
    }
    memset(entry, 0, sizeof(*entry));
    vitte_error_init(&entry->last_error);
}

static void vitte_module_import_release_source(vitte_module_import_t *entry) {
    if (entry == NULL) {
        return;
    }
    if (entry->source_owned && entry->source_buffer != NULL) {
        vitte_fs_free(entry->source_buffer);
    }
    entry->source_buffer = NULL;
    entry->source_size = 0u;
    entry->source_loaded = false;
    entry->source_owned = false;
}

static void vitte_module_release_source(vitte_module_t *module) {
    if (module == NULL) {
        return;
    }
    if (module->source_owned && module->source_buffer != NULL) {
        vitte_fs_free(module->source_buffer);
    }
    module->source_buffer = NULL;
    module->source_size = 0u;
    module->source_owned = false;
    module->source_name = NULL;
}

static void vitte_module_update_stats(vitte_module_t *module) {
    if (module == NULL) {
        return;
    }
    module->stats.import_count = module->import_count;
    module->stats.resolved_import_count = module->resolved_import_count;
    module->stats.token_count = module->token_count;
    module->stats.source_bytes = module->source_size;
}

void vitte_module_options_init(vitte_module_options_t *options) {
    if (options == NULL) {
        return;
    }
    memset(options, 0, sizeof(*options));
    options->read_source = true;
    options->max_source_bytes = VITTE_IMPORT_MAX_SOURCE_BYTES;
    options->max_imports = VITTE_MODULE_MAX_IMPORTS;
    vitte_lexer_options_init(&options->lexer_options);
}

void vitte_module_result_init(vitte_module_result_t *result) {
    if (result == NULL) {
        return;
    }
    memset(result, 0, sizeof(*result));
    result->status = VITTE_STATUS_OK;
    result->state = VITTE_MODULE_STATE_UNINITIALIZED;
    vitte_error_init(&result->last_error);
}

void vitte_module_stats_init(vitte_module_stats_t *stats) {
    if (stats == NULL) {
        return;
    }
    memset(stats, 0, sizeof(*stats));
}

const char *vitte_module_state_name(vitte_module_state_t state) {
    switch (state) {
        case VITTE_MODULE_STATE_UNINITIALIZED:
            return "uninitialized";
        case VITTE_MODULE_STATE_CREATED:
            return "created";
        case VITTE_MODULE_STATE_RESOLVED:
            return "resolved";
        case VITTE_MODULE_STATE_LOADED:
            return "loaded";
        case VITTE_MODULE_STATE_LEXED:
            return "lexed";
        case VITTE_MODULE_STATE_PARSED:
            return "parsed";
        case VITTE_MODULE_STATE_LOWERED_HIR:
            return "lowered-hir";
        case VITTE_MODULE_STATE_LOWERED_IR:
            return "lowered-ir";
        case VITTE_MODULE_STATE_FAILED:
            return "failed";
        default:
            return "unknown";
    }
}

vitte_status_t vitte_module_init(vitte_module_t *module, const vitte_module_options_t *options) {
    vitte_module_options_t defaults;
    size_t index;

    if (module == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    memset(module, 0, sizeof(*module));
    vitte_error_init(&module->last_error);
    vitte_token_init(&module->last_token);
    vitte_module_stats_init(&module->stats);
    if (options == NULL) {
        vitte_module_options_init(&defaults);
        module->options = defaults;
    } else {
        module->options = *options;
    }
    if (module->options.max_source_bytes == 0u) {
        module->options.max_source_bytes = VITTE_IMPORT_MAX_SOURCE_BYTES;
    }
    if (module->options.max_imports == 0u || module->options.max_imports > VITTE_MODULE_MAX_IMPORTS) {
        module->options.max_imports = VITTE_MODULE_MAX_IMPORTS;
    }
    for (index = 0u; index < VITTE_MODULE_MAX_IMPORTS; index++) {
        vitte_error_init(&module->imports[index].last_error);
    }
    module->initialized = true;
    module->state = VITTE_MODULE_STATE_CREATED;
    return VITTE_STATUS_OK;
}

void vitte_module_destroy(vitte_module_t *module) {
    size_t index;

    if (module == NULL) {
        return;
    }
    vitte_module_release_source(module);
    for (index = 0u; index < VITTE_MODULE_MAX_IMPORTS; index++) {
        vitte_module_import_reset(&module->imports[index]);
    }
    memset(module, 0, sizeof(*module));
}

bool vitte_module_is_initialized(const vitte_module_t *module) {
    return module != NULL && module->initialized;
}

const vitte_error_t *vitte_module_last_error(const vitte_module_t *module) {
    return module != NULL ? &module->last_error : vitte_error_last();
}

const vitte_module_stats_t *vitte_module_stats(const vitte_module_t *module) {
    return module != NULL ? &module->stats : NULL;
}

vitte_status_t vitte_module_set_name(vitte_module_t *module, const char *module_name) {
    if (!vitte_module_is_initialized(module) || !vitte_import_validate_module_name(module_name)) {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_MODULE_E_NAME", "invalid module name", module_name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_module_copy_text(module->module_name, sizeof(module->module_name), module_name)) {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_MODULE_E_NAME", "module name too long", module_name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (module->state < VITTE_MODULE_STATE_CREATED) {
        module->state = VITTE_MODULE_STATE_CREATED;
    }
    vitte_error_reset(&module->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_module_set_source_path(vitte_module_t *module, const char *path) {
    vitte_fs_path_t fs_path;

    if (!vitte_module_is_initialized(module) || path == NULL || path[0] == '\0') {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_MODULE_E_PATH", "invalid module source path", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (vitte_fs_path_from_cstr(&fs_path, path) != VITTE_STATUS_OK ||
        !vitte_module_copy_text(module->source_path, sizeof(module->source_path), fs_path.text)) {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_MODULE_E_PATH", "invalid module source path", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    module->source_name = module->source_path;
    if (module->state < VITTE_MODULE_STATE_RESOLVED) {
        module->state = VITTE_MODULE_STATE_RESOLVED;
    }
    vitte_error_reset(&module->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_module_attach_source(
    vitte_module_t *module,
    const char *source_name,
    char *source,
    size_t source_size,
    bool take_ownership
) {
    if (!vitte_module_is_initialized(module) || source == NULL) {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_MODULE_E_SOURCE", "invalid module source attachment", source_name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (source_size == 0u) {
        source_size = strlen(source);
    }
    if (source_size > module->options.max_source_bytes) {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_MODULE_E_SOURCE", "module source exceeds size limit", source_name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_module_release_source(module);
    module->source_buffer = source;
    module->source_size = source_size;
    module->source_owned = take_ownership;
    module->source_name = source_name != NULL ? source_name : module->source_path;
    module->state = VITTE_MODULE_STATE_LOADED;
    module->stats.load_count++;
    vitte_module_update_stats(module);

    if (module->options.lex_on_load) {
        return vitte_module_lex(module);
    }
    vitte_error_reset(&module->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_module_load_source(vitte_module_t *module) {
    vitte_fs_options_t options;
    vitte_error_t fs_error;
    char *buffer = NULL;
    size_t bytes_read = 0u;
    vitte_status_t status;

    if (!vitte_module_is_initialized(module) || module->source_path[0] == '\0') {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_MODULE_E_SOURCE", "module source path is not set", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    vitte_fs_options_init(&options);
    options.max_file_bytes = module->options.max_source_bytes;
    options.null_terminate_reads = true;
    vitte_error_init(&fs_error);
    status = vitte_fs_read_all_alloc(module->source_path, &buffer, &bytes_read, &options, &fs_error);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&module->last_error, &fs_error);
        module->state = VITTE_MODULE_STATE_FAILED;
        module->stats.error_count++;
        return status;
    }
    return vitte_module_attach_source(module, module->source_path, buffer, bytes_read, true);
}

vitte_status_t vitte_module_resolve(
    vitte_module_t *module,
    vitte_import_resolver_t *resolver,
    const char *module_name
) {
    vitte_import_request_t request;
    vitte_import_result_t result;
    vitte_status_t status;

    if (!vitte_module_is_initialized(module) || !vitte_import_resolver_is_initialized(resolver) ||
        !vitte_import_validate_module_name(module_name)) {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_MODULE_E_RESOLVE", "invalid module resolve request", module_name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    vitte_import_request_init(&request);
    request.module_name = module_name;
    request.read_source = false;
    vitte_import_result_init(&result);
    status = vitte_import_resolve(resolver, &request, &result);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&module->last_error, &result.error);
        module->state = VITTE_MODULE_STATE_FAILED;
        module->stats.error_count++;
        vitte_import_result_destroy(&result);
        return status;
    }

    if (vitte_module_set_name(module, module_name) != VITTE_STATUS_OK ||
        vitte_module_set_source_path(module, result.resolved_path.text) != VITTE_STATUS_OK) {
        vitte_import_result_destroy(&result);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    vitte_import_result_destroy(&result);
    if (module->options.read_source) {
        return vitte_module_load_source(module);
    }
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_module_lex(vitte_module_t *module) {
    vitte_lexer_t lexer;
    vitte_token_t token;
    vitte_status_t status;
    size_t token_count = 0u;
    size_t error_count = 0u;

    if (!vitte_module_is_initialized(module) || module->source_buffer == NULL) {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_MODULE_E_LEX", "module source is not loaded", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    status = vitte_lexer_init(&lexer, module->source_name, module->source_buffer, module->source_size, &module->options.lexer_options);
    if (status != VITTE_STATUS_OK) {
        vitte_module_set_error(module, status, "VITTE_MODULE_E_LEX", "failed to initialize module lexer", module->source_name);
        return status;
    }

    for (;;) {
        status = vitte_lexer_next(&lexer, &token);
        token_count++;
        module->last_token = token;
        if (token.kind == VITTE_TOKEN_ERROR) {
            error_count++;
        }
        if (status != VITTE_STATUS_OK || token.kind == VITTE_TOKEN_EOF || token.kind == VITTE_TOKEN_ERROR) {
            break;
        }
    }

    module->token_count = token_count;
    module->stats.lex_count++;
    module->stats.error_count += error_count;
    vitte_module_update_stats(module);
    if (status != VITTE_STATUS_OK) {
        vitte_error_copy(&module->last_error, vitte_lexer_last_error(&lexer));
        module->state = VITTE_MODULE_STATE_FAILED;
        return status;
    }
    module->state = VITTE_MODULE_STATE_LEXED;
    vitte_error_reset(&module->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_module_add_import(vitte_module_t *module, const char *module_name, bool relative) {
    size_t index;
    vitte_module_import_t *entry;

    if (!vitte_module_is_initialized(module) || !vitte_import_validate_module_name(module_name)) {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_MODULE_E_IMPORT", "invalid module import name", module_name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    for (index = 0u; index < module->import_count; index++) {
        if (strcmp(module->imports[index].module_name, module_name) == 0 &&
            module->imports[index].relative == relative) {
            vitte_error_reset(&module->last_error);
            return VITTE_STATUS_OK;
        }
    }
    if (module->import_count >= module->options.max_imports) {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_MODULE_E_IMPORT", "module import capacity exceeded", module_name);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    entry = &module->imports[module->import_count];
    vitte_module_import_reset(entry);
    if (!vitte_module_copy_text(entry->module_name, sizeof(entry->module_name), module_name)) {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_MODULE_E_IMPORT", "module import name too long", module_name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    entry->relative = relative;
    module->import_count++;
    vitte_module_update_stats(module);
    vitte_error_reset(&module->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_module_resolve_imports(vitte_module_t *module, vitte_import_resolver_t *resolver) {
    size_t index;

    if (!vitte_module_is_initialized(module) || !vitte_import_resolver_is_initialized(resolver)) {
        vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_MODULE_E_IMPORT", "invalid import resolver", NULL);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    module->resolved_import_count = 0u;
    for (index = 0u; index < module->import_count; index++) {
        vitte_import_request_t request;
        vitte_import_result_t result;
        vitte_module_import_t *entry = &module->imports[index];
        vitte_status_t status;

        vitte_import_request_init(&request);
        request.module_name = entry->module_name;
        request.importer_path = module->source_path[0] != '\0' ? module->source_path : NULL;
        request.relative = entry->relative;
        request.read_source = false;
        vitte_module_import_release_source(entry);
        entry->resolved = false;
        entry->resolved_path[0] = '\0';
        vitte_import_result_init(&result);
        status = vitte_import_resolve(resolver, &request, &result);
        if (status != VITTE_STATUS_OK) {
            vitte_error_copy(&entry->last_error, &result.error);
            vitte_error_copy(&module->last_error, &result.error);
            module->state = VITTE_MODULE_STATE_FAILED;
            module->stats.error_count++;
            vitte_import_result_destroy(&result);
            return status;
        }

        if (!vitte_module_copy_text(entry->resolved_path, sizeof(entry->resolved_path), result.resolved_path.text)) {
            vitte_import_result_destroy(&result);
            vitte_module_set_error(module, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_MODULE_E_IMPORT", "resolved import path too long", entry->module_name);
            return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
        }
        entry->resolved = true;
        entry->source_loaded = false;
        vitte_error_reset(&entry->last_error);

        if (module->options.read_source) {
            vitte_fs_options_t fs_options;
            vitte_error_t fs_error;
            char *buffer = NULL;
            size_t bytes_read = 0u;

            vitte_fs_options_init(&fs_options);
            fs_options.max_file_bytes = module->options.max_source_bytes;
            fs_options.null_terminate_reads = true;
            vitte_error_init(&fs_error);
            status = vitte_fs_read_all_alloc(entry->resolved_path, &buffer, &bytes_read, &fs_options, &fs_error);
            if (status != VITTE_STATUS_OK) {
                vitte_error_copy(&entry->last_error, &fs_error);
                vitte_error_copy(&module->last_error, &fs_error);
                module->state = VITTE_MODULE_STATE_FAILED;
                module->stats.error_count++;
                vitte_import_result_destroy(&result);
                return status;
            }
            entry->source_buffer = buffer;
            entry->source_size = bytes_read;
            entry->source_loaded = true;
            entry->source_owned = true;
        }

        module->resolved_import_count++;
        vitte_import_result_destroy(&result);
    }

    vitte_module_update_stats(module);
    if (module->state < VITTE_MODULE_STATE_RESOLVED) {
        module->state = VITTE_MODULE_STATE_RESOLVED;
    }
    vitte_error_reset(&module->last_error);
    return VITTE_STATUS_OK;
}
