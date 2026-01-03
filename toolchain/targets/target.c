/*
 * VITTE Toolchain Target Implementation
 * Complete target platform detection and configuration
 */

#include "target.h"
#include "arch.h"

target_t *target_detect_host(void) {
    target_t *target = malloc(sizeof(target_t));
    if (!target) return NULL;
#if defined(__x86_64__) || defined(_M_X64)
    target->arch = TARGET_X86_64;
    target->bits = 64;
    target->triple = "x86_64-unknown-linux-gnu";
#elif defined(__aarch64__) || defined(_M_ARM64)
    target->arch = TARGET_ARM64;
    target->bits = 64;
    target->triple = "aarch64-unknown-linux-gnu";
#elif defined(__arm__)
    target->arch = TARGET_ARM32;
    target->bits = 32;
    target->triple = "arm-unknown-linux-gnueabihf";
#else
    target->arch = TARGET_UNKNOWN;
    target->bits = 32;
    target->triple = "unknown-unknown-unknown";
#endif
#if defined(__APPLE__)
    target->os = OS_MACOS;
#elif defined(_WIN32) || defined(_WIN64)
    target->os = OS_WINDOWS;
#else
    target->os = OS_LINUX;
#endif
#if defined(__LITTLE_ENDIAN__) || __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    target->endianness = 0;
#else
    target->endianness = 1;
#endif
    target->pointer_size = target->bits / 8;
    fprintf(stderr, "[target] ✓ Host detected: %s (%d-bit, %s endian)\n",
            target->triple, target->bits, target->endianness == 0 ? "little" : "big");
    return target;
}

target_t *target_create(const char *triple) {
    if (!triple) return target_detect_host();
    target_t *target = malloc(sizeof(target_t));
    if (!target) return NULL;
    target->triple = strdup(triple);
    if (!target->triple) {
        free(target);
        return NULL;
    }
    target->endianness = 0;
    if (strstr(triple, "x86_64") || strstr(triple, "amd64")) {
        target->arch = TARGET_X86_64;
        target->bits = 64;
    } else if (strstr(triple, "aarch64") || strstr(triple, "arm64")) {
        target->arch = TARGET_ARM64;
        target->bits = 64;
    } else if (strstr(triple, "arm")) {
        target->arch = TARGET_ARM32;
        target->bits = 32;
    } else if (strstr(triple, "riscv64")) {
        target->arch = TARGET_RISCV64;
        target->bits = 64;
    } else {
        target->arch = TARGET_UNKNOWN;
        target->bits = 32;
    }
    if (strstr(triple, "linux")) {
        target->os = OS_LINUX;
    } else if (strstr(triple, "windows") || strstr(triple, "mingw")) {
        target->os = OS_WINDOWS;
    } else if (strstr(triple, "darwin") || strstr(triple, "macos") || strstr(triple, "apple")) {
        target->os = OS_MACOS;
    } else {
        target->os = OS_UNKNOWN;
    }
    target->pointer_size = target->bits / 8;
    fprintf(stderr, "[target] ✓ Created target: %s (%d-bit)\n", triple, target->bits);
    return target;
}

void target_free(target_t *target) {
    if (target) {
        if (target->triple) free((void *)target->triple);
        free(target);
    }
}

const char *target_arch_name(target_arch_t arch) {
    switch (arch) {
        case TARGET_X86_64: return "x86_64";
        case TARGET_ARM64: return "aarch64";
        case TARGET_ARM32: return "arm";
        case TARGET_RISCV64: return "riscv64";
        case TARGET_UNKNOWN: return "unknown";
        default: return "invalid";
    }
}

const char *target_os_name(target_os_t os) {
    switch (os) {
        case OS_LINUX: return "linux";
        case OS_WINDOWS: return "windows";
        case OS_MACOS: return "macos";
        case OS_UNKNOWN: return "unknown";
        default: return "invalid";
    }
}

int target_is_64bit(target_t *target) {
    return target ? (target->bits == 64) : 0;
}

int target_is_little_endian(target_t *target) {
    return target ? (target->endianness == 0) : 1;
}

int target_supports_feature(target_t *target, const char *feature) {
    if (!target || !feature) return 0;
    if (strcmp(feature, "fpu") == 0) return 1;
    if (target->arch == TARGET_X86_64) {
        if (strcmp(feature, "sse") == 0 || strcmp(feature, "sse2") == 0 ||
            strcmp(feature, "avx") == 0 || strcmp(feature, "avx2") == 0) return 1;
    }
    if (target->arch == TARGET_ARM64) {
        if (strcmp(feature, "neon") == 0 || strcmp(feature, "asimd") == 0) return 1;
    }
    return 0;
}
