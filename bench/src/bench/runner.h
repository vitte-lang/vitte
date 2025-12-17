// runner.h - benchmark runner entry point for vitte/bench (C17)
//
// SPDX-License-Identifier: MIT

#ifndef VITTE_BENCH_RUNNER_H
#define VITTE_BENCH_RUNNER_H

#if defined(__cplusplus)
extern "C" {
#endif

// Main runner entry point.
//
// Expected behavior:
//   - Parses CLI options (see bench/options.h)
//   - Selects benchmarks from the registry (see bench/registry.h)
//   - Runs warmup/repeats
//   - Prints human output and optionally writes JSON/CSV outputs
//
// Returns process exit code (0 = success, non-zero = failure).
int bench_runner_run(int argc, char** argv);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_BENCH_RUNNER_H
