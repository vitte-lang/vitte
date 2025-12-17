// sample.h - sample benchmark registration (C17)
//
// This header is optional; it allows other translation units to register the
// sample benchmarks without relying on sample.c's internal static function.
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Register the sample benchmarks into the bench registry.
// Returns 0 on success, negative on error.
int bench_sample_register(void);

#ifdef __cplusplus
}
#endif
