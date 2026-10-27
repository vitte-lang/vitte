#ifndef VITTE_CODEGEN_CODEGEN_H
#define VITTE_CODEGEN_CODEGEN_H

/*
 * Vitte Compiler
 * src/codegen/codegen.h
 *
 * Canonical code-generation orchestration API.
 *
 * This layer connects lowered compiler representations to concrete
 * code-generation backends without depending on a particular output
 * language or object format.
 *
 * C17.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Constants                                                                 */
/* ========================================================================= */

#define VITTE_CODEGEN_MAGIC \
    UINT64_C(0x564954434F444547)

#define VITTE_CODEGEN_DEAD_MAGIC \
    UINT64_C(0x44454144434F4445)

#define VITTE_CODEGEN_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_CODEGEN_FNV_PRIME \
    UINT64_C(1099511628211)

#define VITTE_CODEGEN_DEFAULT_MAX_ERRORS \
    ((size_t)100u)

#define VITTE_CODEGEN_DEFAULT_MAX_UNITS \
    ((size_t)65536u)

#define VITTE_CODEGEN_DEFAULT_INITIAL_UNIT_CAPACITY \
    ((size_t)16u)

/* ========================================================================= */
/* Error                                                                     */
/* ========================================================================= */

typedef enum vitte_codegen_error {
    VITTE_CODEGEN_ERROR_NONE = 0,

    VITTE_CODEGEN_ERROR_INVALID_CODEGEN,
    VITTE_CODEGEN_ERROR_INVALID_ARGUMENT,
    VITTE_CODEGEN_ERROR_INVALID_STATE,
    VITTE_CODEGEN_ERROR_INVALID_CONFIG,
    VITTE_CODEGEN_ERROR_INVALID_TARGET,
    VITTE_CODEGEN_ERROR_INVALID_BACKEND,

    VITTE_CODEGEN_ERROR_DUPLICATE_UNIT,
    VITTE_CODEGEN_ERROR_TOO_MANY_UNITS,
    VITTE_CODEGEN_ERROR_TOO_MANY_ERRORS,

    VITTE_CODEGEN_ERROR_OUTPUT,
    VITTE_CODEGEN_ERROR_OUTPUT_LIMIT,

    VITTE_CODEGEN_ERROR_OVERFLOW,
    VITTE_CODEGEN_ERROR_OUT_OF_MEMORY,

    VITTE_CODEGEN_ERROR_UNSUPPORTED,
    VITTE_CODEGEN_ERROR_CANCELLED,

    VITTE_CODEGEN_ERROR_BACKEND,
    VITTE_CODEGEN_ERROR_INTERNAL,
    VITTE_CODEGEN_ERROR_CORRUPTION,

    VITTE_CODEGEN_ERROR_COUNT
} vitte_codegen_error_t;

/* ========================================================================= */
/* State                                                                     */
/* ========================================================================= */

typedef enum vitte_codegen_state {
    VITTE_CODEGEN_STATE_INVALID = 0,

    VITTE_CODEGEN_STATE_READY,
    VITTE_CODEGEN_STATE_PREPARED,
    VITTE_CODEGEN_STATE_GENERATING,
    VITTE_CODEGEN_STATE_FINISHED,
    VITTE_CODEGEN_STATE_FAILED,
    VITTE_CODEGEN_STATE_CANCELLED,
    VITTE_CODEGEN_STATE_DESTROYED,

    VITTE_CODEGEN_STATE_COUNT
} vitte_codegen_state_t;

/* ========================================================================= */
/* Backend                                                                   */
/* ========================================================================= */

typedef enum vitte_codegen_backend_kind {
    VITTE_CODEGEN_BACKEND_INVALID = 0,

    VITTE_CODEGEN_BACKEND_C17,
    VITTE_CODEGEN_BACKEND_NATIVE,
    VITTE_CODEGEN_BACKEND_BYTECODE,

    VITTE_CODEGEN_BACKEND_COUNT
} vitte_codegen_backend_kind_t;

/* ========================================================================= */
/* Architecture                                                              */
/* ========================================================================= */

typedef enum vitte_codegen_arch {
    VITTE_CODEGEN_ARCH_INVALID = 0,

    VITTE_CODEGEN_ARCH_X86_64,
    VITTE_CODEGEN_ARCH_AARCH64,
    VITTE_CODEGEN_ARCH_RISCV64,
    VITTE_CODEGEN_ARCH_WASM32,

    VITTE_CODEGEN_ARCH_COUNT
} vitte_codegen_arch_t;

/* ========================================================================= */
/* Operating system                                                          */
/* ========================================================================= */

typedef enum vitte_codegen_os {
    VITTE_CODEGEN_OS_INVALID = 0,

    VITTE_CODEGEN_OS_LINUX,
    VITTE_CODEGEN_OS_MACOS,
    VITTE_CODEGEN_OS_WINDOWS,
    VITTE_CODEGEN_OS_FREEBSD,
    VITTE_CODEGEN_OS_NONE,

    VITTE_CODEGEN_OS_COUNT
} vitte_codegen_os_t;

/* ========================================================================= */
/* ABI                                                                       */
/* ========================================================================= */

typedef enum vitte_codegen_abi {
    VITTE_CODEGEN_ABI_INVALID = 0,

    VITTE_CODEGEN_ABI_SYSV,
    VITTE_CODEGEN_ABI_WIN64,
    VITTE_CODEGEN_ABI_AAPCS64,
    VITTE_CODEGEN_ABI_WASM,
    VITTE_CODEGEN_ABI_FREESTANDING,

    VITTE_CODEGEN_ABI_COUNT
} vitte_codegen_abi_t;

/* ========================================================================= */
/* Endianness                                                                */
/* ========================================================================= */

typedef enum vitte_codegen_endian {
    VITTE_CODEGEN_ENDIAN_INVALID = 0,

    VITTE_CODEGEN_ENDIAN_LITTLE,
    VITTE_CODEGEN_ENDIAN_BIG,

    VITTE_CODEGEN_ENDIAN_COUNT
} vitte_codegen_endian_t;

/* ========================================================================= */
/* Optimization                                                              */
/* ========================================================================= */

typedef enum vitte_codegen_optimization {
    VITTE_CODEGEN_OPTIMIZATION_INVALID = 0,

    VITTE_CODEGEN_OPTIMIZATION_NONE,
    VITTE_CODEGEN_OPTIMIZATION_DEBUG,
    VITTE_CODEGEN_OPTIMIZATION_BALANCED,
    VITTE_CODEGEN_OPTIMIZATION_SPEED,
    VITTE_CODEGEN_OPTIMIZATION_SIZE,
    VITTE_CODEGEN_OPTIMIZATION_AGGRESSIVE,

    VITTE_CODEGEN_OPTIMIZATION_COUNT
} vitte_codegen_optimization_t;

/* ========================================================================= */
/* Diagnostic severity                                                       */
/* ========================================================================= */

typedef enum vitte_codegen_severity {
    VITTE_CODEGEN_SEVERITY_INVALID = 0,

    VITTE_CODEGEN_SEVERITY_NOTE,
    VITTE_CODEGEN_SEVERITY_WARNING,
    VITTE_CODEGEN_SEVERITY_ERROR,

    VITTE_CODEGEN_SEVERITY_COUNT
} vitte_codegen_severity_t;

/* ========================================================================= */
/* Source location                                                           */
/* ========================================================================= */

typedef struct vitte_codegen_source_location {
    uint32_t file_id;

    size_t begin;
    size_t end;

    bool valid;
} vitte_codegen_source_location_t;

/* ========================================================================= */
/* Target                                                                    */
/* ========================================================================= */

typedef struct vitte_codegen_target {
    vitte_codegen_arch_t architecture;
    vitte_codegen_os_t operating_system;
    vitte_codegen_abi_t abi;
    vitte_codegen_endian_t endian;

    unsigned pointer_bits;
    unsigned size_bits;
    unsigned char_bits;

    bool hosted;
} vitte_codegen_target_t;

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

typedef struct vitte_codegen_config {
    vitte_codegen_backend_kind_t backend;
    vitte_codegen_optimization_t optimization;

    size_t max_units;
    size_t max_errors;
    size_t max_output_bytes;

    bool deterministic;

    bool emit_debug_info;
    bool emit_source_map;
    bool emit_comments;

    bool runtime_checks;
    bool bounds_checks;
    bool null_checks;
    bool overflow_checks;

    bool fail_fast;
} vitte_codegen_config_t;

/* ========================================================================= */
/* Translation/code-generation unit                                          */
/* ========================================================================= */

typedef struct vitte_codegen_unit {
    /*
     * Stable non-zero unit identifier.
     */
    uint64_t id;

    /*
     * Borrowed canonical unit name.
     *
     * Must remain valid while the code generator uses the unit.
     */
    const char *name;

    /*
     * Opaque lowered representation.
     *
     * Usually HIR, MIR, IR or another backend-ready representation.
     */
    const void *input;

    /*
     * Optional originating source location.
     */
    vitte_codegen_source_location_t location;
} vitte_codegen_unit_t;

/* ========================================================================= */
/* Forward declaration                                                       */
/* ========================================================================= */

typedef struct vitte_codegen vitte_codegen_t;

/* ========================================================================= */
/* Diagnostics                                                               */
/* ========================================================================= */

typedef void
(*vitte_codegen_diagnostic_fn)(
    void *user_data,
    vitte_codegen_severity_t severity,
    vitte_codegen_error_t error,
    const char *message,
    vitte_codegen_source_location_t location);

/* ========================================================================= */
/* Backend interface                                                         */
/* ========================================================================= */

typedef bool
(*vitte_codegen_backend_prepare_fn)(
    vitte_codegen_t *codegen,
    void *backend_data);

typedef bool
(*vitte_codegen_backend_begin_fn)(
    vitte_codegen_t *codegen,
    void *backend_data);

typedef bool
(*vitte_codegen_backend_emit_unit_fn)(
    vitte_codegen_t *codegen,
    const vitte_codegen_unit_t *unit,
    void *backend_data);

typedef bool
(*vitte_codegen_backend_end_fn)(
    vitte_codegen_t *codegen,
    void *backend_data);

typedef void
(*vitte_codegen_backend_destroy_fn)(
    void *backend_data);

typedef struct vitte_codegen_backend_interface {
    /*
     * Optional preparation callback.
     */
    vitte_codegen_backend_prepare_fn prepare;

    /*
     * Optional generation-start callback.
     */
    vitte_codegen_backend_begin_fn begin;

    /*
     * Required unit emitter.
     */
    vitte_codegen_backend_emit_unit_fn emit_unit;

    /*
     * Optional finalization callback.
     */
    vitte_codegen_backend_end_fn end;

    /*
     * Optional backend-data destructor.
     */
    vitte_codegen_backend_destroy_fn destroy;
} vitte_codegen_backend_interface_t;

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

typedef struct vitte_codegen_stats {
    size_t unit_count;
    size_t generated_unit_count;

    size_t error_count;
    size_t warning_count;
    size_t note_count;

    size_t output_bytes;

    uint64_t input_fingerprint;

    bool prepared;
    bool generated;
    bool failed;
    bool cancelled;
} vitte_codegen_stats_t;

/* ========================================================================= */
/* Code generator                                                            */
/* ========================================================================= */

struct vitte_codegen {
    uint64_t magic;

    vitte_codegen_state_t state;
    vitte_codegen_error_t last_error;

    vitte_codegen_config_t config;
    vitte_codegen_target_t target;

    vitte_codegen_unit_t *units;
    size_t unit_count;
    size_t unit_capacity;

    vitte_codegen_backend_interface_t backend_interface;
    void *backend_data;

    vitte_codegen_diagnostic_fn diagnostic;
    void *diagnostic_data;

    vitte_codegen_stats_t stats;

    /*
     * Lifecycle generation.
     *
     * Incremented on reset when possible. Useful for detecting stale
     * external handles.
     */
    uint64_t generation;
};

/* ========================================================================= */
/* Enum names                                                                */
/* ========================================================================= */

const char *
vitte_codegen_error_name(
    vitte_codegen_error_t error);

const char *
vitte_codegen_state_name(
    vitte_codegen_state_t state);

const char *
vitte_codegen_backend_name(
    vitte_codegen_backend_kind_t backend);

const char *
vitte_codegen_arch_name(
    vitte_codegen_arch_t architecture);

const char *
vitte_codegen_os_name(
    vitte_codegen_os_t operating_system);

const char *
vitte_codegen_abi_name(
    vitte_codegen_abi_t abi);

/* ========================================================================= */
/* Defaults                                                                  */
/* ========================================================================= */

vitte_codegen_config_t
vitte_codegen_config_default(void);

vitte_codegen_target_t
vitte_codegen_target_default(void);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

bool
vitte_codegen_config_validate(
    const vitte_codegen_config_t *config);

bool
vitte_codegen_target_validate(
    const vitte_codegen_target_t *target);

bool
vitte_codegen_is_valid(
    const vitte_codegen_t *codegen);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

bool
vitte_codegen_init(
    vitte_codegen_t *codegen,
    const vitte_codegen_config_t *config,
    const vitte_codegen_target_t *target);

void
vitte_codegen_destroy(
    vitte_codegen_t *codegen);

bool
vitte_codegen_reset(
    vitte_codegen_t *codegen);

/* ========================================================================= */
/* Backend                                                                   */
/* ========================================================================= */

bool
vitte_codegen_set_backend(
    vitte_codegen_t *codegen,
    const vitte_codegen_backend_interface_t *interface,
    void *backend_data);

/* ========================================================================= */
/* Diagnostics                                                               */
/* ========================================================================= */

bool
vitte_codegen_set_diagnostic(
    vitte_codegen_t *codegen,
    vitte_codegen_diagnostic_fn diagnostic,
    void *user_data);

/* ========================================================================= */
/* Units                                                                     */
/* ========================================================================= */

bool
vitte_codegen_add_unit(
    vitte_codegen_t *codegen,
    const vitte_codegen_unit_t *unit);

size_t
vitte_codegen_unit_count(
    const vitte_codegen_t *codegen);

const vitte_codegen_unit_t *
vitte_codegen_unit_at(
    const vitte_codegen_t *codegen,
    size_t index);

/* ========================================================================= */
/* Preparation / generation                                                  */
/* ========================================================================= */

bool
vitte_codegen_prepare(
    vitte_codegen_t *codegen);

bool
vitte_codegen_generate(
    vitte_codegen_t *codegen);

/* ========================================================================= */
/* Cancellation                                                              */
/* ========================================================================= */

bool
vitte_codegen_cancel(
    vitte_codegen_t *codegen);

/* ========================================================================= */
/* Output accounting                                                         */
/* ========================================================================= */

/*
 * Account bytes produced by a concrete backend.
 *
 * Concrete backends should call this as output is committed so that global
 * output limits remain enforced by the orchestration layer.
 */
bool
vitte_codegen_account_output(
    vitte_codegen_t *codegen,
    size_t byte_count);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

/*
 * Compute a deterministic fingerprint from code-generation policy, target
 * properties and registered unit identities.
 *
 * Opaque input pointer addresses are deliberately excluded.
 */
uint64_t
vitte_codegen_fingerprint(
    const vitte_codegen_t *codegen);

/* ========================================================================= */
/* State / errors                                                            */
/* ========================================================================= */

vitte_codegen_error_t
vitte_codegen_last_error(
    const vitte_codegen_t *codegen);

vitte_codegen_state_t
vitte_codegen_state(
    const vitte_codegen_t *codegen);

/* ========================================================================= */
/* Statistics                                                                */
/* ========================================================================= */

vitte_codegen_stats_t
vitte_codegen_stats(
    const vitte_codegen_t *codegen);

/* ========================================================================= */
/* Inline queries                                                            */
/* ========================================================================= */

static inline bool
vitte_codegen_is_ready(
    const vitte_codegen_t *codegen)
{
    return
        codegen != NULL &&
        codegen->magic == VITTE_CODEGEN_MAGIC &&
        codegen->state ==
            VITTE_CODEGEN_STATE_READY;
}

static inline bool
vitte_codegen_is_prepared(
    const vitte_codegen_t *codegen)
{
    return
        codegen != NULL &&
        codegen->magic == VITTE_CODEGEN_MAGIC &&
        codegen->state ==
            VITTE_CODEGEN_STATE_PREPARED;
}

static inline bool
vitte_codegen_is_generating(
    const vitte_codegen_t *codegen)
{
    return
        codegen != NULL &&
        codegen->magic == VITTE_CODEGEN_MAGIC &&
        codegen->state ==
            VITTE_CODEGEN_STATE_GENERATING;
}

static inline bool
vitte_codegen_is_finished(
    const vitte_codegen_t *codegen)
{
    return
        codegen != NULL &&
        codegen->magic == VITTE_CODEGEN_MAGIC &&
        codegen->state ==
            VITTE_CODEGEN_STATE_FINISHED;
}

static inline bool
vitte_codegen_has_failed(
    const vitte_codegen_t *codegen)
{
    return
        codegen != NULL &&
        codegen->magic == VITTE_CODEGEN_MAGIC &&
        codegen->state ==
            VITTE_CODEGEN_STATE_FAILED;
}

static inline bool
vitte_codegen_is_cancelled(
    const vitte_codegen_t *codegen)
{
    return
        codegen != NULL &&
        codegen->magic == VITTE_CODEGEN_MAGIC &&
        codegen->state ==
            VITTE_CODEGEN_STATE_CANCELLED;
}

static inline bool
vitte_codegen_has_errors(
    const vitte_codegen_t *codegen)
{
    return
        codegen != NULL &&
        codegen->magic == VITTE_CODEGEN_MAGIC &&
        codegen->stats.error_count != 0u;
}

static inline bool
vitte_codegen_has_warnings(
    const vitte_codegen_t *codegen)
{
    return
        codegen != NULL &&
        codegen->magic == VITTE_CODEGEN_MAGIC &&
        codegen->stats.warning_count != 0u;
}

static inline bool
vitte_codegen_target_is_64_bit(
    const vitte_codegen_target_t *target)
{
    return
        target != NULL &&
        target->pointer_bits == 64u;
}

static inline bool
vitte_codegen_target_is_little_endian(
    const vitte_codegen_target_t *target)
{
    return
        target != NULL &&
        target->endian ==
            VITTE_CODEGEN_ENDIAN_LITTLE;
}

static inline bool
vitte_codegen_target_is_big_endian(
    const vitte_codegen_target_t *target)
{
    return
        target != NULL &&
        target->endian ==
            VITTE_CODEGEN_ENDIAN_BIG;
}

#ifdef __cplusplus
}
#endif

#endif /* VITTE_CODEGEN_CODEGEN_H */
