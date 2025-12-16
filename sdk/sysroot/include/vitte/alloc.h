/**
 * Vitte Allocation Header
 *
 * Custom allocator support for memory management.
 */

#ifndef VITTE_ALLOC_H
#define VITTE_ALLOC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Allocator Interface ========== */

typedef struct {
  void* (*alloc)(size_t size);
  void* (*realloc)(void* ptr, size_t new_size);
  void (*free)(void* ptr);
  void (*reset)(void); /* Optional: reset allocator state */
} vitte_allocator_t;

/**
 * Set global allocator
 */
void vitte_set_allocator(vitte_allocator_t* allocator);

/**
 * Get current allocator
 */
vitte_allocator_t* vitte_get_allocator(void);

/**
 * Reset to default allocator
 */
void vitte_reset_allocator(void);

/* ========== Arena Allocator ========== */

typedef void* vitte_arena_t;

/**
 * Create a new arena allocator
 */
vitte_arena_t vitte_arena_create(size_t capacity);

/**
 * Destroy arena
 */
void vitte_arena_destroy(vitte_arena_t arena);

/**
 * Allocate from arena
 */
void* vitte_arena_alloc(vitte_arena_t arena, size_t size);

/**
 * Reset arena (frees all allocations)
 */
void vitte_arena_reset(vitte_arena_t arena);

/* ========== Tracking ========== */

/**
 * Get allocation statistics
 */
typedef struct {
  size_t total_allocated;
  size_t total_freed;
  size_t active_allocations;
  size_t peak_memory;
} vitte_alloc_stats_t;

vitte_alloc_stats_t vitte_get_alloc_stats(void);

/**
 * Enable allocation tracking
 */
void vitte_enable_alloc_tracking(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_ALLOC_H */
