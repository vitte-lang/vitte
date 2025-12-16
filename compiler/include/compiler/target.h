#ifndef COMPILER_TARGET_H
#define COMPILER_TARGET_H

#include <stdint.h>

typedef enum {
    ARCH_X86_64,
    ARCH_AARCH64,
    ARCH_WASM,
} target_arch_t;

typedef enum {
    OS_LINUX,
    OS_WINDOWS,
    OS_MACOS,
} target_os_t;

typedef struct target_info {
    target_arch_t arch;
    target_os_t os;
    uint32_t pointer_size;
    uint32_t alignment;
} target_info_t;

target_info_t* target_info_create(target_arch_t arch, target_os_t os);
void target_info_destroy(target_info_t *ti);

#endif /* COMPILER_TARGET_H */
