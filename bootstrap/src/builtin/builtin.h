#ifndef VITTE_BOOTSTRAP_BUILTIN_H
#define VITTE_BOOTSTRAP_BUILTIN_H

#include <stdbool.h>
#include <stddef.h>

#include "../api/error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum vitte_builtin_kind {
    VITTE_BUILTIN_KIND_TYPE = 0,
    VITTE_BUILTIN_KIND_FUNCTION,
    VITTE_BUILTIN_KIND_OPERATOR
} vitte_builtin_kind_t;

typedef enum vitte_builtin_type_kind {
    VITTE_BUILTIN_TYPE_VOID = 0,
    VITTE_BUILTIN_TYPE_BOOL,
    VITTE_BUILTIN_TYPE_INT,
    VITTE_BUILTIN_TYPE_I64,
    VITTE_BUILTIN_TYPE_STRING,
    VITTE_BUILTIN_TYPE_NEVER,
    VITTE_BUILTIN_TYPE_ERROR,
    VITTE_BUILTIN_TYPE_COUNT
} vitte_builtin_type_kind_t;

typedef enum vitte_builtin_operator_kind {
    VITTE_BUILTIN_OPERATOR_UNARY = 0,
    VITTE_BUILTIN_OPERATOR_BINARY
} vitte_builtin_operator_kind_t;

typedef enum vitte_builtin_associativity {
    VITTE_BUILTIN_ASSOC_NONE = 0,
    VITTE_BUILTIN_ASSOC_LEFT,
    VITTE_BUILTIN_ASSOC_RIGHT
} vitte_builtin_associativity_t;

typedef enum vitte_builtin_type_class {
    VITTE_BUILTIN_TYPE_CLASS_ANY = 0,
    VITTE_BUILTIN_TYPE_CLASS_NUMERIC,
    VITTE_BUILTIN_TYPE_CLASS_BOOLEAN,
    VITTE_BUILTIN_TYPE_CLASS_TEXTUAL
} vitte_builtin_type_class_t;

typedef struct vitte_builtin_type {
    const char *name;
    vitte_builtin_type_kind_t kind;
    size_t size_hint;
    size_t align_hint;
    bool numeric;
    bool boolean;
    bool textual;
    bool bottom;
    bool error;
} vitte_builtin_type_t;

typedef struct vitte_builtin_function {
    const char *name;
    vitte_builtin_type_kind_t return_type;
    vitte_builtin_type_kind_t parameter_type;
    size_t min_arity;
    size_t max_arity;
    bool variadic;
    bool pure;
    bool noreturn;
} vitte_builtin_function_t;

typedef struct vitte_builtin_operator {
    const char *token;
    vitte_builtin_operator_kind_t kind;
    vitte_builtin_type_class_t operand_class;
    vitte_builtin_type_kind_t return_type;
    unsigned precedence;
    vitte_builtin_associativity_t associativity;
} vitte_builtin_operator_t;

typedef struct vitte_builtin_symbol {
    vitte_builtin_kind_t kind;
    const char *name;
    union {
        const vitte_builtin_type_t *type;
        const vitte_builtin_function_t *function;
        const vitte_builtin_operator_t *operator_info;
    } as;
} vitte_builtin_symbol_t;

typedef struct vitte_builtin_lookup_result {
    vitte_status_t status;
    vitte_builtin_symbol_t symbol;
} vitte_builtin_lookup_result_t;

typedef struct vitte_builtin_registry {
    bool initialized;
    const vitte_builtin_type_t *types;
    size_t type_count;
    const vitte_builtin_function_t *functions;
    size_t function_count;
    const vitte_builtin_operator_t *operators;
    size_t operator_count;
    vitte_error_t last_error;
} vitte_builtin_registry_t;

void vitte_builtin_registry_init(vitte_builtin_registry_t *registry);
void vitte_builtin_registry_reset(vitte_builtin_registry_t *registry);
bool vitte_builtin_registry_is_initialized(const vitte_builtin_registry_t *registry);
const vitte_error_t *vitte_builtin_registry_last_error(const vitte_builtin_registry_t *registry);
vitte_status_t vitte_builtin_registry_validate(vitte_builtin_registry_t *registry);

size_t vitte_builtin_type_count(const vitte_builtin_registry_t *registry);
size_t vitte_builtin_function_count(const vitte_builtin_registry_t *registry);
size_t vitte_builtin_operator_count(const vitte_builtin_registry_t *registry);

const vitte_builtin_type_t *vitte_builtin_type_at(const vitte_builtin_registry_t *registry, size_t index);
const vitte_builtin_function_t *vitte_builtin_function_at(const vitte_builtin_registry_t *registry, size_t index);
const vitte_builtin_operator_t *vitte_builtin_operator_at(const vitte_builtin_registry_t *registry, size_t index);

const vitte_builtin_type_t *vitte_builtin_lookup_type(vitte_builtin_registry_t *registry, const char *name);
const vitte_builtin_function_t *vitte_builtin_lookup_function(vitte_builtin_registry_t *registry, const char *name);
const vitte_builtin_operator_t *vitte_builtin_lookup_operator(
    vitte_builtin_registry_t *registry,
    const char *token,
    vitte_builtin_operator_kind_t kind
);
vitte_builtin_lookup_result_t vitte_builtin_lookup(vitte_builtin_registry_t *registry, const char *name);

const char *vitte_builtin_kind_name(vitte_builtin_kind_t kind);
const char *vitte_builtin_type_kind_name(vitte_builtin_type_kind_t kind);
const vitte_builtin_type_t *vitte_builtin_type_by_kind(vitte_builtin_registry_t *registry, vitte_builtin_type_kind_t kind);

bool vitte_builtin_type_kind_is_valid(vitte_builtin_type_kind_t kind);
bool vitte_builtin_type_is_numeric(vitte_builtin_type_kind_t kind);
bool vitte_builtin_type_is_boolean(vitte_builtin_type_kind_t kind);
bool vitte_builtin_function_accepts_arity(const vitte_builtin_function_t *function, size_t arity);
bool vitte_builtin_operator_accepts(
    const vitte_builtin_operator_t *operator_info,
    vitte_builtin_type_kind_t left,
    vitte_builtin_type_kind_t right
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_BUILTIN_H */
