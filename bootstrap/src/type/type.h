#ifndef VITTE_BOOTSTRAP_TYPE_H
#define VITTE_BOOTSTRAP_TYPE_H

#include <stdbool.h>
#include <stddef.h>

#include "../api/error.h"
#include "../ast/ast.h"
#include "../builtin/builtin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_TYPE_MAX_PROC_PARAMETERS ((size_t)16u)

typedef enum vitte_type_kind {
    VITTE_TYPE_KIND_INVALID = 0,
    VITTE_TYPE_KIND_BUILTIN,
    VITTE_TYPE_KIND_PROC
} vitte_type_kind_t;

typedef struct vitte_type {
    vitte_type_kind_t kind;
    const char *name;
    vitte_builtin_type_kind_t builtin_kind;
    const struct vitte_type *return_type;
    const struct vitte_type *parameter_types[VITTE_TYPE_MAX_PROC_PARAMETERS];
    size_t arity;
    bool variadic;
    bool valid;
    bool error;
} vitte_type_t;

typedef struct vitte_type_registry {
    bool initialized;
    vitte_builtin_registry_t builtins;
    vitte_type_t builtin_types[VITTE_BUILTIN_TYPE_COUNT];
    vitte_error_t last_error;
} vitte_type_registry_t;

void vitte_type_init_invalid(vitte_type_t *type);
void vitte_type_init_proc(
    vitte_type_t *type,
    const char *name,
    const vitte_type_t *return_type,
    const struct vitte_type *const *parameter_types,
    size_t arity,
    bool variadic
);

vitte_status_t vitte_type_registry_init(vitte_type_registry_t *registry);
void vitte_type_registry_destroy(vitte_type_registry_t *registry);
bool vitte_type_registry_is_initialized(const vitte_type_registry_t *registry);
const vitte_error_t *vitte_type_registry_last_error(const vitte_type_registry_t *registry);

const vitte_type_t *vitte_type_builtin(
    vitte_type_registry_t *registry,
    vitte_builtin_type_kind_t kind
);
const vitte_type_t *vitte_type_lookup(
    vitte_type_registry_t *registry,
    const char *name
);
const vitte_type_t *vitte_type_from_ast(
    vitte_type_registry_t *registry,
    const vitte_ast_type_ref_t *type_ref
);

const char *vitte_type_kind_name(vitte_type_kind_t kind);
const char *vitte_type_name(const vitte_type_t *type);
bool vitte_type_is_valid(const vitte_type_t *type);
bool vitte_type_is_builtin(const vitte_type_t *type);
bool vitte_type_is_proc(const vitte_type_t *type);
bool vitte_type_is_error(const vitte_type_t *type);
bool vitte_type_is_void(const vitte_type_t *type);
bool vitte_type_is_bool(const vitte_type_t *type);
bool vitte_type_is_integer(const vitte_type_t *type);
bool vitte_type_is_numeric(const vitte_type_t *type);
bool vitte_type_is_textual(const vitte_type_t *type);
bool vitte_type_equals(const vitte_type_t *left, const vitte_type_t *right);
bool vitte_type_is_assignable(const vitte_type_t *destination, const vitte_type_t *source);
bool vitte_type_is_condition(const vitte_type_t *type);
const vitte_type_t *vitte_type_proc_parameter(const vitte_type_t *type, size_t index);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_TYPE_H */
