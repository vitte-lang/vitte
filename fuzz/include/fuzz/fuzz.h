// fuzz.h
// Umbrella header for Vitte fuzzing utilities (C17).
//
// Include this to get the common fuzz infra headers in one place.
//
// Layout expectation:
//   fuzz/include/fuzz/...
//
// Provided headers:
//   - fuzz_assert.h     (crash/asserter)
//   - fuzz_io.h         (bounded mem reader + file read helpers)
//   - fuzz_dict.h       (simple .dict loader + token store)
//   - fuzz_mutators.h   (mutation helpers)
//   - fuzz_trace.h      (lightweight tracing)
//   - fuzz_util.h       (misc utilities)
//   - fuzz_target.h     (target ABI helpers)
//   - (driver not included: include fuzz_driver.h manually after defining
//       FUZZ_DRIVER_TARGET / standalone options)
//
// Define FUZZ_DRIVER_STANDALONE_MAIN=1 before including fuzz_driver.h
// if you want a standalone main() in a target TU.

#ifndef VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_H
#define VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "fuzz_assert.h"
#include "fuzz_dict.h"
#include "fuzz_io.h"
#include "fuzz_mutators.h"
#include "fuzz_target.h"
#include "fuzz_trace.h"
#include "fuzz_util.h"

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_H
