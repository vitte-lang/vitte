#include "compiler/types.h"
#include <stdlib.h>
#include <string.h>

type_table_t* type_table_create(void) {
    type_table_t *t = (type_table_t *)malloc(sizeof(type_table_t));
    if (!t) return NULL;
    
    t->capacity = 64;
    t->count = 0;
    t->entries = (void *)calloc(t->capacity, sizeof(void *));
    
    if (!t->entries) {
        free(t);
        return NULL;
    }
    
    return t;
}

void type_table_destroy(type_table_t *t) {
    if (!t) return;
    free(t->entries);
    free(t);
}

type_t* type_table_lookup(type_table_t *t, const char *name) {
    if (!t || !name) return NULL;
    // TODO: Implement lookup logic
    return NULL;
}

void type_table_insert(type_table_t *t, const char *name, type_t *ty) {
    if (!t || !name || !ty) return;
    // TODO: Implement insert logic
}
