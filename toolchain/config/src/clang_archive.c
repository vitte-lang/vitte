/*
 * toolchain/src/toolchain/clang_archive.c
 *
 * Vitte toolchain – static library archiver via llvm-ar (preferred) or clang (fallback).
 *
 * Responsibilities:
 *  - Build an archive (.a / .lib-ish for GNU ar format) from object files.
 *  - Optionally run ranlib (or llvm-ranlib) when needed (ELF/Mach-O).
 *
 * Notes:
 *  - On Windows/MSVC, "lib.exe" is typically used for .lib; however llvm-ar can
 *    still create GNU-style archives. For MSVC import libs, lld-link handles
 *    /implib or lib.exe is required; this module focuses on llvm-ar.
 *  - Invocation is separated: this file builds argv and expects the platform
 *    process layer to run it.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "toolchain/clang_archive.h"
#include "toolchain/clang_paths.h"
#include "toolchain/clang_target.h"

/* platform execution (to be provided by your toolchain process layer) */
#include "toolchain/clang_env.h"      /* for environment-derived defaults if any */
#include "toolchain/clang_errors.h"   /* error codes/messages */

/* If you already have a spawn API, wire it here. */
#include "toolchain/toolchain_process.h"

/* -----------------------------------------------------------------------------
 * Internal: argv builder (small, no allocations)
 * -------------------------------------------------------------------------- */

#ifndef TC_AR_MAX_ARGS
#define TC_AR_MAX_ARGS 512u
#endif

typedef struct tc_ar_argv_s {
    const char* argv[TC_AR_MAX_ARGS];
    size_t argc;

    /* simple formatting storage */
    char storage[TC_AR_MAX_ARGS][260];
    size_t storage_used;
} tc_ar_argv;

static void tc_ar_argv_zero(tc_ar_argv* a) {
    if (!a) return;
    a->argc = 0;
    a->storage_used = 0;
}

static int tc_ar_argv_push(tc_ar_argv* a, const char* s) {
    if (!a || !s) return -1;
    if (a->argc >= TC_AR_MAX_ARGS) return -2;
    a->argv[a->argc++] = s;
    return 0;
}

static int tc_ar_argv_push_owned(tc_ar_argv* a, const char* s) {
    if (!a || !s) return -1;
    if (a->argc >= TC_AR_MAX_ARGS) return -2;
    if (a->storage_used >= TC_AR_MAX_ARGS) return -2;

    size_t n = strnlen(s, sizeof(a->storage[0]));
    if (n >= sizeof(a->storage[0])) return -2;
    memcpy(a->storage[a->storage_used], s, n);
    a->storage[a->storage_used][n] = '\0';

    a->argv[a->argc++] = a->storage[a->storage_used++];
    return 0;
}

static int tc_ar_argv_push_concat(tc_ar_argv* a, const char* pfx, const char* val) {
    if (!a || !pfx || !val) return -1;
    if (a->argc >= TC_AR_MAX_ARGS) return -2;
    if (a->storage_used >= TC_AR_MAX_ARGS) return -2;

    char* dst = a->storage[a->storage_used];
    size_t cap = sizeof(a->storage[0]);

    size_t np = strnlen(pfx, cap);
    size_t nv = strnlen(val, cap);
    if (np + nv >= cap) return -2;

    memcpy(dst, pfx, np);
    memcpy(dst + np, val, nv);
    dst[np + nv] = '\0';

    a->argv[a->argc++] = a->storage[a->storage_used++];
    return 0;
}

/* -----------------------------------------------------------------------------
 * Defaults
 * -------------------------------------------------------------------------- */

static bool tc_is_windows_like(const tc_target* t) {
    return t && t->os == TC_OS_WINDOWS;
}

static const char* tc_pick_llvm_ar(const tc_clang_paths* paths) {
    if (paths && paths->llvm_ar[0]) return paths->llvm_ar;
    return "llvm-ar";
}

static const char* tc_pick_llvm_ranlib(const tc_clang_paths* paths) {
    if (paths && paths->llvm_ranlib[0]) return paths->llvm_ranlib;
    return "llvm-ranlib";
}

/* -----------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

tc_toolchain_err tc_clang_archive_build(const tc_clang_paths* paths,
                                        const tc_target* target,
                                        const tc_clang_archive_job* job,
                                        tc_toolchain_process_result* out_res) {
    if (!job || !job->output_path || !job->inputs || job->inputs_count == 0) {
        return TC_TOOLCHAIN_EINVAL;
    }

    tc_ar_argv a;
    tc_ar_argv_zero(&a);

    const char* ar_exe = tc_pick_llvm_ar(paths);

    /* llvm-ar: common flags
     *   rcs  -> replace/insert, create, write index (s)
     *   rcD  -> deterministic (D) often used; llvm-ar accepts 'D' too.
     */
    if (tc_ar_argv_push(&a, ar_exe) != 0) return TC_TOOLCHAIN_EOVERFLOW;

    /* Build the operation string: "rcs" (+ optional "D") */
    {
        const char* op = "rcsD";
        /* If you want non-deterministic timestamps, set job->deterministic=false */
        if (!job->deterministic) op = "rcs";
        if (tc_ar_argv_push(&a, op) != 0) return TC_TOOLCHAIN_EOVERFLOW;
    }

    /* Output archive path */
    if (tc_ar_argv_push(&a, job->output_path) != 0) return TC_TOOLCHAIN_EOVERFLOW;

    /* Inputs */
    for (size_t i = 0; i < job->inputs_count; ++i) {
        const char* in = job->inputs[i];
        if (!in || !*in) return TC_TOOLCHAIN_EINVAL;
        if (tc_ar_argv_push(&a, in) != 0) return TC_TOOLCHAIN_EOVERFLOW;
    }

    /* Execute */
    tc_toolchain_process_result res = {0};
    tc_toolchain_err pe = tc_toolchain_process_spawn(a.argv, a.argc, &res);
    if (out_res) *out_res = res;
    if (pe != TC_TOOLCHAIN_OK) return pe;

    if (res.exit_code != 0) {
        return TC_TOOLCHAIN_EPROCESS;
    }

    /* ranlib step: generally unnecessary when using "s" index, but some flows keep it.
     * Skip on Windows by default unless requested.
     */
    if (job->run_ranlib) {
        if (!tc_is_windows_like(target)) {
            tc_ar_argv_zero(&a);
            const char* ranlib_exe = tc_pick_llvm_ranlib(paths);

            if (tc_ar_argv_push(&a, ranlib_exe) != 0) return TC_TOOLCHAIN_EOVERFLOW;
            if (tc_ar_argv_push(&a, job->output_path) != 0) return TC_TOOLCHAIN_EOVERFLOW;

            tc_toolchain_process_result res2 = {0};
            pe = tc_toolchain_process_spawn(a.argv, a.argc, &res2);
            if (out_res) *out_res = res2;
            if (pe != TC_TOOLCHAIN_OK) return pe;

            if (res2.exit_code != 0) {
                return TC_TOOLCHAIN_EPROCESS;
            }
        }
    }

    (void)paths;
    (void)target;
    return TC_TOOLCHAIN_OK;
}
