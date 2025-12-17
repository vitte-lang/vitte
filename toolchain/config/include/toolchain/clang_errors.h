// toolchain/config/include/toolchain/clang_errors.h
// C17 - Error model for Vitte toolchain (clang/lld driver layer).

#ifndef VITTE_TOOLCHAIN_CLANG_ERRORS_H
#define VITTE_TOOLCHAIN_CLANG_ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Status codes (stable ABI)
//------------------------------------------------------------------------------
//
// Notes:
// - Keep values stable: they may be serialized in logs/telemetry.
// - 0 means success.
// - Negative values are allowed, but we keep everything positive for simpler JSON/export.
//
// Ranges:
//   0                  OK
//   1..99              generic / validation
//   100..199           filesystem / process
//   200..299           argument / response file
//   300..399           clang invocation / compile
//   400..499           lld invocation / link
//   500..599           archive tools (ar/ranlib/strip)
//   1000+              internal / unexpected
//
typedef enum vitte_tc_errc_e {
    VITTE_TC_ERRC_OK = 0,

    // Generic / validation (1..99)
    VITTE_TC_ERRC_INVALID_ARG = 1,
    VITTE_TC_ERRC_NULL_POINTER = 2,
    VITTE_TC_ERRC_BUFFER_TOO_SMALL = 3,
    VITTE_TC_ERRC_OVERFLOW = 4,
    VITTE_TC_ERRC_OUT_OF_MEMORY = 5,
    VITTE_TC_ERRC_UNSUPPORTED = 6,

    // Filesystem / process (100..199)
    VITTE_TC_ERRC_IO = 100,
    VITTE_TC_ERRC_NOT_FOUND = 101,
    VITTE_TC_ERRC_ACCESS = 102,
    VITTE_TC_ERRC_BUSY = 103,
    VITTE_TC_ERRC_BAD_PATH = 104,
    VITTE_TC_ERRC_PROCESS_SPAWN = 120,
    VITTE_TC_ERRC_PROCESS_WAIT = 121,
    VITTE_TC_ERRC_PROCESS_SIGNALED = 122,
    VITTE_TC_ERRC_PROCESS_EXIT_NONZERO = 123,

    // Args / response files (200..299)
    VITTE_TC_ERRC_RSP_OPEN = 200,
    VITTE_TC_ERRC_RSP_WRITE = 201,
    VITTE_TC_ERRC_RSP_TOO_LARGE = 202,
    VITTE_TC_ERRC_RSP_INVALID_TEMPLATE = 203,

    // Clang compile (300..399)
    VITTE_TC_ERRC_CLANG_NOT_FOUND = 300,
    VITTE_TC_ERRC_CLANG_FAILED = 301,
    VITTE_TC_ERRC_CLANG_DIAGNOSTICS = 302,

    // LLD link (400..499)
    VITTE_TC_ERRC_LLD_NOT_FOUND = 400,
    VITTE_TC_ERRC_LLD_FAILED = 401,
    VITTE_TC_ERRC_LLD_DIAGNOSTICS = 402,

    // Archive tools (500..599)
    VITTE_TC_ERRC_AR_NOT_FOUND = 500,
    VITTE_TC_ERRC_AR_FAILED = 501,
    VITTE_TC_ERRC_RANLIB_NOT_FOUND = 510,
    VITTE_TC_ERRC_RANLIB_FAILED = 511,
    VITTE_TC_ERRC_STRIP_NOT_FOUND = 520,
    VITTE_TC_ERRC_STRIP_FAILED = 521,

    // Internal (1000+)
    VITTE_TC_ERRC_INTERNAL = 1000
} vitte_tc_errc;

//------------------------------------------------------------------------------
// Error object
//------------------------------------------------------------------------------

typedef struct vitte_tc_error_s {
    vitte_tc_errc code;

    // Optional OS error (errno on POSIX, GetLastError() on Windows).
    uint32_t os_code;

    // Optional process exit code when VITTE_TC_ERRC_PROCESS_EXIT_NONZERO.
    int32_t  exit_code;

    // Optional "where" (static string): module/function or subsystem.
    const char* where;

    // Optional "what" (static or owned elsewhere): short human-readable message.
    const char* what;
} vitte_tc_error;

// A canonical OK error object.
static inline vitte_tc_error vitte_tc_error_ok(void) {
    vitte_tc_error e;
    e.code = VITTE_TC_ERRC_OK;
    e.os_code = 0;
    e.exit_code = 0;
    e.where = NULL;
    e.what = NULL;
    return e;
}

static inline int vitte_tc_error_is_ok(const vitte_tc_error* e) {
    return (!e) ? 0 : (e->code == VITTE_TC_ERRC_OK);
}

//------------------------------------------------------------------------------
// Introspection helpers
//------------------------------------------------------------------------------

const char* vitte_tc_errc_name(vitte_tc_errc code);
const char* vitte_tc_errc_category(vitte_tc_errc code);

// Format error into user buffer (NUL-terminated).
// Output is intended for logs and CLI diagnostics.
// Returns number of bytes written (excluding NUL) or 0 on invalid args.
size_t vitte_tc_error_format(const vitte_tc_error* err, char* out, size_t out_cap);

//------------------------------------------------------------------------------
// Construction helpers
//------------------------------------------------------------------------------

static inline vitte_tc_error vitte_tc_error_make(vitte_tc_errc code, uint32_t os_code,
                                                 int32_t exit_code, const char* where,
                                                 const char* what) {
    vitte_tc_error e;
    e.code = code;
    e.os_code = os_code;
    e.exit_code = exit_code;
    e.where = where;
    e.what = what;
    return e;
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_TOOLCHAIN_CLANG_ERRORS_H
