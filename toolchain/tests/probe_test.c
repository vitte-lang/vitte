/*
 * toolchain/tests/probe_test.c
 *
 * Vitte toolchain – probe unit test (host + requested targets).
 *
 * Purpose:
 *  - Validate that tc_clang_probe_run() produces a stable report structure.
 *  - Validate parsing/normalization of requested triples through probe layer.
 *  - Provide deterministic, grep-friendly output for CI.
 *
 * Notes:
 *  - This is a "max" unit-style test without a test framework.
 *  - It returns non-zero on failures.
 */

#include <stdio.h>
#include <string.h>

#include "toolchain/clang_probe.h"
#include "toolchain/clang_target.h"
#include "toolchain/clang_errors.h"

static int g_fail = 0;

static void fail(const char* msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    g_fail = 1;
}

static void ok(const char* msg) {
    printf("OK: %s\n", msg);
}

static void print_report_summary(const tc_clang_probe_report* r) {
    printf("host_triple=%s\n", r->host_triple[0] ? r->host_triple : "(empty)");
    printf("can_compile=%d can_link=%d can_archive=%d\n",
           r->can_compile ? 1 : 0,
           r->can_link ? 1 : 0,
           r->can_archive ? 1 : 0);

    printf("clang_ok=%d clangxx_ok=%d lld_ok=%d llvm_ar_ok=%d llvm_ranlib_ok=%d all_ok=%d\n",
           r->status.clang_ok ? 1 : 0,
           r->status.clangxx_ok ? 1 : 0,
           r->status.lld_ok ? 1 : 0,
           r->status.llvm_ar_ok ? 1 : 0,
           r->status.llvm_ranlib_ok ? 1 : 0,
           r->status.all_ok ? 1 : 0);
}

static void run_probe_with_requested(const char* requested) {
    tc_clang_probe_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.probe_via_which = true;
    opts.use_clang_dumpmachine = true;
    opts.requested_triple = requested;

    tc_clang_probe_report r;
    memset(&r, 0, sizeof(r));

    tc_toolchain_err e = tc_clang_probe_run(&opts, &r);
    if (e != TC_TOOLCHAIN_OK) {
        fail("tc_clang_probe_run returned error");
        return;
    }

    print_report_summary(&r);

    if (!r.host_triple[0]) fail("host_triple empty");
    else ok("host_triple non-empty");

    if (requested && requested[0]) {
        printf("requested=%s parse_ok=%d supported=%d normalized=%s\n",
               requested,
               r.requested_triple_parse_ok ? 1 : 0,
               r.requested_supported ? 1 : 0,
               r.requested_target.triple[0] ? r.requested_target.triple : "(empty)");

        if (!r.requested_triple_parse_ok) {
            fail("requested triple parse failed");
            return;
        }

        /* basic sanity: normalized triple should not be empty */
        if (!r.requested_target.triple[0]) fail("requested normalized triple empty");
        else ok("requested normalized triple non-empty");
    }
}

int main(void) {
    /* Host probe only */
    run_probe_with_requested(NULL);

    /* A few known triples */
    run_probe_with_requested("x86_64-unknown-linux-gnu");
    run_probe_with_requested("aarch64-apple-darwin");
    run_probe_with_requested("x86_64-pc-windows-msvc");
    run_probe_with_requested("x86_64-w64-windows-gnu");
    run_probe_with_requested("x86_64-unknown-freebsd");

    /* An invalid triple */
    {
        tc_clang_probe_opts opts;
        memset(&opts, 0, sizeof(opts));
        opts.probe_via_which = true;
        opts.use_clang_dumpmachine = false;
        opts.requested_triple = "not-a-triple";

        tc_clang_probe_report r;
        memset(&r, 0, sizeof(r));

        tc_toolchain_err e = tc_clang_probe_run(&opts, &r);
        if (e != TC_TOOLCHAIN_OK) {
            fail("probe failed on invalid triple (should be non-fatal)");
        } else {
            printf("requested=not-a-triple parse_ok=%d supported=%d\n",
                   r.requested_triple_parse_ok ? 1 : 0,
                   r.requested_supported ? 1 : 0);
            if (r.requested_triple_parse_ok) fail("invalid triple unexpectedly parsed OK");
            else ok("invalid triple rejected");
        }
    }

    return g_fail ? 1 : 0;
}
