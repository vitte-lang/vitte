/*
 * CloneVitte
 * Public Target API
 *
 * Copyright (C) 2026 VitteFoundation
 * SPDX-License-Identifier: LicenseRef-VFPL-1.0
 */

#ifndef CLONEVITTE_TARGET_H
#define CLONEVITTE_TARGET_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <clonevitte/status.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Architecture
 * ============================================================ */

typedef enum CloneVitteArchitecture {
    CLONEVITTE_ARCH_UNKNOWN = 0,

    CLONEVITTE_ARCH_X86,
    CLONEVITTE_ARCH_X86_64,

    CLONEVITTE_ARCH_ARM,
    CLONEVITTE_ARCH_ARMV6,
    CLONEVITTE_ARCH_ARMV7,
    CLONEVITTE_ARCH_AARCH64,

    CLONEVITTE_ARCH_RISCV32,
    CLONEVITTE_ARCH_RISCV64,

    CLONEVITTE_ARCH_MIPS,
    CLONEVITTE_ARCH_MIPS64,

    CLONEVITTE_ARCH_PPC,
    CLONEVITTE_ARCH_PPC64,
    CLONEVITTE_ARCH_PPC64LE,

    CLONEVITTE_ARCH_S390X,

    CLONEVITTE_ARCH_WASM32,
    CLONEVITTE_ARCH_WASM64
} CloneVitteArchitecture;

/* ============================================================
 * Operating system
 * ============================================================ */

typedef enum CloneVitteOperatingSystem {
    CLONEVITTE_OS_UNKNOWN = 0,

    CLONEVITTE_OS_NONE,

    CLONEVITTE_OS_LINUX,
    CLONEVITTE_OS_MACOS,
    CLONEVITTE_OS_WINDOWS,

    CLONEVITTE_OS_FREEBSD,
    CLONEVITTE_OS_OPENBSD,
    CLONEVITTE_OS_NETBSD,
    CLONEVITTE_OS_DRAGONFLY,

    CLONEVITTE_OS_ANDROID,
    CLONEVITTE_OS_IOS,

    CLONEVITTE_OS_WASI
} CloneVitteOperatingSystem;

/* ============================================================
 * Vendor
 * ============================================================ */

typedef enum CloneVitteVendor {
    CLONEVITTE_VENDOR_UNKNOWN = 0,
    CLONEVITTE_VENDOR_NONE,
    CLONEVITTE_VENDOR_PC,
    CLONEVITTE_VENDOR_APPLE
} CloneVitteVendor;

/* ============================================================
 * Environment / ABI
 * ============================================================ */

typedef enum CloneVitteEnvironment {
    CLONEVITTE_ENV_UNKNOWN = 0,
    CLONEVITTE_ENV_NONE,

    CLONEVITTE_ENV_GNU,
    CLONEVITTE_ENV_GNUABI64,
    CLONEVITTE_ENV_GNUEABI,
    CLONEVITTE_ENV_GNUEABIHF,

    CLONEVITTE_ENV_MUSL,
    CLONEVITTE_ENV_MUSLEABI,
    CLONEVITTE_ENV_MUSLEABIHF,

    CLONEVITTE_ENV_MSVC,
    CLONEVITTE_ENV_MINGW,

    CLONEVITTE_ENV_ANDROID,
    CLONEVITTE_ENV_EABI,
    CLONEVITTE_ENV_EABIHF,

    CLONEVITTE_ENV_WASI
} CloneVitteEnvironment;

/* ============================================================
 * Object format
 * ============================================================ */

typedef enum CloneVitteObjectFormat {
    CLONEVITTE_OBJECT_UNKNOWN = 0,

    CLONEVITTE_OBJECT_ELF,
    CLONEVITTE_OBJECT_MACHO,
    CLONEVITTE_OBJECT_COFF,
    CLONEVITTE_OBJECT_WASM
} CloneVitteObjectFormat;

/* ============================================================
 * Endianness
 * ============================================================ */

typedef enum CloneVitteEndianness {
    CLONEVITTE_ENDIAN_UNKNOWN = 0,
    CLONEVITTE_ENDIAN_LITTLE,
    CLONEVITTE_ENDIAN_BIG
} CloneVitteEndianness;

/* ============================================================
 * Pointer width
 * ============================================================ */

typedef enum CloneVittePointerWidth {
    CLONEVITTE_POINTER_WIDTH_UNKNOWN = 0,
    CLONEVITTE_POINTER_WIDTH_16 = 16,
    CLONEVITTE_POINTER_WIDTH_32 = 32,
    CLONEVITTE_POINTER_WIDTH_64 = 64,
    CLONEVITTE_POINTER_WIDTH_128 = 128
} CloneVittePointerWidth;

/* ============================================================
 * Calling convention
 * ============================================================ */

typedef enum CloneVitteCallingConvention {
    CLONEVITTE_CALL_DEFAULT = 0,

    CLONEVITTE_CALL_C,
    CLONEVITTE_CALL_SYSV64,
    CLONEVITTE_CALL_WIN64,

    CLONEVITTE_CALL_STDCALL,
    CLONEVITTE_CALL_FASTCALL,
    CLONEVITTE_CALL_VECTORCALL,

    CLONEVITTE_CALL_AAPCS,
    CLONEVITTE_CALL_AAPCS_VFP,

    CLONEVITTE_CALL_INTERRUPT,
    CLONEVITTE_CALL_NAKED
} CloneVitteCallingConvention;

/* ============================================================
 * CPU feature
 * ============================================================ */

typedef struct CloneVitteCpuFeature {
    const char *name;
    bool enabled;
} CloneVitteCpuFeature;

/* ============================================================
 * Data layout
 * ============================================================ */

typedef struct CloneVitteDataLayout {
    CloneVitteEndianness endianness;
    CloneVittePointerWidth pointer_width;

    uint16_t pointer_alignment;

    uint16_t i8_alignment;
    uint16_t i16_alignment;
    uint16_t i32_alignment;
    uint16_t i64_alignment;
    uint16_t i128_alignment;

    uint16_t f16_alignment;
    uint16_t f32_alignment;
    uint16_t f64_alignment;
    uint16_t f128_alignment;

    uint16_t aggregate_alignment;

    uint16_t stack_alignment;
} CloneVitteDataLayout;

/* ============================================================
 * Target triple
 * ============================================================ */

typedef struct CloneVitteTargetTriple {
    CloneVitteArchitecture architecture;
    CloneVitteVendor vendor;
    CloneVitteOperatingSystem operating_system;
    CloneVitteEnvironment environment;
} CloneVitteTargetTriple;

/* ============================================================
 * Target capabilities
 * ============================================================ */

typedef struct CloneVitteTargetCapabilities {
    bool threads;
    bool atomics;
    bool tls;

    bool dynamic_linking;
    bool shared_libraries;

    bool position_independent_code;

    bool exceptions;
    bool unwind;

    bool signals;

    bool filesystem;
    bool processes;
    bool sockets;

    bool executable_memory;

    bool native_int128;
    bool native_float16;
    bool native_float128;
} CloneVitteTargetCapabilities;

/* ============================================================
 * Target description
 * ============================================================ */

typedef struct CloneVitteTarget {
    CloneVitteTargetTriple triple;

    const char *triple_string;

    const char *cpu;
    const char *cpu_family;

    const CloneVitteCpuFeature *features;
    size_t feature_count;

    CloneVitteObjectFormat object_format;
    CloneVitteDataLayout data_layout;

    CloneVitteCallingConvention default_calling_convention;

    CloneVitteTargetCapabilities capabilities;

    const char *sysroot;

    const char *dynamic_linker;

    const char *object_extension;
    const char *static_library_extension;
    const char *shared_library_extension;
    const char *executable_extension;

    const char *static_library_prefix;
    const char *shared_library_prefix;
} CloneVitteTarget;

/* ============================================================
 * Target builder
 * ============================================================ */

typedef struct CloneVitteTargetBuilder {
    CloneVitteTargetTriple triple;

    const char *cpu;
    const char *features;
    const char *sysroot;

    CloneVitteObjectFormat object_format;

    bool infer_defaults;
} CloneVitteTargetBuilder;

/* ============================================================
 * Host detection
 * ============================================================ */

CloneVitteArchitecture
clonevitte_host_architecture(void);

CloneVitteOperatingSystem
clonevitte_host_operating_system(void);

CloneVitteVendor
clonevitte_host_vendor(void);

CloneVitteEnvironment
clonevitte_host_environment(void);

CloneVitteObjectFormat
clonevitte_host_object_format(void);

CloneVitteEndianness
clonevitte_host_endianness(void);

CloneVittePointerWidth
clonevitte_host_pointer_width(void);

/* ============================================================
 * Target creation
 * ============================================================ */

CloneVitteStatus
clonevitte_target_host(
    CloneVitteTarget *out_target
);

CloneVitteStatus
clonevitte_target_from_triple(
    const char *triple,
    CloneVitteTarget *out_target
);

CloneVitteStatus
clonevitte_target_build(
    const CloneVitteTargetBuilder *builder,
    CloneVitteTarget *out_target
);

void
clonevitte_target_destroy(
    CloneVitteTarget *target
);

/* ============================================================
 * Triple parsing
 * ============================================================ */

CloneVitteStatus
clonevitte_target_triple_parse(
    const char *text,
    CloneVitteTargetTriple *out_triple
);

CloneVitteStatus
clonevitte_target_triple_format(
    const CloneVitteTargetTriple *triple,
    char *buffer,
    size_t buffer_size
);

/* ============================================================
 * Target validation
 * ============================================================ */

CloneVitteStatus
clonevitte_target_validate(
    const CloneVitteTarget *target
);

bool
clonevitte_target_is_valid(
    const CloneVitteTarget *target
);

bool
clonevitte_target_is_supported(
    const CloneVitteTarget *target
);

/* ============================================================
 * Target comparison
 * ============================================================ */

bool
clonevitte_target_equal(
    const CloneVitteTarget *left,
    const CloneVitteTarget *right
);

bool
clonevitte_target_is_host(
    const CloneVitteTarget *target
);

/* ============================================================
 * Architecture queries
 * ============================================================ */

bool
clonevitte_architecture_is_32bit(
    CloneVitteArchitecture architecture
);

bool
clonevitte_architecture_is_64bit(
    CloneVitteArchitecture architecture
);

bool
clonevitte_architecture_is_arm(
    CloneVitteArchitecture architecture
);

bool
clonevitte_architecture_is_x86(
    CloneVitteArchitecture architecture
);

bool
clonevitte_architecture_is_riscv(
    CloneVitteArchitecture architecture
);

bool
clonevitte_architecture_is_wasm(
    CloneVitteArchitecture architecture
);

/* ============================================================
 * Operating system queries
 * ============================================================ */

bool
clonevitte_os_is_unix(
    CloneVitteOperatingSystem operating_system
);

bool
clonevitte_os_is_bsd(
    CloneVitteOperatingSystem operating_system
);

bool
clonevitte_os_is_apple(
    CloneVitteOperatingSystem operating_system
);

/* ============================================================
 * Feature queries
 * ============================================================ */

bool
clonevitte_target_has_feature(
    const CloneVitteTarget *target,
    const char *feature
);

bool
clonevitte_target_has_threads(
    const CloneVitteTarget *target
);

bool
clonevitte_target_has_atomics(
    const CloneVitteTarget *target
);

bool
clonevitte_target_supports_dynamic_linking(
    const CloneVitteTarget *target
);

/* ============================================================
 * Name helpers
 * ============================================================ */

const char *
clonevitte_architecture_name(
    CloneVitteArchitecture architecture
);

const char *
clonevitte_operating_system_name(
    CloneVitteOperatingSystem operating_system
);

const char *
clonevitte_vendor_name(
    CloneVitteVendor vendor
);

const char *
clonevitte_environment_name(
    CloneVitteEnvironment environment
);

const char *
clonevitte_object_format_name(
    CloneVitteObjectFormat format
);

const char *
clonevitte_endianness_name(
    CloneVitteEndianness endianness
);

const char *
clonevitte_calling_convention_name(
    CloneVitteCallingConvention convention
);

/* ============================================================
 * Parsing helpers
 * ============================================================ */

CloneVitteArchitecture
clonevitte_architecture_from_name(
    const char *name
);

CloneVitteOperatingSystem
clonevitte_operating_system_from_name(
    const char *name
);

CloneVitteVendor
clonevitte_vendor_from_name(
    const char *name
);

CloneVitteEnvironment
clonevitte_environment_from_name(
    const char *name
);

CloneVitteCallingConvention
clonevitte_calling_convention_from_name(
    const char *name
);

#ifdef __cplusplus
}
#endif

#endif /* CLONEVITTE_TARGET_H */