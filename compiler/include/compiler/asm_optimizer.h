#ifndef COMPILER_ASM_OPTIMIZER_H
#define COMPILER_ASM_OPTIMIZER_H

#include <stdint.h>

/* Assembly code optimization and analysis */

typedef enum {
    ASM_OPT_NONE = 0,
    ASM_OPT_PEEPHOLE = 1 << 0,      /* Peephole optimization */
    ASM_OPT_INLINE = 1 << 1,         /* Inline small functions */
    ASM_OPT_REGISTER = 1 << 2,       /* Register allocation */
    ASM_OPT_SCHEDULING = 1 << 3,     /* Instruction scheduling */
} asm_opt_flags_t;

typedef struct asm_optimizer {
    asm_opt_flags_t flags;
    int optimization_level;  /* 0-3 */
} asm_optimizer_t;

/* Create optimizer */
asm_optimizer_t* asm_optimizer_create(int opt_level);
void asm_optimizer_destroy(asm_optimizer_t *opt);

/* Optimization passes */
void asm_optimizer_peephole(asm_optimizer_t *opt, void *ir);
void asm_optimizer_register_alloc(asm_optimizer_t *opt, void *ir);
void asm_optimizer_schedule(asm_optimizer_t *opt, void *ir);

#endif /* COMPILER_ASM_OPTIMIZER_H */
