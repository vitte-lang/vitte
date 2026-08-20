#include "import.h"

#include <ctype.h>
#include <string.h>

static void vitte_import_set_error(
    vitte_error_t *error,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (error != NULL) {
        vitte_error_set_details(error, status, code, message, details);
    }
}

static void vitte_import_resolver_set_error(
    vitte_import_resolver_t *resolver,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (resolver != NULL) {
        vitte_import_set_error(&resolver->last_error, status, code, message, details);
    }
}

static bool vitte_import_copy_text(char *destination, size_t capacity, const char *source) {
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

static bool vitte_import_path_from_fs(vitte_import_path_t *out, const vitte_fs_path_t *path) {
    if (out == NULL || path == NULL || path->length >= VITTE_FS_MAX_PATH) {
        return false;
    }
    (void)memcpy(out->text, path->text, path->length + 1u);
    out->length = path->length;
    return true;
}

static bool vitte_import_path_from_cstr(vitte_import_path_t *out, const char *path) {
    vitte_fs_path_t fs_path;

    if (out == NULL || vitte_fs_path_from_cstr(&fs_path, path) != VITTE_STATUS_OK) {
        return false;
    }
    return vitte_import_path_from_fs(out, &fs_path);
}

void vitte_import_options_init(vitte_import_options_t *options) {
    if (options == NULL) {
        return;
    }
    memset(options, 0, sizeof(*options));
    options->read_source = true;
    options->use_cache = true;
    options->max_depth = VITTE_IMPORT_MAX_DEPTH;
    options->max_source_bytes = VITTE_IMPORT_MAX_SOURCE_BYTES;
}

void vitte_import_request_init(vitte_import_request_t *request) {
    if (request == NULL) {
        return;
    }
    memset(request, 0, sizeof(*request));
    request->read_source = true;
}

void vitte_import_result_init(vitte_import_result_t *result) {
    if (result == NULL) {
        return;
    }
    memset(result, 0, sizeof(*result));
    result->status = VITTE_STATUS_OK;
    vitte_error_init(&result->error);
}

void vitte_import_result_destroy(vitte_import_result_t *result) {
    if (result == NULL) {
        return;
    }
    if (result->source_owned) {
        vitte_fs_free(result->source);
    }
    memset(result, 0, sizeof(*result));
}

void vitte_import_result_reset(vitte_import_result_t *result) {
    vitte_import_result_destroy(result);
    vitte_import_result_init(result);
}

void vitte_import_stats_init(vitte_import_stats_t *stats) {
    if (stats == NULL) {
        return;
    }
    memset(stats, 0, sizeof(*stats));
}

bool vitte_import_validate_module_name(const char *module_name) {
    const unsigned char *cursor;
    bool previous_separator = false;

    if (module_name == NULL || module_name[0] == '\0' || strlen(module_name) >= VITTE_IMPORT_MAX_MODULE_NAME) {
        return false;
    }
    if (strstr(module_name, "..") != NULL) {
        return false;
    }
    for (cursor = (const unsigned char *)module_name; *cursor != '\0'; cursor++) {
        bool separator = *cursor == '.' || *cursor == '/';
        if (!(isalnum(*cursor) || *cursor == '_' || separator)) {
            return false;
        }
        if (separator && previous_separator) {
            return false;
        }
        previous_separator = separator;
    }
    return !previous_separator;
}

vitte_status_t vitte_import_module_name_to_path(
    const char *module_name,
    vitte_import_path_t *out,
    vitte_error_t *error
) {
    size_t index;
    size_t length;

    if (out != NULL) {
        out->text[0] = '\0';
        out->length = 0u;
    }
    if (out == NULL || !vitte_import_validate_module_name(module_name)) {
        vitte_import_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IMPORT_E_MODULE", "invalid import module name", module_name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    length = strlen(module_name);
    if (length + 4u >= VITTE_FS_MAX_PATH) {
        vitte_import_set_error(error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IMPORT_E_PATH", "import module path is too long", module_name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    for (index = 0u; index < length; index++) {
        out->text[index] = module_name[index] == '.' ? '/' : module_name[index];
    }
    (void)memcpy(out->text + length, ".vit", 5u);
    out->length = length + 4u;
    if (error != NULL) {
        vitte_error_reset(error);
    }
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_import_resolver_init(
    vitte_import_resolver_t *resolver,
    const vitte_import_options_t *options
) {
    vitte_import_options_t defaults;

    if (resolver == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    memset(resolver, 0, sizeof(*resolver));
    if (options == NULL) {
        vitte_import_options_init(&defaults);
        resolver->options = defaults;
    } else {
        resolver->options = *options;
    }
    if (resolver->options.max_depth == 0u || resolver->options.max_depth > VITTE_IMPORT_MAX_DEPTH) {
        resolver->options.max_depth = VITTE_IMPORT_MAX_DEPTH;
    }
    if (resolver->options.max_source_bytes == 0u) {
        resolver->options.max_source_bytes = VITTE_IMPORT_MAX_SOURCE_BYTES;
    }
    vitte_import_stats_init(&resolver->stats);
    vitte_error_init(&resolver->last_error);
    resolver->initialized = true;
    return VITTE_STATUS_OK;
}

bool vitte_import_resolver_is_initialized(const vitte_import_resolver_t *resolver) {
    return resolver != NULL && resolver->initialized;
}

const vitte_error_t *vitte_import_resolver_last_error(const vitte_import_resolver_t *resolver) {
    return resolver != NULL ? &resolver->last_error : vitte_error_last();
}

void vitte_import_resolver_clear_cache(vitte_import_resolver_t *resolver) {
    size_t index;

    if (resolver == NULL) {
        return;
    }
    for (index = 0u; index < VITTE_IMPORT_MAX_CACHE_ENTRIES; index++) {
        if (resolver->cache[index].occupied && resolver->cache[index].module.source != NULL) {
            vitte_fs_free(resolver->cache[index].module.source);
        }
        memset(&resolver->cache[index], 0, sizeof(resolver->cache[index]));
    }
    resolver->stats.cache_entry_count = 0u;
}

void vitte_import_resolver_destroy(vitte_import_resolver_t *resolver) {
    if (resolver == NULL) {
        return;
    }
    vitte_import_resolver_clear_cache(resolver);
    memset(resolver, 0, sizeof(*resolver));
}

void vitte_import_resolver_clear_search_paths(vitte_import_resolver_t *resolver) {
    if (resolver == NULL) {
        return;
    }
    memset(resolver->search_paths, 0, sizeof(resolver->search_paths));
    resolver->search_path_count = 0u;
    resolver->stats.search_path_count = 0u;
}

vitte_status_t vitte_import_resolver_add_search_path(
    vitte_import_resolver_t *resolver,
    const char *path
) {
    vitte_import_path_t import_path;
    size_t index;

    if (!vitte_import_resolver_is_initialized(resolver) || path == NULL || path[0] == '\0') {
        vitte_import_resolver_set_error(resolver, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IMPORT_E_ARGUMENT", "invalid import search path", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (!vitte_import_path_from_cstr(&import_path, path)) {
        vitte_import_resolver_set_error(resolver, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IMPORT_E_PATH", "invalid import search path", path);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    for (index = 0u; index < resolver->search_path_count; index++) {
        if (strcmp(resolver->search_paths[index].path.text, import_path.text) == 0) {
            vitte_error_reset(&resolver->last_error);
            return VITTE_STATUS_OK;
        }
    }
    if (resolver->search_path_count >= VITTE_IMPORT_MAX_SEARCH_PATHS) {
        vitte_import_resolver_set_error(resolver, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IMPORT_E_PATH", "too many import search paths", path);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    resolver->search_paths[resolver->search_path_count].path = import_path;
    resolver->search_path_count++;
    resolver->stats.search_path_count = resolver->search_path_count;
    vitte_error_reset(&resolver->last_error);
    return VITTE_STATUS_OK;
}

const vitte_import_stats_t *vitte_import_resolver_stats(const vitte_import_resolver_t *resolver) {
    return resolver != NULL ? &resolver->stats : NULL;
}

static const char *vitte_import_request_base_path(const vitte_import_request_t *request, char *buffer, size_t capacity) {
    vitte_fs_path_t parent;

    if (buffer == NULL || capacity == 0u) {
        return "";
    }
    buffer[0] = '\0';
    if (request == NULL || request->importer_path == NULL || request->importer_path[0] == '\0') {
        return buffer;
    }
    if (vitte_fs_parent_path(request->importer_path, &parent) != VITTE_STATUS_OK) {
        return buffer;
    }
    if (!vitte_import_copy_text(buffer, capacity, parent.text)) {
        buffer[0] = '\0';
    }
    return buffer;
}

static vitte_import_cache_entry_t *vitte_import_cache_find(
    vitte_import_resolver_t *resolver,
    const char *module_name,
    const char *base_path
) {
    size_t index;

    if (resolver == NULL || module_name == NULL || base_path == NULL || !resolver->options.use_cache) {
        return NULL;
    }
    for (index = 0u; index < VITTE_IMPORT_MAX_CACHE_ENTRIES; index++) {
        if (resolver->cache[index].occupied &&
            strcmp(resolver->cache[index].module_name, module_name) == 0 &&
            strcmp(resolver->cache[index].base_path, base_path) == 0) {
            return &resolver->cache[index];
        }
    }
    return NULL;
}

static vitte_import_cache_entry_t *vitte_import_cache_slot(vitte_import_resolver_t *resolver) {
    size_t index;

    if (resolver == NULL || !resolver->options.use_cache) {
        return NULL;
    }
    for (index = 0u; index < VITTE_IMPORT_MAX_CACHE_ENTRIES; index++) {
        if (!resolver->cache[index].occupied) {
            return &resolver->cache[index];
        }
    }
    return NULL;
}

static void vitte_import_result_from_cache(vitte_import_result_t *result, const vitte_import_cache_entry_t *entry) {
    if (result == NULL || entry == NULL) {
        return;
    }
    vitte_import_result_reset(result);
    result->status = VITTE_STATUS_OK;
    (void)vitte_import_copy_text(result->module_name, sizeof(result->module_name), entry->module.name);
    result->resolved_path = entry->module.resolved_path;
    result->source = entry->module.source;
    result->source_size = entry->module.source_size;
    result->source_loaded = entry->module.source_loaded;
    result->source_owned = false;
    result->from_cache = true;
    vitte_error_reset(&result->error);
}

static bool vitte_import_try_candidate(
    const vitte_import_path_t *base,
    const vitte_import_path_t *relative,
    vitte_import_path_t *out
) {
    vitte_fs_path_t fs_base;
    vitte_fs_path_t fs_out;

    if (base == NULL || relative == NULL || out == NULL) {
        return false;
    }
    if (vitte_fs_path_from_cstr(&fs_base, base->text) != VITTE_STATUS_OK) {
        return false;
    }
    if (vitte_fs_path_join(&fs_out, &fs_base, relative->text) != VITTE_STATUS_OK) {
        return false;
    }
    if (!vitte_fs_is_file(fs_out.text)) {
        vitte_import_path_t alternate = *relative;
        if (alternate.length < 4u || strcmp(alternate.text + alternate.length - 4u, ".vit") != 0 || alternate.length + 1u >= VITTE_FS_MAX_PATH) {
            return false;
        }
        alternate.text[alternate.length++] = 'l';
        alternate.text[alternate.length] = '\0';
        if (vitte_fs_path_join(&fs_out, &fs_base, alternate.text) != VITTE_STATUS_OK) {
            return false;
        }
        if (!vitte_fs_is_file(fs_out.text)) {
            return false;
        }
    }
    return vitte_import_path_from_fs(out, &fs_out);
}

static bool vitte_import_make_mod_candidate(const vitte_import_path_t *module_path, vitte_import_path_t *out) {
    size_t base_length;
    static const char suffix[] = "/mod.vit";

    if (module_path == NULL || out == NULL || module_path->length < 5u) {
        return false;
    }
    base_length = module_path->length - 4u;
    if (base_length + sizeof(suffix) >= VITTE_FS_MAX_PATH) {
        return false;
    }
    (void)memcpy(out->text, module_path->text, base_length);
    (void)memcpy(out->text + base_length, suffix, sizeof(suffix));
    out->length = base_length + sizeof(suffix) - 1u;
    return true;
}

static vitte_status_t vitte_import_read_source(
    vitte_import_resolver_t *resolver,
    vitte_import_module_t *module
) {
    vitte_fs_options_t options;
    vitte_error_t fs_error;
    vitte_status_t status;

    if (resolver == NULL || module == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    vitte_fs_options_init(&options);
    options.max_file_bytes = resolver->options.max_source_bytes;
    options.null_terminate_reads = true;
    vitte_error_init(&fs_error);
    status = vitte_fs_read_all_alloc(module->resolved_path.text, &module->source, &module->source_size, &options, &fs_error);
    if (status != VITTE_STATUS_OK) {
        vitte_import_resolver_set_error(resolver, status, "VITTE_IMPORT_E_READ", "failed to read imported module source", module->resolved_path.text);
        return status;
    }
    module->source_loaded = true;
    resolver->stats.bytes_read += module->source_size;
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_import_cache_store(
    vitte_import_resolver_t *resolver,
    const char *module_name,
    const char *base_path,
    const vitte_import_path_t *resolved_path,
    bool read_source,
    vitte_import_cache_entry_t **stored
) {
    vitte_import_cache_entry_t *slot;
    vitte_status_t status;

    if (stored != NULL) {
        *stored = NULL;
    }
    if (resolver == NULL || module_name == NULL || base_path == NULL || resolved_path == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    slot = vitte_import_cache_slot(resolver);
    if (slot == NULL) {
        /* The bootstrap graph can contain more importer bases than the bounded cache.
         * Reclaim the cache and continue resolving deterministically. */
        vitte_import_resolver_clear_cache(resolver);
        slot = &resolver->cache[0];
    }
    memset(slot, 0, sizeof(*slot));
    slot->occupied = true;
    if (!vitte_import_copy_text(slot->module_name, sizeof(slot->module_name), module_name) ||
        !vitte_import_copy_text(slot->base_path, sizeof(slot->base_path), base_path) ||
        !vitte_import_copy_text(slot->module.name, sizeof(slot->module.name), module_name)) {
        /* Oversized names are still valid resolver inputs; disable caching for this entry. */
        slot->module_name[0] = '\0';
        slot->base_path[0] = '\0';
        slot->module.name[0] = '\0';
    }
    slot->module.resolved_path = *resolved_path;
    if (read_source) {
        status = vitte_import_read_source(resolver, &slot->module);
        if (status != VITTE_STATUS_OK) {
            memset(slot, 0, sizeof(*slot));
            return status;
        }
    }
    resolver->stats.cache_entry_count++;
    if (stored != NULL) {
        *stored = slot;
    }
    return VITTE_STATUS_OK;
}

static vitte_status_t vitte_import_result_from_module(
    vitte_import_result_t *result,
    const vitte_import_module_t *module,
    bool from_cache
) {
    if (result == NULL || module == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    vitte_import_result_reset(result);
    result->status = VITTE_STATUS_OK;
    if (!vitte_import_copy_text(result->module_name, sizeof(result->module_name), module->name)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    result->resolved_path = module->resolved_path;
    result->source = module->source;
    result->source_size = module->source_size;
    result->source_loaded = module->source_loaded;
    result->source_owned = false;
    result->from_cache = from_cache;
    vitte_error_reset(&result->error);
    return VITTE_STATUS_OK;
}

static bool vitte_import_resolve_relative(
    const vitte_import_request_t *request,
    const vitte_import_path_t *module_path,
    vitte_import_path_t *resolved
) {
    vitte_fs_path_t base;
    vitte_fs_path_t candidate;
    vitte_import_path_t mod_candidate;

    if (request == NULL || request->importer_path == NULL || module_path == NULL || resolved == NULL) {
        return false;
    }
    if (vitte_fs_parent_path(request->importer_path, &base) != VITTE_STATUS_OK) {
        return false;
    }
    if (vitte_fs_path_join(&candidate, &base, module_path->text) == VITTE_STATUS_OK &&
        vitte_fs_is_file(candidate.text)) {
        return vitte_import_path_from_fs(resolved, &candidate);
    }
    if (vitte_import_make_mod_candidate(module_path, &mod_candidate) &&
        vitte_fs_path_join(&candidate, &base, mod_candidate.text) == VITTE_STATUS_OK &&
        vitte_fs_is_file(candidate.text)) {
        return vitte_import_path_from_fs(resolved, &candidate);
    }
    return false;
}

static bool vitte_import_resolve_search_paths(
    vitte_import_resolver_t *resolver,
    const vitte_import_path_t *module_path,
    vitte_import_path_t *resolved
) {
    size_t index;
    vitte_import_path_t mod_candidate;

    if (resolver == NULL || module_path == NULL || resolved == NULL) {
        return false;
    }
    for (index = 0u; index < resolver->search_path_count; index++) {
        if (vitte_import_try_candidate(&resolver->search_paths[index].path, module_path, resolved)) {
            return true;
        }
        if (vitte_import_make_mod_candidate(module_path, &mod_candidate) &&
            vitte_import_try_candidate(&resolver->search_paths[index].path, &mod_candidate, resolved)) {
            return true;
        }
    }
    return false;
}

static bool vitte_import_resolve_ancestor_paths(
    const vitte_import_request_t *request,
    const vitte_import_path_t *module_path,
    vitte_import_path_t *resolved
) {
    vitte_fs_path_t current;
    vitte_fs_path_t parent;
    vitte_import_path_t base;
    vitte_import_path_t mod_candidate;
    bool has_mod_candidate;
    size_t depth;

    if (request == NULL || request->importer_path == NULL ||
        request->importer_path[0] == '\0' || module_path == NULL || resolved == NULL) {
        return false;
    }
    if (vitte_fs_parent_path(request->importer_path, &current) != VITTE_STATUS_OK) {
        return false;
    }
    has_mod_candidate = vitte_import_make_mod_candidate(module_path, &mod_candidate);
    for (depth = 0u; depth < VITTE_IMPORT_MAX_DEPTH; depth++) {
        if (!vitte_import_path_from_fs(&base, &current)) {
            return false;
        }
        if (vitte_import_try_candidate(&base, module_path, resolved) ||
            (has_mod_candidate && vitte_import_try_candidate(&base, &mod_candidate, resolved))) {
            return true;
        }
        if (vitte_fs_parent_path(current.text, &parent) != VITTE_STATUS_OK ||
            strcmp(parent.text, current.text) == 0) {
            break;
        }
        current = parent;
    }
    return false;
}

vitte_status_t vitte_import_resolve(
    vitte_import_resolver_t *resolver,
    const vitte_import_request_t *request,
    vitte_import_result_t *result
) {
    vitte_import_path_t module_path;
    vitte_import_path_t resolved_path;
    vitte_import_cache_entry_t *entry;
    vitte_import_cache_entry_t *stored;
    char base_path[VITTE_FS_MAX_PATH];
    bool should_read_source;
    vitte_status_t status;

    if (result != NULL) {
        vitte_import_result_reset(result);
    }
    if (!vitte_import_resolver_is_initialized(resolver) || request == NULL || result == NULL) {
        vitte_import_resolver_set_error(resolver, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IMPORT_E_ARGUMENT", "invalid import resolve arguments", NULL);
        if (result != NULL) {
            vitte_import_set_error(&result->error, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_IMPORT_E_ARGUMENT", "invalid import resolve arguments", NULL);
            result->status = VITTE_STATUS_ERROR_INVALID_ARGUMENT;
        }
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    resolver->stats.resolve_count++;

    if (request->depth > resolver->options.max_depth) {
        resolver->stats.failed_count++;
        vitte_import_resolver_set_error(resolver, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IMPORT_E_DEPTH", "import depth limit exceeded", request->module_name);
        vitte_import_set_error(&result->error, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_IMPORT_E_DEPTH", "import depth limit exceeded", request->module_name);
        result->status = VITTE_STATUS_ERROR_INVALID_STATE;
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    status = vitte_import_module_name_to_path(request->module_name, &module_path, &resolver->last_error);
    if (status != VITTE_STATUS_OK) {
        resolver->stats.failed_count++;
        vitte_error_copy(&result->error, &resolver->last_error);
        result->status = status;
        return status;
    }

    (void)vitte_import_request_base_path(request, base_path, sizeof(base_path));
    entry = vitte_import_cache_find(resolver, request->module_name, base_path);
    should_read_source = request->read_source || resolver->options.read_source;
    if (entry != NULL && (!should_read_source || entry->module.source_loaded)) {
        resolver->stats.cache_hit_count++;
        vitte_import_result_from_cache(result, entry);
        vitte_error_reset(&resolver->last_error);
        return VITTE_STATUS_OK;
    }
    if (entry != NULL && should_read_source && !entry->module.source_loaded) {
        status = vitte_import_read_source(resolver, &entry->module);
        if (status != VITTE_STATUS_OK) {
            resolver->stats.failed_count++;
            vitte_error_copy(&result->error, &resolver->last_error);
            result->status = status;
            return status;
        }
        resolver->stats.cache_hit_count++;
        vitte_import_result_from_cache(result, entry);
        vitte_error_reset(&resolver->last_error);
        return VITTE_STATUS_OK;
    }
    resolver->stats.cache_miss_count++;

    if (request->relative && vitte_import_resolve_relative(request, &module_path, &resolved_path)) {
        /* resolved */
    } else if (vitte_import_resolve_search_paths(resolver, &module_path, &resolved_path)) {
        /* resolved */
    } else if (vitte_import_resolve_ancestor_paths(request, &module_path, &resolved_path)) {
        /* resolved */
    } else {
        resolver->stats.failed_count++;
        vitte_import_resolver_set_error(resolver, VITTE_STATUS_ERROR_IO, "VITTE_IMPORT_E_NOT_FOUND", "import module was not found", request->module_name);
        vitte_import_set_error(&result->error, VITTE_STATUS_ERROR_IO, "VITTE_IMPORT_E_NOT_FOUND", "import module was not found", request->module_name);
        result->status = VITTE_STATUS_ERROR_IO;
        return VITTE_STATUS_ERROR_IO;
    }

    status = vitte_import_cache_store(resolver, request->module_name, base_path, &resolved_path, should_read_source, &stored);
    if (status != VITTE_STATUS_OK) {
        resolver->stats.failed_count++;
        vitte_error_copy(&result->error, &resolver->last_error);
        result->status = status;
        return status;
    }
    status = vitte_import_result_from_module(result, &stored->module, false);
    if (status != VITTE_STATUS_OK) {
        resolver->stats.failed_count++;
        vitte_import_resolver_set_error(resolver, status, "VITTE_IMPORT_E_RESULT", "failed to copy import result", request->module_name);
        vitte_import_set_error(&result->error, status, "VITTE_IMPORT_E_RESULT", "failed to copy import result", request->module_name);
        result->status = status;
        return status;
    }
    vitte_error_reset(&resolver->last_error);
    return VITTE_STATUS_OK;
}
