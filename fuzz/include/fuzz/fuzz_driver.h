// fuzz_driver.h
// Fuzzing driver glue for Vitte fuzz targets (libFuzzer/AFL/standalone) — C17.
//
// Provides:
//  - unified entrypoints:
//      * LLVMFuzzerTestOneInput (libFuzzer)
//      * afl-style persistent loop helper (optional)
//      * standalone main() helper (opt-in)
//  - common options (max input, seed, iterations)
//  - safe input acquisition + deterministic exit behavior
//
// Usage patterns:
//  1) libFuzzer target:
//      #include "fuzz/fuzz_driver.h"
//      FUZZ_TARGET(MyTarget) { ... }
//      // builds LLVMFuzzerTestOneInput automatically.
//
//  2) Standalone binary:
//      #define FUZZ_DRIVER_STANDALONE_MAIN 1
//      #include "fuzz/fuzz_driver.h"
//      FUZZ_TARGET(MyTarget) { ... }
//      // compile + run: fuzz_target.exe <file>
//
//  3) AFL persistent (external loop):
//      use FUZZ_AFL_PERSISTENT() around a read+call, or implement your own loop.
//
// Notes:
//  - You must implement exactly one FUZZ_TARGET(name) in a TU.
//  - This header is intentionally self-contained except fuzz_assert.h.

#ifndef VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_DRIVER_H
#define VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_DRIVER_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz_assert.h"

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#ifndef FUZZ_DRIVER_MAX_INPUT
// Hard cap for standalone reads. libFuzzer/AFL will provide bounded buffers anyway.
#define FUZZ_DRIVER_MAX_INPUT (16u * 1024u * 1024u) // 16 MiB
#endif

#ifndef FUZZ_DRIVER_STDIN
// For standalone: allow reading from stdin when no file is provided.
#define FUZZ_DRIVER_STDIN 1
#endif

#ifndef FUZZ_DRIVER_STANDALONE_MAIN
// If set to 1 before including this header, it will provide main().
#define FUZZ_DRIVER_STANDALONE_MAIN 0
#endif

#ifndef FUZZ_DRIVER_RETURN_ON_EMPTY
// Some targets want to quickly ignore empty inputs.
#define FUZZ_DRIVER_RETURN_ON_EMPTY 1
#endif

//------------------------------------------------------------------------------
// Target declaration macro
//------------------------------------------------------------------------------

typedef int (*fuzz_target_fn)(const uint8_t* data, size_t size);

#define FUZZ_TARGET(name) static int name(const uint8_t* data, size_t size)

// The user must set FUZZ_DRIVER_TARGET to their function symbol once.
// If they use FUZZ_TARGET(), then set: #define FUZZ_DRIVER_TARGET MyTarget
#ifndef FUZZ_DRIVER_TARGET
// No default; we require explicit selection to avoid ODR surprises.
#endif

//------------------------------------------------------------------------------
// LibFuzzer entrypoint
//------------------------------------------------------------------------------

#if defined(FUZZ_DRIVER_TARGET)
int
LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
#if FUZZ_DRIVER_RETURN_ON_EMPTY
  if (size == 0)
    return 0;
#endif
  return FUZZ_DRIVER_TARGET(data, size);
}
#endif

//------------------------------------------------------------------------------
// AFL persistent helper (optional)
//------------------------------------------------------------------------------

#if defined(__AFL_HAVE_MANUAL_CONTROL) || defined(__AFL_LOOP)
// When building with AFL++ persistent mode, you typically:
//   __AFL_INIT();
//   while (__AFL_LOOP(1000)) { read input; target(input); }
//
// This header offers a macro wrapper to reduce boilerplate.
#define FUZZ_AFL_INIT() __AFL_INIT()
#define FUZZ_AFL_LOOP(n) __AFL_LOOP((n))
#else
#define FUZZ_AFL_INIT()                                                                            \
  do {                                                                                             \
  } while (0)
#define FUZZ_AFL_LOOP(n) (0)
#endif

//------------------------------------------------------------------------------
// Standalone helpers
//------------------------------------------------------------------------------

FUZZ_INLINE static uint8_t*
fuzz_driver__read_all(FILE* f, size_t* out_size) {
  FUZZ_ASSERT(f);
  FUZZ_ASSERT(out_size);

  *out_size = 0;

  uint8_t* buf = (uint8_t*)malloc(1);
  if (!buf)
    FUZZ_OOM("fuzz_driver: malloc failed");

  size_t cap = 1;
  size_t len = 0;

  for (;;) {
    if (len == cap) {
      size_t new_cap = cap * 2;
      if (new_cap < cap) {
        free(buf);
        FUZZ_OOM("fuzz_driver: size overflow");
      }
      if (new_cap > (size_t)FUZZ_DRIVER_MAX_INPUT)
        new_cap = (size_t)FUZZ_DRIVER_MAX_INPUT;

      if (new_cap == cap)
        break; // reached max
      uint8_t* nb = (uint8_t*)realloc(buf, new_cap);
      if (!nb) {
        free(buf);
        FUZZ_OOM("fuzz_driver: realloc failed");
      }
      buf = nb;
      cap = new_cap;
    }

    size_t want = cap - len;
    if (want > 64 * 1024)
      want = 64 * 1024;

    size_t got = fread(buf + len, 1, want, f);
    len += got;

    if (got < want) {
      if (feof(f))
        break;
      if (ferror(f)) {
        free(buf);
        FUZZ_PANIC("fuzz_driver: fread failed");
      }
    }

    if (len >= (size_t)FUZZ_DRIVER_MAX_INPUT)
      break;
  }

  *out_size = len;
  return buf;
}

FUZZ_INLINE static int
fuzz_driver_run_file(const char* path, fuzz_target_fn target) {
  FUZZ_ASSERT(path);
  FUZZ_ASSERT(target);

  FILE* f = fopen(path, "rb");
  if (!f)
    FUZZ_PANIC("fuzz_driver_run_file: fopen failed");

  size_t n = 0;
  uint8_t* buf = fuzz_driver__read_all(f, &n);
  fclose(f);

#if FUZZ_DRIVER_RETURN_ON_EMPTY
  if (n == 0) {
    free(buf);
    return 0;
  }
#endif

  int rc = target(buf, n);
  free(buf);
  return rc;
}

#if FUZZ_DRIVER_STDIN
FUZZ_INLINE static int
fuzz_driver_run_stdin(fuzz_target_fn target) {
  FUZZ_ASSERT(target);

  size_t n = 0;
  uint8_t* buf = fuzz_driver__read_all(stdin, &n);

#if FUZZ_DRIVER_RETURN_ON_EMPTY
  if (n == 0) {
    free(buf);
    return 0;
  }
#endif

  int rc = target(buf, n);
  free(buf);
  return rc;
}
#endif

//------------------------------------------------------------------------------
// Standalone main (opt-in)
//------------------------------------------------------------------------------

#if FUZZ_DRIVER_STANDALONE_MAIN

#ifndef FUZZ_DRIVER_TARGET
#error "FUZZ_DRIVER_STANDALONE_MAIN requires: #define FUZZ_DRIVER_TARGET <your_target_fn>"
#endif

static void
fuzz_driver__usage(const char* argv0) {
  fprintf(stderr,
          "usage:\n"
          "  %s <input_file>\n"
#if FUZZ_DRIVER_STDIN
          "  %s -   (read from stdin)\n"
#endif
          ,
          argv0, argv0);
}

int
main(int argc, char** argv) {
  if (argc < 2) {
    fuzz_driver__usage((argc > 0 && argv) ? argv[0] : "fuzz_target");
    return 2;
  }

  const char* arg = argv[1];
#if FUZZ_DRIVER_STDIN
  if (strcmp(arg, "-") == 0) {
    return fuzz_driver_run_stdin(FUZZ_DRIVER_TARGET);
  }
#endif
  return fuzz_driver_run_file(arg, FUZZ_DRIVER_TARGET);
}

#endif // FUZZ_DRIVER_STANDALONE_MAIN

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_DRIVER_H
