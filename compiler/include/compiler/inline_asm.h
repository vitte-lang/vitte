#ifndef COMPILER_INLINE_ASM_H
#define COMPILER_INLINE_ASM_H

#include <stdint.h>
#include <stdio.h>

/* Inline assembly support for C code generation */

typedef enum {
    ASM_ARCH_X86_64,
    ASM_ARCH_AARCH64,
    ASM_ARCH_WASM,
} asm_arch_t;

typedef enum {
    ASM_SYNTAX_ATT,      /* AT&T syntax (gas) */
    ASM_SYNTAX_INTEL,    /* Intel syntax (nasm, masm) */
} asm_syntax_t;

typedef struct asm_constraint {
    char reg;            /* Register constraint: r, a, b, c, d, etc. */
    const char *type;    /* Type: "r" (register), "m" (memory), "i" (immediate) */
} asm_constraint_t;

typedef struct asm_operand {
    const char *constraint;
    const char *variable;
    int index;
} asm_operand_t;

typedef struct inline_asm {
    const char *template;       /* Assembly template string */
    asm_operand_t *inputs;      /* Input operands */
    uint32_t input_count;
    asm_operand_t *outputs;     /* Output operands */
    uint32_t output_count;
    const char **clobbers;      /* Clobbered registers */
    uint32_t clobber_count;
    int volatile_flag;          /* Mark as volatile */
} inline_asm_t;

/* Create inline assembly structure */
inline_asm_t* inline_asm_create(const char *template);
void inline_asm_destroy(inline_asm_t *asm_code);

/* Add operands */
void inline_asm_add_input(inline_asm_t *asm_code, const char *constraint, const char *variable);
void inline_asm_add_output(inline_asm_t *asm_code, const char *constraint, const char *variable);
void inline_asm_add_clobber(inline_asm_t *asm_code, const char *reg);

/* Mark as volatile */
void inline_asm_set_volatile(inline_asm_t *asm_code, int volatile_flag);

/* Emit to C */
void inline_asm_emit_gcc(FILE *out, inline_asm_t *asm_code);
void inline_asm_emit_msvc(FILE *out, inline_asm_t *asm_code);

#endif /* COMPILER_INLINE_ASM_H */
