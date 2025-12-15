#include "../../include/compiler/optimizer.h"

Optimizer *optimizer_create(void) {
    Optimizer *opt = malloc(sizeof(Optimizer));
    opt->passes = 0;
    return opt;
}

void optimizer_optimize(Optimizer *opt, AST *ast) {
    // Constant folding
    // Dead code elimination
    // Loop unrolling
    // Inlining
    opt->passes++;
}

void optimizer_destroy(Optimizer *opt) {
    free(opt);
}
