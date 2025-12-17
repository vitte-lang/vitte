// C:\Users\vince\Documents\GitHub\vitte\fuzz\src\fuzz_driver_afl.c
// AFL++ driver TU (optional).
//
// Compile this only for AFL++ builds. It provides an AFL-style main loop
// that reads from stdin (AFL @@ redirection handled by afl-fuzz) and calls
// the target.
//
// Contract:
//  - user must provide: int fuzz_target_entry(const uint8_t*, size_t);
//  - or define FUZZ_DRIVER_TARGET symbol via compile flags.
//
// Example build:
//  clang -DFUZZ_DRIVER_TARGET=fuzz_target_entry -D__AFL_HAVE_MANUAL_CONTROL=1 ...

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "fuzz/fuzz_assert.h"
#include "fuzz/fuzz_driver.h"

#ifndef FUZZ_DRIVER_TARGET
// You can define this in your target TU or via compiler flags.
// For example: -DFUZZ_DRIVER_TARGET=fuzz_target_entry
#error "fuzz_driver_afl.c requires FUZZ_DRIVER_TARGET"
#endif

// AFL persistent helper: read stdin into a bounded buffer each iteration.
#ifndef FUZZ_AFL_MAX_INPUT
#define FUZZ_AFL_MAX_INPUT (1u * 1024u * 1024u) // 1 MiB
#endif

static size_t
read_stdin_once(uint8_t* buf, size_t cap) {
  size_t n = fread(buf, 1, cap, stdin);
  return n;
}

int
main(void) {
  static uint8_t buf[FUZZ_AFL_MAX_INPUT];

  FUZZ_AFL_INIT();

#if defined(__AFL_LOOP)
  while (FUZZ_AFL_LOOP(1000)) {
    size_t n = read_stdin_once(buf, sizeof(buf));
    (void)FUZZ_DRIVER_TARGET(buf, n);
  }
#else
  // Non-persistent fallback: single iteration
  size_t n = read_stdin_once(buf, sizeof(buf));
  (void)FUZZ_DRIVER_TARGET(buf, n);
#endif

  return 0;
}
