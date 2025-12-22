// fuzz_mem_contract.c
// Fuzz target: API contract checks for vitte_memcpy/vitte_memset.

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fuzz/fuzz.h"
#include "fuzz/targets/fuzz_target_scaffold.h"

#include "vitte/runtime.h"

static int
check_memcpy_contract(fuzz_rng* rng) {
  enum { BUFSZ = 4096 };
  static uint8_t src[BUFSZ + 128];
  static uint8_t d0[BUFSZ + 128];
  static uint8_t d1[BUFSZ + 128];

  size_t n = fuzz_rng_range(rng, BUFSZ + 1);
  size_t so = fuzz_rng_range(rng, 64);
  size_t o0 = fuzz_rng_range(rng, 64);
  size_t o1 = fuzz_rng_range(rng, 64);

  for (size_t i = 0; i < sizeof(src); ++i)
    src[i] = fuzz_rng_next_u8(rng);
  for (size_t i = 0; i < sizeof(d0); ++i)
    d0[i] = fuzz_rng_next_u8(rng);
  memcpy(d1, d0, sizeof(d1));

  uint8_t* srcp = src + so;
  uint8_t* p0 = d0 + o0;
  uint8_t* p1 = d1 + o1;

  void* r0 = memcpy(p0, srcp, n);
  void* r1 = vitte_memcpy(p1, srcp, n);

  if ((uint8_t*)r0 != p0)
    return 1;
  if ((uint8_t*)r1 != p1)
    return 1;

  if (memcmp(p0, p1, n) != 0)
    return 1;

  return 0;
}

static int
check_memset_contract(fuzz_rng* rng) {
  enum { BUFSZ = 4096 };
  static uint8_t d0[BUFSZ + 128];
  static uint8_t d1[BUFSZ + 128];

  size_t n = fuzz_rng_range(rng, BUFSZ + 1);
  size_t o0 = fuzz_rng_range(rng, 64);
  size_t o1 = fuzz_rng_range(rng, 64);
  int c = (int)fuzz_rng_next_u8(rng);

  for (size_t i = 0; i < sizeof(d0); ++i)
    d0[i] = fuzz_rng_next_u8(rng);
  memcpy(d1, d0, sizeof(d1));

  uint8_t* p0 = d0 + o0;
  uint8_t* p1 = d1 + o1;

  void* r0 = memset(p0, c, n);
  void* r1 = vitte_memset(p1, c, n);

  if ((uint8_t*)r0 != p0)
    return 1;
  if ((uint8_t*)r1 != p1)
    return 1;

  if (memcmp(p0, p1, n) != 0)
    return 1;

  return 0;
}

FUZZ_TARGET(fuzz_mem_contract_target) {
  fuzz_reader r = fuzz_target_reader(data, size);

  uint64_t iters64 = 0;
  (void)fuzz_reader_read_uvar(&r, &iters64, 3);
  size_t iters = (size_t)(iters64 % 1024u);
  if (iters == 0)
    iters = 1;

  fuzz_rng rng;
  fuzz_rng_seed(&rng, fuzz_target_seed64(data, size));

  for (size_t i = 0; i < iters; ++i) {
    if (check_memcpy_contract(&rng))
      return 0;
    if (check_memset_contract(&rng))
      return 0;
  }

  return 0;
}

#define FUZZ_DRIVER_TARGET fuzz_mem_contract_target
#include "fuzz/fuzz_driver.h"
