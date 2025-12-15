#ifndef STEEL_SYMBOLTABLE_H
#define STEEL_SYMBOLTABLE_H

#include <stddef.h>

typedef struct {
    char name[256];
    int type;
    int scope;
} Symbol;

typedef struct {
    Symbol *symbols;
    size_t count;
    size_t capacity;
} SymbolTable;

SymbolTable *symboltable_create(void);
void symboltable_insert(SymbolTable *st, const char *name, Symbol sym);
Symbol *symboltable_lookup(SymbolTable *st, const char *name);
void symboltable_destroy(SymbolTable *st);

#endif
