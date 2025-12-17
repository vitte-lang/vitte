/*
 * src/toolchain/clang_error.c
 *
 * Vitte toolchain – error model (codes, messages, mapping from process results).
 *
 * This file pairs with:
 *   config/include/toolchain/clang_errors.h
 *
 * Conventions:
 *  - tc_toolchain_err is the single error domain for the toolchain C layer.
 *  - mapping helpers convert exit codes / process termination into that domain.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "toolchain/clang_errors.h"
#include "toolchain/toolchain_process.h"

/* -----------------------------------------------------------------------------
 * Message table
 * -------------------------------------------------------------------------- */

typedef struct tc_err_msg_s {
    tc_toolchain_err code;
    const char*      msg;
} tc_err_msg;

static const tc_err_msg g_err_msgs[] = {
    { TC_TOOLCHAIN_OK,           "ok" },

    { TC_TOOLCHAIN_EINVAL,       "invalid argument" },
    { TC_TOOLCHAIN_EOVERFLOW,    "capacity exceeded" },
    { TC_TOOLCHAIN_ENOTFOUND,    "not found" },
    { TC_TOOLCHAIN_EIO,          "i/o error" },
    { TC_TOOLCHAIN_EPERM,        "permission denied" },
    { TC_TOOLCHAIN_ESTATE,       "invalid state" },
    { TC_TOOLCHAIN_EPARSE,       "parse error" },

    { TC_TOOLCHAIN_EPROCESS,     "process failed" },
    { TC_TOOLCHAIN_ETIMEDOUT,    "process timed out" },

    { TC_TOOLCHAIN_ECLANG,       "clang failed" },
    { TC_TOOLCHAIN_ELLVM,        "llvm tool failed" },
    { TC_TOOLCHAIN_ELINK,        "linker failed" },
    { TC_TOOLCHAIN_EARCHIVE,     "archiver failed" },

    { TC_TOOLCHAIN_EUNSUPPORTED, "unsupported feature/target" },
};

static const size_t g_err_msgs_count = sizeof(g_err_msgs) / sizeof(g_err_msgs[0]);

/* -----------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

const char* tc_toolchain_err_str(tc_toolchain_err e) {
    for (size_t i = 0; i < g_err_msgs_count; ++i) {
        if (g_err_msgs[i].code == e) return g_err_msgs[i].msg;
    }
    return "unknown error";
}

bool tc_toolchain_err_is_ok(tc_toolchain_err e) {
    return e == TC_TOOLCHAIN_OK;
}

bool tc_toolchain_err_is_user_error(tc_toolchain_err e) {
    switch (e) {
        case TC_TOOLCHAIN_EINVAL:
        case TC_TOOLCHAIN_EOVERFLOW:
        case TC_TOOLCHAIN_EPARSE:
        case TC_TOOLCHAIN_ENOTFOUND:
        case TC_TOOLCHAIN_EUNSUPPORTED:
            return true;
        default:
            return false;
    }
}

bool tc_toolchain_err_is_process_error(tc_toolchain_err e) {
    switch (e) {
        case TC_TOOLCHAIN_EPROCESS:
        case TC_TOOLCHAIN_ETIMEDOUT:
        case TC_TOOLCHAIN_ECLANG:
        case TC_TOOLCHAIN_ELLVM:
        case TC_TOOLCHAIN_ELINK:
        case TC_TOOLCHAIN_EARCHIVE:
            return true;
        default:
            return false;
    }
}

/* -----------------------------------------------------------------------------
 * Mapping
 * -------------------------------------------------------------------------- */

tc_toolchain_err tc_toolchain_err_from_exit_code(int exit_code, tc_tool_kind kind) {
    if (exit_code == 0) return TC_TOOLCHAIN_OK;

    switch (kind) {
        case TC_TOOL_CLANG:        return TC_TOOLCHAIN_ECLANG;
        case TC_TOOL_LLD:          return TC_TOOLCHAIN_ELINK;
        case TC_TOOL_LLVM_AR:      return TC_TOOLCHAIN_EARCHIVE;
        case TC_TOOL_LLVM_RANLIB:  return TC_TOOLCHAIN_EARCHIVE;
        case TC_TOOL_UNKNOWN:
        default:                   return TC_TOOLCHAIN_EPROCESS;
    }
}

tc_toolchain_err tc_toolchain_err_from_process_result(const tc_toolchain_process_result* r,
                                                      tc_tool_kind kind) {
    if (!r) return TC_TOOLCHAIN_EINVAL;

    if (r->timed_out) return TC_TOOLCHAIN_ETIMEDOUT;

    /* If your process layer reports signals/termination, map that to EPROCESS */
    if (r->was_signaled) return TC_TOOLCHAIN_EPROCESS;

    return tc_toolchain_err_from_exit_code(r->exit_code, kind);
}
