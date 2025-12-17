/*
 * toolchain/tests/probe_smoke_test.c
 *
 * Vitte toolchain – probe smoke test.
 *
 * Purpose:
 *  - Exercise tc_clang_probe_run() and ensure it returns a coherent report.
 *  - Intended for CI and local diagnostics.
 *
 * Build:
 *  - Compile this with the toolchain project (links against toolchain libs).
 * Run:
 *  - ./probe_smoke_test
 *
 * Output:
 *  - Prints key resolved paths + host triple + capability booleans.
 */

#include <stdio.h>
#include <string.h>

#include "toolchain/clang_probe.h"
#include "toolchain/clang_errors.h"

static void print_bool(const char* k, int v) {
    printf("%s=%s\n", k, v ? "true" : "false");
}

int main(void) {
    tc_clang_probe_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.probe_via_which = true;
    opts.use_clang_dumpmachine = true;
    opts.requested_triple = NULL;

    tc_clang_probe_report r;
    memset(&r, 0, sizeof(r));

    tc_toolchain_err e = tc_clang_probe_run(&opts, &r);
    if (e != TC_TOOLCHAIN_OK) {
        fprintf(stderr, "probe failed: %s\n", tc_toolchain_err_str(e));
        return 2;
    }

    printf("host_triple=%s\n", r.host_triple[0] ? r.host_triple : "(empty)");

    printf("paths.clang=%s\n", r.paths.clang[0] ? r.paths.clang : "(empty)");
    printf("paths.clangxx=%s\n", r.paths.clangxx[0] ? r.paths.clangxx : "(empty)");
    printf("paths.lld=%s\n", r.paths.lld[0] ? r.paths.lld : "(empty)");
    printf("paths.llvm_ar=%s\n", r.paths.llvm_ar[0] ? r.paths.llvm_ar : "(empty)");
    printf("paths.llvm_ranlib=%s\n", r.paths.llvm_ranlib[0] ? r.paths.llvm_ranlib : "(empty)");

    print_bool("status.clang_ok", r.status.clang_ok);
    print_bool("status.clangxx_ok", r.status.clangxx_ok);
    print_bool("status.lld_ok", r.status.lld_ok);
    print_bool("status.llvm_ar_ok", r.status.llvm_ar_ok);
    print_bool("status.llvm_ranlib_ok", r.status.llvm_ranlib_ok);
    print_bool("status.all_ok", r.status.all_ok);

    print_bool("can_compile", r.can_compile);
    print_bool("can_link", r.can_link);
    print_bool("can_archive", r.can_archive);

    if (r.env_resolve_err != TC_TOOLCHAIN_OK) {
        printf("env_resolve_err=%s\n", tc_toolchain_err_str(r.env_resolve_err));
    }

    if (r.notes_missing_tools) {
        printf("notes_missing_tools=0x%08x\n", (unsigned)r.notes_missing_tools);
    }

    return 0;
}
