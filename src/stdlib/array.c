#include "../../include/stdlib/array.h"
#include <stdlib.h>

Array *array_create(size_t capacity) {
    Array *arr = malloc(sizeof(Array));
    arr->elements = malloc(sizeof(Value) * capacity);
    arr->size = 0;
    arr->capacity = capacity;
    return arr;
}

void array_push(Array *arr, Value val) {
    if (arr->size >= arr->capacity) {
        arr->capacity *= 2;
        arr->elements = realloc(arr->elements, sizeof(Value) * arr->capacity);
    }
    arr->elements[arr->size++] = val;
}

Value array_get(Array *arr, size_t index) {
    return arr->elements[index];
}

void array_destroy(Array *arr) {
    free(arr->elements);
    free(arr);
}
