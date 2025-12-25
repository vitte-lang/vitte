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
#include <stdlib.h>

#include "toolchain/clang_env.h"
#include "toolchain/clang_paths.h"
#include "toolchain/clang_sdk.h"
#include "toolchain/clang_target.h"
#include "toolchain/clang.h"
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
    bool        verbose;  /* --verbose / -v */
    bool        dry_run;  /* --dry-run */
    bool        print_config; /* --print-config */
    bool        print_flags;  /* --print-flags */

    const char** inputs;
    size_t       inputs_count;

    const char** extra_args;
    size_t       extra_args_count;

    const char** lib_dirs;
    size_t       lib_dirs_count;

    const char** libs;
    size_t       libs_count;

    const char** _inputs_buf;
    const char** _extra_buf;
    const char** _lib_dirs_buf;
    const char** _libs_buf;
    char**       _owned_strs;
    size_t       _owned_strs_count;
} tc_ld_cli;

static void tc_ld_cli_zero(tc_ld_cli* c) {
    memset(c, 0, sizeof(*c));
    c->use_lld = true;
}

static void tc_ld_cli_free(tc_ld_cli* c) {
    if (!c) return;
    free((void*)c->_inputs_buf);
    free((void*)c->_extra_buf);
    free((void*)c->_lib_dirs_buf);
    free((void*)c->_libs_buf);
    if (c->_owned_strs) {
        for (size_t i = 0; i < c->_owned_strs_count; ++i) free(c->_owned_strs[i]);
        free(c->_owned_strs);
    }
    tc_ld_cli_zero(c);
}

static bool tc_arg_is(const char* a, const char* lit) {
    return a && lit && strcmp(a, lit) == 0;
}

static bool tc_arg_has_prefix(const char* a, const char* pfx) {
    if (!a || !pfx) return false;
    size_t n = strlen(pfx);
    return strncmp(a, pfx, n) == 0;
}

static bool tc_arg_has_suffix(const char* a, const char* sfx) {
    if (!a || !sfx) return false;
    size_t na = strlen(a);
    size_t ns = strlen(sfx);
    if (ns > na) return false;
    return strcmp(a + (na - ns), sfx) == 0;
}

static char* tc_dup_with_at_prefix(const char* path) {
    if (!path) return NULL;
    size_t n = strlen(path);
    char* s = (char*)malloc(n + 2);
    if (!s) return NULL;
    s[0] = '@';
    memcpy(s + 1, path, n);
    s[1 + n] = '\0';
    return s;
}

static bool tc_push_owned_str(tc_ld_cli* out, char* s) {
    if (!out || !s) return false;
    size_t new_count = out->_owned_strs_count + 1;
    char** p = (char**)realloc(out->_owned_strs, new_count * sizeof(char*));
    if (!p) return false;
    out->_owned_strs = p;
    out->_owned_strs[out->_owned_strs_count++] = s;
    return true;
}

static tc_toolchain_err tc_ld_parse_args(int argc, char** argv, tc_ld_cli* out) {
    if (!out) return TC_TOOLCHAIN_EINVAL;

    enum { TC_LD_MAX_LIST = 8192, TC_LD_MAX_LIBS = 2048 };

    size_t cap_list = (argc > 0 && (size_t)argc < (size_t)TC_LD_MAX_LIST) ? (size_t)argc : (size_t)TC_LD_MAX_LIST;
    size_t cap_libs = (argc > 0 && (size_t)argc < (size_t)TC_LD_MAX_LIBS) ? (size_t)argc : (size_t)TC_LD_MAX_LIBS;

    const char** inputs = (const char**)calloc(cap_list ? cap_list : 1u, sizeof(const char*));
    const char** extra = (const char**)calloc(cap_list ? cap_list : 1u, sizeof(const char*));
    const char** lib_dirs = (const char**)calloc(cap_libs ? cap_libs : 1u, sizeof(const char*));
    const char** libs = (const char**)calloc(cap_libs ? cap_libs : 1u, sizeof(const char*));
    if (!inputs || !extra || !lib_dirs || !libs) {
        free((void*)inputs); free((void*)extra); free((void*)lib_dirs); free((void*)libs);
        return TC_TOOLCHAIN_ESTATE;
    }

    out->_inputs_buf = inputs;
    out->_extra_buf = extra;
    out->_lib_dirs_buf = lib_dirs;
    out->_libs_buf = libs;

    size_t in_n = 0, ex_n = 0, ld_n = 0, l_n = 0;

    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];

        if (tc_arg_is(a, "-shared")) { out->shared = true; continue; }
        if (tc_arg_is(a, "--dry-run")) { out->dry_run = true; continue; }
        if (tc_arg_is(a, "--print-config")) { out->print_config = true; continue; }
        if (tc_arg_is(a, "--print-flags")) { out->print_flags = true; continue; }
        if (tc_arg_is(a, "--verbose") || tc_arg_is(a, "-v")) { out->verbose = true; continue; }
        if (tc_arg_is(a, "-o") && i + 1 < argc) { out->out_path = argv[++i]; continue; }

        if (tc_arg_is(a, "--target") && i + 1 < argc) { out->target_triple = argv[++i]; continue; }
        if (tc_arg_has_prefix(a, "--target=")) { out->target_triple = a + strlen("--target="); continue; }

        if (tc_arg_is(a, "--sysroot") && i + 1 < argc) { out->sysroot = argv[++i]; continue; }
        if (tc_arg_has_prefix(a, "--sysroot=")) { out->sysroot = a + strlen("--sysroot="); continue; }

        if (tc_arg_is(a, "--rsp") && i + 1 < argc) { out->rsp_path = argv[++i]; continue; }

        if (tc_arg_is(a, "-fuse-ld=lld")) {
            out->use_lld = true;
            if (ex_n >= cap_list) return TC_TOOLCHAIN_EOVERFLOW;
            extra[ex_n++] = a;
            continue;
        }

        if (tc_arg_is(a, "-L") && i + 1 < argc) {
            if (ld_n >= cap_libs) return TC_TOOLCHAIN_EOVERFLOW;
            lib_dirs[ld_n++] = argv[++i];
            continue;
        }
        if (tc_arg_has_prefix(a, "-L")) {
            if (ld_n >= cap_libs) return TC_TOOLCHAIN_EOVERFLOW;
            lib_dirs[ld_n++] = a + 2;
            continue;
        }

        if (tc_arg_is(a, "-l") && i + 1 < argc) {
            if (l_n >= cap_libs) return TC_TOOLCHAIN_EOVERFLOW;
            libs[l_n++] = argv[++i];
            continue;
        }
        if (tc_arg_has_prefix(a, "-l")) {
            if (l_n >= cap_libs) return TC_TOOLCHAIN_EOVERFLOW;
            libs[l_n++] = a + 2;
            continue;
        }

        if (a[0] != '-' && tc_arg_has_suffix(a, ".rsp") && tc_fs_is_file(a)) {
            if (ex_n >= cap_list) return TC_TOOLCHAIN_EOVERFLOW;
            char* at = tc_dup_with_at_prefix(a);
            if (!at || !tc_push_owned_str(out, at)) { free(at); return TC_TOOLCHAIN_ESTATE; }
            extra[ex_n++] = at;
            continue;
        }

        /* Inputs: accept existing files; also accept "-"? keep pass-through. */
        if (a[0] != '-' && tc_fs_is_file(a)) {
            if (in_n >= cap_list) return TC_TOOLCHAIN_EOVERFLOW;
            inputs[in_n++] = a;
            continue;
        }

        /* For a linker driver, many flags start with '-', keep them as extra. */
        if (ex_n >= cap_list) return TC_TOOLCHAIN_EOVERFLOW;
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

    if (!out->print_config && out->inputs_count == 0) return TC_TOOLCHAIN_EINVAL;
    return TC_TOOLCHAIN_OK;
}

/* -----------------------------------------------------------------------------
 * Driver main
 * -------------------------------------------------------------------------- */

static int tc_print_err(tc_toolchain_err e, const char* what) {
    fprintf(stderr, "vitte-ld: %s: %s\n", what ? what : "error", tc_toolchain_err_str(e));
    return (e == TC_TOOLCHAIN_OK) ? 0 : 1;
}

static void tc_driver_log_verbose(const tc_ld_cli* cli,
                                  const tc_clang_paths* paths,
                                  const tc_target* tgt,
                                  const char* sysroot) {
    if (!cli || !cli->verbose) return;
    fprintf(stderr, "vitte-ld: tool=ld\n");
    if (tgt) fprintf(stderr, "vitte-ld: target=%s\n", tgt->triple[0] ? tgt->triple : "(host)");
    fprintf(stderr, "vitte-ld: sysroot=%s\n", (sysroot && sysroot[0]) ? sysroot : "(none)");
    if (paths) {
        fprintf(stderr, "vitte-ld: clang=%s\n", paths->clang[0] ? paths->clang : "clang");
        fprintf(stderr, "vitte-ld: lld=%s\n", paths->lld[0] ? paths->lld : "lld");
    }
}

static void tc_print_argv(const char* tag, const char* const* argv, size_t argc) {
    fprintf(stdout, "%s:", tag ? tag : "argv");
    for (size_t i = 0; i < argc; ++i) {
        const char* a = argv[i] ? argv[i] : "";
        fprintf(stdout, " %s", a);
    }
    fprintf(stdout, "\n");
}

int main(int argc, char** argv) {
    tc_ld_cli cli;
    tc_ld_cli_zero(&cli);

    tc_toolchain_err pe = tc_ld_parse_args(argc, argv, &cli);
    if (pe != TC_TOOLCHAIN_OK) { tc_ld_cli_free(&cli); return tc_print_err(pe, "invalid arguments"); }

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

    tc_driver_log_verbose(&cli, &paths, &tgt, sysroot);

    if (cli.print_config) {
        fprintf(stdout, "tool=vitte-ld\n");
        fprintf(stdout, "target=%s\n", tgt.triple[0] ? tgt.triple : "(host)");
        fprintf(stdout, "sysroot=%s\n", (sysroot && sysroot[0]) ? sysroot : "(none)");
        fprintf(stdout, "clang=%s\n", paths.clang[0] ? paths.clang : "clang");
        fprintf(stdout, "lld=%s\n", paths.lld[0] ? paths.lld : "lld");
        tc_ld_cli_free(&cli);
        return 0;
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

    if (cli.print_flags || cli.dry_run) {
        tc_argv a;
        tc_argv_zero(&a);
        tc_clang_paths eff = paths;
        const char* exe = eff.clang[0] ? eff.clang : "clang";
        memset(eff.clang, 0, sizeof(eff.clang));
        strncpy(eff.clang, exe, sizeof(eff.clang) - 1u);

        tc_clang_link l;
        tc_clang_link_zero(&l);
        l.out_kind = cli.shared ? TC_OUT_SHARED : TC_OUT_EXE;
        l.output_path = job.output_path;
        l.target = job.target;
        l.sysroot_override = job.sysroot_override;
        l.inputs = job.inputs;
        l.inputs_count = job.inputs_count;
        l.lib_dirs = job.lib_dirs;
        l.lib_dirs_count = job.lib_dirs_count;
        l.libs = job.libs;
        l.libs_count = job.libs_count;
        l.extra = job.extra_args;
        l.extra_count = job.extra_args_count;
        l.use_lld = job.use_lld;

        if (tc_clang_build_link_argv(&eff, &common, &l, &a) != TC_CLANG_OK) {
            tc_ld_cli_free(&cli);
            return tc_print_err(TC_TOOLCHAIN_EOVERFLOW, "argv overflow");
        }

        if (cli.print_flags) {
            tc_print_argv("link", a.argv, a.argc);
            tc_ld_cli_free(&cli);
            return 0;
        }
        if (cli.dry_run) {
            tc_print_argv("dry-run link", a.argv, a.argc);
            tc_ld_cli_free(&cli);
            return 0;
        }
    }

    tc_toolchain_err le = tc_clang_link_run(&paths, &common, &job, &res);
    if (le != TC_TOOLCHAIN_OK) { tc_ld_cli_free(&cli); return tc_print_err(le, "link failed"); }

    tc_ld_cli_free(&cli);
    return 0;
}
