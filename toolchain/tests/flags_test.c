/*
 * toolchain/tests/flags_test.c
 *
 * Vitte toolchain – flags layer smoke/unit test.
 *
 * Goal:
 *  - Exercise clang_target -> flags emission (tc_target_to_clang_flags)
 *  - Exercise clang_flags builder (tc_clang_flags_build_compile/link)
 *
 * This test is self-contained and prints deterministic output.
 * Intended to be run in CI: compile + execute on host.
 */

#include <stdio.h>
#include <string.h>

#include "toolchain/clang_target.h"
#include "toolchain/clang_flags.h"
#include "toolchain/clang.h"

static void print_argv(const char* title, const tc_argv* a) {
    printf("== %s ==\n", title);
    for (size_t i = 0; i < a->argc; ++i) {
        printf("  [%zu] %s\n", i, a->argv[i] ? a->argv[i] : "(null)");
    }
}

int main(void) {
    tc_target t;
    tc_target_zero(&t);

    /* Parse and normalize a few canonical triples */
    const char* triples[] = {
        "x86_64-unknown-linux-gnu",
        "aarch64-apple-darwin",
        "x86_64-pc-windows-msvc",
        "x86_64-w64-windows-gnu",
        "x86_64-unknown-freebsd",
    };

    for (size_t i = 0; i < sizeof(triples) / sizeof(triples[0]); ++i) {
        const char* tr = triples[i];

        if (tc_target_parse_triple(&t, tr) != TC_TARGET_OK) {
            printf("parse: fail: %s\n", tr);
            continue;
        }
        tc_target_normalize(&t);

        tc_clang_flags f;
        memset(&f, 0, sizeof(f));

        if (tc_target_to_clang_flags(&t, "SYSROOT", &f) != TC_TARGET_OK) {
            printf("flags: fail: %s\n", tr);
            continue;
        }

        printf("triple: %s\n", t.triple);
        printf("  %s\n", f.target_flag);
        if (f.sysroot_flag[0]) printf("  %s\n", f.sysroot_flag);
        if (f.march_flag[0])   printf("  %s\n", f.march_flag);
        if (f.mcpu_flag[0])    printf("  %s\n", f.mcpu_flag);
        if (f.mabi_flag[0])    printf("  %s\n", f.mabi_flag);
    }

    /* Build a compile argv fragment */
    tc_clang_common c;
    tc_clang_common_zero(&c);
    tc_clang_common_set_std(&c, "c17");
    tc_clang_common_set_sysroot(&c, "SYSROOT");
    c.opt = TC_OPT_O2;
    c.debug = TC_DBG_G2;
    c.pic = true;
    c.lto = false;
    c.warnings = TC_WARN_DEFAULT;

    tc_argv a1;
    tc_argv_zero(&a1);

    (void)tc_target_parse_triple(&t, "x86_64-unknown-linux-gnu");
    tc_target_normalize(&t);

    if (tc_clang_flags_build_compile(&c, &t, NULL, &a1) != TC_TOOLCHAIN_OK) {
        printf("compile builder: fail\n");
        return 2;
    }
    print_argv("compile_flags", &a1);

    /* Build a link argv fragment */
    tc_argv a2;
    tc_argv_zero(&a2);

    if (tc_clang_flags_build_link(&c, &t, NULL, false, true, &a2) != TC_TOOLCHAIN_OK) {
        printf("link builder: fail\n");
        return 3;
    }
    print_argv("link_flags", &a2);

    return 0;
}
