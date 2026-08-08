#ifndef VITTE_BOOTSTRAP_ARENA_STATS_H
#define VITTE_BOOTSTRAP_ARENA_STATS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vitte_arena_stats {
    size_t bytes_reserved;
    size_t bytes_used;
    size_t peak_bytes_used;
    size_t allocation_count;
    size_t failed_allocation_count;
    size_t block_count;
    size_t reset_count;
} vitte_arena_stats_t;

void vitte_arena_stats_init(vitte_arena_stats_t *stats);
void vitte_arena_stats_record_block(vitte_arena_stats_t *stats, size_t capacity);
void vitte_arena_stats_release_block(vitte_arena_stats_t *stats, size_t capacity);
void vitte_arena_stats_record_alloc(vitte_arena_stats_t *stats, size_t bytes);
void vitte_arena_stats_record_failed_alloc(vitte_arena_stats_t *stats);
void vitte_arena_stats_record_reset(vitte_arena_stats_t *stats, size_t bytes_used);
void vitte_arena_stats_set_usage(vitte_arena_stats_t *stats, size_t bytes_used);

#ifdef __cplusplus
}
#endif

#endif /* VITTE_BOOTSTRAP_ARENA_STATS_H */
