#include "arena.h"

#include <stdint.h>
#include <string.h>

static size_t vitte_arena_max_size(size_t left, size_t right) {
    return left > right ? left : right;
}

static size_t vitte_arena_next_capacity(const vitte_arena_t *arena, size_t minimum) {
    size_t capacity;

    if (arena == NULL) {
        return minimum;
    }

    capacity = arena->config.initial_block_size;
    if (arena->current != NULL && arena->current->capacity <= SIZE_MAX / 2u) {
        capacity = arena->current->capacity * 2u;
    }

    capacity = vitte_arena_max_size(capacity, minimum);
    if (arena->config.max_block_size != 0u && capacity > arena->config.max_block_size) {
        capacity = minimum <= arena->config.max_block_size ? arena->config.max_block_size : 0u;
    }

    return capacity;
}

static size_t vitte_arena_used_bytes(const vitte_arena_t *arena) {
    const vitte_arena_block_t *block;
    size_t used = 0u;

    if (arena == NULL) {
        return 0u;
    }

    for (block = arena->first; block != NULL; block = block->next) {
        used += block->offset;
    }

    return used;
}

static void vitte_arena_recompute_storage_stats(vitte_arena_t *arena) {
    vitte_arena_block_t *block;
    size_t reserved = 0u;
    size_t blocks = 0u;

    if (arena == NULL) {
        return;
    }

    for (block = arena->first; block != NULL; block = block->next) {
        reserved += block->capacity;
        blocks++;
    }

    arena->stats.bytes_reserved = reserved;
    arena->stats.block_count = blocks;
    vitte_arena_stats_set_usage(&arena->stats, vitte_arena_used_bytes(arena));
}

static bool vitte_arena_append_block(vitte_arena_t *arena, size_t capacity) {
    vitte_arena_block_t *block;

    if (arena == NULL || capacity == 0u) {
        return false;
    }

    block = vitte_arena_block_create(&arena->config.allocator, capacity);
    if (block == NULL) {
        vitte_arena_stats_record_failed_alloc(&arena->stats);
        vitte_arena_set_error(
            arena,
            VITTE_STATUS_ERROR_OUT_OF_MEMORY,
            "VITTE_ARENA_E_BLOCK",
            "unable to allocate arena block",
            NULL
        );
        return false;
    }

    if (arena->first == NULL) {
        arena->first = block;
    } else {
        arena->current->next = block;
    }
    arena->current = block;
    vitte_arena_stats_record_block(&arena->stats, block->capacity);
    return true;
}

void vitte_arena_config_init(vitte_arena_config_t *config) {
    if (config == NULL) {
        return;
    }

    memset(config, 0, sizeof(*config));
    vitte_allocator_default(&config->allocator);
    config->initial_block_size = VITTE_ARENA_DEFAULT_BLOCK_SIZE;
    config->max_block_size = 0u;
    config->reset_policy = VITTE_ARENA_RESET_KEEP_FIRST_BLOCK;
    config->clear_on_reset = false;
}

vitte_status_t vitte_arena_init(
    vitte_arena_t *arena,
    const vitte_arena_config_t *config
) {
    vitte_arena_config_t defaults;

    if (arena == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    if (config == NULL) {
        vitte_arena_config_init(&defaults);
        config = &defaults;
    }

    memset(arena, 0, sizeof(*arena));
    arena->config = *config;
    if (!vitte_allocator_is_valid(&arena->config.allocator)) {
        vitte_allocator_default(&arena->config.allocator);
    }
    if (arena->config.initial_block_size == 0u) {
        arena->config.initial_block_size = VITTE_ARENA_DEFAULT_BLOCK_SIZE;
    }

    vitte_arena_stats_init(&arena->stats);
    vitte_error_init(&arena->last_error);
    arena->initialized = true;
    arena->owns_arena = false;
    return VITTE_STATUS_OK;
}

vitte_status_t vitte_arena_create(
    const vitte_arena_config_t *config,
    vitte_arena_t **arena
) {
    vitte_arena_config_t defaults;
    const vitte_allocator_t *allocator;
    vitte_arena_t *created;
    vitte_status_t status;

    if (arena == NULL) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    *arena = NULL;
    if (config == NULL) {
        vitte_arena_config_init(&defaults);
        config = &defaults;
    }

    allocator = &config->allocator;
    if (!vitte_allocator_is_valid(allocator)) {
        return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
    }

    created = (vitte_arena_t *)allocator->alloc(allocator->user, sizeof(*created));
    if (created == NULL) {
        return VITTE_STATUS_ERROR_OUT_OF_MEMORY;
    }

    status = vitte_arena_init(created, config);
    if (status != VITTE_STATUS_OK) {
        allocator->free(allocator->user, created);
        return status;
    }

    created->owns_arena = true;
    *arena = created;
    return VITTE_STATUS_OK;
}

void vitte_arena_destroy(vitte_arena_t *arena) {
    bool owns_arena;
    vitte_allocator_t allocator;

    if (arena == NULL) {
        return;
    }

    owns_arena = arena->owns_arena;
    allocator = arena->config.allocator;
    vitte_arena_block_destroy_chain(&allocator, arena->first);
    memset(arena, 0, sizeof(*arena));

    if (owns_arena && vitte_allocator_is_valid(&allocator)) {
        allocator.free(allocator.user, arena);
    }
}

bool vitte_arena_is_initialized(const vitte_arena_t *arena) {
    return arena != NULL && arena->initialized;
}

bool vitte_arena_reserve(vitte_arena_t *arena, size_t capacity) {
    if (!vitte_arena_is_initialized(arena) || capacity == 0u) {
        return false;
    }

    if (arena->current != NULL && vitte_arena_block_remaining(arena->current) >= capacity) {
        return true;
    }

    return vitte_arena_append_block(arena, vitte_arena_next_capacity(arena, capacity));
}

void *vitte_arena_alloc(
    vitte_arena_t *arena,
    size_t size,
    size_t alignment
) {
    void *memory;
    size_t aligned_offset;
    size_t needed;

    if (!vitte_arena_is_initialized(arena) || size == 0u) {
        return NULL;
    }

    if (alignment == 0u) {
        alignment = VITTE_ARENA_DEFAULT_ALIGNMENT;
    }
    if (!vitte_arena_alignment_is_valid(alignment)) {
        vitte_arena_stats_record_failed_alloc(&arena->stats);
        vitte_arena_set_error(arena, VITTE_STATUS_ERROR_INVALID_ARGUMENT, "VITTE_ARENA_E_ALIGNMENT", "invalid allocation alignment", NULL);
        return NULL;
    }

    needed = size;
    if (arena->current != NULL) {
        if (!vitte_arena_align_forward(arena->current->offset, alignment, &aligned_offset)) {
            vitte_arena_stats_record_failed_alloc(&arena->stats);
            return NULL;
        }
        if (aligned_offset <= arena->current->capacity) {
            needed = size + (aligned_offset - arena->current->offset);
        }
    }

    if (arena->current == NULL || vitte_arena_block_remaining(arena->current) < needed) {
        if (!vitte_arena_reserve(arena, size + alignment - 1u)) {
            return NULL;
        }
    }

    memory = vitte_arena_block_alloc(arena->current, size, alignment);
    if (memory == NULL) {
        vitte_arena_stats_record_failed_alloc(&arena->stats);
        vitte_arena_set_error(arena, VITTE_STATUS_ERROR_OUT_OF_MEMORY, "VITTE_ARENA_E_ALLOC", "arena allocation failed", NULL);
        return NULL;
    }

    vitte_arena_stats_set_usage(&arena->stats, vitte_arena_used_bytes(arena));
    arena->stats.allocation_count++;
    return memory;
}

void *vitte_arena_alloc_zeroed(
    vitte_arena_t *arena,
    size_t size,
    size_t alignment
) {
    void *memory = vitte_arena_alloc(arena, size, alignment);

    if (memory != NULL) {
        memset(memory, 0, size);
    }

    return memory;
}

bool vitte_arena_contains(const vitte_arena_t *arena, const void *pointer) {
    const vitte_arena_block_t *block;

    if (arena == NULL || pointer == NULL) {
        return false;
    }

    for (block = arena->first; block != NULL; block = block->next) {
        if (vitte_arena_block_contains(block, pointer)) {
            return true;
        }
    }

    return false;
}

void vitte_arena_reset(vitte_arena_t *arena) {
    vitte_arena_block_t *keep;

    if (!vitte_arena_is_initialized(arena)) {
        return;
    }

    if (arena->config.reset_policy == VITTE_ARENA_RESET_RELEASE_ALL_BLOCKS) {
        vitte_arena_block_destroy_chain(&arena->config.allocator, arena->first);
        arena->first = NULL;
        arena->current = NULL;
    } else {
        keep = arena->first;
        if (keep != NULL) {
            vitte_arena_block_destroy_chain(&arena->config.allocator, keep->next);
            keep->next = NULL;
            if (arena->config.clear_on_reset) {
                memset(keep->memory, 0, keep->capacity);
            }
            keep->offset = 0u;
            arena->current = keep;
        }
    }

    vitte_arena_recompute_storage_stats(arena);
    vitte_arena_stats_record_reset(&arena->stats, 0u);
    vitte_error_reset(&arena->last_error);
}

const vitte_arena_stats_t *vitte_arena_stats(const vitte_arena_t *arena) {
    return arena != NULL ? &arena->stats : NULL;
}

const vitte_error_t *vitte_arena_last_error(const vitte_arena_t *arena) {
    return arena != NULL ? &arena->last_error : vitte_error_last();
}

void vitte_arena_clear_error(vitte_arena_t *arena) {
    if (arena == NULL) {
        return;
    }

    vitte_error_reset(&arena->last_error);
}

void vitte_arena_set_error(
    vitte_arena_t *arena,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
) {
    if (arena == NULL) {
        vitte_error_t error;
        vitte_error_set_details(&error, status, code, message, details);
        return;
    }

    vitte_error_set_details(&arena->last_error, status, code, message, details);
}
