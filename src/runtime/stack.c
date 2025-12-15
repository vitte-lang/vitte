#include "../../include/runtime/stack.h"
#include <stdlib.h>

Stack *stack_create(size_t capacity) {
    Stack *s = malloc(sizeof(Stack));
    s->values = malloc(sizeof(Value) * capacity);
    s->size = 0;
    s->capacity = capacity;
    return s;
}

void stack_push(Stack *s, Value v) {
    if (s->size >= s->capacity) {
        s->capacity *= 2;
        s->values = realloc(s->values, sizeof(Value) * s->capacity);
    }
    s->values[s->size++] = v;
}

Value stack_pop(Stack *s) {
    return s->values[--s->size];
}

void stack_destroy(Stack *s) {
    free(s->values);
    free(s);
}
