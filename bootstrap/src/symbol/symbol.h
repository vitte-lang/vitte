#ifndef VITTE_BOOTSTRAP_SYMBOL_H
#define VITTE_BOOTSTRAP_SYMBOL_H

#include <stdbool.h>
#include <stddef.h>

#include "../api/error.h"
#include "../ast/ast.h"
#include "../builtin/builtin.h"
#include "../type/type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_SYMBOL_MAX_ENTRIES ((size_t)512u)

typedef enum vitte_symbol_kind {
    VITTE_SYMBOL_KIND_UNKNOWN = 0,
    VITTE_SYMBOL_KIND_CONST,
    VITTE_SYMBOL_KIND_PROC,
    VITTE_SYMBOL_KIND_LOCAL,
    VITTE_SYMBOL_KIND_PARAM,
    VITTE_SYMBOL_KIND_BUILTIN_CONST,
    VITTE_SYMBOL_KIND_BUILTIN_FUNC
} vitte_symbol_kind_t;

typedef struct vitte_symbol {
    vitte_symbol_kind_t kind;
    const char *name;
    const vitte_type_t *type;
    const vitte_ast_node_t *declaration;
    const vitte_builtin_constant_t *builtin_constant;
    const vitte_builtin_function_t *builtin_function;
    vitte_type_t owned_type;
    bool mutable_value;
    bool builtin;
    bool initialized;
} vitte_symbol_t;

typedef struct vitte_symbol_table {
    bool initialized;
    vitte_symbol_t entries[VITTE_SYMBOL_MAX_ENTRIES];
    size_t count;
    vitte_error_t last_error;
} vitte_symbol_table_t;

void vitte_symbol_table_init(vitte_symbol_table_t *table);
void vitte_symbol_table_destroy(vitte_symbol_table_t *table);
bool vitte_symbol_table_is_initialized(const vitte_symbol_table_t *table);
const vitte_error_t *vitte_symbol_table_last_error(const vitte_symbol_table_t *table);
void vitte_symbol_table_clear_error(vitte_symbol_table_t *table);

size_t vitte_symbol_count(const vitte_symbol_table_t *table);
const vitte_symbol_t *vitte_symbol_at(const vitte_symbol_table_t *table, size_t index);
const vitte_symbol_t *vitte_symbol_lookup(const vitte_symbol_table_t *table, const char *name);

vitte_status_t vitte_symbol_define(
    vitte_symbol_table_t *table,
    vitte_symbol_kind_t kind,
    const char *name,
    const vitte_type_t *type,
    const vitte_ast_node_t *declaration,
    bool mutable_value,
    const vitte_symbol_t **out_symbol
);

vitte_status_t vitte_symbol_define_proc(
    vitte_symbol_table_t *table,
    const char *name,
    const vitte_type_t *return_type,
    const vitte_type_t *const *parameter_types,
    size_t arity,
    bool variadic,
    const vitte_ast_node_t *declaration,
    const vitte_symbol_t **out_symbol
);

vitte_status_t vitte_symbol_define_builtin_constant(
    vitte_symbol_table_t *table,
    const vitte_builtin_constant_t *builtin_constant,
    const vitte_type_t *type,
    const vitte_symbol_t **out_symbol
);

vitte_status_t vitte_symbol_define_builtin_function(
    vitte_symbol_table_t *table,
    const vitte_builtin_function_t *builtin_function,
    const vitte_type_t *return_type,
    const vitte_symbol_t **out_symbol
);

const char *vitte_symbol_kind_name(vitte_symbol_kind_t kind);
bool vitte_symbol_kind_is_value(vitte_symbol_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_SYMBOL_H */
