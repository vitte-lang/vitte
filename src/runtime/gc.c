#include "../../include/runtime/gc.h"
#include <stdlib.h>

GC *gc_create(void) {
    GC *gc = malloc(sizeof(GC));
    gc->objects = NULL;
    gc->count = 0;
    gc->capacity = 0;
    return gc;
}

void gc_mark_object(GC *gc, void *obj) {
    // TODO: Implement marking
}

void gc_sweep(GC *gc) {
    // TODO: Implement sweep
}

void gc_destroy(GC *gc) {
    free(gc->objects);
    free(gc);
}
