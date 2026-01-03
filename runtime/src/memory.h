#ifndef VITTE_MEMORY_H
#define VITTE_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Memory allocation tracker */
typedef struct {
    void *ptr;
    size_t size;
    const char *source;
    int line;
} memory_block_t;

/* Memory manager */
typedef struct {
    memory_block_t *blocks;
    int block_count;
    int block_capacity;
    
    size_t total_allocated;
    size_t total_freed;
    size_t peak_usage;
    
    bool track_allocations;
} memory_manager_t;

/* Initialization */
memory_manager_t* memory_manager_create(void);
void memory_manager_free(memory_manager_t *mm);
void memory_manager_reset(memory_manager_t *mm);

/* Allocation */
void* memory_malloc(memory_manager_t *mm, size_t size);
void* memory_calloc(memory_manager_t *mm, size_t count, size_t size);
void* memory_realloc(memory_manager_t *mm, void *ptr, size_t new_size);
void memory_free(memory_manager_t *mm, void *ptr);

/* Utilities */
void memory_enable_tracking(memory_manager_t *mm, bool enable);
void memory_dump_blocks(memory_manager_t *mm);
void memory_print_stats(memory_manager_t *mm);
size_t memory_total_allocated(memory_manager_t *mm);
size_t memory_peak_usage(memory_manager_t *mm);
int memory_block_count(memory_manager_t *mm);

#endif
