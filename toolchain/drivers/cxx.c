/*
 * toolchain/drivers/cxx.c
 *
 * Vitte toolchain driver: `vitte-cxx`
 *
 * Purpose:
 *  - Stable C++ compiler/linker entrypoint controlled by Vitte.
 *  - Very similar to toolchain/drivers/cc.c but defaults to C++ mode.
 *
 * Notes:
 *  - This is intentionally simple: it delegates heavy lifting to clang_* modules.
 *  - A full implementation would load TOML (default.toml + cpp_std.toml + target preset).
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "toolchain/clang_env.h"
#include "toolchain/clang_paths.h"
#include "toolchain/clang_sdk.h"
#include "toolchain/clang_target.h"
#include "toolchain/clang_compile.h"
#include "toolchain/clang_link.h"
#include "toolchain/clang_errors.h"
#include "toolchain/toolchain_process.h"
#include "toolchain/toolchain_fs.h"

/* -----------------------------------------------------------------------------
 * Minimal CLI parsing (C++ flavor)
 * -------------------------------------------------------------------------- */

typedef struct tc_cxx_cli_s {
    const char* out_path;
    const char* rsp_path;

    const char* target_triple;
    const char* sysroot;

    bool        compile_only; /* -c */
    bool        shared;       /* -shared */
    bool        use_lld;      /* default true */

    /* passthrough */
    const char** inputs;
    size_t       inputs_count;

    const char** extra_args;
    size_t       extra_args_count;

    const char** lib_dirs;
    size_t       lib_dirs_count;

    const char** libs;
    size_t       libs_count;
} tc_cxx_cli;

static void tc_cxx_cli_zero(tc_cxx_cli* c) {
    memset(c, 0, sizeof(*c));
    c->use_lld = true;
}

static bool tc_arg_is(const char* a, const char* lit) {
    return a && lit && strcmp(a, lit) == 0;
}

static bool tc_arg_has_prefix(const char* a, const char* pfx) {
    if (!a || !pfx) return false;
    size_t n = strlen(pfx);
    return strncmp(a, pfx, n) == 0;
}

static tc_toolchain_err tc_cxx_parse_args(int argc, char** argv, tc_cxx_cli* out) {
    if (!out) return TC_TOOLCHAIN_EINVAL;

    static const char* inputs[4096];
    static const char* extra[4096];
    static const char* lib_dirs[1024];
    static const char* libs[1024];

    size_t in_n = 0, ex_n = 0, ld_n = 0, l_n = 0;

    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];

        if (tc_arg_is(a, "-c")) { out->compile_only = true; continue; }
        if (tc_arg_is(a, "-shared")) { out->shared = true; continue; }

        if (tc_arg_is(a, "-o") && i + 1 < argc) { out->out_path = argv[++i]; continue; }

        if (tc_arg_is(a, "--target") && i + 1 < argc) { out->target_triple = argv[++i]; continue; }
        if (tc_arg_has_prefix(a, "--target=")) { out->target_triple = a + strlen("--target="); continue; }

        if (tc_arg_is(a, "--sysroot") && i + 1 < argc) { out->sysroot = argv[++i]; continue; }
        if (tc_arg_has_prefix(a, "--sysroot=")) { out->sysroot = a + strlen("--sysroot="); continue; }

        if (tc_arg_is(a, "--rsp") && i + 1 < argc) { out->rsp_path = argv[++i]; continue; }

        if (tc_arg_is(a, "-fuse-ld=lld")) { out->use_lld = true; extra[ex_n++] = a; continue; }

        if (tc_arg_is(a, "-L") && i + 1 < argc) { lib_dirs[ld_n++] = argv[++i]; continue; }
        if (tc_arg_has_prefix(a, "-L")) { lib_dirs[ld_n++] = a + 2; continue; }

        if (tc_arg_is(a, "-l") && i + 1 < argc) { libs[l_n++] = argv[++i]; continue; }
        if (tc_arg_has_prefix(a, "-l")) { libs[l_n++] = a + 2; continue; }

        if (a[0] != '-' && tc_fs_is_file(a)) {
            inputs[in_n++] = a;
            continue;
        }

        extra[ex_n++] = a;
    }

    out->inputs = inputs;
    out->inputs_count = in_n;
    out->extra_args = extra;
    out->extra_args_count = ex_n;
    out->lib_dirs = lib_dirs;
    out->lib_dirs_count = ld_n;
    out->libs = libs;
    out->libs_count = l_n;

    if (out->inputs_count == 0) return TC_TOOLCHAIN_EINVAL;
    return TC_TOOLCHAIN_OK;
}

/* -----------------------------------------------------------------------------
 * Driver main
 * -------------------------------------------------------------------------- */

static int tc_print_err(tc_toolchain_err e, const char* what) {
    fprintf(stderr, "vitte-cxx: %s: %s\n", what ? what : "error", tc_toolchain_err_str(e));
    return (e == TC_TOOLCHAIN_OK) ? 0 : 1;
}

int main(int argc, char** argv) {
    tc_cxx_cli cli;
    tc_cxx_cli_zero(&cli);

    tc_toolchain_err pe = tc_cxx_parse_args(argc, argv, &cli);
    if (pe != TC_TOOLCHAIN_OK) return tc_print_err(pe, "invalid arguments");

    /* Resolve tool paths */
    tc_clang_paths paths;
    tc_clang_env_opts eopts = {0};
    eopts.probe_via_which = true;

    tc_toolchain_process_result diag = {0};
    tc_toolchain_err ee = tc_clang_env_resolve(&paths, &eopts, &diag);
    if (ee != TC_TOOLCHAIN_OK) {
        (void)tc_print_err(ee, "toolchain resolve");
    }

    /* Target */
    tc_target tgt;
    tc_target_set_host(&tgt);

    if (cli.target_triple && cli.target_triple[0]) {
        if (tc_target_parse_triple(&tgt, cli.target_triple) == TC_TARGET_OK) {
            tc_target_normalize(&tgt);
        }
    }

    /* SDK/sysroot (best-effort) */
    const char* sysroot = cli.sysroot;
    if (!sysroot || !sysroot[0]) {
        tc_clang_sdk sdk;
        tc_clang_sdk_opts sopts = {0};
        tc_clang_sdk_zero(&sdk);

        if (tc_clang_sdk_detect(&tgt, &sopts, &sdk) == TC_TOOLCHAIN_OK) {
            if (sdk.found && sdk.sysroot[0]) sysroot = sdk.sysroot;
        }
    }

    /* Common flags (policy defaults; would be loaded from cpp_std.toml + profiles) */
    tc_clang_common common;
    tc_clang_common_zero(&common);

    tc_clang_common_set_std(&common, "c++20");
    common.opt = TC_OPT_O2;
    common.debug = TC_DBG_G0;
    common.warnings = TC_WARN_DEFAULT;
    common.pic = true;
    common.pie = true;
    common.lto = false;

    if (sysroot && sysroot[0]) {
        tc_clang_common_set_sysroot(&common, sysroot);
    }

    tc_toolchain_process_result res = {0};

    if (cli.compile_only) {
        const char* in = cli.inputs[0];
        const char* out = cli.out_path ? cli.out_path : "a.o";

        tc_clang_compile_job job;
        memset(&job, 0, sizeof(job));
        job.lang = TC_LANG_CXX;
        job.input_path = in;
        job.output_path = out;
        job.out_kind = TC_COMPILE_OUT_OBJ;
        job.target = tgt;
        job.sysroot_override = sysroot;

        job.force_rsp = (cli.rsp_path && cli.rsp_path[0]);
        job.disable_rsp = false;
        job.rsp_path = cli.rsp_path;

        job.extra_args = cli.extra_args;
        job.extra_args_count = cli.extra_args_count;

        tc_toolchain_err ce = tc_clang_compile_run(&paths, &common, &job, &res);
        if (ce != TC_TOOLCHAIN_OK) return tc_print_err(ce, "compile failed");
        return 0;
    }

    tc_clang_link_job ljob;
    memset(&ljob, 0, sizeof(ljob));

    ljob.out_kind = cli.shared ? TC_LINK_OUT_SHARED : TC_LINK_OUT_EXE;
    ljob.output_path = cli.out_path ? cli.out_path : "a.out";
    ljob.target = tgt;
    ljob.sysroot_override = sysroot;

    ljob.force_rsp = (cli.rsp_path && cli.rsp_path[0]);
    ljob.disable_rsp = false;
    ljob.rsp_path = cli.rsp_path;

    ljob.inputs = cli.inputs;
    ljob.inputs_count = cli.inputs_count;

    ljob.lib_dirs = cli.lib_dirs;
    ljob.lib_dirs_count = cli.lib_dirs_count;

    ljob.libs = cli.libs;
    ljob.libs_count = cli.libs_count;

    ljob.extra_args = cli.extra_args;
    ljob.extra_args_count = cli.extra_args_count;

    ljob.use_lld = cli.use_lld;

    tc_toolchain_err le = tc_clang_link_run(&paths, &common, &ljob, &res);
    if (le != TC_TOOLCHAIN_OK) return tc_print_err(le, "link failed");

    return 0;
}
