/*
 * config/include/toolchain/clang_archive.h
 *
 * Vitte toolchain – archive (.a) builder interface (llvm-ar / llvm-ranlib).
 *
 * Scope:
 *  - Build static archives from object files with deterministic mode.
 *  - Optional ranlib pass (mostly for ELF/Mach-O workflows).
 *
 * Execution model:
 *  - This is a pure interface; the implementation should call the platform
 *    process layer (toolchain_process_*). The result type is declared there.
 */

#ifndef VITTE_TOOLCHAIN_CLANG_ARCHIVE_H
#define VITTE_TOOLCHAIN_CLANG_ARCHIVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

#include "toolchain/clang_paths.h"
#include "toolchain/clang_target.h"
#include "toolchain/clang_errors.h"
#include "toolchain/toolchain_process.h"

/* -----------------------------------------------------------------------------
 * Job model
 * -------------------------------------------------------------------------- */

typedef struct tc_clang_archive_job_s {
    /* Output archive path (e.g. libfoo.a). */
    const char*  output_path;

    /* Input object files (.o/.obj). Must be non-empty. */
    const char** inputs;
    size_t       inputs_count;

    /* Use deterministic archives (llvm-ar rcsD). */
    bool         deterministic;

    /* If true, run llvm-ranlib on non-Windows targets. */
    bool         run_ranlib;
} tc_clang_archive_job;

/* -----------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/*
 * Build a static archive using llvm-ar (and optionally llvm-ranlib).
 *
 * - paths:
 *     If provided, uses paths->llvm_ar / paths->llvm_ranlib if non-empty.
 *     Otherwise falls back to "llvm-ar" / "llvm-ranlib" (PATH).
 * - target:
 *     Used for platform heuristics (e.g., skip ranlib on Windows). May be NULL.
 * - job:
 *     Archive parameters (output + inputs).
 * - out_res:
 *     Optional execution capture (exit code, stdout/stderr) – see toolchain_process.h.
 */
tc_toolchain_err tc_clang_archive_build(const tc_clang_paths* paths,
                                        const tc_target* target,
                                        const tc_clang_archive_job* job,
                                        tc_toolchain_process_result* out_res);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_TOOLCHAIN_CLANG_ARCHIVE_H */
