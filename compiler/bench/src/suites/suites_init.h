// SPDX-License-Identifier: MIT
// -----------------------------------------------------------------------------
// suites_init.h
// -----------------------------------------------------------------------------
// Bench suites initialization helpers.
//
// The bench runner should call:
//   - vitte_bench_suites_init()     before enumerating/running benchmarks.
//   - vitte_bench_suites_teardown() after finishing benchmarks.
//
// This supports both:
//   1) Suite auto-registration via constructor attributes.
//   2) Explicit fallback registration via registry_fallback.c.
// -----------------------------------------------------------------------------

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Ensure suites are registered (auto or fallback).
void vitte_bench_suites_init(void);

// Optional teardown for suites that allocate global fixtures.
void vitte_bench_suites_teardown(void);

#ifdef __cplusplus
} // extern "C"
#endif
