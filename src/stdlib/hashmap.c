#include "../../include/stdlib/hashmap.h"
#include <stdlib.h>
#include <string.h>

HashMap *hashmap_create(size_t capacity) {
    HashMap *map = malloc(sizeof(HashMap));
    map->entries = malloc(sizeof(HashEntry) * capacity);
    map->capacity = capacity;
    map->size = 0;
    return map;
}

static size_t hash_function(const char *key) {
    size_t hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

void hashmap_set(HashMap *map, const char *key, Value val) {
    size_t index = hash_function(key) % map->capacity;
    map->entries[index].key = strdup(key);
    map->entries[index].value = val;
}

Value *hashmap_get(HashMap *map, const char *key) {
    size_t index = hash_function(key) % map->capacity;
    if (strcmp(map->entries[index].key, key) == 0) {
        return &map->entries[index].value;
    }
    return NULL;
}

void hashmap_destroy(HashMap *map) {
    free(map->entries);
    free(map);
}
