#ifndef COMPILER_HIR_H
#define COMPILER_HIR_H

#include <stdint.h>

typedef enum {
    HIR_NODE_MODULE,
    HIR_NODE_FUNC,
    HIR_NODE_BLOCK,
    HIR_NODE_STMT,
    HIR_NODE_EXPR,
    HIR_NODE_CALL,
    HIR_NODE_LOAD,
    HIR_NODE_STORE,
    HIR_NODE_BINOP,
} hir_node_kind_t;

typedef struct hir_node {
    hir_node_kind_t kind;
    void *ty;
    void *data;
} hir_node_t;

typedef struct hir_module {
    hir_node_t **funcs;
    uint32_t func_count;
    uint32_t func_capacity;
} hir_module_t;

hir_module_t* hir_module_create(void);
void hir_module_destroy(hir_module_t *mod);
void hir_module_add_func(hir_module_t *mod, hir_node_t *func);

#endif /* COMPILER_HIR_H */
