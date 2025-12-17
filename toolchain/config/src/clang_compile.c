/*
 * src/toolchain/clang_compile.c
 *
 * Vitte toolchain – compile step via clang (C/C++/ASM) with robust argv building
 * and response files (.rsp) to avoid command line length limits.
 *
 * Features (max):
 *  - Compile to: .o/.obj, .s, LLVM IR (.ll), LLVM BC (.bc)
 *  - Response file emission with proper quoting/escaping
 *  - Dependency file generation: -MMD/-MD, -MF, -MT, -MP
 *  - Deterministic builds: -ffile-prefix-map / -fmacro-prefix-map (optional)
 *  - Diagnostics knobs: -fcolor-diagnostics, -fno-caret-diagnostics, etc.
 *  - C/C++ runtime include injection via tc_clang_common (already supported by clang.h)
 *  - Extra raw args passthrough
 *
 * This unit only spawns processes via toolchain_process.* and writes rsp via toolchain_fs.*.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "toolchain/clang_compile.h"
#include "toolchain/clang.h"
#include "toolchain/clang_paths.h"
#include "toolchain/clang_target.h"
#include "toolchain/clang_errors.h"
#include "toolchain/toolchain_process.h"
#include "toolchain/toolchain_fs.h"

/* -----------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

static const char* tc_pick_clang_exe(const tc_clang_paths* paths, tc_lang lang) {
    if (!paths) return (lang == TC_LANG_CXX) ? "clang++" : "clang";
    if (lang == TC_LANG_CXX) {
        if (paths->clangxx[0]) return paths->clangxx;
        if (paths->clang[0])   return paths->clang; /* acceptable if wrapper */
        return "clang++";
    }
    if (paths->clang[0]) return paths->clang;
    return "clang";
}

/* Conservative: detect whether we should prefer rsp by default. */
static bool tc_should_use_rsp_default(const tc_clang_compile_job* job) {
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
 * clang rsp supports typical shell-like quoting. We emit:
 *  - if no special chars => raw
 *  - else "..." with backslash escaping for " and \ and newlines/tabs
 */
static tc_toolchain_err tc_rsp_write_escaped(tc_fs_file* f, const char* arg) {
    if (!f || !arg) return TC_TOOLCHAIN_EINVAL;

    bool needs_quotes = false;
    for (const char* p = arg; *p; ++p) {
        const unsigned char c = (unsigned char)*p;
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

        /* escape */
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
 * Max compile builder
 * -------------------------------------------------------------------------- */

static tc_toolchain_err tc_build_compile_argv_max(const tc_clang_paths* paths,
                                                  const tc_clang_common* common,
                                                  const tc_clang_compile_job* job,
                                                  tc_argv* out_argv) {
    if (!paths || !common || !job || !out_argv) return TC_TOOLCHAIN_EINVAL;
    if (!job->input_path || !job->output_path) return TC_TOOLCHAIN_EINVAL;

    tc_argv_zero(out_argv);

    /* Choose clang executable (and patch tc_clang_paths view so clang.h uses it) */
    tc_clang_paths eff = *paths;
    const char* exe = tc_pick_clang_exe(paths, job->lang);
    if (exe && *exe) {
        memset(eff.clang, 0, sizeof(eff.clang));
        strncpy(eff.clang, exe, sizeof(eff.clang) - 1u);

        if (job->lang == TC_LANG_CXX) {
            memset(eff.clangxx, 0, sizeof(eff.clangxx));
            strncpy(eff.clangxx, exe, sizeof(eff.clangxx) - 1u);
        }
    }

    /* Use shared builder for baseline flags */
    tc_clang_compile cjob;
    tc_clang_compile_zero(&cjob);
    cjob.lang = job->lang;
    cjob.input_path = job->input_path;
    cjob.output_path = job->output_path;
    cjob.target = job->target;
    cjob.sysroot_override = job->sysroot_override;

    /* Output kind mapping */
    switch (job->out_kind) {
        case TC_COMPILE_OUT_OBJ: cjob.out_kind = TC_OUT_OBJ; break;
        case TC_COMPILE_OUT_ASM: cjob.out_kind = TC_OUT_ASM; break;
        case TC_COMPILE_OUT_IR:  cjob.out_kind = TC_OUT_IR;  break;
        case TC_COMPILE_OUT_BC:  cjob.out_kind = TC_OUT_BC;  break;
        default: cjob.out_kind = TC_OUT_OBJ; break;
    }

    tc_clang_err ce = tc_clang_build_compile_argv(&eff, common, &cjob, out_argv);
    if (ce != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW;

    /* ---- Max extras appended after baseline ---- */

    /* Diagnostics controls */
    if (job->diag_color) {
        (void)tc_argv_push(out_argv, "-fcolor-diagnostics");
    } else {
        (void)tc_argv_push(out_argv, "-fno-color-diagnostics");
    }
    if (job->diag_no_caret) {
        (void)tc_argv_push(out_argv, "-fno-caret-diagnostics");
    }
    if (job->diag_no_column) {
        (void)tc_argv_push(out_argv, "-fno-show-column");
    }
    if (job->diag_time_trace) {
        (void)tc_argv_push(out_argv, "-ftime-trace");
    }

    /* Determinism / path remapping (optional) */
    if (job->file_prefix_map_from && job->file_prefix_map_to) {
        (void)tc_argv_push(out_argv, "-ffile-prefix-map");
        (void)tc_argv_push_concat(out_argv, job->file_prefix_map_from, job->file_prefix_map_to); /* fallback if caller already included '=' */
        /* Better: if caller gives "from=to", pass as one arg. If you want that, pass it via extra_args. */
    }
    if (job->macro_prefix_map_from && job->macro_prefix_map_to) {
        (void)tc_argv_push(out_argv, "-fmacro-prefix-map");
        (void)tc_argv_push_concat(out_argv, job->macro_prefix_map_from, job->macro_prefix_map_to);
    }

    /* Dependency generation */
    if (job->dep.enable) {
        /* -MMD excludes system headers; -MD includes them */
        (void)tc_argv_push(out_argv, job->dep.system_headers ? "-MD" : "-MMD");
        if (job->dep.add_phony_targets) {
            (void)tc_argv_push(out_argv, "-MP");
        }
        if (job->dep.depfile_path && job->dep.depfile_path[0]) {
            (void)tc_argv_push(out_argv, "-MF");
            (void)tc_argv_push(out_argv, job->dep.depfile_path);
        }
        if (job->dep.target_path && job->dep.target_path[0]) {
            (void)tc_argv_push(out_argv, "-MT");
            (void)tc_argv_push(out_argv, job->dep.target_path);
        }
    }

    /* Debug prefix map alternative knobs (optional) */
    if (job->grecord_command_line) {
        (void)tc_argv_push(out_argv, "-grecord-command-line");
    }

    /* Extra raw args passthrough */
    for (size_t i = 0; i < job->extra_args_count; ++i) {
        const char* x = job->extra_args[i];
        if (x && *x) (void)tc_argv_push(out_argv, x);
    }

    return TC_TOOLCHAIN_OK;
}

/* -----------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

tc_toolchain_err tc_clang_compile_run(const tc_clang_paths* paths,
                                      const tc_clang_common* common,
                                      const tc_clang_compile_job* job,
                                      tc_toolchain_process_result* out_res) {
    if (!paths || !common || !job) return TC_TOOLCHAIN_EINVAL;
    if (!job->input_path || !job->output_path) return TC_TOOLCHAIN_EINVAL;

    tc_argv argv;
    tc_toolchain_err e = tc_build_compile_argv_max(paths, common, job, &argv);
    if (e != TC_TOOLCHAIN_OK) return e;

    /* Response file policy */
    bool use_rsp = tc_should_use_rsp_default(job);
    if (use_rsp) {
        if (!job->rsp_path || !job->rsp_path[0]) {
            /* caller asked for rsp but did not provide a path */
            return TC_TOOLCHAIN_EINVAL;
        }

        e = tc_write_rsp_file(job->rsp_path, &argv, 1);
        if (e != TC_TOOLCHAIN_OK) return e;

        /* Invoke: clang @file.rsp */
        const char* execv[3];
        size_t execn = 0;

        execv[execn++] = argv.argv[0];

        /* build "@<rsp_path>" into stable storage */
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
