#include "stats.h"

#include <string.h>

void vitte_arena_stats_init(vitte_arena_stats_t *stats) {
    if (stats == NULL) {
        return;
    }

    memset(stats, 0, sizeof(*stats));
}

void vitte_arena_stats_record_block(vitte_arena_stats_t *stats, size_t capacity) {
    if (stats == NULL) {
        return;
    }

    stats->bytes_reserved += capacity;
    stats->block_count++;
}

void vitte_arena_stats_release_block(vitte_arena_stats_t *stats, size_t capacity) {
    if (stats == NULL) {
        return;
    }

    stats->bytes_reserved = stats->bytes_reserved >= capacity
        ? stats->bytes_reserved - capacity
        : 0u;
    if (stats->block_count > 0u) {
        stats->block_count--;
    }
}

void vitte_arena_stats_record_alloc(vitte_arena_stats_t *stats, size_t bytes) {
    if (stats == NULL) {
        return;
    }

    stats->bytes_used += bytes;
    if (stats->bytes_used > stats->peak_bytes_used) {
        stats->peak_bytes_used = stats->bytes_used;
    }
    stats->allocation_count++;
}

void vitte_arena_stats_record_failed_alloc(vitte_arena_stats_t *stats) {
    if (stats == NULL) {
        return;
    }

    stats->failed_allocation_count++;
}

void vitte_arena_stats_record_reset(vitte_arena_stats_t *stats, size_t bytes_used) {
    if (stats == NULL) {
        return;
    }

    stats->bytes_used = bytes_used;
    stats->reset_count++;
}

void vitte_arena_stats_set_usage(vitte_arena_stats_t *stats, size_t bytes_used) {
    if (stats == NULL) {
        return;
    }

    stats->bytes_used = bytes_used;
    if (stats->bytes_used > stats->peak_bytes_used) {
        stats->peak_bytes_used = stats->bytes_used;
    }
}
