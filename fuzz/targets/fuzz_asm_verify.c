// fuzz_asm_verify.c
// Fuzz target: ASM fastpaths verification and dispatch.

#include <stddef.h>
#include <stdint.h>

#include "fuzz/fuzz.h"
#include "fuzz/targets/fuzz_target_scaffold.h"

#include "vitte/runtime.h"
#include "vitte/asm_verify.h"

FUZZ_TARGET(fuzz_asm_verify_target) {
  fuzz_reader r = fuzz_target_reader(data, size);
  uint8_t mode = fuzz_target_mode(&r);

  // Deterministically scale iterations from input; keep runtime bounded.
  uint64_t iters64 = 0;
  (void)fuzz_reader_read_uvar(&r, &iters64, 3);
  size_t iterations = (size_t)(iters64 % 2048u);
  if (iterations == 0)
    iterations = 1;

  // Run the reference-vs-fastpath comparisons (deterministic).
  (void)vitte_asm_verify_all(iterations);

  // Also exercise the facade dispatchers directly with a few calls.
  // The dispatcher must be safe under repeated calls.
  uint8_t tmp_a[256];
  uint8_t tmp_b[256];
  fuzz_rng rng;
  fuzz_rng_seed(&rng, fuzz_target_seed64(data, size));
  for (size_t i = 0; i < sizeof(tmp_a); ++i)
    tmp_a[i] = fuzz_rng_next_u8(&rng);

  size_t n = (size_t)(mode ? (mode % (uint8_t)sizeof(tmp_a)) : 64u);
  if (n > sizeof(tmp_a))
    n = sizeof(tmp_a);

  (void)vitte_memcpy(tmp_b, tmp_a, n);
  (void)vitte_memset(tmp_b, 0xA5, (n > 8) ? 8 : n);
  (void)vitte_utf8_validate(tmp_b, n);

  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_asm_verify_target
#include "fuzz/fuzz_driver.h"
