#ifndef VITTE_BOOTSTRAP_IMPORT_H
#define VITTE_BOOTSTRAP_IMPORT_H

#include <stdbool.h>
#include <stddef.h>

#include "../api/error.h"
#include "../filesystem/filesystem.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_IMPORT_MAX_MODULE_NAME ((size_t)256u)
#define VITTE_IMPORT_MAX_SEARCH_PATHS ((size_t)32u)
#define VITTE_IMPORT_MAX_CACHE_ENTRIES ((size_t)128u)
#define VITTE_IMPORT_MAX_DEPTH ((size_t)64u)
#define VITTE_IMPORT_MAX_SOURCE_BYTES VITTE_FS_DEFAULT_MAX_FILE_BYTES

typedef struct vitte_import_path {
    char text[VITTE_FS_MAX_PATH];
    size_t length;
} vitte_import_path_t;

typedef struct vitte_import_search_path {
    vitte_import_path_t path;
} vitte_import_search_path_t;

typedef struct vitte_import_options {
    bool read_source;
    bool use_cache;
    size_t max_depth;
    size_t max_source_bytes;
} vitte_import_options_t;

typedef struct vitte_import_request {
    const char *module_name;
    const char *importer_path;
    bool relative;
    bool read_source;
    size_t depth;
} vitte_import_request_t;

typedef struct vitte_import_module {
    char name[VITTE_IMPORT_MAX_MODULE_NAME];
    vitte_import_path_t resolved_path;
    char *source;
    size_t source_size;
    bool source_loaded;
} vitte_import_module_t;

typedef struct vitte_import_result {
    vitte_status_t status;
    char module_name[VITTE_IMPORT_MAX_MODULE_NAME];
    vitte_import_path_t resolved_path;
    char *source;
    size_t source_size;
    bool source_loaded;
    bool source_owned;
    bool from_cache;
    vitte_error_t error;
} vitte_import_result_t;

typedef struct vitte_import_cache_entry {
    bool occupied;
    char module_name[VITTE_IMPORT_MAX_MODULE_NAME];
    char base_path[VITTE_FS_MAX_PATH];
    vitte_import_module_t module;
} vitte_import_cache_entry_t;

typedef struct vitte_import_stats {
    size_t resolve_count;
    size_t cache_hit_count;
    size_t cache_miss_count;
    size_t failed_count;
    size_t bytes_read;
    size_t search_path_count;
    size_t cache_entry_count;
} vitte_import_stats_t;

typedef struct vitte_import_resolver {
    bool initialized;
    vitte_import_options_t options;
    vitte_import_search_path_t search_paths[VITTE_IMPORT_MAX_SEARCH_PATHS];
    size_t search_path_count;
    vitte_import_cache_entry_t cache[VITTE_IMPORT_MAX_CACHE_ENTRIES];
    vitte_import_stats_t stats;
    vitte_error_t last_error;
} vitte_import_resolver_t;

void vitte_import_options_init(vitte_import_options_t *options);
void vitte_import_request_init(vitte_import_request_t *request);
void vitte_import_result_init(vitte_import_result_t *result);
void vitte_import_result_reset(vitte_import_result_t *result);
void vitte_import_result_destroy(vitte_import_result_t *result);
void vitte_import_stats_init(vitte_import_stats_t *stats);

bool vitte_import_validate_module_name(const char *module_name);
vitte_status_t vitte_import_module_name_to_path(
    const char *module_name,
    vitte_import_path_t *out,
    vitte_error_t *error
);

vitte_status_t vitte_import_resolver_init(
    vitte_import_resolver_t *resolver,
    const vitte_import_options_t *options
);
void vitte_import_resolver_destroy(vitte_import_resolver_t *resolver);
bool vitte_import_resolver_is_initialized(const vitte_import_resolver_t *resolver);
const vitte_error_t *vitte_import_resolver_last_error(const vitte_import_resolver_t *resolver);

vitte_status_t vitte_import_resolver_add_search_path(
    vitte_import_resolver_t *resolver,
    const char *path
);
void vitte_import_resolver_clear_search_paths(vitte_import_resolver_t *resolver);
void vitte_import_resolver_clear_cache(vitte_import_resolver_t *resolver);
const vitte_import_stats_t *vitte_import_resolver_stats(const vitte_import_resolver_t *resolver);

vitte_status_t vitte_import_resolve(
    vitte_import_resolver_t *resolver,
    const vitte_import_request_t *request,
    vitte_import_result_t *result
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_IMPORT_H */
