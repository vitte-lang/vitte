// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// registry_fallback.c
// -----------------------------------------------------------------------------
// Fallback suite registry for the compiler bench harness.
//
// Rationale:
// Some platforms / toolchains might not run constructor attributes reliably
// (or the project may disable them). In that case, bench suites that rely on
// auto-registration via `__attribute__((constructor))` will not appear.
//
// This module provides a single function that explicitly registers all known
// suites when called by the bench runner.
//
// If your runner already handles registration, you can keep this file as a
// no-op or compile it conditionally.
// -----------------------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>

// Bench API (provided by the runner / common headers).
// We only need the `bench_register_suite` symbol indirectly through each suite.

// -----------------------------------------------------------------------------
// Optional explicit suite registration symbols
// -----------------------------------------------------------------------------

// Each suite file generated in this repo provides a symbol:
//   void vitte_bench_register_<name>_suite(void);
//
// We declare them weak so linking succeeds even if a suite is not compiled.

#if defined(__GNUC__) || defined(__clang__)
#define VITTE_WEAK __attribute__((weak))
#else
#define VITTE_WEAK
#endif

// core suites
VITTE_WEAK void vitte_bench_register_lex_suite(void);
VITTE_WEAK void vitte_bench_register_parse_suite(void);
VITTE_WEAK void vitte_bench_register_ir_suite(void);
VITTE_WEAK void vitte_bench_register_vm_suite(void);

// optional teardown hooks
VITTE_WEAK void vitte_bench_parse_suite_teardown(void);
VITTE_WEAK void vitte_bench_ir_suite_teardown(void);

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void vitte_bench_register_all_suites(void)
{
    if (vitte_bench_register_lex_suite)   vitte_bench_register_lex_suite();
    if (vitte_bench_register_parse_suite) vitte_bench_register_parse_suite();
    if (vitte_bench_register_ir_suite)    vitte_bench_register_ir_suite();
    if (vitte_bench_register_vm_suite)    vitte_bench_register_vm_suite();
}

void vitte_bench_all_suites_teardown(void)
{
    if (vitte_bench_parse_suite_teardown) vitte_bench_parse_suite_teardown();
    if (vitte_bench_ir_suite_teardown)    vitte_bench_ir_suite_teardown();
}
