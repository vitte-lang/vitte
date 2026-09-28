/*
 * CloneVitte
 * Compiler Configuration API
 *
 * Copyright (C) 2026 VitteFoundation
 * SPDX-License-Identifier: LicenseRef-VFPL-1.0
 */

#ifndef CLONEVITTE_CONFIG_H
#define CLONEVITTE_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Limits
 * ============================================================ */

#define CLONEVITTE_MAX_INCLUDE_PATHS 256u
#define CLONEVITTE_MAX_LIBRARY_PATHS 256u
#define CLONEVITTE_MAX_MODULE_PATHS  256u
#define CLONEVITTE_MAX_DEFINES       256u
#define CLONEVITTE_MAX_FEATURES      256u

/* ============================================================
 * Backend
 * ============================================================ */

typedef enum CloneVitteBackend {
    CLONEVITTE_BACKEND_DEFAULT = 0,
    CLONEVITTE_BACKEND_C17,
    CLONEVITTE_BACKEND_NATIVE,
    CLONEVITTE_BACKEND_LLVM,
    CLONEVITTE_BACKEND_VM
} CloneVitteBackend;

/* ============================================================
 * Optimization
 * ============================================================ */

typedef enum CloneVitteOptimizationLevel {
    CLONEVITTE_OPT_NONE = 0,
    CLONEVITTE_OPT_BASIC,
    CLONEVITTE_OPT_DEFAULT,
    CLONEVITTE_OPT_AGGRESSIVE,
    CLONEVITTE_OPT_SIZE,
    CLONEVITTE_OPT_SIZE_AGGRESSIVE
} CloneVitteOptimizationLevel;

/* ============================================================
 * Output
 * ============================================================ */

typedef enum CloneVitteEmitKind {
    CLONEVITTE_EMIT_EXECUTABLE = 0,
    CLONEVITTE_EMIT_C,
    CLONEVITTE_EMIT_ASSEMBLY,
    CLONEVITTE_EMIT_OBJECT,
    CLONEVITTE_EMIT_HIR,
    CLONEVITTE_EMIT_MIR,
    CLONEVITTE_EMIT_IR
} CloneVitteEmitKind;

/* ============================================================
 * Build profile
 * ============================================================ */

typedef enum CloneVitteBuildProfile {
    CLONEVITTE_PROFILE_DEBUG = 0,
    CLONEVITTE_PROFILE_RELEASE,
    CLONEVITTE_PROFILE_SIZE,
    CLONEVITTE_PROFILE_CUSTOM
} CloneVitteBuildProfile;

/* ============================================================
 * Diagnostic configuration
 * ============================================================ */

typedef enum CloneVitteDiagnosticFormat {
    CLONEVITTE_DIAGNOSTIC_FORMAT_HUMAN = 0,
    CLONEVITTE_DIAGNOSTIC_FORMAT_SHORT,
    CLONEVITTE_DIAGNOSTIC_FORMAT_JSON,
    CLONEVITTE_DIAGNOSTIC_FORMAT_SILENT
} CloneVitteDiagnosticFormat;

typedef enum CloneVitteColorMode {
    CLONEVITTE_COLOR_AUTO = 0,
    CLONEVITTE_COLOR_ALWAYS,
    CLONEVITTE_COLOR_NEVER
} CloneVitteColorMode;

/* ============================================================
 * Panic strategy
 * ============================================================ */

typedef enum CloneVittePanicStrategy {
    CLONEVITTE_PANIC_DEFAULT = 0,
    CLONEVITTE_PANIC_ABORT,
    CLONEVITTE_PANIC_UNWIND
} CloneVittePanicStrategy;

/* ============================================================
 * Sanitizers
 * ============================================================ */

typedef enum CloneVitteSanitizer {
    CLONEVITTE_SANITIZER_NONE      = 0,
    CLONEVITTE_SANITIZER_ADDRESS   = 1u << 0,
    CLONEVITTE_SANITIZER_UNDEFINED = 1u << 1,
    CLONEVITTE_SANITIZER_THREAD    = 1u << 2,
    CLONEVITTE_SANITIZER_MEMORY    = 1u << 3
} CloneVitteSanitizer;

/* ============================================================
 * Link model
 * ============================================================ */

typedef enum CloneVitteLinkMode {
    CLONEVITTE_LINK_DEFAULT = 0,
    CLONEVITTE_LINK_STATIC,
    CLONEVITTE_LINK_DYNAMIC
} CloneVitteLinkMode;

/* ============================================================
 * Target
 * ============================================================ */

typedef struct CloneVitteTargetConfig {
    const char *triple;

    const char *architecture;
    const char *vendor;
    const char *operating_system;
    const char *environment;

    const char *cpu;
    const char *features;

    const char *sysroot;
} CloneVitteTargetConfig;

/* ============================================================
 * Toolchain
 * ============================================================ */

typedef struct CloneVitteToolchainConfig {
    const char *cc;
    const char *cxx;
    const char *assembler;
    const char *linker;
    const char *archiver;
    const char *ranlib;
    const char *strip;

    const char *cflags;
    const char *ldflags;

    const char *sysroot;

    bool use_environment;
} CloneVitteToolchainConfig;

/* ============================================================
 * Defines
 * ============================================================ */

typedef struct CloneVitteDefine {
    const char *name;
    const char *value;
} CloneVitteDefine;

/* ============================================================
 * Diagnostics callback
 * ============================================================ */

struct CloneVitteDiagnostic;

typedef void (*CloneVitteDiagnosticCallback)(
    const struct CloneVitteDiagnostic *diagnostic,
    void *userdata
);

/* ============================================================
 * Frontend
 * ============================================================ */

typedef struct CloneVitteFrontendConfig {
    bool unicode_identifiers;

    bool preserve_source;
    bool preserve_tokens;
    bool preserve_comments;

    bool parser_recovery;

    bool verify_ast;

    uint32_t max_errors;
    uint32_t max_parser_depth;
    uint32_t max_expression_depth;
    uint32_t max_type_depth;
} CloneVitteFrontendConfig;

/* ============================================================
 * Semantic analysis
 * ============================================================ */

typedef struct CloneVitteSemaConfig {
    bool enabled;

    bool name_resolution;
    bool type_checking;
    bool generic_checking;
    bool trait_checking;
    bool visibility_checking;
    bool abi_checking;

    bool warnings_as_errors;

    bool warn_unused;
    bool warn_unreachable;
    bool warn_shadowing;
} CloneVitteSemaConfig;

/* ============================================================
 * Intermediate representations
 * ============================================================ */

typedef struct CloneVitteIrConfig {
    bool build_hir;
    bool build_mir;
    bool build_ir;

    bool verify_hir;
    bool verify_mir;
    bool verify_ir;

    bool dump_hir;
    bool dump_mir;
    bool dump_ir;

    bool optimize_hir;
    bool optimize_mir;
    bool optimize_ir;
} CloneVitteIrConfig;

/* ============================================================
 * Code generation
 * ============================================================ */

typedef struct CloneVitteCodegenConfig {
    CloneVitteBackend backend;
    CloneVitteOptimizationLevel optimization;
    CloneVitteEmitKind emit;
    CloneVitteLinkMode link_mode;

    bool debug_info;
    bool position_independent_code;

    bool lto;
    bool strip_symbols;

    uint32_t sanitizers;

    CloneVittePanicStrategy panic_strategy;
} CloneVitteCodegenConfig;

/* ============================================================
 * C17 backend
 * ============================================================ */

typedef struct CloneVitteC17Config {
    bool enabled;

    bool emit_source;
    bool emit_header;

    bool compile;
    bool link;

    bool keep_generated_files;

    const char *compiler;
    const char *standard;

    const char *extra_cflags;
    const char *extra_ldflags;
} CloneVitteC17Config;

/* ============================================================
 * Runtime
 * ============================================================ */

typedef struct CloneVitteRuntimeConfig {
    bool enabled;

    CloneVitteLinkMode link_mode;

    bool checked_allocations;
    bool zero_initialize;

    const char *directory;
    const char *library;
} CloneVitteRuntimeConfig;

/* ============================================================
 * Diagnostics
 * ============================================================ */

typedef struct CloneVitteDiagnosticsConfig {
    CloneVitteDiagnosticFormat format;
    CloneVitteColorMode color;

    bool show_code;
    bool show_source;
    bool show_file;
    bool show_line;
    bool show_column;
    bool show_span;

    bool show_notes;
    bool show_help;

    uint32_t max_errors;

    CloneVitteDiagnosticCallback callback;
    void *userdata;
} CloneVitteDiagnosticsConfig;

/* ============================================================
 * Search paths
 * ============================================================ */

typedef struct CloneVitteSearchPaths {
    const char **include_paths;
    size_t include_path_count;

    const char **library_paths;
    size_t library_path_count;

    const char **module_paths;
    size_t module_path_count;
} CloneVitteSearchPaths;

/* ============================================================
 * Language
 * ============================================================ */

typedef struct CloneVitteLanguageConfig {
    const char *grammar;

    const char *primary_extension;
    const char *library_extension;
    const char *legacy_extension;

    bool legacy_compatibility;
    bool strict;
} CloneVitteLanguageConfig;

/* ============================================================
 * Cache / incremental compilation
 * ============================================================ */

typedef struct CloneVitteCacheConfig {
    bool enabled;
    bool incremental;

    const char *directory;

    bool hash_sources;
    bool hash_dependencies;
    bool hash_configuration;

    bool reuse_ast;
    bool reuse_hir;
    bool reuse_mir;
    bool reuse_ir;
} CloneVitteCacheConfig;

/* ============================================================
 * Parallel compilation
 * ============================================================ */

typedef struct CloneVitteParallelConfig {
    bool enabled;

    uint32_t jobs;

    bool parallel_parse;
    bool parallel_sema;
    bool parallel_codegen;
} CloneVitteParallelConfig;

/* ============================================================
 * Main configuration
 * ============================================================ */

typedef struct CloneVitteConfig {
    CloneVitteBuildProfile profile;

    CloneVitteLanguageConfig language;
    CloneVitteFrontendConfig frontend;
    CloneVitteSemaConfig sema;
    CloneVitteIrConfig ir;
    CloneVitteCodegenConfig codegen;
    CloneVitteC17Config c17;
    CloneVitteRuntimeConfig runtime;
    CloneVitteDiagnosticsConfig diagnostics;

    CloneVitteTargetConfig target;
    CloneVitteToolchainConfig toolchain;

    CloneVitteSearchPaths search_paths;
    CloneVitteCacheConfig cache;
    CloneVitteParallelConfig parallel;

    const CloneVitteDefine *defines;
    size_t define_count;

    const char **features;
    size_t feature_count;

    const char *input;
    const char *output;

    const char *working_directory;
    const char *build_directory;

    bool deterministic;
    bool reproducible;
} CloneVitteConfig;

/* ============================================================
 * Defaults
 * ============================================================ */

CloneVitteConfig
clonevitte_config_default(void);

CloneVitteFrontendConfig
clonevitte_frontend_config_default(void);

CloneVitteSemaConfig
clonevitte_sema_config_default(void);

CloneVitteIrConfig
clonevitte_ir_config_default(void);

CloneVitteCodegenConfig
clonevitte_codegen_config_default(void);

CloneVitteC17Config
clonevitte_c17_config_default(void);

CloneVitteRuntimeConfig
clonevitte_runtime_config_default(void);

CloneVitteDiagnosticsConfig
clonevitte_diagnostics_config_default(void);

CloneVitteTargetConfig
clonevitte_target_config_default(void);

CloneVitteToolchainConfig
clonevitte_toolchain_config_default(void);

CloneVitteCacheConfig
clonevitte_cache_config_default(void);

CloneVitteParallelConfig
clonevitte_parallel_config_default(void);

/* ============================================================
 * Validation
 * ============================================================ */

bool clonevitte_config_validate(
    const CloneVitteConfig *config
);

bool clonevitte_frontend_config_validate(
    const CloneVitteFrontendConfig *config
);

bool clonevitte_sema_config_validate(
    const CloneVitteSemaConfig *config
);

bool clonevitte_codegen_config_validate(
    const CloneVitteCodegenConfig *config
);

bool clonevitte_target_config_validate(
    const CloneVitteTargetConfig *config
);

bool clonevitte_toolchain_config_validate(
    const CloneVitteToolchainConfig *config
);

/* ============================================================
 * Search paths
 * ============================================================ */

bool clonevitte_config_add_include_path(
    CloneVitteConfig *config,
    const char *path
);

bool clonevitte_config_add_library_path(
    CloneVitteConfig *config,
    const char *path
);

bool clonevitte_config_add_module_path(
    CloneVitteConfig *config,
    const char *path
);

/* ============================================================
 * Defines / features
 * ============================================================ */

bool clonevitte_config_add_define(
    CloneVitteConfig *config,
    const char *name,
    const char *value
);

bool clonevitte_config_add_feature(
    CloneVitteConfig *config,
    const char *feature
);

/* ============================================================
 * Queries
 * ============================================================ */

const char *clonevitte_backend_name(
    CloneVitteBackend backend
);

const char *clonevitte_emit_kind_name(
    CloneVitteEmitKind emit
);

const char *clonevitte_profile_name(
    CloneVitteBuildProfile profile
);

const char *clonevitte_optimization_name(
    CloneVitteOptimizationLevel optimization
);

const char *clonevitte_link_mode_name(
    CloneVitteLinkMode mode
);

bool clonevitte_backend_supported(
    CloneVitteBackend backend
);

/* ============================================================
 * Target helpers
 * ============================================================ */

const char *clonevitte_host_architecture(void);
const char *clonevitte_host_operating_system(void);

CloneVitteTargetConfig
clonevitte_target_host(void);

/* ============================================================
 * Configuration lifecycle
 * ============================================================ */

bool clonevitte_config_clone(
    CloneVitteConfig *destination,
    const CloneVitteConfig *source
);

void clonevitte_config_destroy(
    CloneVitteConfig *config
);

#ifdef __cplusplus
}
#endif

#endif /* CLONEVITTE_CONFIG_H */