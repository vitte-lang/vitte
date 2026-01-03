/*
 * VITTE Toolchain Architecture Implementation
 * Complete architecture specification and register management
 */

#include "arch.h"

static arch_info_t x86_64_info = {
    .register_count = 16,
    .stack_align = 16,
    .frame_pointer_reg = 6,
    .stack_pointer_reg = 7,
    .registers = {
        {"rax", REG_CLASS_INT, 64, 0}, {"rbx", REG_CLASS_INT, 64, 1},
        {"rcx", REG_CLASS_INT, 64, 0}, {"rdx", REG_CLASS_INT, 64, 0},
        {"rsi", REG_CLASS_INT, 64, 0}, {"rdi", REG_CLASS_INT, 64, 0},
        {"rbp", REG_CLASS_INT, 64, 1}, {"rsp", REG_CLASS_INT, 64, 1},
        {"r8", REG_CLASS_INT, 64, 0}, {"r9", REG_CLASS_INT, 64, 0},
        {"r10", REG_CLASS_INT, 64, 0}, {"r11", REG_CLASS_INT, 64, 0},
        {"r12", REG_CLASS_INT, 64, 1}, {"r13", REG_CLASS_INT, 64, 1},
        {"r14", REG_CLASS_INT, 64, 1}, {"r15", REG_CLASS_INT, 64, 1}
    }
};

static arch_info_t arm64_info = {
    .register_count = 31,
    .stack_align = 16,
    .frame_pointer_reg = 29,
    .stack_pointer_reg = 31,
    .registers = {
        {"x0", REG_CLASS_INT, 64, 0}, {"x1", REG_CLASS_INT, 64, 0},
        {"x2", REG_CLASS_INT, 64, 0}, {"x3", REG_CLASS_INT, 64, 0},
        {"x4", REG_CLASS_INT, 64, 0}, {"x5", REG_CLASS_INT, 64, 0},
        {"x6", REG_CLASS_INT, 64, 0}, {"x7", REG_CLASS_INT, 64, 0},
        {"x8", REG_CLASS_INT, 64, 0}, {"x9", REG_CLASS_INT, 64, 0},
        {"x10", REG_CLASS_INT, 64, 0}, {"x11", REG_CLASS_INT, 64, 0},
        {"x12", REG_CLASS_INT, 64, 0}, {"x13", REG_CLASS_INT, 64, 0},
        {"x14", REG_CLASS_INT, 64, 0}, {"x15", REG_CLASS_INT, 64, 0},
    }
};

arch_info_t *arch_get_info(const char *arch_name) {
    if (!arch_name) return NULL;
    arch_info_t *info = malloc(sizeof(arch_info_t));
    if (!info) return NULL;
    if (strcmp(arch_name, "x86_64") == 0) {
        memcpy(info, &x86_64_info, sizeof(arch_info_t));
        fprintf(stderr, "[arch] ✓ Loaded x86_64: %d registers, stack alignment %d bytes\n",
                info->register_count, info->stack_align);
    } else if (strcmp(arch_name, "aarch64") == 0 || strcmp(arch_name, "arm64") == 0) {
        memcpy(info, &arm64_info, sizeof(arch_info_t));
        fprintf(stderr, "[arch] ✓ Loaded aarch64: %d registers, stack alignment %d bytes\n",
                info->register_count, info->stack_align);
    } else {
        fprintf(stderr, "[arch] ✗ Unknown architecture: %s\n", arch_name);
        info->register_count = 0;
        return info;
    }
    fprintf(stderr, "[arch]   FP register: %d, SP register: %d\n",
            info->frame_pointer_reg, info->stack_pointer_reg);
    return info;
}

void arch_free_info(arch_info_t *info) {
    if (info) free(info);
}

int arch_get_register_count(arch_info_t *info) {
    return info ? info->register_count : 0;
}

const register_spec_t *arch_get_register(arch_info_t *info, int index) {
    if (!info || index < 0 || index >= info->register_count) return NULL;
    return &info->registers[index];
}

int arch_register_by_name(arch_info_t *info, const char *name) {
    if (!info || !name) return -1;
    for (int i = 0; i < info->register_count; i++) {
        if (strcmp(info->registers[i].name, name) == 0) {
            fprintf(stderr, "[arch] ✓ Found register '%s' at index %d\n", name, i);
            return i;
        }
    }
    fprintf(stderr, "[arch] ✗ Register '%s' not found\n", name);
    return -1;
}
