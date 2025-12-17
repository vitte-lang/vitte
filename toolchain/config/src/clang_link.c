/*
 * toolchain/config/src/clang_link.c
 *
 * Vitte toolchain – link step via clang (+ optional lld) with response files.
 *
 * Responsibilities:
 *  - Build clang link command line for one link job (exe/shared).
 *  - Prefer response files (.rsp) to avoid command line length limits.
 *  - Delegate spawning to toolchain_process.* and writing rsp to toolchain_fs.*.
 *
 * Depends on:
 *  - toolchain/clang.h              : argv builder + tc_clang_build_link_argv
 *  - toolchain/clang_paths.h        : tool paths
 *  - toolchain/clang_target.h       : target triple helpers
 *  - toolchain/toolchain_process.h  : spawn + result
 *  - toolchain/toolchain_fs.h       : write rsp
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "toolchain/clang_link.h"
#include "toolchain/clang.h"
#include "toolchain/clang_paths.h"
#include "toolchain/clang_target.h"
#include "toolchain/clang_errors.h"
#include "toolchain/toolchain_process.h"
#include "toolchain/toolchain_fs.h"

/* -----------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

static const char* tc_pick_clang_exe(const tc_clang_paths* paths) {
    if (paths && paths->clang[0]) return paths->clang;
    return "clang";
}

static bool tc_should_use_rsp_default(const tc_clang_link_job* job) {
    if (!job) return false;
    if (job->force_rsp) return true;
    if (job->disable_rsp) return false;
#if defined(_WIN32)
    return true;
#else
    return false;
#endif
}

/* Quote/escape for clang response files.
 * Emit raw if safe, otherwise "..." with escaping for " and \ and control chars.
 */
static tc_toolchain_err tc_rsp_write_escaped(tc_fs_file* f, const char* arg) {
    if (!f || !arg) return TC_TOOLCHAIN_EINVAL;

    bool needs_quotes = false;
    for (const char* p = arg; *p; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '"' || c == '\\') {
            needs_quotes = true;
            break;
        }
    }

    if (!needs_quotes) {
        return tc_fs_write_line(f, arg);
    }

    char buf[4096];
    size_t w = 0;

    if (w < sizeof(buf)) buf[w++] = '"';

    for (const char* p = arg; *p; ++p) {
        char c = *p;

        if (c == '"' || c == '\\') {
            if (w + 2 >= sizeof(buf)) return TC_TOOLCHAIN_EOVERFLOW;
            buf[w++] = '\\';
            buf[w++] = c;
            continue;
        }
        if (c == '\n') {
            if (w + 2 >= sizeof(buf)) return TC_TOOLCHAIN_EOVERFLOW;
            buf[w++] = '\\'; buf[w++] = 'n';
            continue;
        }
        if (c == '\r') {
            if (w + 2 >= sizeof(buf)) return TC_TOOLCHAIN_EOVERFLOW;
            buf[w++] = '\\'; buf[w++] = 'r';
            continue;
        }
        if (c == '\t') {
            if (w + 2 >= sizeof(buf)) return TC_TOOLCHAIN_EOVERFLOW;
            buf[w++] = '\\'; buf[w++] = 't';
            continue;
        }

        if (w + 1 >= sizeof(buf)) return TC_TOOLCHAIN_EOVERFLOW;
        buf[w++] = c;
    }

    if (w + 2 > sizeof(buf)) return TC_TOOLCHAIN_EOVERFLOW;
    buf[w++] = '"';
    buf[w] = '\0';

    return tc_fs_write_line(f, buf);
}

static tc_toolchain_err tc_write_rsp_file(const char* rsp_path,
                                          const tc_argv* argv,
                                          size_t skip_first /* skip argv[0] (exe) */) {
    if (!rsp_path || !argv) return TC_TOOLCHAIN_EINVAL;

    tc_fs_file f;
    tc_toolchain_err e = tc_fs_open_write_text(&f, rsp_path);
    if (e != TC_TOOLCHAIN_OK) return e;

    for (size_t i = skip_first; i < argv->argc; ++i) {
        const char* a = argv->argv[i];
        if (!a) continue;

        e = tc_rsp_write_escaped(&f, a);
        if (e != TC_TOOLCHAIN_OK) { tc_fs_close(&f); return e; }
    }

    tc_fs_close(&f);
    return TC_TOOLCHAIN_OK;
}

static tc_toolchain_err tc_spawn_direct(const char* const* argv, size_t argc,
                                        tc_toolchain_process_result* out_res) {
    tc_toolchain_process_result res = {0};
    tc_toolchain_err pe = tc_toolchain_process_spawn(argv, argc, &res);
    if (out_res) *out_res = res;
    if (pe != TC_TOOLCHAIN_OK) return pe;
    if (res.exit_code != 0) return TC_TOOLCHAIN_EPROCESS;
    return TC_TOOLCHAIN_OK;
}

/* -----------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

tc_toolchain_err tc_clang_link_run(const tc_clang_paths* paths,
                                   const tc_clang_common* common,
                                   const tc_clang_link_job* job,
                                   tc_toolchain_process_result* out_res) {
    if (!common || !job) return TC_TOOLCHAIN_EINVAL;
    if (!job->output_path || !job->inputs || job->inputs_count == 0) return TC_TOOLCHAIN_EINVAL;

    tc_clang_paths eff = {0};
    if (paths) eff = *paths;

    /* Ensure clang path used by the shared builder */
    {
        const char* exe = tc_pick_clang_exe(&eff);
        memset(eff.clang, 0, sizeof(eff.clang));
        strncpy(eff.clang, exe, sizeof(eff.clang) - 1u);
    }

    /* Build argv via toolchain/clang.h */
    tc_argv argv;
    tc_argv_zero(&argv);

    tc_clang_link ljob;
    tc_clang_link_zero(&ljob);

    ljob.output_path = job->output_path;
    ljob.target = job->target;
    ljob.sysroot_override = job->sysroot_override;

    ljob.inputs = job->inputs;
    ljob.inputs_count = job->inputs_count;

    ljob.lib_dirs = job->lib_dirs;
    ljob.lib_dirs_count = job->lib_dirs_count;

    ljob.libs = job->libs;
    ljob.libs_count = job->libs_count;

    ljob.extra = job->extra_args;
    ljob.extra_count = job->extra_args_count;

    ljob.use_lld = job->use_lld;

    switch (job->out_kind) {
        case TC_LINK_OUT_EXE:    ljob.out_kind = TC_OUT_EXE; break;
        case TC_LINK_OUT_SHARED: ljob.out_kind = TC_OUT_SHARED; break;
        default:                 ljob.out_kind = TC_OUT_EXE; break;
    }

    tc_clang_err ce = tc_clang_build_link_argv(&eff, common, &ljob, &argv);
    if (ce != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW;

    /* Response file */
    bool use_rsp = tc_should_use_rsp_default(job);
    if (use_rsp) {
        if (!job->rsp_path || !job->rsp_path[0]) return TC_TOOLCHAIN_EINVAL;

        tc_toolchain_err we = tc_write_rsp_file(job->rsp_path, &argv, 1);
        if (we != TC_TOOLCHAIN_OK) return we;

        /* Invoke: clang @file.rsp */
        const char* execv[3];
        size_t execn = 0;

        execv[execn++] = argv.argv[0];

        char atbuf[TC_TOOL_PATH_MAX];
        atbuf[0] = '@';
        size_t n = strnlen(job->rsp_path, sizeof(atbuf) - 2u);
        if (n >= sizeof(atbuf) - 2u) return TC_TOOLCHAIN_EOVERFLOW;
        memcpy(atbuf + 1, job->rsp_path, n);
        atbuf[1 + n] = '\0';

        execv[execn++] = atbuf;
        execv[execn] = NULL;

        return tc_spawn_direct(execv, execn, out_res);
    }

    return tc_spawn_direct(argv.argv, argv.argc, out_res);
}
