#ifndef VITTE_BOOTSTRAP_MODULE_H
#define VITTE_BOOTSTRAP_MODULE_H

#include <stdbool.h>
#include <stddef.h>

#include "../api/error.h"
#include "../import/import.h"
#include "../lexer/lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_MODULE_MAX_IMPORTS ((size_t)256u)

typedef struct vitte_ast vitte_ast_t;
typedef struct vitte_hir vitte_hir_t;
typedef struct vitte_ir vitte_ir_t;

typedef enum vitte_module_state {
    VITTE_MODULE_STATE_UNINITIALIZED = 0,
    VITTE_MODULE_STATE_CREATED,
    VITTE_MODULE_STATE_RESOLVED,
    VITTE_MODULE_STATE_LOADED,
    VITTE_MODULE_STATE_LEXED,
    VITTE_MODULE_STATE_PARSED,
    VITTE_MODULE_STATE_LOWERED_HIR,
    VITTE_MODULE_STATE_LOWERED_IR,
    VITTE_MODULE_STATE_FAILED
} vitte_module_state_t;

typedef struct vitte_module_options {
    bool read_source;
    bool lex_on_load;
    bool track_hir;
    bool track_ir;
    size_t max_source_bytes;
    size_t max_imports;
    vitte_lexer_options_t lexer_options;
} vitte_module_options_t;

typedef struct vitte_module_import {
    char module_name[VITTE_IMPORT_MAX_MODULE_NAME];
    char resolved_path[VITTE_FS_MAX_PATH];
    char *source_buffer;
    size_t source_size;
    bool relative;
    bool resolved;
    bool source_loaded;
    bool source_owned;
    vitte_error_t last_error;
} vitte_module_import_t;

typedef struct vitte_module_stats {
    size_t import_count;
    size_t resolved_import_count;
    size_t token_count;
    size_t source_bytes;
    size_t load_count;
    size_t lex_count;
    size_t error_count;
} vitte_module_stats_t;

typedef struct vitte_module_result {
    vitte_status_t status;
    vitte_module_state_t state;
    size_t token_count;
    size_t import_count;
    size_t source_bytes;
    vitte_error_t last_error;
} vitte_module_result_t;

typedef struct vitte_module {
    bool initialized;
    vitte_module_state_t state;
    vitte_module_options_t options;
    char module_name[VITTE_IMPORT_MAX_MODULE_NAME];
    char source_path[VITTE_FS_MAX_PATH];
    const char *source_name;
    char *source_buffer;
    size_t source_size;
    bool source_owned;
    size_t token_count;
    vitte_token_t last_token;
    vitte_module_import_t imports[VITTE_MODULE_MAX_IMPORTS];
    size_t import_count;
    size_t resolved_import_count;
    vitte_ast_t *ast;
    vitte_hir_t *hir;
    vitte_ir_t *ir;
    vitte_module_stats_t stats;
    vitte_error_t last_error;
} vitte_module_t;

void vitte_module_options_init(vitte_module_options_t *options);
void vitte_module_result_init(vitte_module_result_t *result);
void vitte_module_stats_init(vitte_module_stats_t *stats);

const char *vitte_module_state_name(vitte_module_state_t state);

vitte_status_t vitte_module_init(vitte_module_t *module, const vitte_module_options_t *options);
void vitte_module_destroy(vitte_module_t *module);
bool vitte_module_is_initialized(const vitte_module_t *module);
const vitte_error_t *vitte_module_last_error(const vitte_module_t *module);
const vitte_module_stats_t *vitte_module_stats(const vitte_module_t *module);

vitte_status_t vitte_module_set_name(vitte_module_t *module, const char *module_name);
vitte_status_t vitte_module_set_source_path(vitte_module_t *module, const char *path);
vitte_status_t vitte_module_attach_source(
    vitte_module_t *module,
    const char *source_name,
    char *source,
    size_t source_size,
    bool take_ownership
);
vitte_status_t vitte_module_resolve(
    vitte_module_t *module,
    vitte_import_resolver_t *resolver,
    const char *module_name
);
vitte_status_t vitte_module_load_source(vitte_module_t *module);
vitte_status_t vitte_module_lex(vitte_module_t *module);
vitte_status_t vitte_module_add_import(vitte_module_t *module, const char *module_name, bool relative);
vitte_status_t vitte_module_resolve_imports(vitte_module_t *module, vitte_import_resolver_t *resolver);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_MODULE_H */
