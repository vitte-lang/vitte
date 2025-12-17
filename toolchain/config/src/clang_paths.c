/*
 * toolchain/config/src/clang_paths.c
 *
 * Vitte toolchain – path helpers for clang tool locations.
 *
 * This module provides small utilities around tc_clang_paths:
 *  - validation (tools exist or are callable via PATH)
 *  - choosing the “best” executable name per platform/target
 *  - joining tool names with a provided LLVM bin dir
 *
 * Resolution/probing policy lives in clang_env.c; this file is purely helpers.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "toolchain/clang_paths.h"
#include "toolchain/clang_paths_utils.h"
#include "toolchain/clang_target.h"
#include "toolchain/clang_errors.h"
#include "toolchain/toolchain_fs.h"

/* -----------------------------------------------------------------------------
 * Internal
 * -------------------------------------------------------------------------- */

static bool tc_is_windows_target(const tc_target* t) {
    return t && t->os == TC_OS_WINDOWS;
}

static void tc_set(char* dst, size_t cap, const char* src) {
    if (!dst || cap == 0) return;
    if (!src) src = "";
    size_t n = strnlen(src, cap);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static bool tc_has(const char* s) {
    return s && s[0] != '\0';
}

/* Try join(bin_dir, name) and set if exists. */
static bool tc_try_join_set(char* dst, size_t cap, const char* bin_dir, const char* name) {
    if (!dst || !bin_dir || !name) return false;

    char cand[TC_TOOL_PATH_MAX];
    cand[0] = '\0';

    if (tc_fs_path_join(cand, sizeof(cand), bin_dir, name) != TC_TOOLCHAIN_OK) return false;
    tc_fs_path_normalize(cand, sizeof(cand));

#if defined(_WIN32)
    /* if no extension, try .exe first */
    if (!strchr(name, '.')) {
        char exe_name[128];
        (void)snprintf(exe_name, sizeof(exe_name), "%s.exe", name);
        char cand2[TC_TOOL_PATH_MAX];
        cand2[0] = '\0';
        if (tc_fs_path_join(cand2, sizeof(cand2), bin_dir, exe_name) == TC_TOOLCHAIN_OK) {
            tc_fs_path_normalize(cand2, sizeof(cand2));
            if (tc_fs_is_file(cand2)) {
                tc_set(dst, cap, cand2);
                return true;
            }
        }
    }
#endif

    if (tc_fs_is_file(cand)) {
        tc_set(dst, cap, cand);
        return true;
    }
    return false;
}

/* -----------------------------------------------------------------------------
 * Public helpers
 * -------------------------------------------------------------------------- */

tc_toolchain_err tc_clang_paths_apply_bin_dir(tc_clang_paths* p, const char* bin_dir) {
    if (!p || !bin_dir || !*bin_dir) return TC_TOOLCHAIN_EINVAL;

    /* Only overwrite entries that look like plain names, or are empty. */
    const bool overwrite = true;

    if (overwrite || !tc_has(p->clang))        (void)tc_try_join_set(p->clang, sizeof(p->clang), bin_dir, "clang");
    if (overwrite || !tc_has(p->clangxx))      (void)tc_try_join_set(p->clangxx, sizeof(p->clangxx), bin_dir, "clang++");

#if defined(_WIN32)
    /* prefer lld-link on Windows if present */
    if (overwrite || !tc_has(p->lld)) {
        if (!tc_try_join_set(p->lld, sizeof(p->lld), bin_dir, "lld-link")) {
            (void)tc_try_join_set(p->lld, sizeof(p->lld), bin_dir, "lld");
        }
    }
#else
    if (overwrite || !tc_has(p->lld)) {
        if (!tc_try_join_set(p->lld, sizeof(p->lld), bin_dir, "ld.lld")) {
            (void)tc_try_join_set(p->lld, sizeof(p->lld), bin_dir, "lld");
        }
    }
#endif

    if (overwrite || !tc_has(p->llvm_ar))      (void)tc_try_join_set(p->llvm_ar, sizeof(p->llvm_ar), bin_dir, "llvm-ar");
    if (overwrite || !tc_has(p->llvm_ranlib))  (void)tc_try_join_set(p->llvm_ranlib, sizeof(p->llvm_ranlib), bin_dir, "llvm-ranlib");

    tc_set(p->llvm_bin_dir, sizeof(p->llvm_bin_dir), bin_dir);

    return TC_TOOLCHAIN_OK;
}

tc_toolchain_err tc_clang_paths_choose_defaults(tc_clang_paths* p, const tc_target* target) {
    if (!p) return TC_TOOLCHAIN_EINVAL;

    const bool win = tc_is_windows_target(target);

    if (!tc_has(p->clang))   tc_set(p->clang, sizeof(p->clang), "clang");
    if (!tc_has(p->clangxx)) tc_set(p->clangxx, sizeof(p->clangxx), "clang++");

    if (!tc_has(p->lld)) {
        if (win) tc_set(p->lld, sizeof(p->lld), "lld-link");
        else     tc_set(p->lld, sizeof(p->lld), "ld.lld");
    }

    if (!tc_has(p->llvm_ar))     tc_set(p->llvm_ar, sizeof(p->llvm_ar), "llvm-ar");
    if (!tc_has(p->llvm_ranlib)) tc_set(p->llvm_ranlib, sizeof(p->llvm_ranlib), "llvm-ranlib");

    return TC_TOOLCHAIN_OK;
}

tc_toolchain_err tc_clang_paths_validate(const tc_clang_paths* p, tc_clang_paths_status* out_status) {
    if (!p || !out_status) return TC_TOOLCHAIN_EINVAL;

    memset(out_status, 0, sizeof(*out_status));

    /* “exists” checks only if path is absolute/relative with separators.
     * If it's just a basename (clang), we accept it as PATH-resolvable.
     */
    out_status->clang_ok = tc_fs_is_exe_or_on_path(p->clang);
    out_status->clangxx_ok = tc_fs_is_exe_or_on_path(p->clangxx);
    out_status->lld_ok = tc_fs_is_exe_or_on_path(p->lld);
    out_status->llvm_ar_ok = tc_fs_is_exe_or_on_path(p->llvm_ar);
    out_status->llvm_ranlib_ok = tc_fs_is_exe_or_on_path(p->llvm_ranlib);

    out_status->all_ok =
        out_status->clang_ok &&
        out_status->clangxx_ok &&
        out_status->lld_ok &&
        out_status->llvm_ar_ok &&
        out_status->llvm_ranlib_ok;

    return TC_TOOLCHAIN_OK;
}
