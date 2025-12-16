#include "compiler/hir.h"
#include <stdlib.h>

hir_module_t* hir_module_create(void) {
    hir_module_t *mod = (hir_module_t *)malloc(sizeof(hir_module_t));
    if (!mod) return NULL;
    
    mod->func_capacity = 16;
    mod->func_count = 0;
    mod->funcs = (hir_node_t **)calloc(mod->func_capacity, sizeof(hir_node_t *));
    
    if (!mod->funcs) {
        free(mod);
        return NULL;
    }
    
    return mod;
}

void hir_module_destroy(hir_module_t *mod) {
    if (!mod) return;
    free(mod->funcs);
    free(mod);
}

void hir_module_add_func(hir_module_t *mod, hir_node_t *func) {
    if (!mod || !func) return;
    
    if (mod->func_count >= mod->func_capacity) {
        mod->func_capacity *= 2;
        mod->funcs = (hir_node_t **)realloc(mod->funcs, mod->func_capacity * sizeof(hir_node_t *));
    }
    
    mod->funcs[mod->func_count++] = func;
}
