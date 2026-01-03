#include "garbage_collector.h"
#include "../include/vitte_vm.h"
#include "../include/vitte_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* GC initialization */
gc_heap_t* gc_create(size_t initial_threshold) {
    gc_heap_t *gc = (gc_heap_t *)malloc(sizeof(gc_heap_t));
    if (!gc) return NULL;
    
    gc->objects = (gc_object_t *)malloc(sizeof(gc_object_t) * 1024);
    gc->object_count = 0;
    gc->object_capacity = 1024;
    gc->bytes_allocated = 0;
    gc->next_gc_threshold = initial_threshold;
    gc->gc_runs = 0;
    gc->marked_count = 0;
    gc->collected_count = 0;
    
    return gc;
}

void gc_free(gc_heap_t *gc) {
    if (!gc) return;
    
    for (int i = 0; i < gc->object_count; i++) {
        free(gc->objects[i].ptr);
    }
    
    free(gc->objects);
    free(gc);
}

void gc_reset(gc_heap_t *gc) {
    if (!gc) return;
    
    for (int i = 0; i < gc->object_count; i++) {
        gc->objects[i].is_marked = false;
    }
    
    gc->marked_count = 0;
}

/* Allocation tracking */
void* gc_allocate(gc_heap_t *gc, size_t size) {
    if (!gc) return malloc(size);
    
    void *ptr = malloc(size);
    if (!ptr) return NULL;
    
    /* Track allocation */
    if (gc->object_count >= gc->object_capacity) {
        gc->object_capacity *= 2;
        gc->objects = (gc_object_t *)realloc(gc->objects, 
                                            sizeof(gc_object_t) * gc->object_capacity);
    }
    
    gc->objects[gc->object_count].ptr = ptr;
    gc->objects[gc->object_count].size = size;
    gc->objects[gc->object_count].is_marked = false;
    gc->object_count++;
    
    gc->bytes_allocated += size;
    
    /* Check if GC should run */
    if (gc->bytes_allocated >= gc->next_gc_threshold) {
        /* Will trigger GC in the VM loop */
    }
    
    return ptr;
}

void gc_deallocate(gc_heap_t *gc, void *ptr) {
    if (!gc || !ptr) return;
    
    /* Find and remove the object */
    for (int i = 0; i < gc->object_count; i++) {
        if (gc->objects[i].ptr == ptr) {
            gc->bytes_allocated -= gc->objects[i].size;
            free(ptr);
            
            /* Remove from list */
            gc->objects[i] = gc->objects[gc->object_count - 1];
            gc->object_count--;
            return;
        }
    }
}

/* Mark reachable objects */
void gc_mark_reachable(vitte_vm_t *vm, gc_heap_t *gc) {
    if (!gc) return;
    
    gc_reset(gc);
    
    /* Mark values on stack */
    for (int i = 0; i < vm->stack_top; i++) {
        gc_mark_value(vm->stack[i], gc);
    }
    
    /* Mark values in registers */
    for (int i = 0; i < VITTE_REGISTERS; i++) {
        gc_mark_value(vm->registers[i], gc);
    }
}

void gc_mark_value(vitte_value_t value, gc_heap_t *gc) {
    if (!gc) return;
    
    switch (value.type) {
        case VITTE_STRING: {
            /* Mark string object */
            if (value.as.string.data) {
                for (int i = 0; i < gc->object_count; i++) {
                    if (gc->objects[i].ptr == value.as.string.data) {
                        gc->objects[i].is_marked = true;
                        gc->marked_count++;
                        break;
                    }
                }
            }
            break;
        }
        case VITTE_ARRAY: {
            /* Mark array and its contents */
            if (value.as.pointer) {
                vitte_array_t *arr = (vitte_array_t *)value.as.pointer;
                for (int i = 0; i < gc->object_count; i++) {
                    if (gc->objects[i].ptr == arr) {
                        gc->objects[i].is_marked = true;
                        gc->marked_count++;
                        break;
                    }
                }
                
                /* Mark array elements */
                for (int i = 0; i < arr->count; i++) {
                    gc_mark_value(arr->values[i], gc);
                }
            }
            break;
        }
        default:
            break;
    }
}

/* Sweep unreachable objects */
void gc_sweep(gc_heap_t *gc) {
    if (!gc) return;
    
    int i = 0;
    while (i < gc->object_count) {
        if (!gc->objects[i].is_marked) {
            /* Unreachable - collect it */
            gc->bytes_allocated -= gc->objects[i].size;
            free(gc->objects[i].ptr);
            gc->collected_count++;
            
            /* Remove from list */
            gc->objects[i] = gc->objects[gc->object_count - 1];
            gc->object_count--;
        } else {
            /* Reachable - unmark for next collection */
            gc->objects[i].is_marked = false;
            i++;
        }
    }
}

/* Full GC cycle */
void gc_collect(vitte_vm_t *vm, gc_heap_t *gc) {
    if (!gc || !vm) return;
    
    gc->gc_runs++;
    
    /* Mark phase */
    gc_mark_reachable(vm, gc);
    
    /* Sweep phase */
    gc_sweep(gc);
    
    /* Update threshold */
    gc->next_gc_threshold = gc->bytes_allocated * 2;
    if (gc->next_gc_threshold < 1024 * 1024) {
        gc->next_gc_threshold = 1024 * 1024;  /* At least 1MB */
    }
}

/* Statistics */
void gc_print_stats(gc_heap_t *gc) {
    if (!gc) return;
    
    printf("=== GC Statistics ===\n");
    printf("GC Runs: %d\n", gc->gc_runs);
    printf("Objects Tracked: %d\n", gc->object_count);
    printf("Bytes Allocated: %zu\n", gc->bytes_allocated);
    printf("Next GC Threshold: %zu\n", gc->next_gc_threshold);
    printf("Objects Collected: %d\n", gc->collected_count);
}

size_t gc_bytes_allocated(gc_heap_t *gc) {
    if (!gc) return 0;
    return gc->bytes_allocated;
}
