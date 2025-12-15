#ifndef STEEL_GC_H
#define STEEL_GC_H

#include <stddef.h>

typedef struct {
    void **objects;
    size_t count;
    size_t capacity;
} GC;

GC *gc_create(void);
void gc_mark_object(GC *gc, void *obj);
void gc_sweep(GC *gc);
void gc_destroy(GC *gc);

#endif
