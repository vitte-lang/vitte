/*
 * toolchain/config/include/toolchain/clang.h
 *
 * Vitte toolchain – Clang driver interface (argument building + invocation contract).
 *
 * This header defines:
 *  - an argument vector builder (no allocations required; caller supplies storage)
 *  - a stable set of “known” clang options used by the toolchain
 *  - compile/link convenience builders (C/C++/ASM + LLD)
 *
 * NOTE: This file is interface-only. Execution (spawn/process) is expected to live
 * in the platform layer (e.g. toolchain_process.h/.c).
 */

#ifndef VITTE_TOOLCHAIN_CLANG_H
#define VITTE_TOOLCHAIN_CLANG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "toolchain/clang_target.h"
#include "toolchain/clang_paths.h"
#include "toolchain/c_lang_paths.h"

/* -----------------------------------------------------------------------------
 * Limits / sizes
 * -------------------------------------------------------------------------- */

#ifndef TC_CLANG_MAX_ARGS
#define TC_CLANG_MAX_ARGS 512u
#endif

#ifndef TC_CLANG_MAX_STR
#define TC_CLANG_MAX_STR  260u
#endif

#ifndef TC_CLANG_MAX_DEFS
#define TC_CLANG_MAX_DEFS 128u
#endif

#ifndef TC_CLANG_MAX_INCLUDES
#define TC_CLANG_MAX_INCLUDES 128u
#endif

/* -----------------------------------------------------------------------------
 * Simple fixed string (for inline formatting, zero alloc)
 * -------------------------------------------------------------------------- */

typedef struct tc_str_s {
    char   buf[TC_CLANG_MAX_STR];
    size_t len;
} tc_str;

static inline void tc_str_zero(tc_str* s) {
    if (!s) return;
    s->buf[0] = '\0';
    s->len = 0;
}

static inline bool tc_str_set(tc_str* s, const char* z) {
    if (!s) return false;
    tc_str_zero(s);
    if (!z) return true;
    size_t n = strnlen(z, sizeof(s->buf));
    if (n >= sizeof(s->buf)) return false;
    memcpy(s->buf, z, n);
    s->buf[n] = '\0';
    s->len = n;
    return true;
}

static inline bool tc_str_printf(tc_str* s, const char* a, const char* b) {
    if (!s) return false;
    tc_str_zero(s);
    if (!a) a = "";
    if (!b) b = "";
    /* minimal, common case: concat a + b */
    size_t na = strnlen(a, sizeof(s->buf));
    size_t nb = strnlen(b, sizeof(s->buf));
    if (na + nb >= sizeof(s->buf)) return false;
    memcpy(s->buf, a, na);
    memcpy(s->buf + na, b, nb);
    s->buf[na + nb] = '\0';
    s->len = na + nb;
    return true;
}

/* -----------------------------------------------------------------------------
 * Diagnostics / result
 * -------------------------------------------------------------------------- */

typedef enum tc_clang_err_e {
    TC_CLANG_OK = 0,
    TC_CLANG_EINVAL,
    TC_CLANG_EOVERFLOW
} tc_clang_err;

/* -----------------------------------------------------------------------------
 * Optimization / debug / warnings
 * -------------------------------------------------------------------------- */

typedef enum tc_opt_level_e {
    TC_OPT_0 = 0,
    TC_OPT_1,
    TC_OPT_2,
    TC_OPT_3,
    TC_OPT_S, /* size */
    TC_OPT_Z  /* min size */
} tc_opt_level;

typedef enum tc_debug_level_e {
    TC_DBG_NONE = 0,
    TC_DBG_G1,
    TC_DBG_G2,
    TC_DBG_G3
} tc_debug_level;

typedef enum tc_warnings_e {
    TC_WARN_DEFAULT = 0,
    TC_WARN_NONE,
    TC_WARN_ALL,
    TC_WARN_EXTRA,
    TC_WARN_PEDANTIC
} tc_warnings;

/* -----------------------------------------------------------------------------
 * Language / mode
 * -------------------------------------------------------------------------- */

typedef enum tc_lang_e {
    TC_LANG_C = 0,
    TC_LANG_CXX,
    TC_LANG_ASM
} tc_lang;

typedef enum tc_output_kind_e {
    TC_OUT_OBJ = 0,
    TC_OUT_ASM,
    TC_OUT_IR,     /* -S -emit-llvm */
    TC_OUT_BC,     /* -c -emit-llvm */
    TC_OUT_EXE,
    TC_OUT_SHARED, /* -shared */
    TC_OUT_STATIC  /* via ar (not clang); included for completeness */
} tc_output_kind;

/* -----------------------------------------------------------------------------
 * Arguments builder
 * -------------------------------------------------------------------------- */

typedef struct tc_argv_s {
    const char* argv[TC_CLANG_MAX_ARGS];
    size_t      argc;

    /* storage for formatted flags; each push_fmt uses one slot */
    tc_str      storage[TC_CLANG_MAX_ARGS];
    size_t      storage_used;
} tc_argv;

static inline void tc_argv_zero(tc_argv* a) {
    if (!a) return;
    a->argc = 0;
    a->storage_used = 0;
}

static inline tc_clang_err tc_argv_push(tc_argv* a, const char* s) {
    if (!a || !s) return TC_CLANG_EINVAL;
    if (a->argc >= TC_CLANG_MAX_ARGS) return TC_CLANG_EOVERFLOW;
    a->argv[a->argc++] = s;
    return TC_CLANG_OK;
}

static inline tc_clang_err tc_argv_push_owned(tc_argv* a, const char* z) {
    if (!a || !z) return TC_CLANG_EINVAL;
    if (a->argc >= TC_CLANG_MAX_ARGS) return TC_CLANG_EOVERFLOW;
    if (a->storage_used >= TC_CLANG_MAX_ARGS) return TC_CLANG_EOVERFLOW;

    tc_str* slot = &a->storage[a->storage_used++];
    if (!tc_str_set(slot, z)) return TC_CLANG_EOVERFLOW;

    a->argv[a->argc++] = slot->buf;
    return TC_CLANG_OK;
}

static inline tc_clang_err tc_argv_push_concat(tc_argv* a, const char* prefix, const char* value) {
    if (!a) return TC_CLANG_EINVAL;
    if (a->argc >= TC_CLANG_MAX_ARGS) return TC_CLANG_EOVERFLOW;
    if (a->storage_used >= TC_CLANG_MAX_ARGS) return TC_CLANG_EOVERFLOW;

    tc_str* slot = &a->storage[a->storage_used++];
    if (!tc_str_printf(slot, prefix, value)) return TC_CLANG_EOVERFLOW;

    a->argv[a->argc++] = slot->buf;
    return TC_CLANG_OK;
}

/* Convenience: push "-DNAME=VALUE" */
static inline tc_clang_err tc_argv_push_define(tc_argv* a, const char* name, const char* value_opt) {
    if (!a || !name || !*name) return TC_CLANG_EINVAL;
    if (!value_opt) value_opt = "";

    if (!*value_opt) {
        return tc_argv_push_concat(a, "-D", name);
    }

    /* build "-D" + name + "=" + value */
    if (a->argc >= TC_CLANG_MAX_ARGS) return TC_CLANG_EOVERFLOW;
    if (a->storage_used >= TC_CLANG_MAX_ARGS) return TC_CLANG_EOVERFLOW;

    tc_str* slot = &a->storage[a->storage_used++];
    tc_str_zero(slot);

    size_t nn = strnlen(name, sizeof(slot->buf));
    size_t nv = strnlen(value_opt, sizeof(slot->buf));
    if (nn + 2u + nv >= sizeof(slot->buf)) return TC_CLANG_EOVERFLOW;

    slot->buf[0] = '-';
    slot->buf[1] = 'D';
    memcpy(slot->buf + 2u, name, nn);
    slot->buf[2u + nn] = '=';
    memcpy(slot->buf + 3u + nn, value_opt, nv);
    slot->buf[3u + nn + nv] = '\0';
    slot->len = 3u + nn + nv;

    a->argv[a->argc++] = slot->buf;
    return TC_CLANG_OK;
}

/* Convenience: push "-I<path>" */
static inline tc_clang_err tc_argv_push_include(tc_argv* a, const char* path) {
    if (!a || !path || !*path) return TC_CLANG_EINVAL;
    return tc_argv_push_concat(a, "-I", path);
}

/* -----------------------------------------------------------------------------
 * Clang “known options” helpers
 * -------------------------------------------------------------------------- */

static inline const char* tc_clang_opt_level_flag(tc_opt_level o) {
    switch (o) {
        case TC_OPT_0: return "-O0";
        case TC_OPT_1: return "-O1";
        case TC_OPT_2: return "-O2";
        case TC_OPT_3: return "-O3";
        case TC_OPT_S: return "-Os";
        case TC_OPT_Z: return "-Oz";
        default:       return "-O0";
    }
}

static inline const char* tc_clang_debug_flag(tc_debug_level g) {
    switch (g) {
        case TC_DBG_G1: return "-g1";
        case TC_DBG_G2: return "-g2";
        case TC_DBG_G3: return "-g3";
        case TC_DBG_NONE:
        default:        return NULL;
    }
}

static inline tc_clang_err tc_clang_push_warnings(tc_argv* a, tc_warnings w) {
    if (!a) return TC_CLANG_EINVAL;

    switch (w) {
        case TC_WARN_NONE:
            return tc_argv_push(a, "-w");
        case TC_WARN_ALL:
            return tc_argv_push(a, "-Wall");
        case TC_WARN_EXTRA: {
            tc_clang_err e = tc_argv_push(a, "-Wall");
            if (e) return e;
            return tc_argv_push(a, "-Wextra");
        }
        case TC_WARN_PEDANTIC: {
            tc_clang_err e = tc_argv_push(a, "-Wall");
            if (e) return e;
            e = tc_argv_push(a, "-Wextra");
            if (e) return e;
            return tc_argv_push(a, "-pedantic");
        }
        case TC_WARN_DEFAULT:
        default:
            return TC_CLANG_OK;
    }
}

/* -----------------------------------------------------------------------------
 * Compile/link options model
 * -------------------------------------------------------------------------- */

typedef struct tc_clang_common_s {
    tc_opt_level    opt;
    tc_debug_level  debug;
    tc_warnings     warnings;

    bool            pic;        /* -fPIC */
    bool            pie;        /* -fPIE / -pie (link) */
    bool            lto;        /* -flto */
    bool            sanitize;   /* placeholder toggle; detailed selection elsewhere */

    /* C/C++ standard string: "c17", "c23", "c++20"... (optional) */
    char            std[32];

    /* preprocessor defines/includes */
    const char*     defines[TC_CLANG_MAX_DEFS];
    const char*     define_values[TC_CLANG_MAX_DEFS]; /* may be NULL */
    size_t          defines_count;

    const char*     includes[TC_CLANG_MAX_INCLUDES];
    size_t          includes_count;

    /* sysroot */
    const char*     sysroot; /* may be NULL */
} tc_clang_common;

static inline void tc_clang_common_zero(tc_clang_common* c) {
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->opt = TC_OPT_0;
    c->debug = TC_DBG_NONE;
    c->warnings = TC_WARN_DEFAULT;
}

typedef struct tc_clang_compile_s {
    tc_lang         lang;
    tc_output_kind  out_kind; /* OBJ/ASM/IR/BC */
    const char*     input_path;
    const char*     output_path;

    tc_target       target;
    const char*     sysroot_override; /* if not NULL, overrides common.sysroot */
} tc_clang_compile;

static inline void tc_clang_compile_zero(tc_clang_compile* c) {
    if (!c) return;
    memset(c, 0, sizeof(*c));
    c->lang = TC_LANG_C;
    c->out_kind = TC_OUT_OBJ;
}

typedef struct tc_clang_link_s {
    tc_output_kind  out_kind; /* EXE/SHARED */
    const char*     output_path;

    tc_target       target;
    const char*     sysroot_override;

    /* objects and libs */
    const char**    inputs;     /* object files, .a, etc. */
    size_t          inputs_count;

    const char**    lib_dirs;   /* -L */
    size_t          lib_dirs_count;

    const char**    libs;       /* -l */
    size_t          libs_count;

    /* extra raw args */
    const char**    extra;
    size_t          extra_count;

    bool            use_lld;    /* -fuse-ld=lld */
} tc_clang_link;

static inline void tc_clang_link_zero(tc_clang_link* l) {
    if (!l) return;
    memset(l, 0, sizeof(*l));
    l->out_kind = TC_OUT_EXE;
    l->use_lld = true;
}

/* -----------------------------------------------------------------------------
 * Builder: compile
 * -------------------------------------------------------------------------- */

static inline tc_clang_err tc_clang_build_compile_argv(const tc_clang_paths* paths,
                                                       const tc_clang_common* common,
                                                       const tc_clang_compile* job,
                                                       tc_argv* out) {
    if (!paths || !common || !job || !out) return TC_CLANG_EINVAL;
    if (!job->input_path || !job->output_path) return TC_CLANG_EINVAL;

    tc_argv_zero(out);

    /* clang executable */
    tc_clang_err e = tc_argv_push(out, paths->clang);
    if (e) return e;

    /* language */
    switch (job->lang) {
        case TC_LANG_C:   e = tc_argv_push(out, "-x"); if (e) return e; e = tc_argv_push(out, "c"); if (e) return e; break;
        case TC_LANG_CXX: e = tc_argv_push(out, "-x"); if (e) return e; e = tc_argv_push(out, "c++"); if (e) return e; break;
        case TC_LANG_ASM: e = tc_argv_push(out, "-x"); if (e) return e; e = tc_argv_push(out, "assembler"); if (e) return e; break;
        default: break;
    }

    /* std */
    if (common->std[0]) {
        e = tc_argv_push_concat(out, "-std=", common->std);
        if (e) return e;
    }

    /* optimization / debug */
    e = tc_argv_push(out, tc_clang_opt_level_flag(common->opt));
    if (e) return e;

    const char* g = tc_clang_debug_flag(common->debug);
    if (g) { e = tc_argv_push(out, g); if (e) return e; }

    /* warnings */
    e = tc_clang_push_warnings(out, common->warnings);
    if (e) return e;

    /* PIC */
    if (common->pic) { e = tc_argv_push(out, "-fPIC"); if (e) return e; }

    /* LTO */
    if (common->lto) { e = tc_argv_push(out, "-flto"); if (e) return e; }

    /* target flags */
    tc_clang_flags tf;
    const char* sysroot = job->sysroot_override ? job->sysroot_override : common->sysroot;
    (void)sysroot;
    if (tc_target_is_known(&job->target)) {
        tc_target tmp = job->target;
        tc_target_normalize(&tmp);
        (void)tc_target_build_triple(&tmp);
        tc_target_err te = tc_target_to_clang_flags(&tmp, sysroot, &tf);
        if (te != TC_TARGET_OK) return TC_CLANG_EOVERFLOW;

        if (tf.target_flag[0]) { e = tc_argv_push_owned(out, tf.target_flag); if (e) return e; }
        if (tf.march_flag[0])  { e = tc_argv_push_owned(out, tf.march_flag);  if (e) return e; }
        if (tf.mcpu_flag[0])   { e = tc_argv_push_owned(out, tf.mcpu_flag);   if (e) return e; }
        if (tf.mabi_flag[0])   { e = tc_argv_push_owned(out, tf.mabi_flag);   if (e) return e; }
        if (tf.sysroot_flag[0]){ e = tc_argv_push_owned(out, tf.sysroot_flag);if (e) return e; }
    }

    /* defines */
    for (size_t i = 0; i < common->defines_count; ++i) {
        const char* dn = common->defines[i];
        const char* dv = common->define_values[i];
        e = tc_argv_push_define(out, dn, dv);
        if (e) return e;
    }

    /* include paths */
    for (size_t i = 0; i < common->includes_count; ++i) {
        e = tc_argv_push_include(out, common->includes[i]);
        if (e) return e;
    }

    /* output kind */
    switch (job->out_kind) {
        case TC_OUT_OBJ:
            e = tc_argv_push(out, "-c"); if (e) return e;
            break;
        case TC_OUT_ASM:
            e = tc_argv_push(out, "-S"); if (e) return e;
            break;
        case TC_OUT_IR:
            e = tc_argv_push(out, "-S"); if (e) return e;
            e = tc_argv_push(out, "-emit-llvm"); if (e) return e;
            break;
        case TC_OUT_BC:
            e = tc_argv_push(out, "-c"); if (e) return e;
            e = tc_argv_push(out, "-emit-llvm"); if (e) return e;
            break;
        default:
            break;
    }

    /* -o */
    e = tc_argv_push(out, "-o"); if (e) return e;
    e = tc_argv_push(out, job->output_path); if (e) return e;

    /* input */
    e = tc_argv_push(out, job->input_path);
    if (e) return e;

    return TC_CLANG_OK;
}

/* -----------------------------------------------------------------------------
 * Builder: link
 * -------------------------------------------------------------------------- */

static inline tc_clang_err tc_clang_build_link_argv(const tc_clang_paths* paths,
                                                    const tc_clang_common* common,
                                                    const tc_clang_link* job,
                                                    tc_argv* out) {
    if (!paths || !common || !job || !out) return TC_CLANG_EINVAL;
    if (!job->output_path) return TC_CLANG_EINVAL;

    tc_argv_zero(out);

    tc_clang_err e = tc_argv_push(out, paths->clang);
    if (e) return e;

    /* target flags */
    tc_clang_flags tf;
    const char* sysroot = job->sysroot_override ? job->sysroot_override : common->sysroot;
    if (tc_target_is_known(&job->target)) {
        tc_target tmp = job->target;
        tc_target_normalize(&tmp);
        (void)tc_target_build_triple(&tmp);
        tc_target_err te = tc_target_to_clang_flags(&tmp, sysroot, &tf);
        if (te != TC_TARGET_OK) return TC_CLANG_EOVERFLOW;

        if (tf.target_flag[0]) { e = tc_argv_push_owned(out, tf.target_flag); if (e) return e; }
        if (tf.sysroot_flag[0]){ e = tc_argv_push_owned(out, tf.sysroot_flag); if (e) return e; }
    } else if (sysroot && *sysroot) {
        e = tc_argv_push_concat(out, "--sysroot=", sysroot);
        if (e) return e;
    }

    /* link kind */
    if (job->out_kind == TC_OUT_SHARED) {
        e = tc_argv_push(out, "-shared");
        if (e) return e;
        if (common->pic) { e = tc_argv_push(out, "-fPIC"); if (e) return e; }
    }

    if (common->pie) {
        e = tc_argv_push(out, "-pie");
        if (e) return e;
    }

    if (job->use_lld && paths->lld[0]) {
        e = tc_argv_push(out, "-fuse-ld=lld");
        if (e) return e;
    }

    if (common->lto) {
        e = tc_argv_push(out, "-flto");
        if (e) return e;
    }

    /* library dirs */
    for (size_t i = 0; i < job->lib_dirs_count; ++i) {
        e = tc_argv_push(out, "-L"); if (e) return e;
        e = tc_argv_push(out, job->lib_dirs[i]); if (e) return e;
    }

    /* inputs */
    for (size_t i = 0; i < job->inputs_count; ++i) {
        e = tc_argv_push(out, job->inputs[i]);
        if (e) return e;
    }

    /* libs */
    for (size_t i = 0; i < job->libs_count; ++i) {
        e = tc_argv_push_concat(out, "-l", job->libs[i]);
        if (e) return e;
    }

    /* extra */
    for (size_t i = 0; i < job->extra_count; ++i) {
        e = tc_argv_push(out, job->extra[i]);
        if (e) return e;
    }

    /* -o */
    e = tc_argv_push(out, "-o"); if (e) return e;
    e = tc_argv_push(out, job->output_path); if (e) return e;

    return TC_CLANG_OK;
}

/* -----------------------------------------------------------------------------
 * C runtime include/lib injection (optional helpers)
 * -------------------------------------------------------------------------- */

static inline tc_clang_err tc_clang_add_c_runtime_paths(tc_clang_common* common,
                                                        const tc_c_lang_paths* cpaths) {
    if (!common || !cpaths) return TC_CLANG_EINVAL;

    /* includes */
    if (cpaths->include_count) {
        for (size_t i = 0; i < cpaths->include_count; ++i) {
            if (common->includes_count >= TC_CLANG_MAX_INCLUDES) return TC_CLANG_EOVERFLOW;
            common->includes[common->includes_count++] = cpaths->includes[i];
        }
    }
    return TC_CLANG_OK;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_TOOLCHAIN_CLANG_H */
