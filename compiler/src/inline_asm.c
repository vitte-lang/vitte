#include "compiler/inline_asm.h"
#include <stdlib.h>
#include <string.h>

inline_asm_t* inline_asm_create(const char *template) {
    inline_asm_t *asm_code = (inline_asm_t *)malloc(sizeof(inline_asm_t));
    if (!asm_code) return NULL;
    
    asm_code->template = template;
    asm_code->input_count = 0;
    asm_code->output_count = 0;
    asm_code->clobber_count = 0;
    asm_code->volatile_flag = 0;
    
    asm_code->inputs = (asm_operand_t *)calloc(16, sizeof(asm_operand_t));
    asm_code->outputs = (asm_operand_t *)calloc(16, sizeof(asm_operand_t));
    asm_code->clobbers = (const char **)calloc(16, sizeof(const char *));
    
    if (!asm_code->inputs || !asm_code->outputs || !asm_code->clobbers) {
        free(asm_code->inputs);
        free(asm_code->outputs);
        free(asm_code->clobbers);
        free(asm_code);
        return NULL;
    }
    
    return asm_code;
}

void inline_asm_destroy(inline_asm_t *asm_code) {
    if (!asm_code) return;
    
    free(asm_code->inputs);
    free(asm_code->outputs);
    free(asm_code->clobbers);
    free(asm_code);
}

void inline_asm_add_input(inline_asm_t *asm_code, const char *constraint, const char *variable) {
    if (!asm_code || !constraint || !variable) return;
    
    if (asm_code->input_count < 16) {
        asm_code->inputs[asm_code->input_count].constraint = constraint;
        asm_code->inputs[asm_code->input_count].variable = variable;
        asm_code->inputs[asm_code->input_count].index = asm_code->input_count;
        asm_code->input_count++;
    }
}

void inline_asm_add_output(inline_asm_t *asm_code, const char *constraint, const char *variable) {
    if (!asm_code || !constraint || !variable) return;
    
    if (asm_code->output_count < 16) {
        asm_code->outputs[asm_code->output_count].constraint = constraint;
        asm_code->outputs[asm_code->output_count].variable = variable;
        asm_code->outputs[asm_code->output_count].index = asm_code->output_count;
        asm_code->output_count++;
    }
}

void inline_asm_add_clobber(inline_asm_t *asm_code, const char *reg) {
    if (!asm_code || !reg) return;
    
    if (asm_code->clobber_count < 16) {
        asm_code->clobbers[asm_code->clobber_count] = reg;
        asm_code->clobber_count++;
    }
}

void inline_asm_set_volatile(inline_asm_t *asm_code, int volatile_flag) {
    if (!asm_code) return;
    asm_code->volatile_flag = volatile_flag;
}

/* GCC inline assembly format */
void inline_asm_emit_gcc(FILE *out, inline_asm_t *asm_code) {
    if (!out || !asm_code) return;
    
    fprintf(out, "__asm__ %s(\n", asm_code->volatile_flag ? "volatile" : "");
    fprintf(out, "    \"%s\"\n", asm_code->template);
    
    /* Output constraints */
    if (asm_code->output_count > 0) {
        fprintf(out, "    : ");
        for (uint32_t i = 0; i < asm_code->output_count; i++) {
            if (i > 0) fprintf(out, ", ");
            fprintf(out, "\"%s\"(%s)", asm_code->outputs[i].constraint, asm_code->outputs[i].variable);
        }
        fprintf(out, "\n");
    }
    
    /* Input constraints */
    if (asm_code->input_count > 0) {
        fprintf(out, "    : ");
        for (uint32_t i = 0; i < asm_code->input_count; i++) {
            if (i > 0) fprintf(out, ", ");
            fprintf(out, "\"%s\"(%s)", asm_code->inputs[i].constraint, asm_code->inputs[i].variable);
        }
        fprintf(out, "\n");
    }
    
    /* Clobbers */
    if (asm_code->clobber_count > 0) {
        fprintf(out, "    : ");
        for (uint32_t i = 0; i < asm_code->clobber_count; i++) {
            if (i > 0) fprintf(out, ", ");
            fprintf(out, "\"%s\"", asm_code->clobbers[i]);
        }
        fprintf(out, "\n");
    }
    
    fprintf(out, ");\n");
}

/* MSVC inline assembly format */
void inline_asm_emit_msvc(FILE *out, inline_asm_t *asm_code) {
    if (!out || !asm_code) return;
    
    fprintf(out, "__asm {\n");
    fprintf(out, "    %s\n", asm_code->template);
    fprintf(out, "}\n");
}
