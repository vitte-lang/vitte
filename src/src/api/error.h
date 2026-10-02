#ifndef VITTE_API_ERROR_H
#define VITTE_API_ERROR_H

/*
 * ============================================================================
 * Vitte Core — Infrastructure Error API
 * ============================================================================
 *
 * Lightweight, allocation-free error handling for Vitte infrastructure.
 *
 * This API is intentionally separate from the structured compiler diagnostic
 * system.
 *
 * Infrastructure:
 *
 *     vitte_status_t
 *          |
 *          v
 *     vitte_error_t
 *
 * Source diagnostics:
 *
 *     lexer/parser/resolver/sema/HIR/IR/backend
 *          |
 *          v
 *     vitte_diagnostic_t
 *
 * vitte_error_t represents operation/API failures.
 * vitte_diagnostic_t represents rich compiler diagnostics.
 *
 * Properties:
 *
 *   - C17
 *   - no heap allocation
 *   - fixed-capacity details storage
 *   - thread-local last error
 *   - deterministic status metadata
 *   - safe copying
 *   - NULL-safe public API
 *   - usable from C++
 *   - stable compact status model
 */

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Configuration                                                             */
/* ========================================================================= */

/*
 * Maximum storage reserved inside each vitte_error_t for contextual details.
 *
 * Capacity includes the terminating NUL byte.
 */
#define VITTE_ERROR_DETAILS_CAPACITY ((size_t)4096u)

/*
 * Maximum number of detail characters representable by the inline storage.
 */
#define VITTE_ERROR_DETAILS_MAX_LENGTH \
    (VITTE_ERROR_DETAILS_CAPACITY - (size_t)1u)

/* ========================================================================= */
/* Status                                                                    */
/* ========================================================================= */

/*
 * vitte_status_t is deliberately compact.
 *
 * It represents broad infrastructure failure classes, NOT individual
 * compiler diagnostics.
 *
 * Examples:
 *
 *     unknown identifier       -> diagnostic E04xx
 *     mismatched type          -> diagnostic E05xx
 *     wrong argument count     -> diagnostic E06xx
 *     invalid contract         -> diagnostic E07xx
 *     trait resolution failure -> diagnostic E08xx
 *
 * Those conditions should not each become vitte_status_t values.
 */
typedef enum vitte_status {
    VITTE_STATUS_OK = 0,

    /*
     * Caller supplied an invalid argument.
     */
    VITTE_STATUS_ERROR_INVALID_ARGUMENT,

    /*
     * Operation is invalid for the current object/component state.
     */
    VITTE_STATUS_ERROR_INVALID_STATE,

    /*
     * Memory allocation failed or an allocation-related resource limit was
     * reached.
     */
    VITTE_STATUS_ERROR_OUT_OF_MEMORY,

    /*
     * Filesystem, stream or other I/O operation failed.
     */
    VITTE_STATUS_ERROR_IO,

    /*
     * Parsing failed.
     *
     * Rich parser diagnostics should additionally carry E02xx diagnostics.
     */
    VITTE_STATUS_ERROR_PARSE,

    /*
     * Requested operation, target or feature is unsupported.
     */
    VITTE_STATUS_ERROR_UNSUPPORTED,

    /*
     * Backend/code-generation operation failed.
     *
     * Rich backend diagnostics should additionally carry E09xx diagnostics.
     */
    VITTE_STATUS_ERROR_BACKEND,

    /*
     * Compiler invariant violation or unexpected internal failure.
     *
     * Rich internal diagnostics should additionally carry E00xx diagnostics.
     */
    VITTE_STATUS_ERROR_INTERNAL
} vitte_status_t;

/* ========================================================================= */
/* Error object                                                              */
/* ========================================================================= */

/*
 * Ownership model
 * ---------------
 *
 * status
 *     value
 *
 * code
 *     borrowed string
 *
 * message
 *     borrowed string
 *
 * details
 *     points to details_storage when populated by this API
 *
 * details_storage
 *     owned inline storage
 *
 *
 * Typical canonical OK state:
 *
 *     status  = VITTE_STATUS_OK
 *     code    = "VITTE_OK"
 *     message = "ok"
 *     details = NULL
 */
typedef struct vitte_error {
    vitte_status_t status;

    /*
     * Stable machine-oriented error identifier.
     *
     * Examples:
     *
     *     VITTE_ERROR_INVALID_ARGUMENT
     *     VITTE_IO_E_READ
     *     VITTE_PARSER_E_EXPECTED_TOKEN
     */
    const char *code;

    /*
     * Short human-readable summary.
     */
    const char *message;

    /*
     * Optional contextual information.
     *
     * When produced through this API, this either equals NULL or points to
     * this object's details_storage.
     */
    const char *details;

    /*
     * Inline ownership for contextual details.
     *
     * This keeps the basic error subsystem allocation-free.
     */
    char details_storage[VITTE_ERROR_DETAILS_CAPACITY];
} vitte_error_t;

/* ========================================================================= */
/* Lifetime                                                                  */
/* ========================================================================= */

/*
 * Initialize an error object to the canonical OK state.
 *
 * NULL is accepted and ignored.
 */
void vitte_error_init(
    vitte_error_t *error
);

/*
 * Reset an existing error object to the canonical OK state.
 *
 * Equivalent semantically to vitte_error_init().
 *
 * NULL is accepted and ignored.
 */
void vitte_error_reset(
    vitte_error_t *error
);

/* ========================================================================= */
/* State queries                                                             */
/* ========================================================================= */

/*
 * Return true when error is non-NULL and contains a non-OK status.
 */
bool vitte_error_is_set(
    const vitte_error_t *error
);

/*
 * Return true when error is non-NULL and contains VITTE_STATUS_OK.
 *
 * A NULL pointer is not considered an OK error object.
 */
bool vitte_error_is_ok(
    const vitte_error_t *error
);

/* ========================================================================= */
/* Setters                                                                   */
/* ========================================================================= */

/*
 * Set an error without contextual details.
 *
 * If code is NULL or empty, vitte_status_name(status) is used.
 *
 * If message is NULL or empty, vitte_status_message(status) is used.
 *
 * This operation also updates the calling thread's last-error object.
 */
void vitte_error_set(
    vitte_error_t *error,
    vitte_status_t status,
    const char *code,
    const char *message
);

/*
 * Set an error with optional contextual details.
 *
 * details is copied into the error object's inline details_storage.
 *
 * The caller therefore retains ownership of the input string and may release
 * or modify it immediately after this function returns.
 *
 * Oversized details are truncated safely to:
 *
 *     VITTE_ERROR_DETAILS_MAX_LENGTH
 *
 * bytes and always NUL-terminated.
 *
 * This operation also updates the calling thread's last-error object.
 */
void vitte_error_set_details(
    vitte_error_t *error,
    vitte_status_t status,
    const char *code,
    const char *message,
    const char *details
);

/* ========================================================================= */
/* Copy                                                                      */
/* ========================================================================= */

/*
 * Copy source into destination.
 *
 * Important:
 *
 *     source->details
 *
 * is copied into:
 *
 *     destination->details_storage
 *
 * rather than leaving destination pointing into source storage.
 *
 * Self-copy is valid.
 *
 * If source is NULL, destination is reset to OK.
 *
 * Copying does NOT modify the thread-local last error.
 */
void vitte_error_copy(
    vitte_error_t *destination,
    const vitte_error_t *source
);

/* ========================================================================= */
/* Status metadata                                                           */
/* ========================================================================= */

/*
 * Return the canonical symbolic name for a status.
 *
 * Examples:
 *
 *     VITTE_STATUS_OK
 *         -> "VITTE_OK"
 *
 *     VITTE_STATUS_ERROR_IO
 *         -> "VITTE_ERROR_IO"
 *
 * Unknown numeric values return:
 *
 *     "VITTE_ERROR_UNKNOWN"
 *
 * The returned pointer refers to static storage.
 */
const char *vitte_status_name(
    vitte_status_t status
);

/*
 * Return the canonical human-readable message for a status.
 *
 * Examples:
 *
 *     VITTE_STATUS_OK
 *         -> "ok"
 *
 *     VITTE_STATUS_ERROR_OUT_OF_MEMORY
 *         -> "out of memory"
 *
 * Unknown numeric values return:
 *
 *     "unknown error"
 *
 * The returned pointer refers to static storage.
 */
const char *vitte_status_message(
    vitte_status_t status
);

/* ========================================================================= */
/* Thread-local last error                                                   */
/* ========================================================================= */

/*
 * Replace the calling thread's last-error object.
 *
 * The error is copied.
 *
 * Its details therefore do not reference storage owned by the supplied
 * source object.
 *
 * Passing NULL resets the TLS error to OK.
 */
void vitte_error_set_last(
    const vitte_error_t *error
);

/*
 * Return the calling thread's last-error object.
 *
 * The returned pointer:
 *
 *   - is never NULL;
 *   - refers to thread-local storage;
 *   - remains owned by the Vitte error subsystem;
 *   - must not be freed;
 *   - should be treated as read-only.
 *
 * Each thread receives independent state.
 */
const vitte_error_t *vitte_error_last(void);

/*
 * Reset the calling thread's last error to the canonical OK state.
 */
void vitte_error_clear_last(void);

/* ========================================================================= */
/* Convenience helpers                                                       */
/* ========================================================================= */

/*
 * These helpers deliberately remain header-only so the public ABI does not
 * need additional exported symbols.
 */

static inline bool
vitte_status_is_ok(
    vitte_status_t status
)
{
    return status == VITTE_STATUS_OK;
}

static inline bool
vitte_status_is_error(
    vitte_status_t status
)
{
    return status != VITTE_STATUS_OK;
}

static inline bool
vitte_error_has_details(
    const vitte_error_t *error
)
{
    return error != NULL &&
           error->details != NULL &&
           error->details[0] != '\0';
}

static inline const char *
vitte_error_code(
    const vitte_error_t *error
)
{
    if (error == NULL) {
        return "VITTE_ERROR_UNKNOWN";
    }

    if (error->code != NULL &&
        error->code[0] != '\0') {
        return error->code;
    }

    return vitte_status_name(error->status);
}

static inline const char *
vitte_error_message(
    const vitte_error_t *error
)
{
    if (error == NULL) {
        return "unknown error";
    }

    if (error->message != NULL &&
        error->message[0] != '\0') {
        return error->message;
    }

    return vitte_status_message(error->status);
}

static inline const char *
vitte_error_details(
    const vitte_error_t *error
)
{
    if (!vitte_error_has_details(error)) {
        return NULL;
    }

    return error->details;
}

/* ========================================================================= */
/* Propagation helpers                                                       */
/* ========================================================================= */

/*
 * Return immediately when an expression producing vitte_status_t fails.
 *
 * Example:
 *
 *     vitte_status_t status;
 *
 *     status = vitte_parser_run(parser);
 *     VITTE_RETURN_IF_ERROR(status);
 */
#define VITTE_RETURN_IF_ERROR(expression)                    \
    do {                                                     \
        const vitte_status_t vitte_status__ = (expression);  \
        if (vitte_status__ != VITTE_STATUS_OK) {             \
            return vitte_status__;                           \
        }                                                    \
    } while (0)

/*
 * Evaluate an expression and jump to a local cleanup label when it fails.
 *
 * Requires a local variable named `status`.
 *
 * Example:
 *
 *     vitte_status_t status = VITTE_STATUS_OK;
 *
 *     VITTE_GOTO_IF_ERROR(vitte_operation());
 *
 * cleanup:
 *     ...
 *     return status;
 */
#define VITTE_GOTO_IF_ERROR(expression)                      \
    do {                                                     \
        status = (expression);                               \
        if (status != VITTE_STATUS_OK) {                     \
            goto cleanup;                                    \
        }                                                    \
    } while (0)

/* ========================================================================= */
/* Error-setting convenience macros                                          */
/* ========================================================================= */

/*
 * These macros preserve the ordinary function API while making component
 * failure paths concise.
 */

#define VITTE_ERROR_SET(error_, status_, code_, message_)    \
    vitte_error_set(                                         \
        (error_),                                            \
        (status_),                                           \
        (code_),                                             \
        (message_)                                           \
    )

#define VITTE_ERROR_SET_DETAILS(                             \
    error_,                                                  \
    status_,                                                 \
    code_,                                                   \
    message_,                                                \
    details_                                                 \
)                                                            \
    vitte_error_set_details(                                 \
        (error_),                                            \
        (status_),                                           \
        (code_),                                             \
        (message_),                                          \
        (details_)                                           \
    )

/* ========================================================================= */
/* Common predicates                                                         */
/* ========================================================================= */

static inline bool
vitte_status_is_invalid_argument(
    vitte_status_t status
)
{
    return status == VITTE_STATUS_ERROR_INVALID_ARGUMENT;
}

static inline bool
vitte_status_is_invalid_state(
    vitte_status_t status
)
{
    return status == VITTE_STATUS_ERROR_INVALID_STATE;
}

static inline bool
vitte_status_is_out_of_memory(
    vitte_status_t status
)
{
    return status == VITTE_STATUS_ERROR_OUT_OF_MEMORY;
}

static inline bool
vitte_status_is_io_error(
    vitte_status_t status
)
{
    return status == VITTE_STATUS_ERROR_IO;
}

static inline bool
vitte_status_is_parse_error(
    vitte_status_t status
)
{
    return status == VITTE_STATUS_ERROR_PARSE;
}

static inline bool
vitte_status_is_unsupported(
    vitte_status_t status
)
{
    return status == VITTE_STATUS_ERROR_UNSUPPORTED;
}

static inline bool
vitte_status_is_backend_error(
    vitte_status_t status
)
{
    return status == VITTE_STATUS_ERROR_BACKEND;
}

static inline bool
vitte_status_is_internal_error(
    vitte_status_t status
)
{
    return status == VITTE_STATUS_ERROR_INTERNAL;
}

/* ========================================================================= */
/* Error classification                                                      */
/* ========================================================================= */

/*
 * These are broad infrastructure classifications only.
 *
 * They should not replace diagnostic severity/category.
 */

static inline bool
vitte_error_is_internal(
    const vitte_error_t *error
)
{
    return error != NULL &&
           error->status == VITTE_STATUS_ERROR_INTERNAL;
}

static inline bool
vitte_error_is_backend(
    const vitte_error_t *error
)
{
    return error != NULL &&
           error->status == VITTE_STATUS_ERROR_BACKEND;
}

static inline bool
vitte_error_is_resource_failure(
    const vitte_error_t *error
)
{
    return error != NULL &&
           error->status == VITTE_STATUS_ERROR_OUT_OF_MEMORY;
}

static inline bool
vitte_error_is_external_failure(
    const vitte_error_t *error
)
{
    return error != NULL &&
           error->status == VITTE_STATUS_ERROR_IO;
}

/* ========================================================================= */
/* API design contract                                                       */
/* ========================================================================= */

/*
 * vitte_error_t MUST remain lightweight.
 *
 * Do not add source-facing diagnostic information here merely because a
 * compiler phase requires it.
 *
 * Specifically, these concepts belong to vitte_diagnostic_t:
 *
 *     diagnostic ID
 *     E/W/N/H code namespace
 *     severity
 *     compiler phase
 *     category
 *     primary source span
 *     secondary source spans
 *     labels
 *     declaration location
 *     definition location
 *     use location
 *     value origin
 *     call site
 *     instantiation site
 *     expected type
 *     actual type
 *     symbol context
 *     package context
 *     module context
 *     procedure context
 *     cause chain
 *     notes
 *     help
 *     suggestions
 *     fix-its
 *     applicability
 *     cascade relationship
 *     source-map provenance
 *
 *
 * Intended layering:
 *
 *     operating system
 *          |
 *     filesystem / allocator
 *          |
 *     vitte_error_t
 *          |
 *     driver / compiler phase
 *          |
 *     vitte_diagnostic_t
 *          |
 *     terminal / JSON / SARIF / LSP
 */

/* ========================================================================= */
/* Status design contract                                                    */
/* ========================================================================= */

/*
 * Keep vitte_status_t broad and stable.
 *
 * Do not add statuses such as:
 *
 *     UNKNOWN_IDENTIFIER
 *     TYPE_MISMATCH
 *     INVALID_PRECONDITION
 *     WRONG_ARGUMENT_COUNT
 *     TRAIT_NOT_IMPLEMENTED
 *
 * Those are diagnostic conditions and belong to stable diagnostic codes:
 *
 *     E04xx
 *     E05xx
 *     E07xx
 *     E06xx
 *     E08xx
 *
 * A compact status layer keeps APIs simple:
 *
 *     status -> broad operation result
 *     diagnostic -> precise compiler explanation
 */

/* ========================================================================= */
/* Typical usage                                                             */
/* ========================================================================= */

/*
 * Example:
 *
 *     vitte_status_t
 *     vitte_source_load(...)
 *     {
 *         ...
 *
 *         if (read_failed) {
 *             vitte_error_set_details(
 *                 &source->last_error,
 *                 VITTE_STATUS_ERROR_IO,
 *                 "VITTE_SOURCE_E_READ",
 *                 "unable to read source file",
 *                 path
 *             );
 *
 *             return VITTE_STATUS_ERROR_IO;
 *         }
 *
 *         return VITTE_STATUS_OK;
 *     }
 *
 *
 * A parser may additionally create:
 *
 *     diagnostic E02xx
 *
 * with the actual source span and recovery information.
 */

/* ========================================================================= */
/* C++                                                                       */
/* ========================================================================= */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_API_ERROR_H */
