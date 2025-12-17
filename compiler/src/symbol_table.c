#include "compiler/symbol_table.h"
#include <stdlib.h>
#include <string.h>

symbol_table_t* symbol_table_create(void) {
    symbol_table_t *st = (symbol_table_t *)malloc(sizeof(symbol_table_t));
    if (!st) return NULL;
    
    st->capacity = 128;
    st->count = 0;
    st->scope_level = 0;
    st->symbols = (symbol_t **)calloc(st->capacity, sizeof(symbol_t *));
    
    if (!st->symbols) {
        free(st);
        return NULL;
    }
    
    return st;
}

void symbol_table_destroy(symbol_table_t *st) {
    if (!st) return;
    free(st->symbols);
    free(st);
}

void symbol_table_enter_scope(symbol_table_t *st) {
    if (!st) return;
    st->scope_level++;
}

void symbol_table_exit_scope(symbol_table_t *st) {
    if (!st) return;
    if (st->scope_level > 0) st->scope_level--;
}

symbol_t* symbol_table_lookup(symbol_table_t *st, const char *name) {
    if (!st || !name) return NULL;
    // TODO: Implement lookup logic
    return NULL;
}

void symbol_table_insert(symbol_table_t *st, const char *name, symbol_kind_t kind, void *type) {
    if (!st || !name) return;
    (void)kind;
    (void)type;
    // TODO: Implement insert logic
}
