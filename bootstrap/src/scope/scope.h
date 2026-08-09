#ifndef VITTE_BOOTSTRAP_SCOPE_H
#define VITTE_BOOTSTRAP_SCOPE_H

#include <stdbool.h>
#include <stddef.h>

#include "../api/error.h"
#include "../symbol/symbol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_SCOPE_MAX_FRAMES ((size_t)128u)
#define VITTE_SCOPE_MAX_BINDINGS ((size_t)65536u)

typedef struct vitte_scope_frame {
    size_t binding_start;
    size_t binding_count;
    bool function_scope;
} vitte_scope_frame_t;

typedef struct vitte_scope_binding {
    const char *name;
    const vitte_symbol_t *symbol;
    size_t frame_index;
} vitte_scope_binding_t;

typedef struct vitte_scope_stack {
    bool initialized;
    vitte_scope_frame_t frames[VITTE_SCOPE_MAX_FRAMES];
    size_t frame_count;
    vitte_scope_binding_t bindings[VITTE_SCOPE_MAX_BINDINGS];
    size_t binding_count;
    vitte_error_t last_error;
} vitte_scope_stack_t;

void vitte_scope_stack_init(vitte_scope_stack_t *stack);
void vitte_scope_stack_destroy(vitte_scope_stack_t *stack);
bool vitte_scope_stack_is_initialized(const vitte_scope_stack_t *stack);
const vitte_error_t *vitte_scope_stack_last_error(const vitte_scope_stack_t *stack);
void vitte_scope_stack_clear_error(vitte_scope_stack_t *stack);

size_t vitte_scope_depth(const vitte_scope_stack_t *stack);
vitte_status_t vitte_scope_push(vitte_scope_stack_t *stack, bool function_scope);
vitte_status_t vitte_scope_pop(vitte_scope_stack_t *stack);
vitte_status_t vitte_scope_define(
    vitte_scope_stack_t *stack,
    const char *name,
    const vitte_symbol_t *symbol
);
const vitte_symbol_t *vitte_scope_lookup(
    const vitte_scope_stack_t *stack,
    const char *name
);
const vitte_symbol_t *vitte_scope_lookup_current(
    const vitte_scope_stack_t *stack,
    const char *name
);
bool vitte_scope_is_function_boundary(const vitte_scope_stack_t *stack);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_SCOPE_H */
