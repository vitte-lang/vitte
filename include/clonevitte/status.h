/*
 * CloneVitte
 * Public Status API
 *
 * Copyright (C) 2026 VitteFoundation
 * SPDX-License-Identifier: LicenseRef-VFPL-1.0
 */

#ifndef CLONEVITTE_STATUS_H
#define CLONEVITTE_STATUS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Generic status
 * ============================================================ */

typedef enum CloneVitteStatus {
    CLONEVITTE_STATUS_OK = 0,

    /* Generic failures */
    CLONEVITTE_STATUS_ERROR,
    CLONEVITTE_STATUS_INVALID_ARGUMENT,
    CLONEVITTE_STATUS_INVALID_STATE,
    CLONEVITTE_STATUS_NOT_FOUND,
    CLONEVITTE_STATUS_ALREADY_EXISTS,
    CLONEVITTE_STATUS_NOT_SUPPORTED,
    CLONEVITTE_STATUS_NOT_IMPLEMENTED,
    CLONEVITTE_STATUS_CANCELLED,

    /* Resource failures */
    CLONEVITTE_STATUS_OUT_OF_MEMORY,
    CLONEVITTE_STATUS_LIMIT_EXCEEDED,
    CLONEVITTE_STATUS_OVERFLOW,
    CLONEVITTE_STATUS_UNDERFLOW,

    /* I/O */
    CLONEVITTE_STATUS_IO_ERROR,
    CLONEVITTE_STATUS_FILE_NOT_FOUND,
    CLONEVITTE_STATUS_FILE_READ_ERROR,
    CLONEVITTE_STATUS_FILE_WRITE_ERROR,
    CLONEVITTE_STATUS_PERMISSION_DENIED,

    /* Configuration */
    CLONEVITTE_STATUS_CONFIG_ERROR,
    CLONEVITTE_STATUS_INVALID_CONFIG,
    CLONEVITTE_STATUS_INVALID_TARGET,
    CLONEVITTE_STATUS_INVALID_TOOLCHAIN,

    /* Source */
    CLONEVITTE_STATUS_SOURCE_ERROR,
    CLONEVITTE_STATUS_INVALID_SOURCE,
    CLONEVITTE_STATUS_INVALID_ENCODING,

    /* Lexer */
    CLONEVITTE_STATUS_LEX_ERROR,
    CLONEVITTE_STATUS_INVALID_TOKEN,
    CLONEVITTE_STATUS_UNTERMINATED_STRING,
    CLONEVITTE_STATUS_UNTERMINATED_COMMENT,
    CLONEVITTE_STATUS_INVALID_ESCAPE,
    CLONEVITTE_STATUS_INVALID_NUMBER,

    /* Parser */
    CLONEVITTE_STATUS_PARSE_ERROR,
    CLONEVITTE_STATUS_UNEXPECTED_TOKEN,
    CLONEVITTE_STATUS_EXPECTED_TOKEN,
    CLONEVITTE_STATUS_UNEXPECTED_EOF,
    CLONEVITTE_STATUS_INVALID_SYNTAX,
    CLONEVITTE_STATUS_PARSER_DEPTH_EXCEEDED,

    /* AST */
    CLONEVITTE_STATUS_AST_ERROR,
    CLONEVITTE_STATUS_INVALID_AST,
    CLONEVITTE_STATUS_AST_CONTRACT_VIOLATION,

    /* Name resolution */
    CLONEVITTE_STATUS_RESOLUTION_ERROR,
    CLONEVITTE_STATUS_UNRESOLVED_NAME,
    CLONEVITTE_STATUS_DUPLICATE_SYMBOL,
    CLONEVITTE_STATUS_AMBIGUOUS_SYMBOL,
    CLONEVITTE_STATUS_VISIBILITY_ERROR,

    /* Type system */
    CLONEVITTE_STATUS_TYPE_ERROR,
    CLONEVITTE_STATUS_TYPE_MISMATCH,
    CLONEVITTE_STATUS_UNKNOWN_TYPE,
    CLONEVITTE_STATUS_INVALID_CAST,
    CLONEVITTE_STATUS_GENERIC_ERROR,
    CLONEVITTE_STATUS_TRAIT_ERROR,

    /* Semantic analysis */
    CLONEVITTE_STATUS_SEMANTIC_ERROR,
    CLONEVITTE_STATUS_INVALID_OPERATION,
    CLONEVITTE_STATUS_INVALID_CONTROL_FLOW,
    CLONEVITTE_STATUS_INVALID_RETURN,
    CLONEVITTE_STATUS_INVALID_ABI,

    /* HIR */
    CLONEVITTE_STATUS_HIR_ERROR,
    CLONEVITTE_STATUS_INVALID_HIR,
    CLONEVITTE_STATUS_HIR_VERIFY_ERROR,

    /* MIR */
    CLONEVITTE_STATUS_MIR_ERROR,
    CLONEVITTE_STATUS_INVALID_MIR,
    CLONEVITTE_STATUS_MIR_VERIFY_ERROR,

    /* IR */
    CLONEVITTE_STATUS_IR_ERROR,
    CLONEVITTE_STATUS_INVALID_IR,
    CLONEVITTE_STATUS_IR_VERIFY_ERROR,

    /* Optimization */
    CLONEVITTE_STATUS_OPTIMIZATION_ERROR,

    /* Backend */
    CLONEVITTE_STATUS_BACKEND_ERROR,
    CLONEVITTE_STATUS_BACKEND_UNAVAILABLE,
    CLONEVITTE_STATUS_UNSUPPORTED_TARGET,
    CLONEVITTE_STATUS_CODEGEN_ERROR,

    /* External C toolchain */
    CLONEVITTE_STATUS_C_COMPILER_ERROR,
    CLONEVITTE_STATUS_ASSEMBLER_ERROR,
    CLONEVITTE_STATUS_ARCHIVER_ERROR,

    /* Linker */
    CLONEVITTE_STATUS_LINK_ERROR,

    /* Runtime */
    CLONEVITTE_STATUS_RUNTIME_ERROR,

    /* Internal compiler failure */
    CLONEVITTE_STATUS_INTERNAL_ERROR,

    CLONEVITTE_STATUS_COUNT
} CloneVitteStatus;

/* ============================================================
 * Status category
 * ============================================================ */

typedef enum CloneVitteStatusCategory {
    CLONEVITTE_STATUS_CATEGORY_SUCCESS = 0,
    CLONEVITTE_STATUS_CATEGORY_GENERIC,
    CLONEVITTE_STATUS_CATEGORY_RESOURCE,
    CLONEVITTE_STATUS_CATEGORY_IO,
    CLONEVITTE_STATUS_CATEGORY_CONFIG,
    CLONEVITTE_STATUS_CATEGORY_SOURCE,
    CLONEVITTE_STATUS_CATEGORY_LEXER,
    CLONEVITTE_STATUS_CATEGORY_PARSER,
    CLONEVITTE_STATUS_CATEGORY_AST,
    CLONEVITTE_STATUS_CATEGORY_RESOLUTION,
    CLONEVITTE_STATUS_CATEGORY_TYPE,
    CLONEVITTE_STATUS_CATEGORY_SEMANTIC,
    CLONEVITTE_STATUS_CATEGORY_HIR,
    CLONEVITTE_STATUS_CATEGORY_MIR,
    CLONEVITTE_STATUS_CATEGORY_IR,
    CLONEVITTE_STATUS_CATEGORY_OPTIMIZATION,
    CLONEVITTE_STATUS_CATEGORY_BACKEND,
    CLONEVITTE_STATUS_CATEGORY_TOOLCHAIN,
    CLONEVITTE_STATUS_CATEGORY_LINKER,
    CLONEVITTE_STATUS_CATEGORY_RUNTIME,
    CLONEVITTE_STATUS_CATEGORY_INTERNAL
} CloneVitteStatusCategory;

/* ============================================================
 * Process exit codes
 * ============================================================ */

typedef enum CloneVitteExitCode {
    CLONEVITTE_EXIT_SUCCESS = 0,

    CLONEVITTE_EXIT_FAILURE = 1,

    CLONEVITTE_EXIT_USAGE_ERROR = 2,
    CLONEVITTE_EXIT_CONFIG_ERROR = 3,
    CLONEVITTE_EXIT_IO_ERROR = 4,

    CLONEVITTE_EXIT_LEX_ERROR = 10,
    CLONEVITTE_EXIT_PARSE_ERROR = 11,
    CLONEVITTE_EXIT_SEMANTIC_ERROR = 12,

    CLONEVITTE_EXIT_HIR_ERROR = 20,
    CLONEVITTE_EXIT_MIR_ERROR = 21,
    CLONEVITTE_EXIT_IR_ERROR = 22,

    CLONEVITTE_EXIT_CODEGEN_ERROR = 30,
    CLONEVITTE_EXIT_LINK_ERROR = 31,

    CLONEVITTE_EXIT_INTERNAL_ERROR = 70
} CloneVitteExitCode;

/* ============================================================
 * Status classification
 * ============================================================ */

bool
clonevitte_status_is_ok(
    CloneVitteStatus status
);

bool
clonevitte_status_is_error(
    CloneVitteStatus status
);

bool
clonevitte_status_is_frontend_error(
    CloneVitteStatus status
);

bool
clonevitte_status_is_ir_error(
    CloneVitteStatus status
);

bool
clonevitte_status_is_backend_error(
    CloneVitteStatus status
);

bool
clonevitte_status_is_internal_error(
    CloneVitteStatus status
);

/* ============================================================
 * Status information
 * ============================================================ */

CloneVitteStatusCategory
clonevitte_status_category(
    CloneVitteStatus status
);

const char *
clonevitte_status_name(
    CloneVitteStatus status
);

const char *
clonevitte_status_description(
    CloneVitteStatus status
);

const char *
clonevitte_status_category_name(
    CloneVitteStatusCategory category
);

/* ============================================================
 * Exit code conversion
 * ============================================================ */

CloneVitteExitCode
clonevitte_status_exit_code(
    CloneVitteStatus status
);

const char *
clonevitte_exit_code_name(
    CloneVitteExitCode code
);

/* ============================================================
 * Utility macros
 * ============================================================ */

#define CLONEVITTE_SUCCEEDED(status) \
    ((status) == CLONEVITTE_STATUS_OK)

#define CLONEVITTE_FAILED(status) \
    ((status) != CLONEVITTE_STATUS_OK)

#define CLONEVITTE_RETURN_IF_ERROR(expr)          \
    do {                                          \
        CloneVitteStatus cv_status__ = (expr);    \
        if (cv_status__ != CLONEVITTE_STATUS_OK)  \
            return cv_status__;                   \
    } while (0)

#define CLONEVITTE_GOTO_IF_ERROR(expr, label)     \
    do {                                          \
        CloneVitteStatus cv_status__ = (expr);    \
        if (cv_status__ != CLONEVITTE_STATUS_OK)  \
            goto label;                           \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* CLONEVITTE_STATUS_H */