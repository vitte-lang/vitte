#ifndef VITTE_BOOTSTRAP_SEMA_H
#define VITTE_BOOTSTRAP_SEMA_H

#include <stdbool.h>
#include <stddef.h>

#include "../api/error.h"
#include "../ast/ast.h"
#include "../builtin/builtin.h"
#include "../constant_fold/constant.h"
#include "../diagnostic/diagnostic.h"
#include "../scope/scope.h"
#include "../symbol/symbol.h"
#include "../type/type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_sema_options {
    size_t max_depth;
    bool require_main_proc;
    bool allow_shadowing;
    bool enable_constant_folding;
} vitte_sema_options_t;

typedef struct vitte_sema_stats {
    size_t decl_count;
    size_t stmt_count;
    size_t expr_count;
    size_t symbol_count;
    size_t scope_push_count;
    size_t constant_fold_count;
    size_t error_count;
    size_t warning_count;
} vitte_sema_stats_t;

typedef struct vitte_sema_result {
    vitte_status_t status;
    size_t error_count;
    size_t warning_count;
    bool main_found;
    vitte_error_t last_error;
} vitte_sema_result_t;

typedef struct vitte_sema {
    bool initialized;
    vitte_sema_options_t options;
    const vitte_ast_t *ast;
    vitte_diagnostic_bag_t *diagnostics;
    vitte_type_registry_t types;
    vitte_symbol_table_t symbols;
    vitte_scope_stack_t scopes;
    vitte_builtin_registry_t builtins;
    vitte_constant_folder_t constants;
    const vitte_type_t *current_return_type;
    const vitte_symbol_t *current_function;
    size_t depth;
    bool main_found;
    vitte_sema_stats_t stats;
    vitte_error_t last_error;
} vitte_sema_t;

void vitte_sema_options_init(vitte_sema_options_t *options);
void vitte_sema_stats_init(vitte_sema_stats_t *stats);
void vitte_sema_result_init(vitte_sema_result_t *result);

vitte_status_t vitte_sema_init(
    vitte_sema_t *sema,
    const vitte_sema_options_t *options,
    vitte_diagnostic_bag_t *diagnostics
);
void vitte_sema_destroy(vitte_sema_t *sema);
bool vitte_sema_is_initialized(const vitte_sema_t *sema);
const vitte_error_t *vitte_sema_last_error(const vitte_sema_t *sema);
const vitte_sema_stats_t *vitte_sema_stats(const vitte_sema_t *sema);

vitte_status_t vitte_sema_analyze(
    vitte_sema_t *sema,
    const vitte_ast_t *ast,
    vitte_sema_result_t *result
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_SEMA_H */
