#ifndef VITTE_GARBAGE_COLLECTOR_H
#define VITTE_GARBAGE_COLLECTOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct vitte_vm vitte_vm_t;
typedef struct vitte_value vitte_value_t;

/* Memory object tracking */
typedef struct {
    void *ptr;
    size_t size;
    bool is_marked;
} gc_object_t;

/* GC heap structure */
typedef struct {
    gc_object_t *objects;
    int object_count;
    int object_capacity;
    
    size_t bytes_allocated;
    size_t next_gc_threshold;
    
    int gc_runs;
    int marked_count;
    int collected_count;
} gc_heap_t;

/* GC functions */
gc_heap_t* gc_create(size_t initial_threshold);
void gc_free(gc_heap_t *gc);
void gc_reset(gc_heap_t *gc);

/* Allocation tracking */
void* gc_allocate(gc_heap_t *gc, size_t size);
void gc_deallocate(gc_heap_t *gc, void *ptr);

/* GC phases */
void gc_mark_reachable(vitte_vm_t *vm, gc_heap_t *gc);
void gc_mark_value(vitte_value_t value, gc_heap_t *gc);
void gc_sweep(gc_heap_t *gc);
void gc_collect(vitte_vm_t *vm, gc_heap_t *gc);

/* Statistics */
void gc_print_stats(gc_heap_t *gc);
size_t gc_bytes_allocated(gc_heap_t *gc);

#endif
