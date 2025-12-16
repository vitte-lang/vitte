#ifndef COMPILER_SEMA_H
#define COMPILER_SEMA_H

typedef struct sema_ctx {
    void *type_table;
    void *symbol_table;
    void *diag;
} sema_ctx_t;

sema_ctx_t* sema_create(void);
void sema_destroy(sema_ctx_t *ctx);

int sema_analyze(sema_ctx_t *ctx, void *ast);

#endif /* COMPILER_SEMA_H */
