#ifndef VITTE_BOOTSTRAP_API_CONTEXT_H
#define VITTE_BOOTSTRAP_API_CONTEXT_H

#include <stdbool.h>
#include <stddef.h>

#include "error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*vitte_alloc_fn)(void *user, size_t size);
typedef void *(*vitte_realloc_fn)(void *user, void *pointer, size_t size);
typedef void (*vitte_free_fn)(void *user, void *pointer);

typedef struct vitte_allocator {
    void *user;
    vitte_alloc_fn alloc;
    vitte_realloc_fn realloc;
    vitte_free_fn free;
} vitte_allocator_t;

typedef struct vitte_api_config {
    vitte_allocator_t allocator;
    const char *root_path;
    const char *sysroot_path;
    bool deterministic;
} vitte_api_config_t;

typedef struct vitte_context {
    bool initialized;
    bool owns_context;
    bool deterministic;
    vitte_allocator_t allocator;
    vitte_error_t last_error;
    const char *root_path;
    const char *sysroot_path;
} vitte_context_t;

void vitte_allocator_default(vitte_allocator_t *allocator);
bool vitte_allocator_is_valid(const vitte_allocator_t *allocator);

void *vitte_context_alloc(vitte_context_t *context, size_t size);
void *vitte_context_realloc(vitte_context_t *context, void *pointer, size_t size);
void vitte_context_free(vitte_context_t *context, void *pointer);

void vitte_api_config_init(vitte_api_config_t *config);

vitte_status_t vitte_context_init(
    vitte_context_t *context,
    const vitte_api_config_t *config
);

vitte_status_t vitte_context_create(
    const vitte_api_config_t *config,
    vitte_context_t **context
);

void vitte_context_destroy(vitte_context_t *context);
void vitte_context_reset_error(vitte_context_t *context);

bool vitte_context_is_initialized(const vitte_context_t *context);

vitte_allocator_t *vitte_context_allocator(vitte_context_t *context);
const vitte_allocator_t *vitte_context_allocator_const(const vitte_context_t *context);

const vitte_error_t *vitte_context_last_error(const vitte_context_t *context);
void vitte_context_set_error(
    vitte_context_t *context,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
);

const char *vitte_context_root_path(const vitte_context_t *context);
const char *vitte_context_sysroot_path(const vitte_context_t *context);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_API_CONTEXT_H */
