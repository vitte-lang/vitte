#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

memory_manager_t* memory_manager_create(void) {
    memory_manager_t *mm = (memory_manager_t *)malloc(sizeof(memory_manager_t));
    if (!mm) return NULL;
    
    mm->blocks = (memory_block_t *)malloc(sizeof(memory_block_t) * 1024);
    mm->block_count = 0;
    mm->block_capacity = 1024;
    mm->total_allocated = 0;
    mm->total_freed = 0;
    mm->peak_usage = 0;
    mm->track_allocations = false;
    
    return mm;
}

void memory_manager_free(memory_manager_t *mm) {
    if (!mm) return;
    
    for (int i = 0; i < mm->block_count; i++) {
        free(mm->blocks[i].ptr);
    }
    
    free(mm->blocks);
    free(mm);
}

void memory_manager_reset(memory_manager_t *mm) {
    if (!mm) return;
    
    mm->block_count = 0;
    mm->total_allocated = 0;
    mm->total_freed = 0;
    mm->peak_usage = 0;
}

void* memory_malloc(memory_manager_t *mm, size_t size) {
    if (!mm) return malloc(size);
    
    void *ptr = malloc(size);
    if (!ptr) return NULL;
    
    if (mm->track_allocations) {
        if (mm->block_count >= mm->block_capacity) {
            mm->block_capacity *= 2;
            mm->blocks = (memory_block_t *)realloc(mm->blocks, 
                                                   sizeof(memory_block_t) * mm->block_capacity);
        }
        
        mm->blocks[mm->block_count].ptr = ptr;
        mm->blocks[mm->block_count].size = size;
        mm->blocks[mm->block_count].source = "malloc";
        mm->blocks[mm->block_count].line = 0;
        mm->block_count++;
    }
    
    mm->total_allocated += size;
    if (mm->total_allocated - mm->total_freed > mm->peak_usage) {
        mm->peak_usage = mm->total_allocated - mm->total_freed;
    }
    
    return ptr;
}

void* memory_calloc(memory_manager_t *mm, size_t count, size_t size) {
    if (!mm) return calloc(count, size);
    
    size_t total = count * size;
    void *ptr = calloc(count, size);
    if (!ptr) return NULL;
    
    if (mm->track_allocations) {
        if (mm->block_count >= mm->block_capacity) {
            mm->block_capacity *= 2;
            mm->blocks = (memory_block_t *)realloc(mm->blocks, 
                                                   sizeof(memory_block_t) * mm->block_capacity);
        }
        
        mm->blocks[mm->block_count].ptr = ptr;
        mm->blocks[mm->block_count].size = total;
        mm->blocks[mm->block_count].source = "calloc";
        mm->blocks[mm->block_count].line = 0;
        mm->block_count++;
    }
    
    mm->total_allocated += total;
    if (mm->total_allocated - mm->total_freed > mm->peak_usage) {
        mm->peak_usage = mm->total_allocated - mm->total_freed;
    }
    
    return ptr;
}

void* memory_realloc(memory_manager_t *mm, void *ptr, size_t new_size) {
    if (!mm) return realloc(ptr, new_size);
    
    void *new_ptr = realloc(ptr, new_size);
    if (!new_ptr) return NULL;
    
    if (mm->track_allocations) {
        for (int i = 0; i < mm->block_count; i++) {
            if (mm->blocks[i].ptr == ptr) {
                mm->total_freed += mm->blocks[i].size;
                mm->blocks[i].ptr = new_ptr;
                mm->blocks[i].size = new_size;
                break;
            }
        }
    }
    
    mm->total_allocated += new_size;
    if (mm->total_allocated - mm->total_freed > mm->peak_usage) {
        mm->peak_usage = mm->total_allocated - mm->total_freed;
    }
    
    return new_ptr;
}

void memory_free(memory_manager_t *mm, void *ptr) {
    if (!mm || !ptr) {
        free(ptr);
        return;
    }
    
    if (mm->track_allocations) {
        for (int i = 0; i < mm->block_count; i++) {
            if (mm->blocks[i].ptr == ptr) {
                mm->total_freed += mm->blocks[i].size;
                free(ptr);
                
                mm->blocks[i] = mm->blocks[mm->block_count - 1];
                mm->block_count--;
                return;
            }
        }
    }
    
    free(ptr);
}

void memory_enable_tracking(memory_manager_t *mm, bool enable) {
    if (!mm) return;
    mm->track_allocations = enable;
}

void memory_dump_blocks(memory_manager_t *mm) {
    if (!mm) return;
    
    printf("=== Memory Blocks ===\n");
    for (int i = 0; i < mm->block_count; i++) {
        printf("[%d] %p - %zu bytes (from %s:%d)\n", 
               i, mm->blocks[i].ptr, mm->blocks[i].size, 
               mm->blocks[i].source, mm->blocks[i].line);
    }
    printf("\n");
}

void memory_print_stats(memory_manager_t *mm) {
    if (!mm) return;
    
    printf("=== Memory Statistics ===\n");
    printf("Total Allocated: %zu bytes\n", mm->total_allocated);
    printf("Total Freed:     %zu bytes\n", mm->total_freed);
    printf("Current Usage:   %zu bytes\n", mm->total_allocated - mm->total_freed);
    printf("Peak Usage:      %zu bytes\n", mm->peak_usage);
    printf("Blocks Tracked:  %d\n", mm->block_count);
    printf("\n");
}

size_t memory_total_allocated(memory_manager_t *mm) {
    return mm ? mm->total_allocated : 0;
}

size_t memory_peak_usage(memory_manager_t *mm) {
    return mm ? mm->peak_usage : 0;
}

int memory_block_count(memory_manager_t *mm) {
    return mm ? mm->block_count : 0;
}
