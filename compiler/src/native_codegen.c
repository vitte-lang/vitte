#include "compiler/native_codegen.h"
#include <stdlib.h>
#include <string.h>

/* X86-64 register names */
static const char* x86_64_registers[] = {
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8", "r9",
    "r10", "r11", "r12", "r13", "r14", "r15"
};

/* ARM64 register names */
static const char* aarch64_registers[] = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",
    "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15"
};

native_codegen_t* native_codegen_create(FILE *out, native_target_t target) {
    native_codegen_t *gen = (native_codegen_t *)malloc(sizeof(native_codegen_t));
    if (!gen) return NULL;
    
    gen->out = out;
    gen->target = target;
    gen->indent_level = 0;
    gen->label_count = 0;
    gen->temp_var_count = 0;
    
    return gen;
}

void native_codegen_destroy(native_codegen_t *gen) {
    if (!gen) return;
    free(gen);
}

int native_codegen_emit_function(native_codegen_t *gen, void *ir_func) {
    if (!gen || !ir_func) return -1;
    
    /* TODO: Implement native code generation from IR
     * - Instruction selection
     * - Register allocation
     * - Code emission
     */
    
    return 0;
}

int native_codegen_emit_preamble(native_codegen_t *gen) {
    if (!gen) return -1;
    
    if (gen->target == NATIVE_X86_64) {
        fprintf(gen->out, ".globl main\n");
        fprintf(gen->out, ".text\n");
        fprintf(gen->out, "main:\n");
        fprintf(gen->out, "    push %%rbp\n");
        fprintf(gen->out, "    mov %%rsp, %%rbp\n");
    } else if (gen->target == NATIVE_AARCH64) {
        fprintf(gen->out, ".global main\n");
        fprintf(gen->out, ".section .text\n");
        fprintf(gen->out, "main:\n");
        fprintf(gen->out, "    stp x29, x30, [sp, #-16]!\n");
        fprintf(gen->out, "    mov x29, sp\n");
    }
    
    return 0;
}

int native_codegen_emit_epilogue(native_codegen_t *gen) {
    if (!gen) return -1;
    
    if (gen->target == NATIVE_X86_64) {
        fprintf(gen->out, "    mov %%rbp, %%rsp\n");
        fprintf(gen->out, "    pop %%rbp\n");
        fprintf(gen->out, "    ret\n");
    } else if (gen->target == NATIVE_AARCH64) {
        fprintf(gen->out, "    ldp x29, x30, [sp], #16\n");
        fprintf(gen->out, "    ret\n");
    }
    
    return 0;
}

const char* native_get_register(native_target_t target, int idx) {
    if (target == NATIVE_X86_64) {
        if (idx >= 0 && idx < 14) {
            return x86_64_registers[idx];
        }
    } else if (target == NATIVE_AARCH64) {
        if (idx >= 0 && idx < 16) {
            return aarch64_registers[idx];
        }
    }
    return NULL;
}

uint32_t native_get_label(native_codegen_t *gen) {
    if (!gen) return 0;
    return gen->label_count++;
}
