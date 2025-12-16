#ifndef COMPILER_TYPES_H
#define COMPILER_TYPES_H

#include <stdint.h>

typedef enum {
    TYPE_VOID,
    TYPE_I8,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,
    TYPE_F32,
    TYPE_F64,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_STR,
    TYPE_PTR,
    TYPE_ARRAY,
    TYPE_STRUCT,
    TYPE_ENUM,
    TYPE_FUNC,
    TYPE_ALIAS,
} type_kind_t;

typedef struct type {
    type_kind_t kind;
    uint32_t size;
    uint32_t align;
    void *extra;
} type_t;

typedef struct type_table {
    void *entries;
    uint32_t count;
    uint32_t capacity;
} type_table_t;

type_table_t* type_table_create(void);
void type_table_destroy(type_table_t *t);

type_t* type_table_lookup(type_table_t *t, const char *name);
void type_table_insert(type_table_t *t, const char *name, type_t *ty);

#endif /* COMPILER_TYPES_H */
