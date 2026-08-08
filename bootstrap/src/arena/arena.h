#ifndef VITTE_BOOTSTRAP_ARENA_H
#define VITTE_BOOTSTRAP_ARENA_H

#include <stdbool.h>
#include <stddef.h>

#include "block.h"
#include "../api/error.h"
#include "stats.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VITTE_ARENA_DEFAULT_BLOCK_SIZE ((size_t)4096u)
#define VITTE_ARENA_DEFAULT_ALIGNMENT ((size_t)_Alignof(max_align_t))

typedef enum vitte_arena_reset_policy {
    VITTE_ARENA_RESET_KEEP_FIRST_BLOCK = 0,
    VITTE_ARENA_RESET_RELEASE_ALL_BLOCKS
} vitte_arena_reset_policy_t;

typedef struct vitte_arena_config {
    vitte_allocator_t allocator;
    size_t initial_block_size;
    size_t max_block_size;
    vitte_arena_reset_policy_t reset_policy;
    bool clear_on_reset;
} vitte_arena_config_t;

typedef struct vitte_arena {
    bool initialized;
    bool owns_arena;
    vitte_arena_config_t config;
    vitte_arena_block_t *first;
    vitte_arena_block_t *current;
    vitte_arena_stats_t stats;
    vitte_error_t last_error;
} vitte_arena_t;

void vitte_arena_config_init(vitte_arena_config_t *config);

vitte_status_t vitte_arena_init(
    vitte_arena_t *arena,
    const vitte_arena_config_t *config
);

vitte_status_t vitte_arena_create(
    const vitte_arena_config_t *config,
    vitte_arena_t **arena
);

void vitte_arena_destroy(vitte_arena_t *arena);
bool vitte_arena_is_initialized(const vitte_arena_t *arena);

void *vitte_arena_alloc(
    vitte_arena_t *arena,
    size_t size,
    size_t alignment
);

void *vitte_arena_alloc_zeroed(
    vitte_arena_t *arena,
    size_t size,
    size_t alignment
);

bool vitte_arena_reserve(vitte_arena_t *arena, size_t capacity);
bool vitte_arena_contains(const vitte_arena_t *arena, const void *pointer);
void vitte_arena_reset(vitte_arena_t *arena);

const vitte_arena_stats_t *vitte_arena_stats(const vitte_arena_t *arena);
const vitte_error_t *vitte_arena_last_error(const vitte_arena_t *arena);
void vitte_arena_clear_error(vitte_arena_t *arena);

void vitte_arena_set_error(
    vitte_arena_t *arena,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_ARENA_H */
