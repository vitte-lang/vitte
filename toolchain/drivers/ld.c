/*
 * toolchain/drivers/ld.c
 *
 * Vitte toolchain driver: `vitte-ld`
 *
 * Purpose:
 *  - Stable linker entrypoint controlled by Vitte.
 *  - Use clang driver for linking (recommended) or invoke lld directly if needed.
 *
 * Model:
 *  - Build a tc_clang_link_job and delegate to tc_clang_link_run().
 *  - Supports -shared, -o, -L, -l, --target, --sysroot, --rsp
 *
 * Notes:
 *  - This is intentionally conservative: it does not try to emulate every `ld`
 *    option. It is a “project linker driver”.
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
#include "toolchain/clang_link.h"
#include "toolchain/clang_errors.h"
#include "toolchain/toolchain_process.h"
#include "toolchain/toolchain_fs.h"

/* -----------------------------------------------------------------------------
 * Minimal CLI parsing
 * -------------------------------------------------------------------------- */

typedef struct tc_ld_cli_s {
    const char* out_path;
    const char* rsp_path;

    const char* target_triple;
    const char* sysroot;

    bool        shared;   /* -shared */
    bool        use_lld;  /* default true */

    const char** inputs;
    size_t       inputs_count;

    const char** extra_args;
    size_t       extra_args_count;

    const char** lib_dirs;
    size_t       lib_dirs_count;

    const char** libs;
    size_t       libs_count;
} tc_ld_cli;

static void tc_ld_cli_zero(tc_ld_cli* c) {
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

static tc_toolchain_err tc_ld_parse_args(int argc, char** argv, tc_ld_cli* out) {
    if (!out) return TC_TOOLCHAIN_EINVAL;

    static const char* inputs[8192];
    static const char* extra[8192];
    static const char* lib_dirs[2048];
    static const char* libs[2048];

    size_t in_n = 0, ex_n = 0, ld_n = 0, l_n = 0;

    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];

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

        /* Inputs: accept existing files; also accept "-"? keep pass-through. */
        if (a[0] != '-' && tc_fs_is_file(a)) {
            inputs[in_n++] = a;
            continue;
        }

        /* For a linker driver, many flags start with '-', keep them as extra. */
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
    fprintf(stderr, "vitte-ld: %s: %s\n", what ? what : "error", tc_toolchain_err_str(e));
    return (e == TC_TOOLCHAIN_OK) ? 0 : 1;
}

int main(int argc, char** argv) {
    tc_ld_cli cli;
    tc_ld_cli_zero(&cli);

    tc_toolchain_err pe = tc_ld_parse_args(argc, argv, &cli);
    if (pe != TC_TOOLCHAIN_OK) return tc_print_err(pe, "invalid arguments");

    tc_clang_paths paths;
    tc_clang_env_opts eopts = {0};
    eopts.probe_via_which = true;

    tc_toolchain_process_result diag = {0};
    tc_toolchain_err ee = tc_clang_env_resolve(&paths, &eopts, &diag);
    if (ee != TC_TOOLCHAIN_OK) {
        (void)tc_print_err(ee, "toolchain resolve");
    }

    tc_target tgt;
    tc_target_set_host(&tgt);

    if (cli.target_triple && cli.target_triple[0]) {
        if (tc_target_parse_triple(&tgt, cli.target_triple) == TC_TARGET_OK) {
            tc_target_normalize(&tgt);
        }
    }

    const char* sysroot = cli.sysroot;
    if (!sysroot || !sysroot[0]) {
        tc_clang_sdk sdk;
        tc_clang_sdk_opts sopts = {0};
        tc_clang_sdk_zero(&sdk);

        if (tc_clang_sdk_detect(&tgt, &sopts, &sdk) == TC_TOOLCHAIN_OK) {
            if (sdk.found && sdk.sysroot[0]) sysroot = sdk.sysroot;
        }
    }

    /* Common defaults (policy). */
    tc_clang_common common;
    tc_clang_common_zero(&common);

    tc_clang_common_set_std(&common, "c17"); /* irrelevant for link but keeps struct consistent */
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

    tc_clang_link_job job;
    memset(&job, 0, sizeof(job));

    job.out_kind = cli.shared ? TC_LINK_OUT_SHARED : TC_LINK_OUT_EXE;
    job.output_path = cli.out_path ? cli.out_path : (cli.shared ? "a.so" : "a.out");

    job.target = tgt;
    job.sysroot_override = sysroot;

    job.force_rsp = (cli.rsp_path && cli.rsp_path[0]);
    job.disable_rsp = false;
    job.rsp_path = cli.rsp_path;

    job.inputs = cli.inputs;
    job.inputs_count = cli.inputs_count;

    job.lib_dirs = cli.lib_dirs;
    job.lib_dirs_count = cli.lib_dirs_count;

    job.libs = cli.libs;
    job.libs_count = cli.libs_count;

    job.extra_args = cli.extra_args;
    job.extra_args_count = cli.extra_args_count;

    job.use_lld = cli.use_lld;

    tc_toolchain_err le = tc_clang_link_run(&paths, &common, &job, &res);
    if (le != TC_TOOLCHAIN_OK) return tc_print_err(le, "link failed");

    return 0;
}
