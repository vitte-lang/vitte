#ifndef VITTE_API_ERROR_H
#define VITTE_API_ERROR_H

/*
 * Vitte Compiler
 * src/api/error.h
 *
 * Canonical infrastructure error API.
 *
 * This header defines Vitte's compact, allocation-free infrastructure error
 * model.
 *
 * It deliberately does NOT define source-facing compiler diagnostics.
 *
 * Infrastructure:
 *
 *     vitte_status_t
 *          |
 *          v
 *     vitte_error_t
 *
 * Compiler diagnostics:
 *
 *     lexer/parser/import/sema/HIR/IR/backend
 *          |
 *          v
 *     vitte_diagnostic_t
 *
 * The error API answers:
 *
 *     "Did an infrastructure operation fail, and why?"
 *
 * The diagnostic API answers:
 *
 *     "Where did compilation fail, why, what caused it, and how can the
 *      programmer fix it?"
 *
 * Design:
 *
 *   - ISO C17
 *   - allocation-free error object
 *   - bounded inline details storage
 *   - borrowed code/message metadata
 *   - owned contextual details
 *   - thread-local last-error facility
 *   - deterministic status metadata
 *   - canonical OK state
 *   - defensive unknown-status handling
 *   - self-copy safe implementation
 *   - explicit normalization/validation
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
 * Maximum size of contextual details, including the terminating NUL byte.
 *
 * Details longer than this capacity are deterministically truncated by the
 * implementation to:
 *
 *     VITTE_ERROR_DETAILS_CAPACITY - 1
 *
 * bytes.
 *
 * code and message are not stored in this buffer. They are borrowed strings.
 */
#ifndef VITTE_ERROR_DETAILS_CAPACITY
#define VITTE_ERROR_DETAILS_CAPACITY ((size_t)1024u)
#endif

/* ========================================================================= */
/* Status                                                                    */
/* ========================================================================= */

/*
 * Infrastructure status codes.
 *
 * Keep this enumeration deliberately compact.
 *
 * Source-level conditions such as:
 *
 *     unknown identifier
 *     unknown type
 *     type mismatch
 *     wrong argument count
 *     failed contract
 *     invalid constant expression
 *
 * belong to the structured diagnostic subsystem and its stable E/W/N/H
 * diagnostic codes rather than to this enumeration.
 *
 * Existing numeric values should remain stable once exposed through the
 * public API.
 */
typedef enum vitte_status {
    VITTE_STATUS_OK = 0,

    VITTE_STATUS_ERROR_INVALID_ARGUMENT,
    VITTE_STATUS_ERROR_INVALID_STATE,
    VITTE_STATUS_ERROR_OUT_OF_MEMORY,
    VITTE_STATUS_ERROR_IO,
    VITTE_STATUS_ERROR_PARSE,
    VITTE_STATUS_ERROR_UNSUPPORTED,
    VITTE_STATUS_ERROR_BACKEND,
    VITTE_STATUS_ERROR_INTERNAL
} vitte_status_t;

/* ========================================================================= */
/* Error object                                                              */
/* ========================================================================= */

/*
 * Small infrastructure error object.
 *
 * Ownership:
 *
 *     status
 *         stored by value.
 *
 *     code
 *         borrowed immutable string.
 *
 *     message
 *         borrowed immutable string.
 *
 *     details
 *         NULL or, for canonical objects produced by the implementation,
 *         equal to details_storage.
 *
 *     details_storage
 *         inline object-owned storage.
 *
 *
 * code/message should normally point to static or otherwise sufficiently
 * long-lived strings.
 *
 * Dynamic contextual information belongs in details because details are
 * copied into object-owned storage.
 *
 *
 * IMPORTANT
 * ---------
 *
 * Because details may point into details_storage, a raw structure assignment:
 *
 *     destination = source;
 *
 * can leave destination.details pointing into source.details_storage.
 *
 * Use:
 *
 *     vitte_error_copy()
 *
 * when copying error objects whose independent lifetime matters.
 *
 * vitte_error_normalize() can also repair the internal details pointer after
 * external/raw movement.
 */
typedef struct vitte_error {
    vitte_status_t status;

    const char *code;
    const char *message;

    const char *details;

    char details_storage[VITTE_ERROR_DETAILS_CAPACITY];
} vitte_error_t;

/* ========================================================================= */
/* Status metadata                                                           */
/* ========================================================================= */

/*
 * Return the stable symbolic name associated with status.
 *
 * Examples:
 *
 *     VITTE_STATUS_OK
 *         -> "VITTE_OK"
 *
 *     VITTE_STATUS_ERROR_IO
 *         -> "VITTE_ERROR_IO"
 *
 * Unknown values return:
 *
 *     "VITTE_ERROR_UNKNOWN"
 *
 * The returned pointer has static storage duration.
 */
const char *
vitte_status_name(
    vitte_status_t status
);

/*
 * Return the default human-readable message associated with status.
 *
 * Examples:
 *
 *     VITTE_STATUS_OK
 *         -> "ok"
 *
 *     VITTE_STATUS_ERROR_IO
 *         -> "I/O error"
 *
 * Unknown values return:
 *
 *     "unknown error"
 *
 * The returned pointer has static storage duration.
 */
const char *
vitte_status_message(
    vitte_status_t status
);

/*
 * Return true when status is VITTE_STATUS_OK.
 */
bool
vitte_status_is_ok(
    vitte_status_t status
);

/*
 * Return true when status is not VITTE_STATUS_OK.
 *
 * Note that unknown numeric status values are therefore considered errors.
 * Use vitte_status_is_known() when enum validity must also be checked.
 */
bool
vitte_status_is_error(
    vitte_status_t status
);

/*
 * Return true when status is one of the statuses defined by vitte_status_t.
 */
bool
vitte_status_is_known(
    vitte_status_t status
);

/* ========================================================================= */
/* Lifetime                                                                  */
/* ========================================================================= */

/*
 * Initialize an error object to the canonical OK state:
 *
 *     status  = VITTE_STATUS_OK
 *     code    = "VITTE_OK"
 *     message = "ok"
 *     details = NULL
 *
 * The function does not modify the thread-local last error.
 */
void
vitte_error_init(
    vitte_error_t *error
);

/*
 * Reset an existing error object to the canonical OK state.
 *
 * Equivalent in resulting state to vitte_error_init().
 *
 * The function does not modify the thread-local last error.
 */
void
vitte_error_reset(
    vitte_error_t *error
);

/* ========================================================================= */
/* State queries                                                             */
/* ========================================================================= */

/*
 * Return true when error is non-NULL and contains a non-OK status.
 */
bool
vitte_error_is_set(
    const vitte_error_t *error
);

/*
 * Return true when error is non-NULL and contains VITTE_STATUS_OK.
 */
bool
vitte_error_is_ok(
    const vitte_error_t *error
);

/*
 * Validate the canonical representation of an error object.
 *
 * Validation checks the invariants expected from objects produced by this
 * implementation, including:
 *
 *     - known status;
 *     - non-NULL code;
 *     - non-NULL message;
 *     - terminated details storage;
 *     - details ownership relationship;
 *     - canonical absence of details for OK.
 *
 * This function performs no mutation.
 */
bool
vitte_error_validate(
    const vitte_error_t *error
);

/* ========================================================================= */
/* Assignment                                                                */
/* ========================================================================= */

/*
 * Assign status, code and message.
 *
 * If code is NULL or empty, vitte_status_name(status) is used.
 *
 * If message is NULL or empty, vitte_status_message(status) is used.
 *
 * Existing details are cleared.
 *
 * Established Vitte API semantics are preserved:
 *
 *     assigning an error also publishes it as the current thread's
 *     last error.
 *
 * code and message are borrowed; they are not copied.
 */
void
vitte_error_set(
    vitte_error_t *error,
    vitte_status_t status,
    const char *code,
    const char *message
);

/*
 * Assign status, code, message and contextual details.
 *
 * code/message:
 *     borrowed.
 *
 * details:
 *     copied immediately into details_storage.
 *
 * If details exceeds the inline capacity, the retained representation is
 * truncated to VITTE_ERROR_DETAILS_CAPACITY - 1 bytes and NUL-terminated.
 *
 * The implementation is alias-safe when details points into the destination
 * object's existing details_storage.
 *
 * Like vitte_error_set(), this function updates the calling thread's
 * last-error object.
 */
void
vitte_error_set_details(
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
 * Copy source into destination while repairing destination-owned details.
 *
 * Unlike a raw structure assignment, this function guarantees that when the
 * resulting destination has details:
 *
 *     destination->details == destination->details_storage
 *
 * Self-copy is supported.
 *
 * A NULL source resets destination to the canonical OK state.
 *
 * This operation does NOT update thread-local last-error state.
 */
void
vitte_error_copy(
    vitte_error_t *destination,
    const vitte_error_t *source
);

/* ========================================================================= */
/* Normalization                                                             */
/* ========================================================================= */

/*
 * Normalize/repair an error object.
 *
 * This is primarily useful after:
 *
 *     - raw structure movement;
 *     - foreign API population;
 *     - deserialization;
 *     - partially initialized external objects.
 *
 * Normalization:
 *
 *     - restores default code/message when absent;
 *     - copies details into object-owned storage;
 *     - rebinds details to details_storage;
 *     - canonicalizes OK to contain no details.
 *
 * It does not update thread-local last-error state.
 */
void
vitte_error_normalize(
    vitte_error_t *error
);

/* ========================================================================= */
/* Details                                                                   */
/* ========================================================================= */

/*
 * Return true when error contains non-empty contextual details.
 */
bool
vitte_error_has_details(
    const vitte_error_t *error
);

/*
 * Return the bounded byte length of contextual details.
 *
 * Returns zero when:
 *
 *     error == NULL
 *     details == NULL
 *     details is empty
 */
size_t
vitte_error_details_length(
    const vitte_error_t *error
);

/* ========================================================================= */
/* Thread-local last error                                                   */
/* ========================================================================= */

/*
 * Publish error as the current thread's last infrastructure error.
 *
 * The error is copied into thread-local storage.
 *
 * Passing NULL clears the last error to canonical OK.
 *
 * The copy is independent from the source object's details storage.
 */
void
vitte_error_set_last(
    const vitte_error_t *error
);

/*
 * Return the calling thread's last infrastructure error.
 *
 * The returned pointer:
 *
 *     - is never NULL;
 *     - refers to thread-local storage;
 *     - remains owned by the error subsystem;
 *     - must not be modified by the caller.
 *
 * Before the first error, it represents canonical OK.
 */
const vitte_error_t *
vitte_error_last(void);

/*
 * Reset the calling thread's last error to canonical OK.
 */
void
vitte_error_clear_last(void);

/* ========================================================================= */
/* Convenience helpers                                                       */
/* ========================================================================= */

/*
 * Return the status stored in error.
 *
 * NULL is treated as VITTE_STATUS_ERROR_INVALID_ARGUMENT because NULL does
 * not represent a valid error object.
 */
static inline vitte_status_t
vitte_error_status(
    const vitte_error_t *error)
{
    return
        error != NULL
            ? error->status
            : VITTE_STATUS_ERROR_INVALID_ARGUMENT;
}

/*
 * Return error code or NULL.
 */
static inline const char *
vitte_error_code(
    const vitte_error_t *error)
{
    return
        error != NULL
            ? error->code
            : NULL;
}

/*
 * Return human-readable error message or NULL.
 */
static inline const char *
vitte_error_message(
    const vitte_error_t *error)
{
    return
        error != NULL
            ? error->message
            : NULL;
}

/*
 * Return contextual details or NULL.
 */
static inline const char *
vitte_error_details(
    const vitte_error_t *error)
{
    return
        error != NULL
            ? error->details
            : NULL;
}

/*
 * Return true when error contains the specified status.
 */
static inline bool
vitte_error_has_status(
    const vitte_error_t *error,
    vitte_status_t status)
{
    return
        error != NULL &&
        error->status == status;
}

/*
 * Return true when the current thread has a non-OK last error.
 */
static inline bool
vitte_error_last_is_set(void)
{
    return
        vitte_error_is_set(
            vitte_error_last());
}

/*
 * Return the current thread's last status.
 */
static inline vitte_status_t
vitte_error_last_status(void)
{
    return
        vitte_error_last()->status;
}

/* ========================================================================= */
/* Usage model                                                               */
/* ========================================================================= */

/*
 * Typical infrastructure failure:
 *
 *     vitte_status_t
 *     vitte_source_open(...)
 *     {
 *         if (invalid_argument) {
 *             vitte_error_set(
 *                 &context->last_error,
 *                 VITTE_STATUS_ERROR_INVALID_ARGUMENT,
 *                 "VITTE_SOURCE_E_INVALID_ARGUMENT",
 *                 "invalid source argument");
 *
 *             return VITTE_STATUS_ERROR_INVALID_ARGUMENT;
 *         }
 *
 *         return VITTE_STATUS_OK;
 *     }
 *
 *
 * Contextual information belongs in details:
 *
 *     vitte_error_set_details(
 *         &context->last_error,
 *         VITTE_STATUS_ERROR_IO,
 *         "VITTE_SOURCE_E_READ",
 *         "unable to read source file",
 *         path);
 *
 *
 * Source-facing compilation errors additionally belong in the structured
 * diagnostic subsystem:
 *
 *     VITTE_STATUS_ERROR_PARSE
 *         +
 *     E02xx diagnostic
 *
 *     VITTE_STATUS_ERROR_BACKEND
 *         +
 *     E09xx diagnostic
 *
 * vitte_status_t controls infrastructure flow.
 * vitte_diagnostic_t explains the source-level failure.
 */

/* ========================================================================= */
/* ABI / ownership guarantees                                                */
/* ========================================================================= */

/*
 * The public object intentionally contains no heap-owned pointer.
 *
 * No destroy function is required.
 *
 * code/message are borrowed and must remain valid for the required lifetime.
 *
 * details are copied into inline storage by the public setters/copy helpers.
 *
 * Raw structure assignment is discouraged because details may point into the
 * source object's inline storage.
 *
 * Prefer:
 *
 *     vitte_error_copy(&destination, &source);
 *
 * when destination must remain independent.
 */

/* ========================================================================= */
/* Thread-safety guarantees                                                  */
/* ========================================================================= */

/*
 * Separate vitte_error_t objects can be used concurrently.
 *
 * vitte_error_last() is backed by C17 _Thread_local storage, so each thread
 * has an independent last-error state.
 *
 * Sharing one ordinary vitte_error_t instance between threads still requires
 * external synchronization when at least one thread mutates it.
 */

/* ========================================================================= */
/* Diagnostic separation                                                     */
/* ========================================================================= */

/*
 * Do not add the following to vitte_error_t:
 *
 *     source spans
 *     labels
 *     source excerpts
 *     severity
 *     diagnostic phase/origin
 *     expected/found types
 *     declaration locations
 *     fix-its
 *     suggestions
 *     causal chains
 *     parser recovery state
 *     SARIF data
 *     LSP data
 *
 * Those belong to diagnostic/diagnostic.h and related diagnostic modules.
 *
 * This separation keeps low-level infrastructure usable independently of the
 * compiler frontend and avoids coupling allocator/filesystem/API failures to
 * AST/source-manager structures.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_API_ERROR_H */