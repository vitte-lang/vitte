#ifndef VITTE_DIAGNOSTIC_REGISTRY_H
#define VITTE_DIAGNOSTIC_REGISTRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "diagnostic.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Vitte diagnostic registry.
 *
 * The registry is the canonical metadata database for stable compiler
 * diagnostics.
 *
 * It maps internal symbolic identifiers:
 *
 *     VITTE_TYPE_E_MISMATCH
 *
 * to stable public codes:
 *
 *     E0501
 *
 * and associates them with:
 *
 *     - default severity;
 *     - compiler phase/origin;
 *     - category;
 *     - title;
 *     - canonical explanation;
 *     - deprecation state.
 *
 * Public diagnostic ranges:
 *
 *     E0000-E0099  infrastructure/compiler
 *     E0100-E0199  lexer
 *     E0200-E0299  parser
 *     E0300-E0399  modules/imports/packages
 *     E0400-E0499  names/scopes
 *     E0500-E0599  types
 *     E0600-E0699  calls/procedures
 *     E0700-E0799  contracts
 *     E0800-E0899  generics/traits/impl
 *     E0900-E0999  C17 backend
 *     E1000-E1099  IR/lowering
 *     E1100-E1199  control flow
 *     E1200-E1299  memory/references/pointers
 *     E1300-E1399  unsafe/low-level
 *     E1400-E1499  FFI/ABI
 *     E1500-E1599  constant evaluation
 *     E1600-E1699  pattern matching
 *     E1700-E1799  concurrency/async
 *     E1800-E1899  macros/compiler/pass
 *     E1900-E1999  target/platform
 *
 *     Wxxxx        warnings
 *     Nxxxx        notes
 *     Hxxxx        help/fix guidance
 *
 * Stable public codes are part of the compiler's user-facing interface.
 * Existing codes must not be silently reassigned to unrelated diagnostics.
 */

/* ========================================================================= */
/* Public code limits                                                        */
/* ========================================================================= */

#define VITTE_DIAGNOSTIC_PUBLIC_CODE_LENGTH ((size_t)5u)

#define VITTE_DIAGNOSTIC_ERROR_CODE_MIN     0u
#define VITTE_DIAGNOSTIC_ERROR_CODE_MAX     1999u

#define VITTE_DIAGNOSTIC_RANGE_WIDTH        100u

/* ========================================================================= */
/* Diagnostic ranges                                                         */
/* ========================================================================= */

typedef enum vitte_diagnostic_registry_range {
    VITTE_DIAGNOSTIC_RANGE_UNKNOWN = 0,

    VITTE_DIAGNOSTIC_RANGE_INFRASTRUCTURE,
    VITTE_DIAGNOSTIC_RANGE_LEXER,
    VITTE_DIAGNOSTIC_RANGE_PARSER,
    VITTE_DIAGNOSTIC_RANGE_MODULE,
    VITTE_DIAGNOSTIC_RANGE_NAME,
    VITTE_DIAGNOSTIC_RANGE_TYPE,
    VITTE_DIAGNOSTIC_RANGE_CALL,
    VITTE_DIAGNOSTIC_RANGE_CONTRACT,
    VITTE_DIAGNOSTIC_RANGE_GENERIC,
    VITTE_DIAGNOSTIC_RANGE_BACKEND,
    VITTE_DIAGNOSTIC_RANGE_IR,
    VITTE_DIAGNOSTIC_RANGE_CONTROL_FLOW,
    VITTE_DIAGNOSTIC_RANGE_MEMORY,
    VITTE_DIAGNOSTIC_RANGE_UNSAFE,
    VITTE_DIAGNOSTIC_RANGE_FFI,
    VITTE_DIAGNOSTIC_RANGE_CONST_EVAL,
    VITTE_DIAGNOSTIC_RANGE_PATTERN,
    VITTE_DIAGNOSTIC_RANGE_CONCURRENCY,
    VITTE_DIAGNOSTIC_RANGE_MACRO,
    VITTE_DIAGNOSTIC_RANGE_TARGET,

    /*
     * Non-error namespaces.
     *
     * W/N/H ranges are classified primarily by their prefix rather than the
     * numeric portion.
     */
    VITTE_DIAGNOSTIC_RANGE_WARNING,
    VITTE_DIAGNOSTIC_RANGE_NOTE,
    VITTE_DIAGNOSTIC_RANGE_HELP
} vitte_diagnostic_registry_range_t;

/* ========================================================================= */
/* Registry entry                                                            */
/* ========================================================================= */

/*
 * One immutable diagnostic metadata record.
 *
 * All string pointers normally refer to static storage.
 */
typedef struct vitte_diagnostic_registry_entry {
    /*
     * Stable internal symbolic identifier.
     *
     * Example:
     *
     *     VITTE_TYPE_E_MISMATCH
     */
    const char *internal_code;

    /*
     * Stable public user-facing code.
     *
     * Example:
     *
     *     E0501
     */
    const char *public_code;

    /*
     * Default severity before command-line policy such as -Werror.
     */
    vitte_diagnostic_severity_t default_severity;

    /*
     * Compiler phase that canonically owns this diagnostic.
     */
    vitte_diagnostic_origin_t origin;

    /*
     * Stable human/machine-readable category.
     *
     * Examples:
     *
     *     "lexer"
     *     "syntax"
     *     "module"
     *     "name"
     *     "type"
     *     "contract"
     *     "backend"
     */
    const char *category;

    /*
     * Short title suitable for:
     *
     *     error[E0501]: type mismatch
     */
    const char *title;

    /*
     * Canonical short explanation used by:
     *
     *     vitte explain E0501
     *
     * Source-specific details belong to vitte_diagnostic_t rather than here.
     */
    const char *explanation;

    /*
     * Deprecated entries remain registered so old public codes can continue
     * to be recognized/documented without being reused for another meaning.
     */
    bool deprecated;
} vitte_diagnostic_registry_entry_t;

/* ========================================================================= */
/* Validation result                                                         */
/* ========================================================================= */

typedef struct vitte_diagnostic_registry_validation {
    /*
     * Overall registry validity.
     */
    bool valid;

    /*
     * Total number of entries inspected.
     */
    size_t entry_count;

    /*
     * Entries with malformed or inconsistent metadata.
     */
    size_t invalid_entry_count;

    /*
     * Duplicate internal symbolic identifiers.
     */
    size_t duplicate_internal_count;

    /*
     * Duplicate public E/W/N/H codes.
     */
    size_t duplicate_public_count;

    /*
     * Index of the first malformed/inconsistent entry.
     *
     * VITTE_DIAGNOSTIC_INDEX_NONE when none exists.
     */
    size_t first_invalid_index;
} vitte_diagnostic_registry_validation_t;

/* ========================================================================= */
/* Registry access                                                           */
/* ========================================================================= */

/*
 * Return the number of registry entries.
 *
 * Registry ordering is deterministic.
 */
size_t
vitte_diagnostic_registry_count(void);

/*
 * Return an immutable registry entry by index.
 *
 * Returns NULL if index is outside the registry.
 */
const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_registry_at(
    size_t index
);

/*
 * Return the complete immutable registry storage.
 *
 * count, when non-NULL, receives the number of entries.
 *
 * The returned memory must not be modified or freed.
 */
const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_registry_entries(
    size_t *count
);

/* ========================================================================= */
/* Lookup                                                                    */
/* ========================================================================= */

/*
 * Find a diagnostic by either:
 *
 *     - public code;
 *     - internal symbolic identifier.
 *
 * Exact registry spelling is expected.
 *
 * Interactive case-insensitive normalization belongs to explain.c.
 */
const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_registry_find(
    const char *code
);

/*
 * Find a diagnostic by public E/W/N/H code only.
 */
const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_registry_find_public(
    const char *public_code
);

/*
 * Find a diagnostic by internal symbolic identifier only.
 */
const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_registry_find_internal(
    const char *internal_code
);

/*
 * Return true if a public or internal identifier exists.
 */
bool
vitte_diagnostic_registry_contains(
    const char *code
);

/* ========================================================================= */
/* Public-code parsing                                                       */
/* ========================================================================= */

/*
 * Return true if text has the syntactic form:
 *
 *     E0000
 *     W0123
 *     N0001
 *     H0042
 *
 * This only validates the code syntax. It does not imply that the code is
 * registered.
 */
bool
vitte_diagnostic_registry_public_code_is_valid(
    const char *public_code
);

/*
 * Parse the four numeric digits of a public code.
 *
 * Example:
 *
 *     E0501 -> 501
 *
 * Returns true on success.
 */
bool
vitte_diagnostic_registry_public_number(
    const char *public_code,
    unsigned *number
);

/*
 * Return the public-code namespace character:
 *
 *     E
 *     W
 *     N
 *     H
 *
 * Returns '\0' for malformed input.
 */
char
vitte_diagnostic_registry_public_prefix(
    const char *public_code
);

/* ========================================================================= */
/* Range classification                                                      */
/* ========================================================================= */

/*
 * Classify a public diagnostic code into its canonical diagnostic range.
 *
 * For E-codes, the numeric portion determines the subsystem.
 *
 * W/N/H codes are classified as warning/note/help namespaces.
 */
vitte_diagnostic_registry_range_t
vitte_diagnostic_registry_range(
    const char *public_code
);

/*
 * Return the stable textual name of a registry range.
 *
 * Typical results:
 *
 *     "infrastructure"
 *     "lexer"
 *     "parser"
 *     "module"
 *     "name"
 *     "type"
 *     "call"
 *     "contract"
 *     "generic"
 *     "backend"
 *     "ir"
 *     "control-flow"
 *     "memory"
 *     "unsafe"
 *     "ffi"
 *     "const-eval"
 *     "pattern"
 *     "concurrency"
 *     "macro"
 *     "target"
 *     "warning"
 *     "note"
 *     "help"
 *     "unknown"
 */
const char *
vitte_diagnostic_registry_range_name(
    vitte_diagnostic_registry_range_t range
);

/*
 * Return true if range is one of the recognized registry ranges.
 */
bool
vitte_diagnostic_registry_range_is_valid(
    vitte_diagnostic_registry_range_t range
);

/* ========================================================================= */
/* Range boundaries                                                          */
/* ========================================================================= */

/*
 * Return the first numeric E-code value belonging to a range.
 *
 * Example:
 *
 *     VITTE_DIAGNOSTIC_RANGE_TYPE -> 500
 *
 * Returns false for W/N/H/UNKNOWN ranges because those namespaces are not
 * represented by one canonical E-code interval.
 */
bool
vitte_diagnostic_registry_range_begin(
    vitte_diagnostic_registry_range_t range,
    unsigned *number
);

/*
 * Return the final numeric E-code value belonging to a range.
 *
 * Example:
 *
 *     VITTE_DIAGNOSTIC_RANGE_TYPE -> 599
 */
bool
vitte_diagnostic_registry_range_end(
    vitte_diagnostic_registry_range_t range,
    unsigned *number
);

/* ========================================================================= */
/* Severity                                                                  */
/* ========================================================================= */

/*
 * Return the canonical severity implied by a public-code namespace.
 *
 * E -> ERROR
 * W -> WARNING
 * N -> NOTE
 * H -> HELP
 *
 * Returns false for malformed public codes.
 *
 * Individual E-code registry entries may still use FATAL when appropriate;
 * validation treats FATAL as compatible with the E namespace.
 */
bool
vitte_diagnostic_registry_public_severity(
    const char *public_code,
    vitte_diagnostic_severity_t *severity
);

/*
 * Count registry entries with an exact default severity.
 */
size_t
vitte_diagnostic_registry_count_severity(
    vitte_diagnostic_severity_t severity
);

/* ========================================================================= */
/* Origin                                                                    */
/* ========================================================================= */

/*
 * Count entries owned by a compiler origin/phase.
 */
size_t
vitte_diagnostic_registry_count_origin(
    vitte_diagnostic_origin_t origin
);

/* ========================================================================= */
/* Category                                                                  */
/* ========================================================================= */

/*
 * Count entries whose category exactly matches category.
 *
 * Registry categories are stable lowercase identifiers. This low-level
 * function performs an exact comparison; interactive case-insensitive
 * filtering belongs to explain.c.
 */
size_t
vitte_diagnostic_registry_count_category(
    const char *category
);

/* ========================================================================= */
/* Validation                                                                */
/* ========================================================================= */

/*
 * Validate the complete registry.
 *
 * Validation should verify:
 *
 *     - non-empty internal identifier;
 *     - valid public-code syntax;
 *     - valid severity;
 *     - valid origin;
 *     - non-empty category;
 *     - non-empty title;
 *     - non-empty explanation;
 *     - public prefix/severity consistency;
 *     - E-code range/origin consistency where applicable;
 *     - unique public codes;
 *     - unique internal identifiers.
 *
 * result may be NULL when only the status is required.
 *
 * Expected initialization:
 *
 *     validation.first_invalid_index =
 *         VITTE_DIAGNOSTIC_INDEX_NONE;
 */
vitte_status_t
vitte_diagnostic_registry_validate(
    vitte_diagnostic_registry_validation_t *result
);

/*
 * Validate one registry entry independently.
 *
 * This does not detect duplicates because duplicate detection requires access
 * to the complete registry.
 */
bool
vitte_diagnostic_registry_entry_is_valid(
    const vitte_diagnostic_registry_entry_t *entry
);

/* ========================================================================= */
/* Metadata application                                                      */
/* ========================================================================= */

/*
 * Apply canonical registry metadata to a diagnostic.
 *
 * The diagnostic's code may be either the internal or public identifier.
 *
 * On success, canonical metadata can be used to normalize:
 *
 *     - public code;
 *     - short code;
 *     - severity;
 *     - origin;
 *     - category.
 *
 * Source-specific message/details/spans are not replaced.
 */
vitte_status_t
vitte_diagnostic_registry_apply(
    vitte_diagnostic_t *diagnostic
);

/*
 * Apply a known registry entry directly.
 *
 * This avoids performing another lookup when the caller already has the
 * canonical entry.
 */
vitte_status_t
vitte_diagnostic_registry_apply_entry(
    vitte_diagnostic_t *diagnostic,
    const vitte_diagnostic_registry_entry_t *entry
);

/* ========================================================================= */
/* Fingerprint                                                               */
/* ========================================================================= */

/*
 * Return a deterministic fingerprint of registry metadata.
 *
 * The fingerprint is useful for:
 *
 *     - generated documentation cache invalidation;
 *     - protocol/cache compatibility;
 *     - golden tests;
 *     - debugging;
 *     - LSP metadata caching.
 *
 * It is not a public diagnostic identifier and must never replace E/W/N/H
 * codes.
 */
uint64_t
vitte_diagnostic_registry_fingerprint(void);

/*
 * Compute the deterministic fingerprint of one registry entry.
 */
uint64_t
vitte_diagnostic_registry_entry_fingerprint(
    const vitte_diagnostic_registry_entry_t *entry
);

/* ========================================================================= */
/* Iteration                                                                 */
/* ========================================================================= */

/*
 * Registry visitor callback.
 *
 * Returning false stops iteration.
 */
typedef bool
(*vitte_diagnostic_registry_visitor_t)(
    const vitte_diagnostic_registry_entry_t *entry,
    size_t index,
    void *user_data
);

/*
 * Visit registry entries in deterministic registry order.
 *
 * Returns the number of entries passed to the visitor.
 */
size_t
vitte_diagnostic_registry_visit(
    vitte_diagnostic_registry_visitor_t visitor,
    void *user_data
);

/* ========================================================================= */
/* Convenience                                                               */
/* ========================================================================= */

static inline bool
vitte_diagnostic_registry_is_error_code(
    const char *public_code
)
{
    return vitte_diagnostic_registry_public_prefix(
        public_code
    ) == 'E';
}

static inline bool
vitte_diagnostic_registry_is_warning_code(
    const char *public_code
)
{
    return vitte_diagnostic_registry_public_prefix(
        public_code
    ) == 'W';
}

static inline bool
vitte_diagnostic_registry_is_note_code(
    const char *public_code
)
{
    return vitte_diagnostic_registry_public_prefix(
        public_code
    ) == 'N';
}

static inline bool
vitte_diagnostic_registry_is_help_code(
    const char *public_code
)
{
    return vitte_diagnostic_registry_public_prefix(
        public_code
    ) == 'H';
}

/*
 * Return the registry entry for a diagnostic's current code.
 */
static inline const vitte_diagnostic_registry_entry_t *
vitte_diagnostic_registry_for_diagnostic(
    const vitte_diagnostic_t *diagnostic
)
{
    if (diagnostic == NULL) {
        return NULL;
    }

    if (diagnostic->code != NULL &&
        diagnostic->code[0] != '\0') {

        const vitte_diagnostic_registry_entry_t *entry;

        entry =
            vitte_diagnostic_registry_find(
                diagnostic->code
            );

        if (entry != NULL) {
            return entry;
        }
    }

    if (diagnostic->short_code != NULL &&
        diagnostic->short_code[0] != '\0') {

        return vitte_diagnostic_registry_find(
            diagnostic->short_code
        );
    }

    return NULL;
}

/* ========================================================================= */
/* Stability guarantees                                                      */
/* ========================================================================= */

/*
 * Public codes
 * ------------
 *
 * A released public diagnostic code must retain its semantic meaning.
 *
 * If a diagnostic becomes obsolete, mark its registry entry deprecated rather
 * than reusing the public code for an unrelated condition.
 *
 *
 * Internal identifiers
 * --------------------
 *
 * Internal symbolic identifiers should also remain stable whenever practical
 * because tests, compiler components and external tooling may reference them.
 *
 *
 * Registry order
 * --------------
 *
 * Registry iteration is deterministic, but callers should identify
 * diagnostics by stable code rather than by array index.
 *
 *
 * Fingerprints
 * ------------
 *
 * Registry fingerprints are implementation metadata. They may change when
 * registry metadata changes and are not ABI-stable identifiers.
 *
 *
 * Explanations
 * ------------
 *
 * Registry explanations describe the general diagnostic condition.
 *
 * Concrete source-specific information belongs in vitte_diagnostic_t:
 *
 *     - exact spans;
 *     - labels;
 *     - declaration origin;
 *     - expected/found types;
 *     - symbol;
 *     - call context;
 *     - instantiation chain;
 *     - causes;
 *     - notes;
 *     - help;
 *     - fix-its.
 */

#ifdef __cplusplus
}
#endif

#endif /* VITTE_DIAGNOSTIC_REGISTRY_H */
