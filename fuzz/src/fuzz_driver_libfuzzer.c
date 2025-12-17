// C:\Users\vince\Documents\GitHub\vitte\fuzz\src\fuzz_driver_libfuzzer.c
// libFuzzer driver TU (optional).
//
// Most setups define LLVMFuzzerTestOneInput directly in the target TU via
// fuzz_driver.h. This file exists if you prefer to keep the target function
// separate and link a single driver.
//
// Contract:
//  - user must provide FUZZ_DRIVER_TARGET at compile time or in a header include.
//
// Build example:
//  clang -fsanitize=fuzzer,address -DFUZZ_DRIVER_TARGET=my_target fuzz_driver_libfuzzer.c
//  my_target.c

#include "fuzz/fuzz_driver.h"

#ifndef FUZZ_DRIVER_TARGET
#error "fuzz_driver_libfuzzer.c requires FUZZ_DRIVER_TARGET"
#endif
