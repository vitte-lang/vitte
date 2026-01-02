// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// suites_init.c
// -----------------------------------------------------------------------------
// Central initialization point for bench suites.
//
// This file exists to support two registration strategies:
//   1) Auto-registration via constructor attributes in each suite TU.
//   2) Explicit registration via `vitte_bench_register_all_suites()` fallback.
//
// The bench runner can call:
//   - `vitte_bench_suites_init()` before enumerating suites/cases.
//   - `vitte_bench_suites_teardown()` after finishing benchmarks.
//
// If constructors are enabled and reliable, init is effectively a no-op.
// If not, init will register all suites explicitly.
// -----------------------------------------------------------------------------

#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
    #define VITTE_WEAK __attribute__((weak))
#else
    #define VITTE_WEAK
#endif

// Fallback registry (weak: may not be compiled in some configurations).
VITTE_WEAK void vitte_bench_register_all_suites(void);
VITTE_WEAK void vitte_bench_all_suites_teardown(void);

// Optional: runner may provide a way to ask if any suites exist.
// If present and returns count>0 after constructors, we can skip fallback.
VITTE_WEAK const void* bench_get_suites(size_t* out_count);

void vitte_bench_suites_init(void)
{
    // If the runner provides `bench_get_suites`, and constructors have already
    // registered suites, avoid double-registration.
    if (bench_get_suites)
    {
        size_t n = 0;
        (void)bench_get_suites(&n);
        if (n > 0)
            return;
    }

    // Otherwise attempt fallback explicit registration.
    if (vitte_bench_register_all_suites)
        vitte_bench_register_all_suites();
}

void vitte_bench_suites_teardown(void)
{
    if (vitte_bench_all_suites_teardown)
        vitte_bench_all_suites_teardown();
}
