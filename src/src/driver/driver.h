#ifndef VITTE_DRIVER_DRIVER_H
#define VITTE_DRIVER_DRIVER_H

/*
 * Vitte Compiler
 * src/driver/driver.h
 *
 * Canonical public API for compiler-driver orchestration.
 *
 * The driver coordinates the compiler pipeline:
 *
 *     CLI / embedding API
 *              |
 *              v
 *          +--------+
 *          | driver |
 *          +--------+
 *              |
 *      +-------+-------+
 *      |       |       |
 *      v       v       v
 *   frontend  IR    codegen/backend
 *      |               |
 *      v               v
 * diagnostics       toolchain
 *                      |
 *                      v
 *                    link
 *                      |
 *                      v
 *                    execute
 *
 * The driver owns orchestration and lifecycle policy.
 *
 * It does NOT implement:
 *
 *   - lexing
 *   - parsing
 *   - AST construction
 *   - import resolution
 *   - name resolution
 *   - type checking
 *   - contract checking
 *   - constant folding
 *   - HIR lowering
 *   - IR lowering
 *   - optimization
 *   - concrete code generation
 *   - C17 emission
 *   - native assembly
 *   - linking
 *   - process execution
 *   - diagnostic rendering
 *
 * Those responsibilities belong to their dedicated subsystems and are
 * connected to the driver through the pipeline callback interface.
 *
 * Ownership
 * ---------
 *
 * The driver owns:
 *
 *   - its input descriptor array;
 *   - orchestration state;
 *   - statistics;
 *   - pipeline configuration.
 *
 * The driver does NOT own:
 *
 *   - input name strings;
 *   - input user_data;
 *   - pipeline user_data;
 *   - diagnostic user_data;
 *   - cancellation user_data.
 *
 * Borrowed objects must remain valid for the duration of their use.
 *
 * Threading
 * ---------
 *
 * A single vitte_driver_t is not internally synchronized.
 *
 * Different driver objects may be used concurrently provided that the
 * installed compiler subsystems and callbacks are themselves safe for that
 * usage.
 *
 * Language
 * --------
 *
 * ISO C17.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* API version                                                               */
/* ========================================================================= */

#define VITTE_DRIVER_API_VERSION_MAJOR 1u
#define VITTE_DRIVER_API_VERSION_MINOR 0u
#define VITTE_DRIVER_API_VERSION_PATCH 0u

/* ========================================================================= */
/* Internal object signatures                                                */
/* ========================================================================= */

/*
 * Public because vitte_driver_t is currently a transparent structure.
 *
 * External users should never modify these values directly.
 */
#define VITTE_DRIVER_MAGIC \
    UINT64_C(0x5649545445445256)

#define VITTE_DRIVER_DEAD_MAGIC \
    UINT64_C(0x4445414444525621)

/* ========================================================================= */
/* Deterministic hashing                                                     */
/* ========================================================================= */

#define VITTE_DRIVER_FNV_OFFSET \
    UINT64_C(14695981039346656037)

#define VITTE_DRIVER_FNV_PRIME \
    UINT64_C(1099511628211)

/* ========================================================================= */
/* Defaults                                                                  */
/* ========================================================================= */

#define VITTE_DRIVER_DEFAULT_MAX_INPUTS \
    ((size_t)65536u)

#define VITTE_DRIVER_DEFAULT_INITIAL_INPUT_CAPACITY \
    ((size_t)16u)

#define VITTE_DRIVER_DEFAULT_MAX_ERRORS \
    ((size_t)100u)

/*
 * Zero means unlimited.
 */
#define VITTE_DRIVER_DEFAULT_MAX_WARNINGS \
    ((size_t)0u)

/*
 * Zero means unlimited.
 */
#define VITTE_DRIVER_DEFAULT_MAX_OUTPUT_BYTES \
    ((size_t)0u)

/* ========================================================================= */
/* Errors                                                                    */
/* ========================================================================= */

typedef enum vitte_driver_error {
    VITTE_DRIVER_ERROR_NONE = 0,

    VITTE_DRIVER_ERROR_INVALID_DRIVER,
    VITTE_DRIVER_ERROR_INVALID_ARGUMENT,
    VITTE_DRIVER_ERROR_INVALID_STATE,
    VITTE_DRIVER_ERROR_INVALID_CONFIG,

    VITTE_DRIVER_ERROR_NO_INPUT,
    VITTE_DRIVER_ERROR_DUPLICATE_INPUT,
    VITTE_DRIVER_ERROR_TOO_MANY_INPUTS,

    VITTE_DRIVER_ERROR_SOURCE,
    VITTE_DRIVER_ERROR_LEXER,
    VITTE_DRIVER_ERROR_PARSER,
    VITTE_DRIVER_ERROR_IMPORT,
    VITTE_DRIVER_ERROR_NAME_RESOLUTION,
    VITTE_DRIVER_ERROR_TYPE_CHECK,
    VITTE_DRIVER_ERROR_CONTRACT,
    VITTE_DRIVER_ERROR_CONSTANT_FOLD,
    VITTE_DRIVER_ERROR_HIR,
    VITTE_DRIVER_ERROR_IR,
    VITTE_DRIVER_ERROR_OPTIMIZATION,
    VITTE_DRIVER_ERROR_CODEGEN,
    VITTE_DRIVER_ERROR_BACKEND,
    VITTE_DRIVER_ERROR_TOOLCHAIN,
    VITTE_DRIVER_ERROR_LINK,
    VITTE_DRIVER_ERROR_EXECUTION,

    VITTE_DRIVER_ERROR_TOO_MANY_ERRORS,
    VITTE_DRIVER_ERROR_OUTPUT_LIMIT,

    VITTE_DRIVER_ERROR_CANCELLED,

    VITTE_DRIVER_ERROR_OVERFLOW,
    VITTE_DRIVER_ERROR_OUT_OF_MEMORY,

    VITTE_DRIVER_ERROR_UNSUPPORTED,
    VITTE_DRIVER_ERROR_INTERNAL,
    VITTE_DRIVER_ERROR_CORRUPTION,

    VITTE_DRIVER_ERROR_COUNT
} vitte_driver_error_t;

/* ========================================================================= */
/* Driver state                                                              */
/* ========================================================================= */

typedef enum vitte_driver_state {
    VITTE_DRIVER_STATE_INVALID = 0,

    VITTE_DRIVER_STATE_READY,
    VITTE_DRIVER_STATE_PREPARING,
    VITTE_DRIVER_STATE_PREPARED,
    VITTE_DRIVER_STATE_RUNNING,
    VITTE_DRIVER_STATE_FINISHED,
    VITTE_DRIVER_STATE_FAILED,
    VITTE_DRIVER_STATE_CANCELLED,
    VITTE_DRIVER_STATE_DESTROYED,

    VITTE_DRIVER_STATE_COUNT
} vitte_driver_state_t;

/* ========================================================================= */
/* Commands                                                                  */
/* ========================================================================= */

typedef enum vitte_driver_command {
    VITTE_DRIVER_COMMAND_INVALID = 0,

    VITTE_DRIVER_COMMAND_BUILD,
    VITTE_DRIVER_COMMAND_CHECK,
    VITTE_DRIVER_COMMAND_RUN,
    VITTE_DRIVER_COMMAND_TEST,

    VITTE_DRIVER_COMMAND_COUNT
} vitte_driver_command_t;

/* ========================================================================= */
/* Compiler phases                                                           */
/* ========================================================================= */

typedef enum vitte_driver_phase {
    VITTE_DRIVER_PHASE_NONE = 0,

    VITTE_DRIVER_PHASE_PREPARE,

    VITTE_DRIVER_PHASE_LOAD_SOURCE,

    VITTE_DRIVER_PHASE_LEX,
    VITTE_DRIVER_PHASE_PARSE,

    VITTE_DRIVER_PHASE_IMPORTS,
    VITTE_DRIVER_PHASE_NAMES,
    VITTE_DRIVER_PHASE_TYPES,
    VITTE_DRIVER_PHASE_CONTRACTS,
    VITTE_DRIVER_PHASE_CONSTANT_FOLD,

    VITTE_DRIVER_PHASE_HIR,
    VITTE_DRIVER_PHASE_IR,

    VITTE_DRIVER_PHASE_OPTIMIZE,

    VITTE_DRIVER_PHASE_CODEGEN,
    VITTE_DRIVER_PHASE_BACKEND,

    VITTE_DRIVER_PHASE_TOOLCHAIN,
    VITTE_DRIVER_PHASE_LINK,

    VITTE_DRIVER_PHASE_EXECUTE,

    VITTE_DRIVER_PHASE_FINALIZE,

    VITTE_DRIVER_PHASE_COUNT
} vitte_driver_phase_t;

/* ========================================================================= */
/* Diagnostic severity                                                       */
/* ========================================================================= */

typedef enum vitte_driver_severity {
    VITTE_DRIVER_SEVERITY_INVALID = 0,

    VITTE_DRIVER_SEVERITY_NOTE,
    VITTE_DRIVER_SEVERITY_WARNING,
    VITTE_DRIVER_SEVERITY_ERROR,

    VITTE_DRIVER_SEVERITY_COUNT
} vitte_driver_severity_t;

/* ========================================================================= */
/* Optimization                                                              */
/* ========================================================================= */

typedef enum vitte_driver_optimization {
    VITTE_DRIVER_OPTIMIZATION_DEFAULT = 0,

    VITTE_DRIVER_OPTIMIZATION_NONE,
    VITTE_DRIVER_OPTIMIZATION_DEBUG,
    VITTE_DRIVER_OPTIMIZATION_BALANCED,
    VITTE_DRIVER_OPTIMIZATION_SPEED,
    VITTE_DRIVER_OPTIMIZATION_SIZE,
    VITTE_DRIVER_OPTIMIZATION_AGGRESSIVE,

    VITTE_DRIVER_OPTIMIZATION_COUNT
} vitte_driver_optimization_t;

/* ========================================================================= */
/* Backend                                                                   */
/* ========================================================================= */

typedef enum vitte_driver_backend {
    VITTE_DRIVER_BACKEND_DEFAULT = 0,

    VITTE_DRIVER_BACKEND_C17,
    VITTE_DRIVER_BACKEND_NATIVE,
    VITTE_DRIVER_BACKEND_BYTECODE,

    VITTE_DRIVER_BACKEND_COUNT
} vitte_driver_backend_t;

/* ========================================================================= */
/* Source location                                                           */
/* ========================================================================= */

/*
 * Byte-oriented source range.
 *
 * begin is inclusive.
 * end is exclusive.
 *
 * valid == false means that the diagnostic or operation is not associated
 * with a concrete source range.
 */
typedef struct vitte_driver_source_location {
    uint32_t file_id;

    size_t begin;
    size_t end;

    bool valid;
} vitte_driver_source_location_t;

/* ========================================================================= */
/* Input                                                                     */
/* ========================================================================= */

/*
 * Input descriptor.
 *
 * name:
 *     Borrowed input name/path.
 *
 * user_data:
 *     Optional subsystem-defined source object.
 *
 * primary:
 *     Indicates an explicitly requested compilation root rather than a source
 *     discovered indirectly through imports/modules.
 */
typedef struct vitte_driver_input {
    uint64_t id;

    const char *name;
    const void *user_data;

    bool primary;
} vitte_driver_input_t;

/* ========================================================================= */
/* Diagnostic                                                                */
/* ========================================================================= */

/*
 * Lightweight driver-level diagnostic envelope.
 *
 * Full compiler diagnostics may contain considerably more information:
 *
 *   - multiple labels;
 *   - declaration locations;
 *   - cause chains;
 *   - notes;
 *   - help;
 *   - fix-its;
 *   - source-map information.
 *
 * Those belong to the diagnostic subsystem.
 *
 * This structure only carries enough information for orchestration,
 * accounting and simple embedding callbacks.
 */
typedef struct vitte_driver_diagnostic {
    vitte_driver_severity_t severity;
    vitte_driver_phase_t phase;

    /*
     * Borrowed strings.
     *
     * Example code:
     *
     *     "E0501"
     */
    const char *code;
    const char *message;

    vitte_driver_source_location_t location;
} vitte_driver_diagnostic_t;

/* ========================================================================= */
/* Driver configuration                                                      */
/* ========================================================================= */

typedef struct vitte_driver_config {
    vitte_driver_command_t command;

    vitte_driver_backend_t backend;
    vitte_driver_optimization_t optimization;

    size_t max_inputs;
    size_t max_errors;

    /*
     * Zero means unlimited.
     */
    size_t max_warnings;

    /*
     * Zero means unlimited.
     */
    size_t max_output_bytes;

    bool deterministic;

    bool warnings_as_errors;
    bool fail_fast;

    bool emit_debug_info;
    bool emit_source_map;
    bool emit_comments;

    bool runtime_checks;
    bool bounds_checks;
    bool null_checks;
    bool overflow_checks;

    bool incremental;

    /*
     * At most one stop_after_* option may be enabled.
     */
    bool stop_after_parse;
    bool stop_after_semantic;
    bool stop_after_ir;
    bool stop_after_codegen;

    /*
     * Explicitly request execution after a normal build.
     *
     * RUN and TEST commands already imply execution.
     */
    bool execute_after_build;
} vitte_driver_config_t;

/* ========================================================================= */
/* Per-phase statistics                                                      */
/* ========================================================================= */

typedef struct vitte_driver_phase_stats {
    uint64_t invocations;
    uint64_t successes;
    uint64_t failures;

    /*
     * Best-effort wall-clock timing.
     *
     * UINT64_MAX may be used as a saturated value.
     */
    uint64_t elapsed_nanoseconds;
} vitte_driver_phase_stats_t;

/* ========================================================================= */
/* Global driver statistics                                                  */
/* ========================================================================= */

typedef struct vitte_driver_stats {
    size_t input_count;
    size_t processed_input_count;

    size_t error_count;
    size_t warning_count;
    size_t note_count;

    size_t output_bytes;

    uint64_t phase_count;

    uint64_t input_fingerprint;
    uint64_t configuration_fingerprint;

    uint64_t elapsed_nanoseconds;

    vitte_driver_phase_stats_t phases[VITTE_DRIVER_PHASE_COUNT];

    bool prepared;
    bool finished;
    bool failed;
    bool cancelled;
} vitte_driver_stats_t;

/* ========================================================================= */
/* Forward declaration                                                       */
/* ========================================================================= */

typedef struct vitte_driver vitte_driver_t;

/* ========================================================================= */
/* Pipeline callback types                                                   */
/* ========================================================================= */

/*
 * Prepare the complete compiler pipeline.
 *
 * Typical work:
 *
 *   - initialize source manager;
 *   - initialize frontend context;
 *   - initialize semantic context;
 *   - initialize IR context;
 *   - initialize codegen/backend;
 *   - validate target/toolchain configuration.
 */
typedef bool (*vitte_driver_prepare_fn)(
    vitte_driver_t *driver,
    void *user_data);

/*
 * Global phase callback.
 *
 * Used for phases that operate on the complete compilation graph or program.
 */
typedef bool (*vitte_driver_phase_fn)(
    vitte_driver_t *driver,
    vitte_driver_phase_t phase,
    void *user_data);

/*
 * Per-input phase callback.
 *
 * Used for source loading, lexing and parsing.
 */
typedef bool (*vitte_driver_input_phase_fn)(
    vitte_driver_t *driver,
    vitte_driver_phase_t phase,
    const vitte_driver_input_t *input,
    void *user_data);

/*
 * Finalization is invoked after the pipeline has completed or failed after
 * entering the main run sequence.
 *
 * success communicates the state of the pipeline before finalization.
 */
typedef bool (*vitte_driver_finalize_fn)(
    vitte_driver_t *driver,
    bool success,
    void *user_data);

/*
 * Destroy callback for subsystem-owned state associated with pipeline_user_data.
 *
 * The driver itself does not free pipeline_user_data.
 */
typedef void (*vitte_driver_destroy_fn)(
    vitte_driver_t *driver,
    void *user_data);

/*
 * Driver-level diagnostic callback.
 *
 * The callback does not own diagnostic or its strings.
 */
typedef void (*vitte_driver_diagnostic_fn)(
    vitte_driver_t *driver,
    const vitte_driver_diagnostic_t *diagnostic,
    void *user_data);

/*
 * Cooperative cancellation callback.
 *
 * Return true to request cancellation.
 */
typedef bool (*vitte_driver_cancel_fn)(
    const vitte_driver_t *driver,
    void *user_data);

/* ========================================================================= */
/* Pipeline                                                                  */
/* ========================================================================= */

typedef struct vitte_driver_pipeline {
    vitte_driver_prepare_fn prepare;

    /* --------------------------------------------------------------------- */
    /* Source/frontend                                                       */
    /* --------------------------------------------------------------------- */

    vitte_driver_input_phase_fn load_source;

    vitte_driver_input_phase_fn lex;
    vitte_driver_input_phase_fn parse;

    /* --------------------------------------------------------------------- */
    /* Semantic analysis                                                     */
    /* --------------------------------------------------------------------- */

    vitte_driver_phase_fn resolve_imports;
    vitte_driver_phase_fn resolve_names;
    vitte_driver_phase_fn check_types;
    vitte_driver_phase_fn check_contracts;
    vitte_driver_phase_fn fold_constants;

    /* --------------------------------------------------------------------- */
    /* Intermediate representations                                          */
    /* --------------------------------------------------------------------- */

    vitte_driver_phase_fn lower_hir;
    vitte_driver_phase_fn lower_ir;

    /* --------------------------------------------------------------------- */
    /* Optimization                                                          */
    /* --------------------------------------------------------------------- */

    vitte_driver_phase_fn optimize;

    /* --------------------------------------------------------------------- */
    /* Generation                                                            */
    /* --------------------------------------------------------------------- */

    vitte_driver_phase_fn codegen;
    vitte_driver_phase_fn backend;

    /* --------------------------------------------------------------------- */
    /* Native/external toolchain                                             */
    /* --------------------------------------------------------------------- */

    vitte_driver_phase_fn toolchain;
    vitte_driver_phase_fn link;

    /* --------------------------------------------------------------------- */
    /* Runtime                                                               */
    /* --------------------------------------------------------------------- */

    vitte_driver_phase_fn execute;

    /* --------------------------------------------------------------------- */
    /* Lifecycle                                                             */
    /* --------------------------------------------------------------------- */

    vitte_driver_finalize_fn finalize;
    vitte_driver_destroy_fn destroy;
} vitte_driver_pipeline_t;

/* ========================================================================= */
/* Driver object                                                             */
/* ========================================================================= */

/*
 * The structure is transparent for now to support static allocation and
 * low-level compiler integration.
 *
 * Callers should nevertheless treat fields as implementation details and use
 * the API below rather than modifying them directly.
 */
struct vitte_driver {
    uint64_t magic;

    vitte_driver_state_t state;
    vitte_driver_error_t last_error;
    vitte_driver_phase_t phase;

    vitte_driver_config_t config;

    vitte_driver_input_t *inputs;
    size_t input_count;
    size_t input_capacity;

    vitte_driver_pipeline_t pipeline;
    void *pipeline_user_data;

    vitte_driver_diagnostic_fn diagnostic;
    void *diagnostic_user_data;

    vitte_driver_cancel_fn cancel;
    void *cancel_user_data;

    vitte_driver_stats_t stats;

    uint64_t next_input_id;
    uint64_t generation;

    bool cancellation_requested;
    bool pipeline_installed;
};

/* ========================================================================= */
/* Enum names                                                                */
/* ========================================================================= */

/*
 * Returned strings have static storage duration.
 */
const char *
vitte_driver_error_name(vitte_driver_error_t error);

const char *
vitte_driver_state_name(vitte_driver_state_t state);

const char *
vitte_driver_command_name(vitte_driver_command_t command);

const char *
vitte_driver_phase_name(vitte_driver_phase_t phase);

const char *
vitte_driver_severity_name(vitte_driver_severity_t severity);

const char *
vitte_driver_backend_name(vitte_driver_backend_t backend);

const char *
vitte_driver_optimization_name(
    vitte_driver_optimization_t optimization);

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

vitte_driver_config_t
vitte_driver_config_default(void);

bool
vitte_driver_config_validate(
    const vitte_driver_config_t *config);

/* ========================================================================= */
/* Structural validation                                                     */
/* ========================================================================= */

bool
vitte_driver_is_valid(const vitte_driver_t *driver);

/* ========================================================================= */
/* Lifecycle                                                                 */
/* ========================================================================= */

/*
 * config == NULL selects vitte_driver_config_default().
 */
bool
vitte_driver_init(
    vitte_driver_t *driver,
    const vitte_driver_config_t *config);

/*
 * Destroy the driver.
 *
 * If installed, pipeline.destroy is invoked before driver-owned memory is
 * released.
 */
void
vitte_driver_destroy(vitte_driver_t *driver);

/*
 * Reset compilation state while retaining:
 *
 *   - allocated input capacity;
 *   - configuration;
 *   - pipeline;
 *   - callback registrations.
 *
 * Input descriptors themselves are removed.
 */
bool
vitte_driver_reset(vitte_driver_t *driver);

/* ========================================================================= */
/* Pipeline configuration                                                    */
/* ========================================================================= */

/*
 * Install the concrete compiler pipeline.
 *
 * pipeline is copied.
 * user_data is borrowed.
 *
 * Must be called while the driver is READY.
 */
bool
vitte_driver_set_pipeline(
    vitte_driver_t *driver,
    const vitte_driver_pipeline_t *pipeline,
    void *user_data);

void
vitte_driver_set_diagnostic_callback(
    vitte_driver_t *driver,
    vitte_driver_diagnostic_fn callback,
    void *user_data);

void
vitte_driver_set_cancel_callback(
    vitte_driver_t *driver,
    vitte_driver_cancel_fn callback,
    void *user_data);

/* ========================================================================= */
/* Input management                                                          */
/* ========================================================================= */

/*
 * Add a compilation input.
 *
 * name is borrowed.
 * user_data is borrowed.
 *
 * Duplicate names are rejected.
 */
bool
vitte_driver_add_input(
    vitte_driver_t *driver,
    const char *name,
    const void *user_data,
    bool primary);

size_t
vitte_driver_input_count(const vitte_driver_t *driver);

const vitte_driver_input_t *
vitte_driver_input_at(
    const vitte_driver_t *driver,
    size_t index);

/* ========================================================================= */
/* Compilation                                                               */
/* ========================================================================= */

/*
 * Prepare the pipeline.
 *
 * This validates that:
 *
 *   - at least one input exists;
 *   - a pipeline is installed;
 *   - cancellation has not been requested.
 *
 * The pipeline prepare callback is then invoked when present.
 */
bool
vitte_driver_prepare(vitte_driver_t *driver);

/*
 * Execute the configured compilation pipeline.
 *
 * If the driver is READY, prepare is performed automatically.
 *
 * Normal ordering:
 *
 *   prepare
 *   load-source
 *   lexer
 *   parser
 *   imports
 *   names
 *   types
 *   contracts
 *   constant-fold
 *   HIR
 *   IR
 *   optimize
 *   codegen
 *   backend
 *   toolchain
 *   link
 *   execute
 *   finalize
 *
 * Stop-after policies and CHECK mode may terminate successfully before later
 * phases.
 */
bool
vitte_driver_run(vitte_driver_t *driver);

/* ========================================================================= */
/* Cancellation                                                              */
/* ========================================================================= */

/*
 * Request cooperative cancellation.
 *
 * When called during compilation, the active callback is not forcibly
 * interrupted. Cancellation is observed at the next driver cancellation point.
 */
bool
vitte_driver_cancel(vitte_driver_t *driver);

/* ========================================================================= */
/* Output accounting                                                         */
/* ========================================================================= */

/*
 * Account generated output.
 *
 * This allows backends/toolchain adapters to enforce the global driver output
 * limit without giving the driver ownership of their output buffers.
 */
bool
vitte_driver_account_output(
    vitte_driver_t *driver,
    size_t bytes);

/* ========================================================================= */
/* Diagnostics                                                               */
/* ========================================================================= */

/*
 * Account and dispatch a diagnostic.
 *
 * Driver policy applies:
 *
 *   - warnings-as-errors;
 *   - maximum error count;
 *   - maximum warning count;
 *   - fail-fast.
 *
 * The original diagnostic severity is preserved when passed to the registered
 * callback. warnings_as_errors affects driver accounting/stopping policy.
 */
bool
vitte_driver_emit_diagnostic(
    vitte_driver_t *driver,
    const vitte_driver_diagnostic_t *diagnostic);

/* ========================================================================= */
/* Deterministic fingerprints                                                */
/* ========================================================================= */

/*
 * Fingerprint input descriptors.
 *
 * user_data addresses are intentionally excluded because pointer addresses are
 * process-specific and unsuitable for deterministic fingerprints.
 */
uint64_t
vitte_driver_input_fingerprint(
    const vitte_driver_t *driver);

/*
 * Fingerprint semantic driver configuration.
 */
uint64_t
vitte_driver_configuration_fingerprint(
    const vitte_driver_t *driver);

/* ========================================================================= */
/* Queries                                                                   */
/* ========================================================================= */

vitte_driver_state_t
vitte_driver_state(const vitte_driver_t *driver);

vitte_driver_error_t
vitte_driver_last_error(const vitte_driver_t *driver);

vitte_driver_phase_t
vitte_driver_current_phase(const vitte_driver_t *driver);

vitte_driver_stats_t
vitte_driver_stats(const vitte_driver_t *driver);

uint64_t
vitte_driver_generation(const vitte_driver_t *driver);

/* ========================================================================= */
/* Inline enum validation                                                    */
/* ========================================================================= */

static inline bool
vitte_driver_error_is_valid(vitte_driver_error_t error)
{
    return
        error >= VITTE_DRIVER_ERROR_NONE &&
        error < VITTE_DRIVER_ERROR_COUNT;
}

static inline bool
vitte_driver_state_is_valid(vitte_driver_state_t state)
{
    return
        state > VITTE_DRIVER_STATE_INVALID &&
        state < VITTE_DRIVER_STATE_COUNT;
}

static inline bool
vitte_driver_command_is_valid_public(
    vitte_driver_command_t command)
{
    return
        command > VITTE_DRIVER_COMMAND_INVALID &&
        command < VITTE_DRIVER_COMMAND_COUNT;
}

static inline bool
vitte_driver_phase_is_valid_public(
    vitte_driver_phase_t phase)
{
    return
        phase >= VITTE_DRIVER_PHASE_NONE &&
        phase < VITTE_DRIVER_PHASE_COUNT;
}

static inline bool
vitte_driver_severity_is_valid_public(
    vitte_driver_severity_t severity)
{
    return
        severity > VITTE_DRIVER_SEVERITY_INVALID &&
        severity < VITTE_DRIVER_SEVERITY_COUNT;
}

static inline bool
vitte_driver_backend_is_valid_public(
    vitte_driver_backend_t backend)
{
    return
        backend >= VITTE_DRIVER_BACKEND_DEFAULT &&
        backend < VITTE_DRIVER_BACKEND_COUNT;
}

static inline bool
vitte_driver_optimization_is_valid_public(
    vitte_driver_optimization_t optimization)
{
    return
        optimization >= VITTE_DRIVER_OPTIMIZATION_DEFAULT &&
        optimization < VITTE_DRIVER_OPTIMIZATION_COUNT;
}

/* ========================================================================= */
/* Inline lifecycle queries                                                  */
/* ========================================================================= */

static inline bool
vitte_driver_is_ready(const vitte_driver_t *driver)
{
    return
        driver != NULL &&
        driver->magic == VITTE_DRIVER_MAGIC &&
        driver->state == VITTE_DRIVER_STATE_READY;
}

static inline bool
vitte_driver_is_prepared(const vitte_driver_t *driver)
{
    return
        driver != NULL &&
        driver->magic == VITTE_DRIVER_MAGIC &&
        driver->state == VITTE_DRIVER_STATE_PREPARED;
}

static inline bool
vitte_driver_is_running(const vitte_driver_t *driver)
{
    return
        driver != NULL &&
        driver->magic == VITTE_DRIVER_MAGIC &&
        driver->state == VITTE_DRIVER_STATE_RUNNING;
}

static inline bool
vitte_driver_is_finished(const vitte_driver_t *driver)
{
    return
        driver != NULL &&
        driver->magic == VITTE_DRIVER_MAGIC &&
        driver->state == VITTE_DRIVER_STATE_FINISHED;
}

static inline bool
vitte_driver_has_failed(const vitte_driver_t *driver)
{
    return
        driver != NULL &&
        driver->magic == VITTE_DRIVER_MAGIC &&
        driver->state == VITTE_DRIVER_STATE_FAILED;
}

static inline bool
vitte_driver_was_cancelled(const vitte_driver_t *driver)
{
    return
        driver != NULL &&
        driver->magic == VITTE_DRIVER_MAGIC &&
        driver->state == VITTE_DRIVER_STATE_CANCELLED;
}

static inline bool
vitte_driver_has_error(const vitte_driver_t *driver)
{
    return
        driver != NULL &&
        driver->magic == VITTE_DRIVER_MAGIC &&
        driver->last_error != VITTE_DRIVER_ERROR_NONE;
}

/* ========================================================================= */
/* Inline command queries                                                    */
/* ========================================================================= */

static inline bool
vitte_driver_command_compiles(
    vitte_driver_command_t command)
{
    return
        command == VITTE_DRIVER_COMMAND_BUILD ||
        command == VITTE_DRIVER_COMMAND_CHECK ||
        command == VITTE_DRIVER_COMMAND_RUN ||
        command == VITTE_DRIVER_COMMAND_TEST;
}

static inline bool
vitte_driver_command_executes(
    vitte_driver_command_t command)
{
    return
        command == VITTE_DRIVER_COMMAND_RUN ||
        command == VITTE_DRIVER_COMMAND_TEST;
}

/* ========================================================================= */
/* Inline configuration queries                                              */
/* ========================================================================= */

static inline bool
vitte_driver_uses_c17_backend(const vitte_driver_t *driver)
{
    return
        driver != NULL &&
        driver->magic == VITTE_DRIVER_MAGIC &&
        driver->config.backend == VITTE_DRIVER_BACKEND_C17;
}

static inline bool
vitte_driver_runtime_checks_enabled(const vitte_driver_t *driver)
{
    return
        driver != NULL &&
        driver->magic == VITTE_DRIVER_MAGIC &&
        driver->config.runtime_checks;
}

static inline bool
vitte_driver_all_safety_checks_enabled(
    const vitte_driver_t *driver)
{
    return
        driver != NULL &&
        driver->magic == VITTE_DRIVER_MAGIC &&
        driver->config.runtime_checks &&
        driver->config.bounds_checks &&
        driver->config.null_checks &&
        driver->config.overflow_checks;
}

static inline bool
vitte_driver_is_deterministic(const vitte_driver_t *driver)
{
    return
        driver != NULL &&
        driver->magic == VITTE_DRIVER_MAGIC &&
        driver->config.deterministic;
}

/* ========================================================================= */
/* Inline diagnostic constructors                                            */
/* ========================================================================= */

static inline vitte_driver_source_location_t
vitte_driver_source_location_invalid(void)
{
    vitte_driver_source_location_t location = {0};

    location.valid = false;

    return location;
}

static inline vitte_driver_source_location_t
vitte_driver_source_location_make(
    uint32_t file_id,
    size_t begin,
    size_t end)
{
    vitte_driver_source_location_t location = {0};

    location.file_id = file_id;
    location.begin = begin;
    location.end = end;
    location.valid = begin <= end;

    return location;
}

static inline vitte_driver_diagnostic_t
vitte_driver_diagnostic_make(
    vitte_driver_severity_t severity,
    vitte_driver_phase_t phase,
    const char *code,
    const char *message,
    vitte_driver_source_location_t location)
{
    vitte_driver_diagnostic_t diagnostic = {0};

    diagnostic.severity = severity;
    diagnostic.phase = phase;
    diagnostic.code = code;
    diagnostic.message = message;
    diagnostic.location = location;

    return diagnostic;
}

/* ========================================================================= */
/* Inline statistics queries                                                 */
/* ========================================================================= */

static inline const vitte_driver_phase_stats_t *
vitte_driver_phase_statistics(
    const vitte_driver_t *driver,
    vitte_driver_phase_t phase)
{
    if (driver == NULL ||
        driver->magic != VITTE_DRIVER_MAGIC ||
        phase <= VITTE_DRIVER_PHASE_NONE ||
        phase >= VITTE_DRIVER_PHASE_COUNT) {
        return NULL;
    }

    return &driver->stats.phases[phase];
}

static inline size_t
vitte_driver_error_count(const vitte_driver_t *driver)
{
    if (driver == NULL ||
        driver->magic != VITTE_DRIVER_MAGIC) {
        return 0u;
    }

    return driver->stats.error_count;
}

static inline size_t
vitte_driver_warning_count(const vitte_driver_t *driver)
{
    if (driver == NULL ||
        driver->magic != VITTE_DRIVER_MAGIC) {
        return 0u;
    }

    return driver->stats.warning_count;
}

static inline size_t
vitte_driver_note_count(const vitte_driver_t *driver)
{
    if (driver == NULL ||
        driver->magic != VITTE_DRIVER_MAGIC) {
        return 0u;
    }

    return driver->stats.note_count;
}

static inline size_t
vitte_driver_output_bytes(const vitte_driver_t *driver)
{
    if (driver == NULL ||
        driver->magic != VITTE_DRIVER_MAGIC) {
        return 0u;
    }

    return driver->stats.output_bytes;
}

/* ========================================================================= */
/* Compile-time invariants                                                   */
/* ========================================================================= */

_Static_assert(
    VITTE_DRIVER_PHASE_COUNT > VITTE_DRIVER_PHASE_FINALIZE,
    "driver phase enum invariant");

_Static_assert(
    VITTE_DRIVER_ERROR_COUNT > VITTE_DRIVER_ERROR_INTERNAL,
    "driver error enum invariant");

_Static_assert(
    VITTE_DRIVER_STATE_COUNT > VITTE_DRIVER_STATE_DESTROYED,
    "driver state enum invariant");

_Static_assert(
    VITTE_DRIVER_COMMAND_COUNT > VITTE_DRIVER_COMMAND_TEST,
    "driver command enum invariant");

_Static_assert(
    VITTE_DRIVER_BACKEND_COUNT > VITTE_DRIVER_BACKEND_BYTECODE,
    "driver backend enum invariant");

_Static_assert(
    VITTE_DRIVER_OPTIMIZATION_COUNT >
        VITTE_DRIVER_OPTIMIZATION_AGGRESSIVE,
    "driver optimization enum invariant");

_Static_assert(
    VITTE_DRIVER_DEFAULT_MAX_INPUTS > 0u,
    "driver must permit at least one input");

_Static_assert(
    VITTE_DRIVER_DEFAULT_MAX_ERRORS > 0u,
    "driver must permit at least one error");

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DRIVER_DRIVER_H */
