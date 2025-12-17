/*
 * toolchain/config/src/clang_probe.c
 *
 * Vitte toolchain – probe clang toolchain & infer host/target defaults.
 *
 * Responsibilities (max):
 *  - Detect host target triple (via macros + optional clang -dumpmachine).
 *  - Detect clang/llvm tool availability (paths/env/probing) using clang_env + clang_paths_utils.
 *  - Validate a requested target triple (compile-time + runtime constraints).
 *  - Provide a compact “probe report” usable by `vitte doctor` / diagnostics.
 *
 * Notes:
 *  - All probing is best-effort and non-fatal unless explicitly requested.
 *  - No heap allocation; report uses fixed-size buffers.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "toolchain/clang_probe.h"
#include "toolchain/clang_env.h"
#include "toolchain/clang_paths.h"
#include "toolchain/clang_paths_utils.h"
#include "toolchain/clang_target.h"
#include "toolchain/clang_errors.h"
#include "toolchain/toolchain_process.h"
#include "toolchain/toolchain_fs.h"

/* -----------------------------------------------------------------------------
 * Small helpers
 * -------------------------------------------------------------------------- */

static void tc_set(char* dst, size_t cap, const char* src) {
    if (!dst || cap == 0) return;
    if (!src) src = "";
    size_t n = strnlen(src, cap);
    if (n >= cap) n = cap - 1;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static bool tc_has(const char* s) { return s && s[0] != '\0'; }

/* -----------------------------------------------------------------------------
 * Optional: run "<clang> -dumpmachine" to get host triple.
 * If it fails, returns false and leaves out_triple unchanged.
 * -------------------------------------------------------------------------- */

static bool tc_try_clang_dumpmachine(const char* clang_exe,
                                    char out_triple[TC_TARGET_TRIPLE_MAX]) {
    if (!clang_exe || !*clang_exe) return false;

    const char* argv[] = { clang_exe, "-dumpmachine", NULL };
    tc_toolchain_process_result r = {0};

    if (tc_toolchain_process_spawn(argv, 2, &r) != TC_TOOLCHAIN_OK) return false;
    if (r.exit_code != 0) return false;

    /* Expect stdout in r.stdout_buf (toolchain_process.h contract).
     * If your process layer doesn't capture stdout, this will stay empty.
     */
    if (!r.stdout_buf || !r.stdout_len) return false;

    /* Trim whitespace/newlines. */
    size_t n = r.stdout_len;
    while (n && (r.stdout_buf[n - 1] == '\n' || r.stdout_buf[n - 1] == '\r' ||
                 r.stdout_buf[n - 1] == ' '  || r.stdout_buf[n - 1] == '\t')) {
        n--;
    }
    if (n == 0) return false;

    if (n >= TC_TARGET_TRIPLE_MAX) n = TC_TARGET_TRIPLE_MAX - 1u;
    memcpy(out_triple, r.stdout_buf, n);
    out_triple[n] = '\0';
    return true;
}

/* -----------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

tc_toolchain_err tc_clang_probe_run(const tc_clang_probe_opts* opts,
                                    tc_clang_probe_report* out_report) {
    if (!out_report) return TC_TOOLCHAIN_EINVAL;
    memset(out_report, 0, sizeof(*out_report));

    /* 1) Resolve paths */
    tc_clang_env_opts eopts;
    memset(&eopts, 0, sizeof(eopts));
    eopts.probe_via_which = (opts ? opts->probe_via_which : true);

    tc_toolchain_process_result diag = {0};
    tc_toolchain_err e = tc_clang_env_resolve(&out_report->paths, &eopts, &diag);
    if (e != TC_TOOLCHAIN_OK) {
        out_report->env_resolve_err = e;
        /* keep going with defaults */
    }

    /* 2) Determine host target triple (best-effort) */
    tc_target host;
    tc_target_set_host(&host);
    tc_target_normalize(&host);

    char triple[TC_TARGET_TRIPLE_MAX] = {0};
    (void)tc_target_build_triple(&host);
    tc_set(triple, sizeof(triple), host.triple);

    /* optionally refine via clang -dumpmachine */
    if (opts && opts->use_clang_dumpmachine) {
        char t2[TC_TARGET_TRIPLE_MAX] = {0};
        if (tc_try_clang_dumpmachine(out_report->paths.clang, t2)) {
            tc_set(triple, sizeof(triple), t2);
            (void)tc_target_parse_triple(&host, t2);
            tc_target_normalize(&host);
        }
    }

    tc_set(out_report->host_triple, sizeof(out_report->host_triple), triple);
    out_report->host_target = host;

    /* 3) Choose defaults (target-sensitive) and validate tool availability */
    (void)tc_clang_paths_choose_defaults(&out_report->paths, &host);

    tc_clang_paths_status st;
    memset(&st, 0, sizeof(st));
    (void)tc_clang_paths_validate(&out_report->paths, &st);

    out_report->status = st;

    /* 4) Probe optional features */
    out_report->can_compile = st.clang_ok;
    out_report->can_link = st.clang_ok && st.lld_ok;
    out_report->can_archive = st.llvm_ar_ok;

    /* 5) If requested, validate a target triple */
    if (opts && tc_has(opts->requested_triple)) {
        tc_target req;
        tc_target_err te = tc_target_parse_triple(&req, opts->requested_triple);
        out_report->requested_triple_parse_ok = (te == TC_TARGET_OK);
        if (te == TC_TARGET_OK) {
            tc_target_normalize(&req);
            out_report->requested_target = req;
            out_report->requested_supported = (req.arch != TC_ARCH_UNKNOWN && req.os != TC_OS_UNKNOWN);
        } else {
            out_report->requested_supported = false;
        }
    }

    /* 6) Basic sanity notes */
    if (!st.clang_ok) out_report->notes_missing_tools |= TC_PROBE_NOTE_NO_CLANG;
    if (!st.lld_ok) out_report->notes_missing_tools |= TC_PROBE_NOTE_NO_LLD;
    if (!st.llvm_ar_ok) out_report->notes_missing_tools |= TC_PROBE_NOTE_NO_LLVM_AR;
    if (!st.llvm_ranlib_ok) out_report->notes_missing_tools |= TC_PROBE_NOTE_NO_LLVM_RANLIB;

    return TC_TOOLCHAIN_OK;
}
