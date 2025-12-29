/*
 * toolchain/drivers/cc.c
 *
 * Vitte toolchain driver: `vitte-cc`
 *
 * Purpose:
 *  - Provide a stable, Vitte-controlled C/C++ compiler entrypoint.
 *  - Translate a higher-level config (default.toml + target preset + profiles)
 *    into clang compile/link invocations.
 *
 * Behavior:
 *  - If "-c" present => compile only (clang_compile_run)
 *  - Else => link (clang_link_run)
 *
 * Notes:
 *  - This is a "max" implementation skeleton that still stays dependency-light.
 *  - It does not implement a full TOML parser here; it assumes toolchain/config
 *    loader exists and returns a structured tc_cc_config.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "toolchain/clang_env.h"
#include "toolchain/clang_probe.h"
#include "toolchain/clang_paths.h"
#include "toolchain/clang_paths_utils.h"
#include "toolchain/clang_sdk.h"
#include "toolchain/clang_target.h"
#include "toolchain/clang.h"
#include "toolchain/clang_compile.h"
#include "toolchain/clang_link.h"
#include "toolchain/clang_errors.h"
#include "toolchain/toolchain_process.h"
#include "toolchain/toolchain_fs.h"

/* -----------------------------------------------------------------------------
 * Minimal CLI parsing
 * -------------------------------------------------------------------------- */

typedef struct tc_cc_cli_s {
    const char* out_path;
    const char* rsp_path;

    const char* target_triple;
    const char* sysroot;

    bool        compile_only;  /* -c */
    bool        shared;        /* -shared */
    bool        use_lld;        /* default true */
    bool        verbose;       /* --verbose / -v */
    bool        dry_run;       /* --dry-run */
    bool        print_config;  /* --print-config */
    bool        print_flags;   /* --print-flags */

    tc_lang     lang;          /* forced, else inferred by extension */

    /* passthrough */
    const char** inputs;
    size_t       inputs_count;

    const char** extra_args;
    size_t       extra_args_count;

    const char** lib_dirs;
    size_t       lib_dirs_count;

    const char** libs;
    size_t       libs_count;

    /* owned allocations (arg lists, synthesized args) */
    const char** _inputs_buf;
    const char** _extra_buf;
    const char** _lib_dirs_buf;
    const char** _libs_buf;
    char**       _owned_strs;
    size_t       _owned_strs_count;
} tc_cc_cli;

static void tc_cc_cli_zero(tc_cc_cli* c) {
    memset(c, 0, sizeof(*c));
    c->use_lld = true;
    c->lang = TC_LANG_C;
}

static void tc_cc_cli_free(tc_cc_cli* c) {
    if (!c) return;
    free((void*)c->_inputs_buf);
    free((void*)c->_extra_buf);
    free((void*)c->_lib_dirs_buf);
    free((void*)c->_libs_buf);
    if (c->_owned_strs) {
        for (size_t i = 0; i < c->_owned_strs_count; ++i) free(c->_owned_strs[i]);
        free(c->_owned_strs);
    }
    tc_cc_cli_zero(c);
}

static bool tc_arg_is(const char* a, const char* lit) {
    return a && lit && strcmp(a, lit) == 0;
}

static bool tc_arg_has_prefix(const char* a, const char* pfx) {
    if (!a || !pfx) return false;
    size_t n = strlen(pfx);
    return strncmp(a, pfx, n) == 0;
}

static tc_lang tc_infer_lang_from_path(const char* path) {
    if (!path) return TC_LANG_C;
    const char* dot = strrchr(path, '.');
    if (!dot) return TC_LANG_C;

    if (strcmp(dot, ".c") == 0) return TC_LANG_C;
    if (strcmp(dot, ".cc") == 0 || strcmp(dot, ".cpp") == 0 || strcmp(dot, ".cxx") == 0) return TC_LANG_CXX;
    if (strcmp(dot, ".m") == 0) return TC_LANG_C;   /* ObjC treated as C for now */
    if (strcmp(dot, ".mm") == 0) return TC_LANG_CXX; /* ObjC++ */
    if (strcmp(dot, ".s") == 0 || strcmp(dot, ".S") == 0) return TC_LANG_ASM;

    return TC_LANG_C;
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

static bool tc_push_owned_str(tc_cc_cli* out, char* s) {
    if (!out || !s) return false;
    size_t new_count = out->_owned_strs_count + 1;
    char** p = (char**)realloc(out->_owned_strs, new_count * sizeof(char*));
    if (!p) return false;
    out->_owned_strs = p;
    out->_owned_strs[out->_owned_strs_count++] = s;
    return true;
}

/* Very small argv splitter: classify args into inputs/libs/-L/-l and passthrough. */
static tc_toolchain_err tc_cc_parse_args(int argc, char** argv, tc_cc_cli* out) {
    if (!out) return TC_TOOLCHAIN_EINVAL;

    enum { TC_CC_MAX_LIST = 4096, TC_CC_MAX_LIBS = 1024 };

    size_t cap_list = (argc > 0 && (size_t)argc < (size_t)TC_CC_MAX_LIST) ? (size_t)argc : (size_t)TC_CC_MAX_LIST;
    size_t cap_libs = (argc > 0 && (size_t)argc < (size_t)TC_CC_MAX_LIBS) ? (size_t)argc : (size_t)TC_CC_MAX_LIBS;

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

        if (tc_arg_is(a, "-c")) { out->compile_only = true; continue; }
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

        if (tc_arg_is(a, "-x") && i + 1 < argc) {
            const char* x = argv[++i];
            if (strcmp(x, "c") == 0) out->lang = TC_LANG_C;
            else if (strcmp(x, "c++") == 0) out->lang = TC_LANG_CXX;
            else if (strcmp(x, "assembler") == 0) out->lang = TC_LANG_ASM;
            else {
                if (ex_n + 2 > cap_list) return TC_TOOLCHAIN_EOVERFLOW;
                extra[ex_n++] = "-x";
                extra[ex_n++] = x;
            }
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

        /* Consume response file as a positional: "file.rsp" -> "@file.rsp" (clang-like). */
        if (a[0] != '-' && tc_arg_has_suffix(a, ".rsp") && tc_fs_is_file(a)) {
            if (ex_n >= cap_list) return TC_TOOLCHAIN_EOVERFLOW;
            char* at = tc_dup_with_at_prefix(a);
            if (!at || !tc_push_owned_str(out, at)) { free(at); return TC_TOOLCHAIN_ESTATE; }
            extra[ex_n++] = at;
            continue;
        }

        /* Input if it looks like a file; else passthrough */
        if (a[0] != '-' && tc_fs_is_file(a)) {
            if (in_n >= cap_list) return TC_TOOLCHAIN_EOVERFLOW;
            inputs[in_n++] = a;
            continue;
        }

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

    /* infer lang from first input if user didn't force */
    if (out->lang == TC_LANG_C && out->inputs_count > 0) {
        out->lang = tc_infer_lang_from_path(out->inputs[0]);
    }

    return TC_TOOLCHAIN_OK;
}

/* -----------------------------------------------------------------------------
 * Driver main
 * -------------------------------------------------------------------------- */

static int tc_print_err(tc_toolchain_err e, const char* what) {
    fprintf(stderr, "vitte-cc: %s: %s\n", what ? what : "error", tc_toolchain_err_str(e));
    return (e == TC_TOOLCHAIN_OK) ? 0 : 1;
}

static void tc_driver_log_verbose(const tc_cc_cli* cli,
                                  const tc_clang_paths* paths,
                                  const tc_target* tgt,
                                  const char* sysroot) {
    if (!cli || !cli->verbose) return;
    fprintf(stderr, "vitte-cc: tool=cc\n");
    if (tgt) fprintf(stderr, "vitte-cc: target=%s\n", tgt->triple[0] ? tgt->triple : "(host)");
    fprintf(stderr, "vitte-cc: sysroot=%s\n", (sysroot && sysroot[0]) ? sysroot : "(none)");
    if (paths) {
        fprintf(stderr, "vitte-cc: clang=%s\n", paths->clang[0] ? paths->clang : "clang");
        fprintf(stderr, "vitte-cc: clangxx=%s\n", paths->clangxx[0] ? paths->clangxx : "clang++");
        fprintf(stderr, "vitte-cc: lld=%s\n", paths->lld[0] ? paths->lld : "lld");
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
    tc_cc_cli cli;
    tc_cc_cli_zero(&cli);

    tc_toolchain_err pe = tc_cc_parse_args(argc, argv, &cli);
    if (pe != TC_TOOLCHAIN_OK) { tc_cc_cli_free(&cli); return tc_print_err(pe, "invalid arguments"); }

    /* Resolve tool paths */
    tc_clang_paths paths;
    tc_clang_env_opts eopts = {0};
    eopts.probe_via_which = true;

    tc_toolchain_process_result diag = {0};
    tc_toolchain_err ee = tc_clang_env_resolve(&paths, &eopts, &diag);
    if (ee != TC_TOOLCHAIN_OK) {
        /* keep going with PATH names, but report */
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

    /* SDK / sysroot (best-effort) */
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
        fprintf(stdout, "tool=vitte-cc\n");
        fprintf(stdout, "target=%s\n", tgt.triple[0] ? tgt.triple : "(host)");
        fprintf(stdout, "sysroot=%s\n", (sysroot && sysroot[0]) ? sysroot : "(none)");
        fprintf(stdout, "clang=%s\n", paths.clang[0] ? paths.clang : "clang");
        fprintf(stdout, "clangxx=%s\n", paths.clangxx[0] ? paths.clangxx : "clang++");
        fprintf(stdout, "lld=%s\n", paths.lld[0] ? paths.lld : "lld");
        tc_cc_cli_free(&cli);
        return 0;
    }

    /* Common flags (policy defaults; in a real driver, load from TOML configs) */
    tc_clang_common common;
    tc_clang_common_zero(&common);

    tc_clang_common_set_std(&common, (cli.lang == TC_LANG_CXX) ? "c++20" : "c17");
    common.opt = TC_OPT_O2;
    common.debug = TC_DBG_G0;
    common.warnings = TC_WARN_DEFAULT;
    common.pic = true;
    common.pie = true;
    common.lto = false;

    if (sysroot && sysroot[0]) {
        tc_clang_common_set_sysroot(&common, sysroot);
    }

    /* If compile-only: compile each input into one object (simple).
     * A production driver would compile each TU to a distinct output in obj dir.
     */
    tc_toolchain_process_result res = {0};

    if (cli.compile_only) {
        const char* in = cli.inputs[0];
        const char* out = cli.out_path ? cli.out_path : "a.o";

        tc_clang_compile_job job;
        memset(&job, 0, sizeof(job));
        job.lang = cli.lang;
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

        if (cli.print_flags || cli.dry_run) {
            tc_clang_paths eff = paths;
            const char* clang_exe = eff.clang[0] ? eff.clang : "clang";
            /* tc_clang_compile_run() builds argv internally; for diagnostics we can re-run it via rsp disabled by default? */
            job.disable_rsp = true;
            (void)clang_exe;
        }

        if (cli.print_flags || cli.dry_run) {
            /* Build a representative argv (no spawn) using the shared builder. */
            tc_argv a;
            tc_argv_zero(&a);
            tc_clang_paths eff = paths;
            const char* exe = (cli.lang == TC_LANG_CXX) ? (eff.clangxx[0] ? eff.clangxx : "clang++")
                                                        : (eff.clang[0] ? eff.clang : "clang");
            memset(eff.clang, 0, sizeof(eff.clang));
            strncpy(eff.clang, exe, sizeof(eff.clang) - 1u);
            tc_clang_compile cjob;
            tc_clang_compile_zero(&cjob);
            cjob.lang = job.lang;
            cjob.input_path = job.input_path;
            cjob.output_path = job.output_path;
            cjob.target = job.target;
            cjob.sysroot_override = job.sysroot_override;
            cjob.out_kind = TC_OUT_OBJ;
            if (tc_clang_build_compile_argv(&eff, &common, &cjob, &a) != TC_CLANG_OK) {
                tc_cc_cli_free(&cli);
                return tc_print_err(TC_TOOLCHAIN_EOVERFLOW, "argv overflow");
            }
            for (size_t i = 0; i < cli.extra_args_count; ++i) {
                const char* x = cli.extra_args[i];
                if (x && *x) (void)tc_argv_push(&a, x);
            }

            if (cli.print_flags) {
                tc_print_argv("compile", a.argv, a.argc);
                tc_cc_cli_free(&cli);
                return 0;
            }
            if (cli.dry_run) {
                tc_print_argv("dry-run compile", a.argv, a.argc);
                tc_cc_cli_free(&cli);
                return 0;
            }
        }

        tc_toolchain_err ce = tc_clang_compile_run(&paths, &common, &job, &res);
        if (ce != TC_TOOLCHAIN_OK) { tc_cc_cli_free(&cli); return tc_print_err(ce, "compile failed"); }
        tc_cc_cli_free(&cli);
        return 0;
    }

    /* Link: use inputs as objects (or sources if user passed them; clang will compile+link).
     * Here we assume objects; for full fidelity, pass through as-is.
     */
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
        l.output_path = ljob.output_path;
        l.target = ljob.target;
        l.sysroot_override = ljob.sysroot_override;
        l.inputs = ljob.inputs;
        l.inputs_count = ljob.inputs_count;
        l.lib_dirs = ljob.lib_dirs;
        l.lib_dirs_count = ljob.lib_dirs_count;
        l.libs = ljob.libs;
        l.libs_count = ljob.libs_count;
        l.extra = ljob.extra_args;
        l.extra_count = ljob.extra_args_count;
        l.use_lld = ljob.use_lld;

        if (tc_clang_build_link_argv(&eff, &common, &l, &a) != TC_CLANG_OK) {
            tc_cc_cli_free(&cli);
            return tc_print_err(TC_TOOLCHAIN_EOVERFLOW, "argv overflow");
        }

        if (cli.print_flags) {
            tc_print_argv("link", a.argv, a.argc);
            tc_cc_cli_free(&cli);
            return 0;
        }
        if (cli.dry_run) {
            tc_print_argv("dry-run link", a.argv, a.argc);
            tc_cc_cli_free(&cli);
            return 0;
        }
    }

    tc_toolchain_err le = tc_clang_link_run(&paths, &common, &ljob, &res);
    if (le != TC_TOOLCHAIN_OK) { tc_cc_cli_free(&cli); return tc_print_err(le, "link failed"); }

    tc_cc_cli_free(&cli);
    return 0;
}
