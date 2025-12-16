#ifndef COMPILER_LOWERING_H
#define COMPILER_LOWERING_H

typedef struct lowering_ctx {
    void *type_table;
    void *target_info;
} lowering_ctx_t;

lowering_ctx_t* lowering_ctx_create(void);
void lowering_ctx_destroy(lowering_ctx_t *ctx);

void* lower_hir_to_ir(lowering_ctx_t *ctx, void *hir);

#endif /* COMPILER_LOWERING_H */
