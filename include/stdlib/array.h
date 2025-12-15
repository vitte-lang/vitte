#ifndef STEEL_ARRAY_H
#define STEEL_ARRAY_H

#include "../runtime/value.h"
#include <stddef.h>

typedef struct {
    Value *elements;
    size_t size;
    size_t capacity;
} Array;

Array *array_create(size_t capacity);
void array_push(Array *arr, Value val);
Value array_get(Array *arr, size_t index);
void array_destroy(Array *arr);

#endif
