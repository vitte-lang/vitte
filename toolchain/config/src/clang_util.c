/*
 * toolchain/config/src/clang/util.c
 *
 * Vitte toolchain – clang utilities (paths, quoting, rsp, small helpers).
 *
 * This module is meant to be shared by clang_compile.c / clang_link.c / clang_probe.c.
 * No heap allocations. All helpers are conservative and portable.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "toolchain/clang/util.h"
#include "toolchain/clang_errors.h"
#include "toolchain/toolchain_fs.h"

/* -----------------------------------------------------------------------------
 * String helpers
 * -------------------------------------------------------------------------- */

size_t tc_util_strlcpy(char* dst, size_t cap, const char* src) {
    if (!dst || cap == 0) return 0;
    if (!src) src = "";
    size_t n = 0;

    while (src[n] && (n + 1u) < cap) {
        dst[n] = src[n];
        n++;
    }
    dst[n] = '\0';

    while (src[n]) n++;
    return n;
}

size_t tc_util_strlcat(char* dst, size_t cap, const char* src) {
    if (!dst || cap == 0) return 0;
    size_t dlen = strnlen(dst, cap);
    if (dlen >= cap) return dlen;

    if (!src) src = "";
    size_t n = 0;

    while (src[n] && (dlen + n + 1u) < cap) {
        dst[dlen + n] = src[n];
        n++;
    }
    dst[dlen + n] = '\0';

    while (src[n]) n++;
    return dlen + n;
}

bool tc_util_is_empty(const char* s) {
    return !s || s[0] == '\0';
}

bool tc_util_has_sep(const char* s) {
    if (!s) return false;
    for (const char* p = s; *p; ++p) {
        if (*p == '/' || *p == '\\') return true;
    }
    return false;
}

/* -----------------------------------------------------------------------------
 * Path helpers
 * -------------------------------------------------------------------------- */

tc_toolchain_err tc_util_path_join(char* out, size_t cap,
                                   const char* a, const char* b) {
    return tc_fs_path_join(out, cap, a, b);
}

void tc_util_path_normalize(char* path, size_t cap) {
    tc_fs_path_normalize(path, cap);
}

bool tc_util_is_file(const char* path) {
    return tc_fs_is_file(path);
}

bool tc_util_is_dir(const char* path) {
    return tc_fs_is_dir(path);
}

bool tc_util_is_exe_or_on_path(const char* path_or_name) {
    return tc_fs_is_exe_or_on_path(path_or_name);
}

/* -----------------------------------------------------------------------------
 * Quoting / response files
 * -------------------------------------------------------------------------- */

bool tc_util_rsp_needs_quotes(const char* arg) {
    if (!arg) return false;
    for (const char* p = arg; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '"' || c == '\\') {
            return true;
        }
    }
    return false;
}

/* Escape an argument for clang response files.
 * If no quoting needed => returns raw copy.
 * Else wraps in "..." and escapes: \, ", and \n/\r/\t as sequences.
 */
tc_toolchain_err tc_util_rsp_escape(const char* arg,
                                   char* out, size_t cap) {
    if (!out || cap == 0) return TC_TOOLCHAIN_EINVAL;
    out[0] = '\0';
    if (!arg) arg = "";

    if (!tc_util_rsp_needs_quotes(arg)) {
        size_t n = strnlen(arg, cap);
        if (n >= cap) return TC_TOOLCHAIN_EOVERFLOW;
        memcpy(out, arg, n);
        out[n] = '\0';
        return TC_TOOLCHAIN_OK;
    }

    size_t w = 0;
    if (w + 1 >= cap) return TC_TOOLCHAIN_EOVERFLOW;
    out[w++] = '"';

    for (const char* p = arg; *p; ++p) {
        char c = *p;

        if (c == '"' || c == '\\') {
            if (w + 2 >= cap) return TC_TOOLCHAIN_EOVERFLOW;
            out[w++] = '\\';
            out[w++] = c;
            continue;
        }
        if (c == '\n') {
            if (w + 2 >= cap) return TC_TOOLCHAIN_EOVERFLOW;
            out[w++] = '\\'; out[w++] = 'n';
            continue;
        }
        if (c == '\r') {
            if (w + 2 >= cap) return TC_TOOLCHAIN_EOVERFLOW;
            out[w++] = '\\'; out[w++] = 'r';
            continue;
        }
        if (c == '\t') {
            if (w + 2 >= cap) return TC_TOOLCHAIN_EOVERFLOW;
            out[w++] = '\\'; out[w++] = 't';
            continue;
        }

        if (w + 1 >= cap) return TC_TOOLCHAIN_EOVERFLOW;
        out[w++] = c;
    }

    if (w + 2 > cap) return TC_TOOLCHAIN_EOVERFLOW;
    out[w++] = '"';
    out[w] = '\0';
    return TC_TOOLCHAIN_OK;
}

tc_toolchain_err tc_util_rsp_write_file(const char* rsp_path,
                                        const char* const* argv,
                                        size_t argc,
                                        size_t skip_first) {
    if (!rsp_path || !argv) return TC_TOOLCHAIN_EINVAL;

    tc_fs_file f;
    tc_toolchain_err e = tc_fs_open_write_text(&f, rsp_path);
    if (e != TC_TOOLCHAIN_OK) return e;

    char line[4096];

    for (size_t i = skip_first; i < argc; ++i) {
        const char* a = argv[i];
        if (!a) continue;

        e = tc_util_rsp_escape(a, line, sizeof(line));
        if (e != TC_TOOLCHAIN_OK) { tc_fs_close(&f); return e; }

        e = tc_fs_write_line(&f, line);
        if (e != TC_TOOLCHAIN_OK) { tc_fs_close(&f); return e; }
    }

    tc_fs_close(&f);
    return TC_TOOLCHAIN_OK;
}
