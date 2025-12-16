#ifndef COMPILER_SYMBOL_TABLE_H
#define COMPILER_SYMBOL_TABLE_H

#include <stdint.h>

typedef enum {
    SYM_VAR,
    SYM_FUNC,
    SYM_TYPE,
    SYM_CONST,
} symbol_kind_t;

typedef struct symbol {
    const char *name;
    symbol_kind_t kind;
    void *type;
    void *value;
    uint32_t scope_level;
} symbol_t;

typedef struct symbol_table {
    symbol_t **symbols;
    uint32_t count;
    uint32_t capacity;
    uint32_t scope_level;
} symbol_table_t;

symbol_table_t* symbol_table_create(void);
void symbol_table_destroy(symbol_table_t *st);

void symbol_table_enter_scope(symbol_table_t *st);
void symbol_table_exit_scope(symbol_table_t *st);

symbol_t* symbol_table_lookup(symbol_table_t *st, const char *name);
void symbol_table_insert(symbol_table_t *st, const char *name, symbol_kind_t kind, void *type);

#endif /* COMPILER_SYMBOL_TABLE_H */
