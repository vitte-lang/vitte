#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "hir.h"
#include "ir.h"
#include "types.h"
#include "driver.h"

typedef struct compiler_ctx {
    void *alloc;
    void *ast_module;
    void *hir_module;
    void *ir_module;
    void *type_table;
} compiler_ctx_t;

compiler_ctx_t* compiler_create(void);
void compiler_destroy(compiler_ctx_t *ctx);

#endif /* COMPILER_H */
