#include "context.h"

#include <stdlib.h>
#include <string.h>

static void *vitte_default_alloc(void *user, size_t size) {
    (void)user;
    return malloc(size == 0u ? 1u : size);
}

static void *vitte_default_realloc(void *user, void *pointer, size_t size) {
    (void)user;
    return realloc(pointer, size == 0u ? 1u : size);
}

static void vitte_default_free(void *user, void *pointer) {
    (void)user;
    free(pointer);
}

void vitte_allocator_default(vitte_allocator_t *allocator) {
    if (allocator == NULL) {
        return;
    }

    allocator->user = NULL;
    allocator->alloc = vitte_default_alloc;
    allocator->realloc = vitte_default_realloc;
    allocator->free = vitte_default_free;
}

bool vitte_allocator_is_valid(const vitte_allocator_t *allocator) {
    return allocator != NULL &&
        allocator->alloc != NULL &&
        allocator->realloc != NULL &&
        allocator->free != NULL;
}

void *vitte_context_alloc(vitte_context_t *context, size_t size) {
    if (context == NULL || !vitte_allocator_is_valid(&context->allocator)) {
        return NULL;
    }

    return context->allocator.alloc(context->allocator.user, size);
}

void *vitte_context_realloc(vitte_context_t *context, void *pointer, size_t size) {
    if (context == NULL || !vitte_allocator_is_valid(&context->allocator)) {
        return NULL;
    }

    return context->allocator.realloc(context->allocator.user, pointer, size);
}

void vitte_context_free(vitte_context_t *context, void *pointer) {
    if (context == NULL || !vitte_allocator_is_valid(&context->allocator)) {
        return;
    }

    context->allocator.free(context->allocator.user, pointer);
}

void vitte_api_config_init(vitte_api_config_t *config) {
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(*config));
    vitte_allocator_default(&config->allocator);
    config->root_path = ".";
    config->sysroot_path = NULL;
    config->deterministic = true;
}

vitte_status_t vitte_context_init(
    vitte_context_t *context,
    const vitte_api_config_t *config
) {
    vitte_api_config_t defaults;

    if (context == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (config == NULL) {
        vitte_api_config_init(&defaults);
        config = &defaults;
    }

    if (!vitte_allocator_is_valid(&config->allocator)) {
        memset(context, 0, sizeof(*context));
        vitte_error_set(
            &context->last_error,
            VITTE_STATUS_ERROR_INVALID_ARGUMENT,
            "VITTE_API_E_ALLOCATOR",
            "invalid allocator"
        );
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    memset(context, 0, sizeof(*context));
    context->initialized = true;
    context->owns_context = false;
    context->deterministic = config->deterministic;
    context->allocator = config->allocator;
    context->root_path = config->root_path != NULL ? config->root_path : ".";
    context->sysroot_path = config->sysroot_path;
    vitte_error_init(&context->last_error);

    return VITTE_STATUS_OK;
}

vitte_status_t vitte_context_create(
    const vitte_api_config_t *config,
    vitte_context_t **context
) {
    vitte_api_config_t defaults;
    const vitte_allocator_t *allocator;
    vitte_context_t *created;
    vitte_status_t status;

    if (context == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    *context = NULL;

    if (config == NULL) {
        vitte_api_config_init(&defaults);
        config = &defaults;
    }

    allocator = &config->allocator;
    if (!vitte_allocator_is_valid(allocator)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    created = (vitte_context_t *)allocator->alloc(allocator->user, sizeof(*created));
    if (created == NULL) {
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    status = vitte_context_init(created, config);
    if (status != VITTE_STATUS_OK) {
        allocator->free(allocator->user, created);
        return status;
    }

    created->owns_context = true;
    *context = created;
    return VITTE_STATUS_OK;
}

void vitte_context_destroy(vitte_context_t *context) {
    bool owns_context;
    vitte_allocator_t allocator;

    if (context == NULL) {
        return;
    }

    owns_context = context->owns_context;
    allocator = context->allocator;
    memset(context, 0, sizeof(*context));

    if (owns_context && vitte_allocator_is_valid(&allocator)) {
        allocator.free(allocator.user, context);
    }
}

void vitte_context_reset_error(vitte_context_t *context) {
    if (context == NULL) {
        return;
    }

    vitte_error_reset(&context->last_error);
}

bool vitte_context_is_initialized(const vitte_context_t *context) {
    return context != NULL && context->initialized;
}

vitte_allocator_t *vitte_context_allocator(vitte_context_t *context) {
    return context != NULL ? &context->allocator : NULL;
}

const vitte_allocator_t *vitte_context_allocator_const(const vitte_context_t *context) {
    return context != NULL ? &context->allocator : NULL;
}

const vitte_error_t *vitte_context_last_error(const vitte_context_t *context) {
    return context != NULL ? &context->last_error : vitte_error_last();
}

void vitte_context_set_error(
    vitte_context_t *context,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (context == NULL) {
        vitte_error_t error;
        vitte_error_set_details(&error, status, code, message, details);
        return;
    }

    vitte_error_set_details(&context->last_error, status, code, message, details);
}

const char *vitte_context_root_path(const vitte_context_t *context) {
    return context != NULL ? context->root_path : NULL;
}

const char *vitte_context_sysroot_path(const vitte_context_t *context) {
    return context != NULL ? context->sysroot_path : NULL;
}
