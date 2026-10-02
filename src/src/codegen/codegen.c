
/*

 * Vitte Compiler

 * src/codegen/codegen.c

 *

 * Canonical code-generation orchestration layer.

 *

 * This subsystem sits between the compiler's lowered representation and

 * concrete output backends.

 *

 * Responsibilities:

 *   - code-generation lifecycle

 *   - target/backend selection

 *   - translation-unit orchestration

 *   - deterministic generation

 *   - callback-driven backend integration

 *   - output accounting

 *   - diagnostic propagation

 *   - cancellation

 *   - resource/output limits

 *   - source-location propagation

 *   - statistics

 *   - deterministic fingerprints

 *

 * It deliberately does NOT implement:

 *   - AST/HIR/IR ownership

 *   - C17 syntax emission

 *   - object-file encoding

 *   - native assembler

 *   - linker operation

 *   - diagnostic rendering

 *

 * C17.

 */

#include <stdbool.h>

#include <stddef.h>

#include <stdint.h>

#include <stdlib.h>

#include <string.h>

/* ========================================================================= */

/* Constants                                                                 */

/* ========================================================================= */

#define VITTE_CODEGEN_MAGIC UINT64_C(0x564954434F444547)

#define VITTE_CODEGEN_DEAD_MAGIC UINT64_C(0x44454144434F4445)

#define VITTE_CODEGEN_FNV_OFFSET UINT64_C(14695981039346656037)

#define VITTE_CODEGEN_FNV_PRIME UINT64_C(1099511628211)

#define VITTE_CODEGEN_DEFAULT_MAX_ERRORS ((size_t)100u)

#define VITTE_CODEGEN_DEFAULT_MAX_UNITS ((size_t)65536u)

#define VITTE_CODEGEN_DEFAULT_INITIAL_UNIT_CAPACITY ((size_t)16u)

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

/* Backend kind                                                              */

/* ========================================================================= */

typedef enum vitte_codegen_backend_kind {

    VITTE_CODEGEN_BACKEND_INVALID = 0,

    VITTE_CODEGEN_BACKEND_C17,

    /*

     * Reserved architectural slots.

     *

     * They do not imply that these backends are currently implemented.

     */

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

/* Unit                                                                      */

/* ========================================================================= */

typedef struct vitte_codegen_unit {

    uint64_t id;

    const char *name;

    const void *input;

    vitte_codegen_source_location_t location;

} vitte_codegen_unit_t;

/* ========================================================================= */

/* Diagnostic callback                                                       */

/* ========================================================================= */

typedef void

(*vitte_codegen_diagnostic_fn)(

    void *user_data,

    vitte_codegen_severity_t severity,

    vitte_codegen_error_t error,

    const char *message,

    vitte_codegen_source_location_t location);

/* ========================================================================= */

/* Backend callbacks                                                         */

/* ========================================================================= */

struct vitte_codegen;

typedef bool

(*vitte_codegen_backend_prepare_fn)(

    struct vitte_codegen *codegen,

    void *backend_data);

typedef bool

(*vitte_codegen_backend_begin_fn)(

    struct vitte_codegen *codegen,

    void *backend_data);

typedef bool

(*vitte_codegen_backend_emit_unit_fn)(

    struct vitte_codegen *codegen,

    const vitte_codegen_unit_t *unit,

    void *backend_data);

typedef bool

(*vitte_codegen_backend_end_fn)(

    struct vitte_codegen *codegen,

    void *backend_data);

typedef void

(*vitte_codegen_backend_destroy_fn)(

    void *backend_data);

/* ========================================================================= */

/* Backend interface                                                         */

/* ========================================================================= */

typedef struct vitte_codegen_backend_interface {

    vitte_codegen_backend_prepare_fn prepare;

    vitte_codegen_backend_begin_fn begin;

    vitte_codegen_backend_emit_unit_fn emit_unit;

    vitte_codegen_backend_end_fn end;

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

typedef struct vitte_codegen {

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

    uint64_t generation;

} vitte_codegen_t;

/* ========================================================================= */

/* Safe arithmetic                                                           */

/* ========================================================================= */

static bool

vitte_codegen_size_add(

    size_t left,

    size_t right,

    size_t *result)

{

    if (result == NULL) {

        return false;

    }

    if (left > SIZE_MAX - right) {

        return false;

    }

    *result = left + right;

    return true;

}

static bool

vitte_codegen_size_mul(

    size_t left,

    size_t right,

    size_t *result)

{

    if (result == NULL) {

        return false;

    }

    if (left != 0u &&

        right > SIZE_MAX / left) {

        return false;

    }

    *result = left * right;

    return true;

}

/* ========================================================================= */

/* Hash                                                                      */

/* ========================================================================= */

static uint64_t

vitte_codegen_hash_bytes(

    uint64_t hash,

    const void *data,

    size_t length)

{

    const unsigned char *bytes;

    size_t index;

    bytes =

        (const unsigned char *)data;

    for (index = 0u;

         index < length;

         ++index) {

        hash ^= (uint64_t)bytes[index];

        hash *= VITTE_CODEGEN_FNV_PRIME;

    }

    return hash;

}

static uint64_t

vitte_codegen_hash_u64(

    uint64_t hash,

    uint64_t value)

{

    unsigned shift;

    for (shift = 0u;

         shift < 64u;

         shift += 8u) {

        unsigned char byte;

        byte =

            (unsigned char)(

                (value >> shift) &

                UINT64_C(0xff));

        hash =

            vitte_codegen_hash_bytes(

                hash,

                &byte,

                1u);

    }

    return hash;

}

static uint64_t

vitte_codegen_hash_string(

    uint64_t hash,

    const char *text)

{

    if (text == NULL) {

        return

            vitte_codegen_hash_u64(

                hash,

                UINT64_MAX);

    }

    hash =

        vitte_codegen_hash_bytes(

            hash,

            text,

            strlen(text));

    return

        vitte_codegen_hash_u64(

            hash,

            UINT64_C(0));

}

/* ========================================================================= */

/* Enum names                                                                */

/* ========================================================================= */

const char *

vitte_codegen_error_name(

    vitte_codegen_error_t error)

{

    switch (error) {

        case VITTE_CODEGEN_ERROR_NONE:

            return "none";

        case VITTE_CODEGEN_ERROR_INVALID_CODEGEN:

            return "invalid-codegen";

        case VITTE_CODEGEN_ERROR_INVALID_ARGUMENT:

            return "invalid-argument";

        case VITTE_CODEGEN_ERROR_INVALID_STATE:

            return "invalid-state";

        case VITTE_CODEGEN_ERROR_INVALID_CONFIG:

            return "invalid-config";

        case VITTE_CODEGEN_ERROR_INVALID_TARGET:

            return "invalid-target";

        case VITTE_CODEGEN_ERROR_INVALID_BACKEND:

            return "invalid-backend";

        case VITTE_CODEGEN_ERROR_DUPLICATE_UNIT:

            return "duplicate-unit";

        case VITTE_CODEGEN_ERROR_TOO_MANY_UNITS:

            return "too-many-units";

        case VITTE_CODEGEN_ERROR_TOO_MANY_ERRORS:

            return "too-many-errors";

        case VITTE_CODEGEN_ERROR_OUTPUT:

            return "output";

        case VITTE_CODEGEN_ERROR_OUTPUT_LIMIT:

            return "output-limit";

        case VITTE_CODEGEN_ERROR_OVERFLOW:

            return "overflow";

        case VITTE_CODEGEN_ERROR_OUT_OF_MEMORY:

            return "out-of-memory";

        case VITTE_CODEGEN_ERROR_UNSUPPORTED:

            return "unsupported";

        case VITTE_CODEGEN_ERROR_CANCELLED:

            return "cancelled";

        case VITTE_CODEGEN_ERROR_BACKEND:

            return "backend";

        case VITTE_CODEGEN_ERROR_INTERNAL:

            return "internal";

        case VITTE_CODEGEN_ERROR_CORRUPTION:

            return "corruption";

        case VITTE_CODEGEN_ERROR_COUNT:

            return "count";

    }

    return "invalid";

}

const char *

vitte_codegen_state_name(

    vitte_codegen_state_t state)

{

    switch (state) {

        case VITTE_CODEGEN_STATE_INVALID:

            return "invalid";

        case VITTE_CODEGEN_STATE_READY:

            return "ready";

        case VITTE_CODEGEN_STATE_PREPARED:

            return "prepared";

        case VITTE_CODEGEN_STATE_GENERATING:

            return "generating";

        case VITTE_CODEGEN_STATE_FINISHED:

            return "finished";

        case VITTE_CODEGEN_STATE_FAILED:

            return "failed";

        case VITTE_CODEGEN_STATE_CANCELLED:

            return "cancelled";

        case VITTE_CODEGEN_STATE_DESTROYED:

            return "destroyed";

        case VITTE_CODEGEN_STATE_COUNT:

            return "count";

    }

    return "invalid";

}

const char *

vitte_codegen_backend_name(

    vitte_codegen_backend_kind_t backend)

{

    switch (backend) {

        case VITTE_CODEGEN_BACKEND_INVALID:

            return "invalid";

        case VITTE_CODEGEN_BACKEND_C17:

            return "c17";

        case VITTE_CODEGEN_BACKEND_NATIVE:

            return "native";

        case VITTE_CODEGEN_BACKEND_BYTECODE:

            return "bytecode";

        case VITTE_CODEGEN_BACKEND_COUNT:

            return "count";

    }

    return "invalid";

}

const char *

vitte_codegen_arch_name(

    vitte_codegen_arch_t architecture)

{

    switch (architecture) {

        case VITTE_CODEGEN_ARCH_INVALID:

            return "invalid";

        case VITTE_CODEGEN_ARCH_X86_64:

            return "x86_64";

        case VITTE_CODEGEN_ARCH_AARCH64:

            return "aarch64";

        case VITTE_CODEGEN_ARCH_RISCV64:

            return "riscv64";

        case VITTE_CODEGEN_ARCH_WASM32:

            return "wasm32";

        case VITTE_CODEGEN_ARCH_COUNT:

            return "count";

    }

    return "invalid";

}

const char *

vitte_codegen_os_name(

    vitte_codegen_os_t operating_system)

{

    switch (operating_system) {

        case VITTE_CODEGEN_OS_INVALID:

            return "invalid";

        case VITTE_CODEGEN_OS_LINUX:

            return "linux";

        case VITTE_CODEGEN_OS_MACOS:

            return "macos";

        case VITTE_CODEGEN_OS_WINDOWS:

            return "windows";

        case VITTE_CODEGEN_OS_FREEBSD:

            return "freebsd";

        case VITTE_CODEGEN_OS_NONE:

            return "none";

        case VITTE_CODEGEN_OS_COUNT:

            return "count";

    }

    return "invalid";

}

const char *

vitte_codegen_abi_name(

    vitte_codegen_abi_t abi)

{

    switch (abi) {

        case VITTE_CODEGEN_ABI_INVALID:

            return "invalid";

        case VITTE_CODEGEN_ABI_SYSV:

            return "sysv";

        case VITTE_CODEGEN_ABI_WIN64:

            return "win64";

        case VITTE_CODEGEN_ABI_AAPCS64:

            return "aapcs64";

        case VITTE_CODEGEN_ABI_WASM:

            return "wasm";

        case VITTE_CODEGEN_ABI_FREESTANDING:

            return "freestanding";

        case VITTE_CODEGEN_ABI_COUNT:

            return "count";

    }

    return "invalid";

}

/* ========================================================================= */

/* Defaults                                                                  */

/* ========================================================================= */

vitte_codegen_config_t

vitte_codegen_config_default(void)

{

    vitte_codegen_config_t config;

    memset(

        &config,

        0,

        sizeof(config));

    config.backend =

        VITTE_CODEGEN_BACKEND_C17;

    config.optimization =

        VITTE_CODEGEN_OPTIMIZATION_NONE;

    config.max_units =

        VITTE_CODEGEN_DEFAULT_MAX_UNITS;

    config.max_errors =

        VITTE_CODEGEN_DEFAULT_MAX_ERRORS;

    config.max_output_bytes = 0u;

    config.deterministic = true;

    config.emit_debug_info = false;

    config.emit_source_map = true;

    config.emit_comments = false;

    config.runtime_checks = true;

    config.bounds_checks = true;

    config.null_checks = true;

    config.overflow_checks = true;

    config.fail_fast = false;

    return config;

}

vitte_codegen_target_t

vitte_codegen_target_default(void)

{

    vitte_codegen_target_t target;

    memset(

        &target,

        0,

        sizeof(target));

#if defined(__aarch64__) || defined(__arm64__)

    target.architecture =

        VITTE_CODEGEN_ARCH_AARCH64;

#elif defined(__x86_64__) || defined(_M_X64)

    target.architecture =

        VITTE_CODEGEN_ARCH_X86_64;

#elif defined(__riscv) && (__riscv_xlen == 64)

    target.architecture =

        VITTE_CODEGEN_ARCH_RISCV64;

#elif defined(__wasm32__)

    target.architecture =

        VITTE_CODEGEN_ARCH_WASM32;

#else

    target.architecture =

        VITTE_CODEGEN_ARCH_INVALID;

#endif

#if defined(__APPLE__)

    target.operating_system =

        VITTE_CODEGEN_OS_MACOS;

#elif defined(_WIN32)

    target.operating_system =

        VITTE_CODEGEN_OS_WINDOWS;

#elif defined(__FreeBSD__)

    target.operating_system =

        VITTE_CODEGEN_OS_FREEBSD;

#elif defined(__linux__)

    target.operating_system =

        VITTE_CODEGEN_OS_LINUX;

#else

    target.operating_system =

        VITTE_CODEGEN_OS_NONE;

#endif

    if (target.architecture ==

            VITTE_CODEGEN_ARCH_AARCH64) {

        target.abi =

            VITTE_CODEGEN_ABI_AAPCS64;

    } else if (target.operating_system ==

               VITTE_CODEGEN_OS_WINDOWS) {

        target.abi =

            VITTE_CODEGEN_ABI_WIN64;

    } else if (target.architecture ==

               VITTE_CODEGEN_ARCH_WASM32) {

        target.abi =

            VITTE_CODEGEN_ABI_WASM;

    } else {

        target.abi =

            VITTE_CODEGEN_ABI_SYSV;

    }

    target.endian =

        VITTE_CODEGEN_ENDIAN_LITTLE;

    target.pointer_bits =

        (unsigned)(sizeof(void *) * 8u);

    target.size_bits =

        (unsigned)(sizeof(size_t) * 8u);

    target.char_bits = 8u;

    target.hosted = true;

    return target;

}

/* ========================================================================= */

/* Validation                                                                */

/* ========================================================================= */

bool

vitte_codegen_config_validate(

    const vitte_codegen_config_t *config)

{

    if (config == NULL) {

        return false;

    }

    if (config->backend <=

            VITTE_CODEGEN_BACKEND_INVALID ||

        config->backend >=

            VITTE_CODEGEN_BACKEND_COUNT) {

        return false;

    }

    if (config->optimization <=

            VITTE_CODEGEN_OPTIMIZATION_INVALID ||

        config->optimization >=

            VITTE_CODEGEN_OPTIMIZATION_COUNT) {

        return false;

    }

    if (config->max_units == 0u) {

        return false;

    }

    if (config->max_errors == 0u) {

        return false;

    }

    return true;

}

bool

vitte_codegen_target_validate(

    const vitte_codegen_target_t *target)

{

    if (target == NULL) {

        return false;

    }

    if (target->architecture <=

            VITTE_CODEGEN_ARCH_INVALID ||

        target->architecture >=

            VITTE_CODEGEN_ARCH_COUNT) {

        return false;

    }

    if (target->operating_system <=

            VITTE_CODEGEN_OS_INVALID ||

        target->operating_system >=

            VITTE_CODEGEN_OS_COUNT) {

        return false;

    }

    if (target->abi <=

            VITTE_CODEGEN_ABI_INVALID ||

        target->abi >=

            VITTE_CODEGEN_ABI_COUNT) {

        return false;

    }

    if (target->endian <=

            VITTE_CODEGEN_ENDIAN_INVALID ||

        target->endian >=

            VITTE_CODEGEN_ENDIAN_COUNT) {

        return false;

    }

    if (target->pointer_bits == 0u ||

        target->size_bits == 0u ||

        target->char_bits == 0u) {

        return false;

    }

    return true;

}

bool

vitte_codegen_is_valid(

    const vitte_codegen_t *codegen)

{

    if (codegen == NULL) {

        return false;

    }

    if (codegen->magic !=

        VITTE_CODEGEN_MAGIC) {

        return false;

    }

    if (codegen->state <=

            VITTE_CODEGEN_STATE_INVALID ||

        codegen->state >=

            VITTE_CODEGEN_STATE_DESTROYED) {

        return false;

    }

    if (!vitte_codegen_config_validate(

            &codegen->config)) {

        return false;

    }

    if (!vitte_codegen_target_validate(

            &codegen->target)) {

        return false;

    }

    if (codegen->unit_count >

        codegen->unit_capacity) {

        return false;

    }

    if (codegen->unit_count != 0u &&

        codegen->units == NULL) {

        return false;

    }

    return true;

}

/* ========================================================================= */

/* Diagnostics                                                               */

/* ========================================================================= */

static void

vitte_codegen_emit_diagnostic(

    vitte_codegen_t *codegen,

    vitte_codegen_severity_t severity,

    vitte_codegen_error_t error,

    const char *message,

    vitte_codegen_source_location_t location)

{

    if (codegen == NULL) {

        return;

    }

    switch (severity) {

        case VITTE_CODEGEN_SEVERITY_NOTE:

            ++codegen->stats.note_count;

            break;

        case VITTE_CODEGEN_SEVERITY_WARNING:

            ++codegen->stats.warning_count;

            break;

        case VITTE_CODEGEN_SEVERITY_ERROR:

            ++codegen->stats.error_count;

            break;

        case VITTE_CODEGEN_SEVERITY_INVALID:

        case VITTE_CODEGEN_SEVERITY_COUNT:

            break;

    }

    if (codegen->diagnostic != NULL) {

        codegen->diagnostic(

            codegen->diagnostic_data,

            severity,

            error,

            message,

            location);

    }

}

static bool

vitte_codegen_fail(

    vitte_codegen_t *codegen,

    vitte_codegen_error_t error,

    const char *message)

{

    vitte_codegen_source_location_t location;

    memset(

        &location,

        0,

        sizeof(location));

    if (codegen == NULL) {

        return false;

    }

    codegen->last_error = error;

    if (error ==

        VITTE_CODEGEN_ERROR_CANCELLED) {

        codegen->state =

            VITTE_CODEGEN_STATE_CANCELLED;

        codegen->stats.cancelled = true;

    } else {

        codegen->state =

            VITTE_CODEGEN_STATE_FAILED;

        codegen->stats.failed = true;

    }

    vitte_codegen_emit_diagnostic(

        codegen,

        VITTE_CODEGEN_SEVERITY_ERROR,

        error,

        message,

        location);

    return false;

}

/* ========================================================================= */

/* Capacity                                                                  */

/* ========================================================================= */

static bool

vitte_codegen_reserve_units(

    vitte_codegen_t *codegen,

    size_t required)

{

    vitte_codegen_unit_t *new_units;

    size_t capacity;

    size_t bytes;

    if (required <=

        codegen->unit_capacity) {

        return true;

    }

    if (required >

        codegen->config.max_units) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_TOO_MANY_UNITS,

                "code generation unit limit exceeded");

    }

    capacity =

        codegen->unit_capacity;

    if (capacity == 0u) {

        capacity =

            VITTE_CODEGEN_DEFAULT_INITIAL_UNIT_CAPACITY;

    }

    while (capacity < required) {

        size_t next;

        if (!vitte_codegen_size_add(

                capacity,

                capacity,

                &next)) {

            return

                vitte_codegen_fail(

                    codegen,

                    VITTE_CODEGEN_ERROR_OVERFLOW,

                    "code generation unit capacity overflow");

        }

        capacity = next;

        if (capacity >

            codegen->config.max_units) {

            capacity =

                codegen->config.max_units;

            break;

        }

    }

    if (capacity < required) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_TOO_MANY_UNITS,

                "code generation unit limit exceeded");

    }

    if (!vitte_codegen_size_mul(

            capacity,

            sizeof(*new_units),

            &bytes)) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_OVERFLOW,

                "code generation allocation size overflow");

    }

    new_units =

        (vitte_codegen_unit_t *)realloc(

            codegen->units,

            bytes);

    if (new_units == NULL) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_OUT_OF_MEMORY,

                "unable to grow code generation unit storage");

    }

    codegen->units = new_units;

    codegen->unit_capacity = capacity;

    return true;

}

/* ========================================================================= */

/* Lifecycle                                                                 */

/* ========================================================================= */

bool

vitte_codegen_init(

    vitte_codegen_t *codegen,

    const vitte_codegen_config_t *config,

    const vitte_codegen_target_t *target)

{

    vitte_codegen_config_t effective_config;

    vitte_codegen_target_t effective_target;

    if (codegen == NULL) {

        return false;

    }

    effective_config =

        config != NULL

            ? *config

            : vitte_codegen_config_default();

    effective_target =

        target != NULL

            ? *target

            : vitte_codegen_target_default();

    if (!vitte_codegen_config_validate(

            &effective_config)) {

        return false;

    }

    if (!vitte_codegen_target_validate(

            &effective_target)) {

        return false;

    }

    memset(

        codegen,

        0,

        sizeof(*codegen));

    codegen->magic =

        VITTE_CODEGEN_MAGIC;

    codegen->state =

        VITTE_CODEGEN_STATE_READY;

    codegen->last_error =

        VITTE_CODEGEN_ERROR_NONE;

    codegen->config =

        effective_config;

    codegen->target =

        effective_target;

    codegen->generation = 1u;

    return true;

}

void

vitte_codegen_destroy(

    vitte_codegen_t *codegen)

{

    if (codegen == NULL ||

        codegen->magic !=

            VITTE_CODEGEN_MAGIC) {

        return;

    }

    if (codegen->backend_interface.destroy !=

        NULL) {

        codegen->backend_interface.destroy(

            codegen->backend_data);

    }

    free(codegen->units);

    codegen->units = NULL;

    codegen->unit_count = 0u;

    codegen->unit_capacity = 0u;

    codegen->backend_data = NULL;

    memset(

        &codegen->backend_interface,

        0,

        sizeof(codegen->backend_interface));

    codegen->state =

        VITTE_CODEGEN_STATE_DESTROYED;

    codegen->magic =

        VITTE_CODEGEN_DEAD_MAGIC;

}

/* ========================================================================= */

/* Backend binding                                                           */

/* ========================================================================= */

bool

vitte_codegen_set_backend(

    vitte_codegen_t *codegen,

    const vitte_codegen_backend_interface_t *interface,

    void *backend_data)

{

    if (!vitte_codegen_is_valid(

            codegen) ||

        interface == NULL ||

        interface->emit_unit == NULL) {

        return false;

    }

    if (codegen->state !=

        VITTE_CODEGEN_STATE_READY) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_INVALID_STATE,

                "backend can only be configured in ready state");

    }

    codegen->backend_interface =

        *interface;

    codegen->backend_data =

        backend_data;

    return true;

}

/* ========================================================================= */

/* Diagnostic binding                                                        */

/* ========================================================================= */

bool

vitte_codegen_set_diagnostic(

    vitte_codegen_t *codegen,

    vitte_codegen_diagnostic_fn diagnostic,

    void *user_data)

{

    if (!vitte_codegen_is_valid(

            codegen)) {

        return false;

    }

    codegen->diagnostic =

        diagnostic;

    codegen->diagnostic_data =

        user_data;

    return true;

}

/* ========================================================================= */

/* Units                                                                     */

/* ========================================================================= */

static bool

vitte_codegen_unit_id_exists(

    const vitte_codegen_t *codegen,

    uint64_t id)

{

    size_t index;

    for (index = 0u;

         index < codegen->unit_count;

         ++index) {

        if (codegen->units[index].id ==

            id) {

            return true;

        }

    }

    return false;

}

bool

vitte_codegen_add_unit(

    vitte_codegen_t *codegen,

    const vitte_codegen_unit_t *unit)

{

    size_t required;

    if (!vitte_codegen_is_valid(

            codegen) ||

        unit == NULL ||

        unit->id == 0u ||

        unit->name == NULL ||

        unit->name[0] == '\0' ||

        unit->input == NULL) {

        return false;

    }

    if (codegen->state !=

        VITTE_CODEGEN_STATE_READY) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_INVALID_STATE,

                "units can only be added in ready state");

    }

    if (vitte_codegen_unit_id_exists(

            codegen,

            unit->id)) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_DUPLICATE_UNIT,

                "duplicate code generation unit identifier");

    }

    if (!vitte_codegen_size_add(

            codegen->unit_count,

            1u,

            &required)) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_OVERFLOW,

                "code generation unit count overflow");

    }

    if (!vitte_codegen_reserve_units(

            codegen,

            required)) {

        return false;

    }

    codegen->units[

        codegen->unit_count] = *unit;

    ++codegen->unit_count;

    codegen->stats.unit_count =

        codegen->unit_count;

    return true;

}

size_t

vitte_codegen_unit_count(

    const vitte_codegen_t *codegen)

{

    if (!vitte_codegen_is_valid(

            codegen)) {

        return 0u;

    }

    return codegen->unit_count;

}

const vitte_codegen_unit_t *

vitte_codegen_unit_at(

    const vitte_codegen_t *codegen,

    size_t index)

{

    if (!vitte_codegen_is_valid(

            codegen) ||

        index >= codegen->unit_count) {

        return NULL;

    }

    return &codegen->units[index];

}

/* ========================================================================= */

/* Fingerprint                                                               */

/* ========================================================================= */

uint64_t

vitte_codegen_fingerprint(

    const vitte_codegen_t *codegen)

{

    uint64_t hash;

    size_t index;

    if (!vitte_codegen_is_valid(

            codegen)) {

        return 0u;

    }

    hash = VITTE_CODEGEN_FNV_OFFSET;

    hash =

        vitte_codegen_hash_u64(

            hash,

            (uint64_t)codegen->config.backend);

    hash =

        vitte_codegen_hash_u64(

            hash,

            (uint64_t)codegen->config.optimization);

    hash =

        vitte_codegen_hash_u64(

            hash,

            (uint64_t)codegen->target.architecture);

    hash =

        vitte_codegen_hash_u64(

            hash,

            (uint64_t)codegen->target.operating_system);

    hash =

        vitte_codegen_hash_u64(

            hash,

            (uint64_t)codegen->target.abi);

    hash =

        vitte_codegen_hash_u64(

            hash,

            (uint64_t)codegen->target.endian);

    hash =

        vitte_codegen_hash_u64(

            hash,

            (uint64_t)codegen->target.pointer_bits);

    hash =

        vitte_codegen_hash_u64(

            hash,

            (uint64_t)codegen->target.size_bits);

    for (index = 0u;

         index < codegen->unit_count;

         ++index) {

        const vitte_codegen_unit_t *unit;

        unit = &codegen->units[index];

        hash =

            vitte_codegen_hash_u64(

                hash,

                unit->id);

        hash =

            vitte_codegen_hash_string(

                hash,

                unit->name);

        /*

         * input is deliberately NOT hashed by pointer value.

         *

         * Pointer addresses are process-specific and would destroy

         * deterministic fingerprints.

         */

    }

    return hash;

}

/* ========================================================================= */

/* Prepare                                                                   */

/* ========================================================================= */

bool

vitte_codegen_prepare(

    vitte_codegen_t *codegen)

{

    if (!vitte_codegen_is_valid(

            codegen)) {

        return false;

    }

    if (codegen->state !=

        VITTE_CODEGEN_STATE_READY) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_INVALID_STATE,

                "code generator is not in ready state");

    }

    if (codegen->backend_interface.emit_unit ==

        NULL) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_INVALID_BACKEND,

                "no code generation backend is installed");

    }

    if (codegen->backend_interface.prepare !=

        NULL) {

        if (!codegen->backend_interface.prepare(

                codegen,

                codegen->backend_data)) {

            if (codegen->state ==

                    VITTE_CODEGEN_STATE_FAILED ||

                codegen->state ==

                    VITTE_CODEGEN_STATE_CANCELLED) {

                return false;

            }

            return

                vitte_codegen_fail(

                    codegen,

                    VITTE_CODEGEN_ERROR_BACKEND,

                    "backend preparation failed");

        }

    }

    codegen->stats.input_fingerprint =

        vitte_codegen_fingerprint(

            codegen);

    codegen->stats.prepared = true;

    codegen->state =

        VITTE_CODEGEN_STATE_PREPARED;

    codegen->last_error =

        VITTE_CODEGEN_ERROR_NONE;

    return true;

}

/* ========================================================================= */

/* Cancellation                                                              */

/* ========================================================================= */

bool

vitte_codegen_cancel(

    vitte_codegen_t *codegen)

{

    if (!vitte_codegen_is_valid(

            codegen)) {

        return false;

    }

    if (codegen->state ==

            VITTE_CODEGEN_STATE_FINISHED ||

        codegen->state ==

            VITTE_CODEGEN_STATE_FAILED ||

        codegen->state ==

            VITTE_CODEGEN_STATE_CANCELLED) {

        return false;

    }

    return

        vitte_codegen_fail(

            codegen,

            VITTE_CODEGEN_ERROR_CANCELLED,

            "code generation cancelled");

}

/* ========================================================================= */

/* Output accounting                                                         */

/* ========================================================================= */

bool

vitte_codegen_account_output(

    vitte_codegen_t *codegen,

    size_t byte_count)

{

    size_t total;

    if (!vitte_codegen_is_valid(

            codegen)) {

        return false;

    }

    if (!vitte_codegen_size_add(

            codegen->stats.output_bytes,

            byte_count,

            &total)) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_OVERFLOW,

                "generated output byte count overflow");

    }

    if (codegen->config.max_output_bytes != 0u &&

        total >

            codegen->config.max_output_bytes) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_OUTPUT_LIMIT,

                "generated output exceeds configured limit");

    }

    codegen->stats.output_bytes = total;

    return true;

}

/* ========================================================================= */

/* Generation                                                                */

/* ========================================================================= */

bool

vitte_codegen_generate(

    vitte_codegen_t *codegen)

{

    size_t index;

    if (!vitte_codegen_is_valid(

            codegen)) {

        return false;

    }

    if (codegen->state ==

        VITTE_CODEGEN_STATE_READY) {

        if (!vitte_codegen_prepare(

                codegen)) {

            return false;

        }

    }

    if (codegen->state !=

        VITTE_CODEGEN_STATE_PREPARED) {

        return

            vitte_codegen_fail(

                codegen,

                VITTE_CODEGEN_ERROR_INVALID_STATE,

                "code generator is not prepared");

    }

    codegen->state =

        VITTE_CODEGEN_STATE_GENERATING;

    if (codegen->backend_interface.begin !=

        NULL) {

        if (!codegen->backend_interface.begin(

                codegen,

                codegen->backend_data)) {

            if (codegen->state ==

                    VITTE_CODEGEN_STATE_FAILED ||

                codegen->state ==

                    VITTE_CODEGEN_STATE_CANCELLED) {

                return false;

            }

            return

                vitte_codegen_fail(

                    codegen,

                    VITTE_CODEGEN_ERROR_BACKEND,

                    "backend begin operation failed");

        }

    }

    for (index = 0u;

         index < codegen->unit_count;

         ++index) {

        if (codegen->state ==

            VITTE_CODEGEN_STATE_CANCELLED) {

            return false;

        }

        if (!codegen->backend_interface.emit_unit(

                codegen,

                &codegen->units[index],

                codegen->backend_data)) {

            if (codegen->state ==

                    VITTE_CODEGEN_STATE_FAILED ||

                codegen->state ==

                    VITTE_CODEGEN_STATE_CANCELLED) {

                return false;

            }

            return

                vitte_codegen_fail(

                    codegen,

                    VITTE_CODEGEN_ERROR_BACKEND,

                    "backend failed to emit translation unit");

        }

        ++codegen->stats.generated_unit_count;

        if (codegen->stats.error_count >=

            codegen->config.max_errors) {

            return

                vitte_codegen_fail(

                    codegen,

                    VITTE_CODEGEN_ERROR_TOO_MANY_ERRORS,

                    "maximum code generation error count reached");

        }

        if (codegen->config.fail_fast &&

            codegen->stats.error_count != 0u) {

            return

                vitte_codegen_fail(

                    codegen,

                    VITTE_CODEGEN_ERROR_BACKEND,

                    "code generation stopped after first error");

        }

    }

    if (codegen->backend_interface.end !=

        NULL) {

        if (!codegen->backend_interface.end(

                codegen,

                codegen->backend_data)) {

            if (codegen->state ==

                    VITTE_CODEGEN_STATE_FAILED ||

                codegen->state ==

                    VITTE_CODEGEN_STATE_CANCELLED) {

                return false;

            }

            return

                vitte_codegen_fail(

                    codegen,

                    VITTE_CODEGEN_ERROR_BACKEND,

                    "backend finalization failed");

        }

    }

    codegen->state =

        VITTE_CODEGEN_STATE_FINISHED;

    codegen->last_error =

        VITTE_CODEGEN_ERROR_NONE;

    codegen->stats.generated = true;

    return true;

}

/* ========================================================================= */

/* Error state                                                               */

/* ========================================================================= */

vitte_codegen_error_t

vitte_codegen_last_error(

    const vitte_codegen_t *codegen)

{

    if (codegen == NULL ||

        codegen->magic !=

            VITTE_CODEGEN_MAGIC) {

        return

            VITTE_CODEGEN_ERROR_INVALID_CODEGEN;

    }

    return codegen->last_error;

}

/* ========================================================================= */

/* State                                                                     */

/* ========================================================================= */

vitte_codegen_state_t

vitte_codegen_state(

    const vitte_codegen_t *codegen)

{

    if (codegen == NULL ||

        codegen->magic !=

            VITTE_CODEGEN_MAGIC) {

        return

            VITTE_CODEGEN_STATE_INVALID;

    }

    return codegen->state;

}

/* ========================================================================= */

/* Statistics                                                                */

/* ========================================================================= */

vitte_codegen_stats_t

vitte_codegen_stats(

    const vitte_codegen_t *codegen)

{

    vitte_codegen_stats_t stats;

    memset(

        &stats,

        0,

        sizeof(stats));

    if (codegen == NULL ||

        codegen->magic !=

            VITTE_CODEGEN_MAGIC) {

        return stats;

    }

    return codegen->stats;

}

/* ========================================================================= */

/* Reset                                                                     */

/* ========================================================================= */

bool

vitte_codegen_reset(

    vitte_codegen_t *codegen)

{

    if (codegen == NULL ||

        codegen->magic !=

            VITTE_CODEGEN_MAGIC) {

        return false;

    }

    if (codegen->state ==

        VITTE_CODEGEN_STATE_GENERATING) {

        return false;

    }

    codegen->unit_count = 0u;

    memset(

        &codegen->stats,

        0,

        sizeof(codegen->stats));

    codegen->last_error =

        VITTE_CODEGEN_ERROR_NONE;

    codegen->state =

        VITTE_CODEGEN_STATE_READY;

    if (codegen->generation != UINT64_MAX) {

        ++codegen->generation;

    }

    return true;

}

/* ========================================================================= */

/* End                                                                       */

/* ========================================================================= */
