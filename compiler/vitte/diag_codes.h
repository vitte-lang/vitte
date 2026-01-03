

// SPDX-License-Identifier: MIT
// diag_codes.h
//
// Diagnostic codes for the Vitte toolchain.
//
// Conventions:
//  - Code space is stable. Do not renumber existing codes.
//  - Numeric form is a u32: (category << 16) | index.
//  - 0 is reserved for "none".
//  - Names are ASCII, stable, and suitable for telemetry / golden tests.
//
// This header is intentionally self-contained: it does not depend on any other
// Vitte headers.

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// Severity
//------------------------------------------------------------------------------

typedef enum vitte_diag_severity
{
    VITTE_DIAG_NOTE = 1,
    VITTE_DIAG_WARNING = 2,
    VITTE_DIAG_ERROR = 3,
    VITTE_DIAG_FATAL = 4,
} vitte_diag_severity;

//------------------------------------------------------------------------------
// Category
//------------------------------------------------------------------------------

typedef enum vitte_diag_category
{
    VITTE_DIAG_CAT_NONE = 0,

    // Front-end
    VITTE_DIAG_CAT_LEXER = 1,
    VITTE_DIAG_CAT_PARSER = 2,
    VITTE_DIAG_CAT_SEMA = 3,
    VITTE_DIAG_CAT_RESOLVE = 4,
    VITTE_DIAG_CAT_TYPE = 5,

    // Middle/back
    VITTE_DIAG_CAT_IR = 10,
    VITTE_DIAG_CAT_CODEGEN = 11,
    VITTE_DIAG_CAT_LINK = 12,

    // Runtime / tooling
    VITTE_DIAG_CAT_IO = 20,
    VITTE_DIAG_CAT_CFG = 21,
    VITTE_DIAG_CAT_TOOLCHAIN = 22,

    // Catch-all
    VITTE_DIAG_CAT_INTERNAL = 30,
} vitte_diag_category;

//------------------------------------------------------------------------------
// Code packing
//------------------------------------------------------------------------------

#define VITTE_DIAG_CODE(cat, idx) ((uint32_t)(((uint32_t)(cat) << 16) | ((uint32_t)(idx) & 0xFFFFu)))
#define VITTE_DIAG_CODE_CAT(code) ((uint32_t)((uint32_t)(code) >> 16))
#define VITTE_DIAG_CODE_IDX(code) ((uint32_t)((uint32_t)(code) & 0xFFFFu))

//------------------------------------------------------------------------------
// Diagnostic codes (stable)
//------------------------------------------------------------------------------

// NOTE: Keep this list sorted by (category, index).
// Additions should append within the appropriate category.

typedef enum vitte_diag_code
{
    VITTE_DIAG_NONE = 0,

    // --- LEXER (0x0001_????) -------------------------------------------------
    VITTE_DIAG_LEX_INVALID_BYTE              = VITTE_DIAG_CODE(VITTE_DIAG_CAT_LEXER, 1),
    VITTE_DIAG_LEX_INVALID_UTF8              = VITTE_DIAG_CODE(VITTE_DIAG_CAT_LEXER, 2),
    VITTE_DIAG_LEX_UNTERMINATED_STRING       = VITTE_DIAG_CODE(VITTE_DIAG_CAT_LEXER, 3),
    VITTE_DIAG_LEX_INVALID_ESCAPE            = VITTE_DIAG_CODE(VITTE_DIAG_CAT_LEXER, 4),
    VITTE_DIAG_LEX_INVALID_NUMBER            = VITTE_DIAG_CODE(VITTE_DIAG_CAT_LEXER, 5),
    VITTE_DIAG_LEX_OVERFLOW_INT              = VITTE_DIAG_CODE(VITTE_DIAG_CAT_LEXER, 6),
    VITTE_DIAG_LEX_OVERFLOW_FLOAT            = VITTE_DIAG_CODE(VITTE_DIAG_CAT_LEXER, 7),
    VITTE_DIAG_LEX_NUL_IN_SOURCE             = VITTE_DIAG_CODE(VITTE_DIAG_CAT_LEXER, 8),
    VITTE_DIAG_LEX_UNEXPECTED_EOF            = VITTE_DIAG_CODE(VITTE_DIAG_CAT_LEXER, 9),

    // --- PARSER (0x0002_????) ------------------------------------------------
    VITTE_DIAG_PARSE_UNEXPECTED_TOKEN        = VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 1),
    VITTE_DIAG_PARSE_EXPECTED_TOKEN          = VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 2),
    VITTE_DIAG_PARSE_EXPECTED_IDENT          = VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 3),
    VITTE_DIAG_PARSE_EXPECTED_TYPE           = VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 4),
    VITTE_DIAG_PARSE_EXPECTED_EXPR           = VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 5),
    VITTE_DIAG_PARSE_EXPECTED_STMT           = VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 6),
    VITTE_DIAG_PARSE_EXPECTED_BLOCK_END      = VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 7),
    VITTE_DIAG_PARSE_MISMATCHED_DELIM        = VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 8),
    VITTE_DIAG_PARSE_TRAILING_TOKENS         = VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 9),
    VITTE_DIAG_PARSE_RECURSION_LIMIT         = VITTE_DIAG_CODE(VITTE_DIAG_CAT_PARSER, 10),

    // --- SEMA (0x0003_????) --------------------------------------------------
    VITTE_DIAG_SEMA_UNDEFINED_NAME           = VITTE_DIAG_CODE(VITTE_DIAG_CAT_SEMA, 1),
    VITTE_DIAG_SEMA_REDEFINED_NAME           = VITTE_DIAG_CODE(VITTE_DIAG_CAT_SEMA, 2),
    VITTE_DIAG_SEMA_INVALID_ASSIGN_TARGET    = VITTE_DIAG_CODE(VITTE_DIAG_CAT_SEMA, 3),
    VITTE_DIAG_SEMA_INVALID_BREAK            = VITTE_DIAG_CODE(VITTE_DIAG_CAT_SEMA, 4),
    VITTE_DIAG_SEMA_INVALID_CONTINUE         = VITTE_DIAG_CODE(VITTE_DIAG_CAT_SEMA, 5),
    VITTE_DIAG_SEMA_RETURN_OUTSIDE_FN        = VITTE_DIAG_CODE(VITTE_DIAG_CAT_SEMA, 6),
    VITTE_DIAG_SEMA_CONST_ASSIGN             = VITTE_DIAG_CODE(VITTE_DIAG_CAT_SEMA, 7),
    VITTE_DIAG_SEMA_UNREACHABLE              = VITTE_DIAG_CODE(VITTE_DIAG_CAT_SEMA, 8),

    // --- RESOLVE (0x0004_????) ----------------------------------------------
    VITTE_DIAG_RESOLVE_AMBIGUOUS             = VITTE_DIAG_CODE(VITTE_DIAG_CAT_RESOLVE, 1),
    VITTE_DIAG_RESOLVE_PRIVATE_ITEM          = VITTE_DIAG_CODE(VITTE_DIAG_CAT_RESOLVE, 2),
    VITTE_DIAG_RESOLVE_CYCLE                 = VITTE_DIAG_CODE(VITTE_DIAG_CAT_RESOLVE, 3),

    // --- TYPE (0x0005_????) --------------------------------------------------
    VITTE_DIAG_TYPE_MISMATCH                 = VITTE_DIAG_CODE(VITTE_DIAG_CAT_TYPE, 1),
    VITTE_DIAG_TYPE_CANNOT_INFER             = VITTE_DIAG_CODE(VITTE_DIAG_CAT_TYPE, 2),
    VITTE_DIAG_TYPE_UNKNOWN                  = VITTE_DIAG_CODE(VITTE_DIAG_CAT_TYPE, 3),
    VITTE_DIAG_TYPE_ARITY_MISMATCH           = VITTE_DIAG_CODE(VITTE_DIAG_CAT_TYPE, 4),

    // --- IR (0x000A_????) ----------------------------------------------------
    VITTE_DIAG_IR_INVALID                    = VITTE_DIAG_CODE(VITTE_DIAG_CAT_IR, 1),
    VITTE_DIAG_IR_UNSUPPORTED                = VITTE_DIAG_CODE(VITTE_DIAG_CAT_IR, 2),

    // --- CODEGEN (0x000B_????) ----------------------------------------------
    VITTE_DIAG_CG_UNSUPPORTED_TARGET         = VITTE_DIAG_CODE(VITTE_DIAG_CAT_CODEGEN, 1),
    VITTE_DIAG_CG_INTERNAL_ASSERT            = VITTE_DIAG_CODE(VITTE_DIAG_CAT_CODEGEN, 2),

    // --- LINK (0x000C_????) --------------------------------------------------
    VITTE_DIAG_LINK_FAILED                   = VITTE_DIAG_CODE(VITTE_DIAG_CAT_LINK, 1),

    // --- IO (0x0014_????) ----------------------------------------------------
    VITTE_DIAG_IO_OPEN_FAILED                = VITTE_DIAG_CODE(VITTE_DIAG_CAT_IO, 1),
    VITTE_DIAG_IO_READ_FAILED                = VITTE_DIAG_CODE(VITTE_DIAG_CAT_IO, 2),
    VITTE_DIAG_IO_WRITE_FAILED               = VITTE_DIAG_CODE(VITTE_DIAG_CAT_IO, 3),
    VITTE_DIAG_IO_PATH_TOO_LONG              = VITTE_DIAG_CODE(VITTE_DIAG_CAT_IO, 4),

    // --- CFG (0x0015_????) ---------------------------------------------------
    VITTE_DIAG_CFG_INVALID                   = VITTE_DIAG_CODE(VITTE_DIAG_CAT_CFG, 1),
    VITTE_DIAG_CFG_MISSING_FIELD             = VITTE_DIAG_CODE(VITTE_DIAG_CAT_CFG, 2),

    // --- TOOLCHAIN (0x0016_????) --------------------------------------------
    VITTE_DIAG_TC_MISSING_TOOL               = VITTE_DIAG_CODE(VITTE_DIAG_CAT_TOOLCHAIN, 1),
    VITTE_DIAG_TC_BAD_TOOL_OUTPUT            = VITTE_DIAG_CODE(VITTE_DIAG_CAT_TOOLCHAIN, 2),

    // --- INTERNAL (0x001E_????) ---------------------------------------------
    VITTE_DIAG_INTERNAL_ERROR                = VITTE_DIAG_CODE(VITTE_DIAG_CAT_INTERNAL, 1),
    VITTE_DIAG_INTERNAL_UNREACHABLE          = VITTE_DIAG_CODE(VITTE_DIAG_CAT_INTERNAL, 2),

} vitte_diag_code;

//------------------------------------------------------------------------------
// Metadata lookups
//------------------------------------------------------------------------------

// Returns a stable, human-readable name like "LEX_INVALID_BYTE".
const char* vitte_diag_code_name(uint32_t code);

// Returns the category of the code.
vitte_diag_category vitte_diag_code_category(uint32_t code);

// Returns a default severity for the code.
vitte_diag_severity vitte_diag_code_default_severity(uint32_t code);

// Returns a short default message for the code (may be NULL if unknown).
const char* vitte_diag_code_default_message(uint32_t code);

// Whether the code is known to this table.
bool vitte_diag_code_is_known(uint32_t code);

//------------------------------------------------------------------------------
// Inline implementation (header-only)
//------------------------------------------------------------------------------

typedef struct vitte_diag_code_info
{
    uint32_t code;
    vitte_diag_severity sev;
    const char* name;
    const char* msg;
} vitte_diag_code_info;

static inline const vitte_diag_code_info* vitte_diag__table_(size_t* out_n)
{
    // Keep this list aligned with enum above.
    static const vitte_diag_code_info k[] = {
        { VITTE_DIAG_LEX_INVALID_BYTE,            VITTE_DIAG_ERROR,   "LEX_INVALID_BYTE",            "invalid byte in source" },
        { VITTE_DIAG_LEX_INVALID_UTF8,            VITTE_DIAG_ERROR,   "LEX_INVALID_UTF8",            "invalid UTF-8 sequence" },
        { VITTE_DIAG_LEX_UNTERMINATED_STRING,     VITTE_DIAG_ERROR,   "LEX_UNTERMINATED_STRING",     "unterminated string literal" },
        { VITTE_DIAG_LEX_INVALID_ESCAPE,          VITTE_DIAG_ERROR,   "LEX_INVALID_ESCAPE",          "invalid escape sequence" },
        { VITTE_DIAG_LEX_INVALID_NUMBER,          VITTE_DIAG_ERROR,   "LEX_INVALID_NUMBER",          "invalid numeric literal" },
        { VITTE_DIAG_LEX_OVERFLOW_INT,            VITTE_DIAG_ERROR,   "LEX_OVERFLOW_INT",            "integer literal overflow" },
        { VITTE_DIAG_LEX_OVERFLOW_FLOAT,          VITTE_DIAG_ERROR,   "LEX_OVERFLOW_FLOAT",          "float literal overflow" },
        { VITTE_DIAG_LEX_NUL_IN_SOURCE,           VITTE_DIAG_ERROR,   "LEX_NUL_IN_SOURCE",           "NUL byte in source" },
        { VITTE_DIAG_LEX_UNEXPECTED_EOF,          VITTE_DIAG_ERROR,   "LEX_UNEXPECTED_EOF",          "unexpected end of file" },

        { VITTE_DIAG_PARSE_UNEXPECTED_TOKEN,      VITTE_DIAG_ERROR,   "PARSE_UNEXPECTED_TOKEN",      "unexpected token" },
        { VITTE_DIAG_PARSE_EXPECTED_TOKEN,        VITTE_DIAG_ERROR,   "PARSE_EXPECTED_TOKEN",        "expected token" },
        { VITTE_DIAG_PARSE_EXPECTED_IDENT,        VITTE_DIAG_ERROR,   "PARSE_EXPECTED_IDENT",        "expected identifier" },
        { VITTE_DIAG_PARSE_EXPECTED_TYPE,         VITTE_DIAG_ERROR,   "PARSE_EXPECTED_TYPE",         "expected type" },
        { VITTE_DIAG_PARSE_EXPECTED_EXPR,         VITTE_DIAG_ERROR,   "PARSE_EXPECTED_EXPR",         "expected expression" },
        { VITTE_DIAG_PARSE_EXPECTED_STMT,         VITTE_DIAG_ERROR,   "PARSE_EXPECTED_STMT",         "expected statement" },
        { VITTE_DIAG_PARSE_EXPECTED_BLOCK_END,    VITTE_DIAG_ERROR,   "PARSE_EXPECTED_BLOCK_END",    "expected block terminator" },
        { VITTE_DIAG_PARSE_MISMATCHED_DELIM,      VITTE_DIAG_ERROR,   "PARSE_MISMATCHED_DELIM",      "mismatched delimiter" },
        { VITTE_DIAG_PARSE_TRAILING_TOKENS,       VITTE_DIAG_WARNING, "PARSE_TRAILING_TOKENS",       "trailing tokens after parse" },
        { VITTE_DIAG_PARSE_RECURSION_LIMIT,       VITTE_DIAG_FATAL,   "PARSE_RECURSION_LIMIT",       "parser recursion limit exceeded" },

        { VITTE_DIAG_SEMA_UNDEFINED_NAME,         VITTE_DIAG_ERROR,   "SEMA_UNDEFINED_NAME",         "undefined name" },
        { VITTE_DIAG_SEMA_REDEFINED_NAME,         VITTE_DIAG_ERROR,   "SEMA_REDEFINED_NAME",         "redefined name" },
        { VITTE_DIAG_SEMA_INVALID_ASSIGN_TARGET,  VITTE_DIAG_ERROR,   "SEMA_INVALID_ASSIGN_TARGET",  "invalid assignment target" },
        { VITTE_DIAG_SEMA_INVALID_BREAK,          VITTE_DIAG_ERROR,   "SEMA_INVALID_BREAK",          "break outside loop" },
        { VITTE_DIAG_SEMA_INVALID_CONTINUE,       VITTE_DIAG_ERROR,   "SEMA_INVALID_CONTINUE",       "continue outside loop" },
        { VITTE_DIAG_SEMA_RETURN_OUTSIDE_FN,      VITTE_DIAG_ERROR,   "SEMA_RETURN_OUTSIDE_FN",      "return outside function" },
        { VITTE_DIAG_SEMA_CONST_ASSIGN,           VITTE_DIAG_ERROR,   "SEMA_CONST_ASSIGN",           "cannot assign to const" },
        { VITTE_DIAG_SEMA_UNREACHABLE,            VITTE_DIAG_WARNING, "SEMA_UNREACHABLE",            "unreachable code" },

        { VITTE_DIAG_RESOLVE_AMBIGUOUS,           VITTE_DIAG_ERROR,   "RESOLVE_AMBIGUOUS",           "ambiguous resolution" },
        { VITTE_DIAG_RESOLVE_PRIVATE_ITEM,        VITTE_DIAG_ERROR,   "RESOLVE_PRIVATE_ITEM",        "access to private item" },
        { VITTE_DIAG_RESOLVE_CYCLE,               VITTE_DIAG_ERROR,   "RESOLVE_CYCLE",               "resolution cycle" },

        { VITTE_DIAG_TYPE_MISMATCH,               VITTE_DIAG_ERROR,   "TYPE_MISMATCH",               "type mismatch" },
        { VITTE_DIAG_TYPE_CANNOT_INFER,           VITTE_DIAG_ERROR,   "TYPE_CANNOT_INFER",           "cannot infer type" },
        { VITTE_DIAG_TYPE_UNKNOWN,                VITTE_DIAG_ERROR,   "TYPE_UNKNOWN",                "unknown type" },
        { VITTE_DIAG_TYPE_ARITY_MISMATCH,         VITTE_DIAG_ERROR,   "TYPE_ARITY_MISMATCH",         "type arity mismatch" },

        { VITTE_DIAG_IR_INVALID,                  VITTE_DIAG_FATAL,   "IR_INVALID",                  "invalid IR" },
        { VITTE_DIAG_IR_UNSUPPORTED,              VITTE_DIAG_ERROR,   "IR_UNSUPPORTED",              "unsupported IR feature" },

        { VITTE_DIAG_CG_UNSUPPORTED_TARGET,       VITTE_DIAG_FATAL,   "CG_UNSUPPORTED_TARGET",       "unsupported target" },
        { VITTE_DIAG_CG_INTERNAL_ASSERT,          VITTE_DIAG_FATAL,   "CG_INTERNAL_ASSERT",          "codegen internal assertion" },

        { VITTE_DIAG_LINK_FAILED,                 VITTE_DIAG_FATAL,   "LINK_FAILED",                 "link failed" },

        { VITTE_DIAG_IO_OPEN_FAILED,              VITTE_DIAG_ERROR,   "IO_OPEN_FAILED",              "failed to open path" },
        { VITTE_DIAG_IO_READ_FAILED,              VITTE_DIAG_ERROR,   "IO_READ_FAILED",              "failed to read" },
        { VITTE_DIAG_IO_WRITE_FAILED,             VITTE_DIAG_ERROR,   "IO_WRITE_FAILED",             "failed to write" },
        { VITTE_DIAG_IO_PATH_TOO_LONG,            VITTE_DIAG_ERROR,   "IO_PATH_TOO_LONG",            "path too long" },

        { VITTE_DIAG_CFG_INVALID,                 VITTE_DIAG_ERROR,   "CFG_INVALID",                 "invalid configuration" },
        { VITTE_DIAG_CFG_MISSING_FIELD,           VITTE_DIAG_ERROR,   "CFG_MISSING_FIELD",           "missing configuration field" },

        { VITTE_DIAG_TC_MISSING_TOOL,             VITTE_DIAG_FATAL,   "TC_MISSING_TOOL",             "missing tool" },
        { VITTE_DIAG_TC_BAD_TOOL_OUTPUT,          VITTE_DIAG_ERROR,   "TC_BAD_TOOL_OUTPUT",          "tool produced invalid output" },

        { VITTE_DIAG_INTERNAL_ERROR,              VITTE_DIAG_FATAL,   "INTERNAL_ERROR",              "internal error" },
        { VITTE_DIAG_INTERNAL_UNREACHABLE,        VITTE_DIAG_FATAL,   "INTERNAL_UNREACHABLE",        "internal unreachable" },
    };

    if (out_n) *out_n = sizeof(k) / sizeof(k[0]);
    return k;
}

static inline const vitte_diag_code_info* vitte_diag__find_(uint32_t code)
{
    size_t n = 0;
    const vitte_diag_code_info* t = vitte_diag__table_(&n);
    // Small table: linear scan is fine and deterministic.
    for (size_t i = 0; i < n; i++)
    {
        if (t[i].code == code)
            return &t[i];
    }
    return NULL;
}

static inline bool vitte_diag_code_is_known(uint32_t code)
{
    if (code == 0) return false;
    return vitte_diag__find_(code) != NULL;
}

static inline const char* vitte_diag_code_name(uint32_t code)
{
    const vitte_diag_code_info* it = vitte_diag__find_(code);
    return it ? it->name : "UNKNOWN";
}

static inline vitte_diag_category vitte_diag_code_category(uint32_t code)
{
    return (vitte_diag_category)VITTE_DIAG_CODE_CAT(code);
}

static inline vitte_diag_severity vitte_diag_code_default_severity(uint32_t code)
{
    const vitte_diag_code_info* it = vitte_diag__find_(code);
    if (it) return it->sev;

    // Default: errors for unknown non-zero codes.
    if (code == 0) return VITTE_DIAG_NOTE;
    return VITTE_DIAG_ERROR;
}

static inline const char* vitte_diag_code_default_message(uint32_t code)
{
    const vitte_diag_code_info* it = vitte_diag__find_(code);
    return it ? it->msg : NULL;
}

#ifdef __cplusplus
} // extern "C"
#endif