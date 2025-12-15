#ifndef STEEL_HASHMAP_H
#define STEEL_HASHMAP_H

#include "../runtime/value.h"
#include <stddef.h>

typedef struct {
    char *key;
    Value value;
} HashEntry;

typedef struct {
    HashEntry *entries;
    size_t capacity;
    size_t size;
} HashMap;

HashMap *hashmap_create(size_t capacity);
void hashmap_set(HashMap *map, const char *key, Value val);
Value *hashmap_get(HashMap *map, const char *key);
void hashmap_destroy(HashMap *map);

#endif
