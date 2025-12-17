/*
 * src/toolchain/clang_flags.c
 *
 * Vitte toolchain – compute clang flags for compile/link from structured config.
 *
 * Goals:
 *  - Provide a single place to translate tc_clang_common + tc_target into
 *    deterministic, reproducible clang/ld flags.
 *  - Keep all “policy” (warnings, sanitizers, PIC/PIE, LTO, std) here.
 *  - Output is an argv builder (tc_argv) that can be appended to.
 *
 * This module is used by clang_compile.c and clang_link.c.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "toolchain/clang_flags.h"
#include "toolchain/clang.h"
#include "toolchain/clang_target.h"
#include "toolchain/clang_errors.h"

/* -----------------------------------------------------------------------------
 * Internal helpers
 * -------------------------------------------------------------------------- */

static tc_toolchain_err tc_push_opt_debug(tc_argv* a, const tc_clang_common* c) {
    if (!a || !c) return TC_TOOLCHAIN_EINVAL;

    tc_clang_err e = tc_argv_push(a, tc_clang_opt_level_flag(c->opt));
    if (e) return TC_TOOLCHAIN_EOVERFLOW;

    const char* g = tc_clang_debug_flag(c->debug);
    if (g) {
        e = tc_argv_push(a, g);
        if (e) return TC_TOOLCHAIN_EOVERFLOW;
    }

    return TC_TOOLCHAIN_OK;
}

static tc_toolchain_err tc_push_warnings(tc_argv* a, const tc_clang_common* c) {
    if (!a || !c) return TC_TOOLCHAIN_EINVAL;

    tc_clang_err e = tc_clang_push_warnings(a, c->warnings);
    if (e) return TC_TOOLCHAIN_EOVERFLOW;

    return TC_TOOLCHAIN_OK;
}

static tc_toolchain_err tc_push_std(tc_argv* a, const tc_clang_common* c) {
    if (!a || !c) return TC_TOOLCHAIN_EINVAL;
    if (!c->std[0]) return TC_TOOLCHAIN_OK;

    tc_clang_err e = tc_argv_push_concat(a, "-std=", c->std);
    if (e) return TC_TOOLCHAIN_EOVERFLOW;
    return TC_TOOLCHAIN_OK;
}

static tc_toolchain_err tc_push_defines_includes(tc_argv* a, const tc_clang_common* c) {
    if (!a || !c) return TC_TOOLCHAIN_EINVAL;

    for (size_t i = 0; i < c->defines_count; ++i) {
        const char* dn = c->defines[i];
        const char* dv = c->define_values[i];
        tc_clang_err e = tc_argv_push_define(a, dn, dv);
        if (e) return TC_TOOLCHAIN_EOVERFLOW;
    }

    for (size_t i = 0; i < c->includes_count; ++i) {
        const char* inc = c->includes[i];
        if (!inc || !*inc) continue;
        tc_clang_err e = tc_argv_push_include(a, inc);
        if (e) return TC_TOOLCHAIN_EOVERFLOW;
    }

    return TC_TOOLCHAIN_OK;
}

static tc_toolchain_err tc_push_pic_lto(tc_argv* a, const tc_clang_common* c) {
    if (!a || !c) return TC_TOOLCHAIN_EINVAL;

    if (c->pic) {
        if (tc_argv_push(a, "-fPIC") != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW;
    }
    if (c->lto) {
        if (tc_argv_push(a, "-flto") != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW;
    }

    return TC_TOOLCHAIN_OK;
}

static tc_toolchain_err tc_push_target_sysroot(tc_argv* a,
                                               const tc_target* t,
                                               const char* sysroot) {
    if (!a) return TC_TOOLCHAIN_EINVAL;
    if (!t || !tc_target_is_known(t)) return TC_TOOLCHAIN_OK;

    tc_target tmp = *t;
    tc_target_normalize(&tmp);
    (void)tc_target_build_triple(&tmp);

    tc_clang_flags f;
    tc_target_err te = tc_target_to_clang_flags(&tmp, sysroot, &f);
    if (te != TC_TARGET_OK) return TC_TOOLCHAIN_EOVERFLOW;

    if (f.target_flag[0])  { if (tc_argv_push_owned(a, f.target_flag)  != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW; }
    if (f.march_flag[0])   { if (tc_argv_push_owned(a, f.march_flag)   != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW; }
    if (f.mcpu_flag[0])    { if (tc_argv_push_owned(a, f.mcpu_flag)    != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW; }
    if (f.mabi_flag[0])    { if (tc_argv_push_owned(a, f.mabi_flag)    != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW; }
    if (f.sysroot_flag[0]) { if (tc_argv_push_owned(a, f.sysroot_flag) != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW; }

    return TC_TOOLCHAIN_OK;
}

/* -----------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------- */

tc_toolchain_err tc_clang_flags_build_compile(const tc_clang_common* common,
                                              const tc_target* target,
                                              const char* sysroot_override,
                                              tc_argv* out) {
    if (!common || !out) return TC_TOOLCHAIN_EINVAL;

    /* append to out (caller may already have exe + -x), so don't zero */
    const char* sysroot = sysroot_override ? sysroot_override : common->sysroot;

    tc_toolchain_err e;

    e = tc_push_std(out, common);            if (e) return e;
    e = tc_push_opt_debug(out, common);      if (e) return e;
    e = tc_push_warnings(out, common);       if (e) return e;
    e = tc_push_pic_lto(out, common);        if (e) return e;
    e = tc_push_target_sysroot(out, target, sysroot); if (e) return e;
    e = tc_push_defines_includes(out, common); if (e) return e;

    return TC_TOOLCHAIN_OK;
}

tc_toolchain_err tc_clang_flags_build_link(const tc_clang_common* common,
                                           const tc_target* target,
                                           const char* sysroot_override,
                                           bool is_shared,
                                           bool use_lld,
                                           tc_argv* out) {
    if (!common || !out) return TC_TOOLCHAIN_EINVAL;

    const char* sysroot = sysroot_override ? sysroot_override : common->sysroot;

    tc_toolchain_err e;

    /* target/sysroot first for link */
    e = tc_push_target_sysroot(out, target, sysroot); if (e) return e;

    if (is_shared) {
        if (tc_argv_push(out, "-shared") != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW;
        if (common->pic) {
            if (tc_argv_push(out, "-fPIC") != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW;
        }
    }

    if (common->pie) {
        if (tc_argv_push(out, "-pie") != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW;
    }

    if (use_lld) {
        if (tc_argv_push(out, "-fuse-ld=lld") != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW;
    }

    if (common->lto) {
        if (tc_argv_push(out, "-flto") != TC_CLANG_OK) return TC_TOOLCHAIN_EOVERFLOW;
    }

    return TC_TOOLCHAIN_OK;
}
