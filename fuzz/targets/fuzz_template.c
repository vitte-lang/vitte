// fuzz_template.c
// Minimal reference for wiring a fuzz harness in this tree.
//
// Instructions:
//   1) Copy this file to fuzz/targets/<name>.c
//   2) Replace the target body with real logic calling your component.
//   3) Define FUZZ_DRIVER_TARGET to the function symbol you declared via FUZZ_TARGET.
//   4) Include "fuzz/fuzz_driver.h" last to generate the libFuzzer entrypoint.
//
// Notes:
//   - Keep harnesses deterministic; no rand(), time(), etc.
//   - Prefer reading modes/flags from the input (see fuzz_target_mode()).
//   - Use fuzz_target_require_min() if your component dislikes empty buffers.
//   - build_targets.sh only builds files that define FUZZ_DRIVER_TARGET.

#include <stddef.h>
#include <stdint.h>

#include "fuzz/fuzz.h"
#include "fuzz/targets/fuzz_target_scaffold.h"

// Example target; replace with your own symbol.
FUZZ_TARGET(example_target) {
  fuzz_reader r = fuzz_target_reader(data, size);
  fuzz_target_require_min(data, size, 1);

  // Consume a mode byte to branch deterministically.
  uint8_t mode = fuzz_target_mode(&r);
  FUZZ_TRACE_TAG_U32("mode", (uint32_t)mode);

  // Drain a few spans to exercise the reader helpers.
  fuzz_scaffold_consume_chunks(&r, 4);

  return 0;
}

// Uncomment once you copy/rename this file for a real harness.
// #define FUZZ_DRIVER_TARGET example_target
// #include "fuzz/fuzz_driver.h"
