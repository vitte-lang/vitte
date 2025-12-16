#ifndef COMPILER_OPTIMIZER_H
#define COMPILER_OPTIMIZER_H

typedef struct optimizer {
    int opt_level;
    int inline_threshold;
} optimizer_t;

optimizer_t* optimizer_create(int opt_level);
void optimizer_destroy(optimizer_t *opt);

void* optimizer_optimize_ir(optimizer_t *opt, void *ir_module);

#endif /* COMPILER_OPTIMIZER_H */
