#include "scope.h"

#include <string.h>

static void vitte_scope_set_error(
    vitte_scope_stack_t *stack,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (stack != NULL) {
        vitte_error_set_details(&stack->last_error, status, code, message, details);
    }
}

void vitte_scope_stack_init(vitte_scope_stack_t *stack) {
    if (stack == NULL) {
        return;
    }
    memset(stack, 0, sizeof(*stack));
    vitte_error_init(&stack->last_error);
    stack->initialized = true;
    stack->frame_count = 1u;
}

void vitte_scope_stack_destroy(vitte_scope_stack_t *stack) {
    if (stack == NULL) {
        return;
    }
    memset(stack, 0, sizeof(*stack));
}

bool vitte_scope_stack_is_initialized(const vitte_scope_stack_t *stack) {
    return stack != NULL && stack->initialized && stack->frame_count > 0u;
}

const vitte_error_t *vitte_scope_stack_last_error(const vitte_scope_stack_t *stack) {
    return stack != NULL ? &stack->last_error : vitte_error_last();
}

void vitte_scope_stack_clear_error(vitte_scope_stack_t *stack) {
    if (stack != NULL) {
        vitte_error_reset(&stack->last_error);
    }
}

size_t vitte_scope_depth(const vitte_scope_stack_t *stack) {
    return vitte_scope_stack_is_initialized(stack) ? stack->frame_count - 1u : 0u;
}

vitte_status_t vitte_scope_push(vitte_scope_stack_t *stack, bool function_scope) {
    vitte_scope_frame_t *frame;

    if (!vitte_scope_stack_is_initialized(stack)) {
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (stack->frame_count >= VITTE_SCOPE_MAX_FRAMES) {
        vitte_scope_set_error(stack, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_SCOPE_E_LIMIT", "scope frame stack is full", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    frame = &stack->frames[stack->frame_count];
    memset(frame, 0, sizeof(*frame));
    frame->binding_start = stack->binding_count;
    frame->function_scope = function_scope;
    stack->frame_count++;
    vitte_error_reset(&stack->last_error);
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_scope_pop(vitte_scope_stack_t *stack) {
    vitte_scope_frame_t *frame;

    if (!vitte_scope_stack_is_initialized(stack) || stack->frame_count <= 1u) {
        vitte_scope_set_error(stack, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_SCOPE_E_POP", "cannot pop bootstrap global scope", NULL);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    frame = &stack->frames[stack->frame_count - 1u];
    stack->binding_count = frame->binding_start;
    memset(frame, 0, sizeof(*frame));
    stack->frame_count--;
    vitte_error_reset(&stack->last_error);
    return VITTE_STATUS_OK;
}

const vitte_symbol_t *vitte_scope_lookup_current(
    const vitte_scope_stack_t *stack,
    const char *name
) {
    size_t index;
    const vitte_scope_frame_t *frame;

    if (!vitte_scope_stack_is_initialized(stack) || name == NULL || name[0] == '\0') {
        return NULL;
    }
    frame = &stack->frames[stack->frame_count - 1u];
    for (index = stack->binding_count; index > frame->binding_start; index--) {
        const vitte_scope_binding_t *binding = &stack->bindings[index - 1u];
        if (binding->name != NULL && strcmp(binding->name, name) == 0) {
            return binding->symbol;
        }
    }
    return NULL;
}

vitte_status_t vitte_scope_define(
    vitte_scope_stack_t *stack,
    const char *name,
    const vitte_symbol_t *symbol
) {
    vitte_scope_frame_t *frame;
    vitte_scope_binding_t *binding;

    if (!vitte_scope_stack_is_initialized(stack) || name == NULL || name[0] == '\0' || symbol == NULL) {
        vitte_scope_set_error(stack, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_SCOPE_E_DEFINE", "invalid scope definition", name);
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }
    if (vitte_scope_lookup_current(stack, name) != NULL) {
        vitte_scope_set_error(stack, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_SCOPE_E_DUPLICATE", "duplicate symbol in current scope", name);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }
    if (stack->binding_count >= VITTE_SCOPE_MAX_BINDINGS) {
        vitte_scope_set_error(stack, VITTE_STATUS_ERROR_INVALID_STATE, "VITTE_SCOPE_E_LIMIT", "scope binding table is full", name);
        return VITTE_STATUS_ERROR_INVALID_STATE;
    }

    frame = &stack->frames[stack->frame_count - 1u];
    binding = &stack->bindings[stack->binding_count];
    binding->name = name;
    binding->symbol = symbol;
    binding->frame_index = stack->frame_count - 1u;
    stack->binding_count++;
    frame->binding_count++;
    vitte_error_reset(&stack->last_error);
    return VITTE_STATUS_OK;
}

const vitte_symbol_t *vitte_scope_lookup(
    const vitte_scope_stack_t *stack,
    const char *name
) {
    size_t index;

    if (!vitte_scope_stack_is_initialized(stack) || name == NULL || name[0] == '\0') {
        return NULL;
    }
    for (index = stack->binding_count; index > 0u; index--) {
        const vitte_scope_binding_t *binding = &stack->bindings[index - 1u];
        if (binding->name != NULL && strcmp(binding->name, name) == 0) {
            return binding->symbol;
        }
    }
    return NULL;
}

bool vitte_scope_is_function_boundary(const vitte_scope_stack_t *stack) {
    if (!vitte_scope_stack_is_initialized(stack)) {
        return false;
    }
    return stack->frames[stack->frame_count - 1u].function_scope;
}
