#include "../../include/compiler/symboltable.h"
#include <stdlib.h>
#include <string.h>

SymbolTable *symboltable_create(void) {
    SymbolTable *st = malloc(sizeof(SymbolTable));
    st->symbols = NULL;
    st->count = 0;
    st->capacity = 0;
    return st;
}

void symboltable_insert(SymbolTable *st, const char *name, Symbol sym) {
    if (st->count >= st->capacity) {
        st->capacity = (st->capacity + 1) * 2;
        st->symbols = realloc(st->symbols, sizeof(Symbol) * st->capacity);
    }
    strcpy(st->symbols[st->count].name, name);
    st->symbols[st->count].type = sym.type;
    st->count++;
}

Symbol *symboltable_lookup(SymbolTable *st, const char *name) {
    for (size_t i = 0; i < st->count; i++) {
        if (strcmp(st->symbols[i].name, name) == 0) {
            return &st->symbols[i];
        }
    }
    return NULL;
}

void symboltable_destroy(SymbolTable *st) {
    free(st->symbols);
    free(st);
}
