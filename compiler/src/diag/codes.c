// SPDX-License-Identifier: MIT
// codes.c
//
// Diagnostic codes registry (max).
//
// Goals:
//  - Provide stable, enumerable diagnostic codes across the compiler.
//  - Offer helpers to map code -> string name and a short default message.
//  - Keep numeric values stable for CI, tests, and tooling.
//
// Integration:
//  - This file can be used standalone.
//  - If you already have `codes.h`, align it with the public API below.

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "codes.h"

//------------------------------------------------------------------------------
// Helper macros
//------------------------------------------------------------------------------

#define STEEL_DIAG_CODE_TABLE(X) \
    /* General */ \
    X(STEEL_D0001_INTERNAL,              1,  "internal",              "internal compiler error") \
    X(STEEL_D0002_OUT_OF_MEMORY,         2,  "oom",                   "out of memory") \
    X(STEEL_D0003_UNIMPLEMENTED,         3,  "unimplemented",         "feature not implemented") \
    X(STEEL_D0004_INVALID_STATE,         4,  "invalid_state",         "invalid compiler state") \
    \
    /* Lexing */ \
    X(STEEL_D0100_LEX_ERROR,           100,  "lex_error",             "lexical error") \
    X(STEEL_D0101_INVALID_CHAR,        101,  "invalid_char",          "invalid character") \
    X(STEEL_D0102_UNTERMINATED_STRING, 102,  "unterminated_string",  "unterminated string literal") \
    X(STEEL_D0103_INVALID_ESCAPE,      103,  "invalid_escape",        "invalid escape sequence") \
    X(STEEL_D0104_INVALID_NUMBER,      104,  "invalid_number",        "invalid numeric literal") \
    X(STEEL_D0105_UNTERMINATED_CHAR,   105,  "unterminated_char",     "unterminated char literal") \
    X(STEEL_D0106_INVALID_UTF8,        106,  "invalid_utf8",          "invalid UTF-8 sequence") \
    \
    /* Parsing */ \
    X(STEEL_D0200_PARSE_ERROR,          200,  "parse_error",           "parse error") \
    X(STEEL_D0201_UNEXPECTED_TOKEN,     201,  "unexpected_token",      "unexpected token") \
    X(STEEL_D0202_EXPECTED_TOKEN,       202,  "expected_token",        "expected token") \
    X(STEEL_D0203_EXPECTED_IDENT,       203,  "expected_ident",        "expected identifier") \
    X(STEEL_D0204_EXPECTED_TYPE,        204,  "expected_type",         "expected type") \
    X(STEEL_D0205_EXPECTED_EXPR,        205,  "expected_expr",         "expected expression") \
    X(STEEL_D0206_EXPECTED_STMT,        206,  "expected_stmt",         "expected statement") \
    X(STEEL_D0207_MISMATCHED_END,       207,  "mismatched_end",        "mismatched .end") \
    X(STEEL_D0208_UNEXPECTED_EOF,       208,  "unexpected_eof",        "unexpected end of file") \
    \
    /* Name resolution */ \
    X(STEEL_D0300_RESOLVE_ERROR,        300,  "resolve_error",         "name resolution error") \
    X(STEEL_D0301_UNDEFINED_NAME,       301,  "undefined_name",        "undefined name") \
    X(STEEL_D0302_DUPLICATE_DEF,        302,  "duplicate_def",         "duplicate definition") \
    X(STEEL_D0303_SHADOWING,            303,  "shadowing",             "name shadowing") \
    \
    /* Type checking */ \
    X(STEEL_D0400_TYPE_ERROR,           400,  "type_error",            "type error") \
    X(STEEL_D0401_TYPE_MISMATCH,        401,  "type_mismatch",         "type mismatch") \
    X(STEEL_D0402_NOT_CALLABLE,         402,  "not_callable",          "value is not callable") \
    X(STEEL_D0403_NOT_INDEXABLE,        403,  "not_indexable",         "value is not indexable") \
    X(STEEL_D0404_NOT_MEMBER,           404,  "not_member",            "unknown member") \
    X(STEEL_D0405_INVALID_CONDITION,    405,  "invalid_condition",     "condition must be bool") \
    X(STEEL_D0406_RETURN_MISMATCH,      406,  "return_mismatch",       "return type mismatch") \
    \
    /* IR / Codegen */ \
    X(STEEL_D0500_IR_ERROR,             500,  "ir_error",              "IR error") \
    X(STEEL_D0501_INVALID_IR,           501,  "invalid_ir",            "invalid IR") \
    X(STEEL_D0502_VERIFY_FAILED,        502,  "verify_failed",         "IR verification failed") \
    X(STEEL_D0503_CODEGEN_ERROR,        503,  "codegen_error",         "code generation error") \
    \
    /* Tooling / Muffin */ \
    X(STEEL_D0600_MANIFEST_ERROR,       600,  "manifest_error",        "manifest error") \
    X(STEEL_D0601_BAD_VERSION,          601,  "bad_version",           "invalid version") \
    X(STEEL_D0602_BAD_PATH,             602,  "bad_path",              "invalid path") \
    X(STEEL_D0603_DEP_ERROR,            603,  "dep_error",             "dependency error") \
    X(STEEL_D0604_TOOLCHAIN_ERROR,      604,  "toolchain_error",       "toolchain error")

//------------------------------------------------------------------------------
// Public API implementation
//------------------------------------------------------------------------------

const char* steel_diag_code_name(steel_diag_code c)
{
    switch ((int)c)
    {
#define X(sym, val, name, msg) case (val): return (name);
        STEEL_DIAG_CODE_TABLE(X)
#undef X
        default: return "unknown";
    }
}

const char* steel_diag_code_message(steel_diag_code c)
{
    switch ((int)c)
    {
#define X(sym, val, name, msg) case (val): return (msg);
        STEEL_DIAG_CODE_TABLE(X)
#undef X
        default: return "";
    }
}

bool steel_diag_code_is_error(steel_diag_code c)
{
    // Convention: codes >= 1 and < 1000.
    // Severity is decided by emitter, but we provide a heuristic:
    //  - 0 is OK
    //  - parsing / lexing / resolve / type / ir / manifest are errors by default.
    int v = (int)c;
    if (v == 0) return false;

    if (v >= 1 && v < 1000)
        return true;

    return false;
}

bool steel_diag_code_is_warn(steel_diag_code c)
{
    // If you add dedicated warning codes, implement here.
    (void)c;
    return false;
}

size_t steel_diag_code_table_count(void)
{
    // Count entries at compile-time via expansion.
    enum { STEEL_DIAG_CODE_COUNT = 0
#define X(sym, val, name, msg) + 1
        STEEL_DIAG_CODE_TABLE(X)
#undef X
    };

    return (size_t)STEEL_DIAG_CODE_COUNT;
}

bool steel_diag_code_table_at(size_t idx, steel_diag_code* out_code, const char** out_name, const char** out_msg)
{
    if (!out_code || !out_name || !out_msg)
        return false;

    size_t i = 0;
#define X(sym, val, name, msg) \
    if (i == idx) { *out_code = (steel_diag_code)(val); *out_name = (name); *out_msg = (msg); return true; } \
    i++;

    STEEL_DIAG_CODE_TABLE(X)
#undef X

    return false;
}
