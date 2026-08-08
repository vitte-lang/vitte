#ifndef VITTE_BOOTSTRAP_CONSTANT_FOLD_H
#define VITTE_BOOTSTRAP_CONSTANT_FOLD_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "../api/error.h"
#include "../ast/ast.h"
#include "../builtin/builtin.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum vitte_constant_kind {
    VITTE_CONSTANT_NONE = 0,
    VITTE_CONSTANT_BOOL,
    VITTE_CONSTANT_INT,
    VITTE_CONSTANT_I64,
    VITTE_CONSTANT_STRING,
    VITTE_CONSTANT_ERROR
} vitte_constant_kind_t;

typedef struct vitte_constant_options {
    size_t max_depth;
    bool allow_builtin_constants;
    bool allow_string_concat;
    bool detect_overflow;
} vitte_constant_options_t;

typedef struct vitte_constant_stats {
    size_t expressions_seen;
    size_t expressions_folded;
    size_t builtin_constants_resolved;
    size_t overflow_count;
    size_t divide_by_zero_count;
    size_t unsupported_count;
    size_t error_count;
} vitte_constant_stats_t;

typedef struct vitte_constant_value {
    vitte_constant_kind_t kind;
    vitte_builtin_type_kind_t type;
    bool valid;
    union {
        bool boolean;
        int64_t integer;
        const char *string;
    } as;
} vitte_constant_value_t;

typedef struct vitte_constant_result {
    vitte_status_t status;
    bool folded;
    vitte_constant_value_t value;
    const vitte_ast_expr_t *source;
    const char *diagnostic;
} vitte_constant_result_t;

typedef struct vitte_constant_folder {
    bool initialized;
    vitte_constant_options_t options;
    vitte_constant_stats_t stats;
    vitte_builtin_registry_t builtins;
    vitte_error_t last_error;
} vitte_constant_folder_t;

void vitte_constant_options_init(vitte_constant_options_t *options);
void vitte_constant_value_init(vitte_constant_value_t *value);
void vitte_constant_result_init(vitte_constant_result_t *result);

vitte_status_t vitte_constant_folder_init(
    vitte_constant_folder_t *folder,
    const vitte_constant_options_t *options
);

void vitte_constant_folder_destroy(vitte_constant_folder_t *folder);
bool vitte_constant_folder_is_initialized(const vitte_constant_folder_t *folder);
const vitte_error_t *vitte_constant_folder_last_error(const vitte_constant_folder_t *folder);
void vitte_constant_folder_clear_error(vitte_constant_folder_t *folder);
const vitte_constant_stats_t *vitte_constant_folder_stats(const vitte_constant_folder_t *folder);

vitte_status_t vitte_constant_eval_expr(
    vitte_constant_folder_t *folder,
    const vitte_ast_expr_t *expr,
    vitte_constant_result_t *result
);

const char *vitte_constant_kind_name(vitte_constant_kind_t kind);
bool vitte_constant_value_is_truthy(const vitte_constant_value_t *value);
bool vitte_constant_values_equal(const vitte_constant_value_t *left, const vitte_constant_value_t *right);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_CONSTANT_FOLD_H */
