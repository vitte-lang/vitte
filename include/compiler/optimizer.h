#ifndef STEEL_OPTIMIZER_H
#define STEEL_OPTIMIZER_H

#include "ast.h"

typedef struct {
    int passes;
} Optimizer;

Optimizer *optimizer_create(void);
void optimizer_optimize(Optimizer *opt, AST *ast);
void optimizer_destroy(Optimizer *opt);

#endif
