// C:\Users\vince\Documents\GitHub\vitte\fuzz\src\fuzz_main.c
// Standalone fuzz runner TU (optional).
//
// This builds a normal executable that runs the target against a file or stdin.
// Useful for CI smoke or quick reproduction without libFuzzer.
//
// Contract:
//  - user must provide FUZZ_DRIVER_TARGET (function symbol)
//  - target must accept (data,size) and return int
//
// Build example:
//  clang -DFUZZ_DRIVER_TARGET=my_target -DFUZZ_DRIVER_STANDALONE_MAIN=1 \
//        fuzz_main.c my_target.c -o my_target_run

#define FUZZ_DRIVER_STANDALONE_MAIN 1
#include "fuzz/fuzz_driver.h"

#ifndef FUZZ_DRIVER_TARGET
#error "fuzz_main.c requires FUZZ_DRIVER_TARGET"
#endif
