

#include "bench/bench.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/*
  bm_hash.c (complet max)

  Goal:
  - Micro benchmark for hashing throughput over byte buffers.

  Design:
  - Uses a small corpus of fixed-size buffers (16, 64, 256, 1024, 4096 bytes)
  - Mixes sequential and pseudo-random access to avoid perfect cache predictability
  - Hash core: FNV-1a 64-bit, plus a final avalanche mix (splitmix64) to better
    represent a typical hash pipeline.

  Notes:
  - This benchmark is called many times inside the micro runner loop.
  - Keep per-call work moderate; this function does a small batch of hashes.
*/

static volatile uint64_t sink = 0;
static uint32_t rng_state = 0xA11CE5u;

static BENCH_INLINE uint32_t rng_u32(void) {
  rng_state = (uint32_t)(rng_state * 1664525u + 1013904223u);
  return rng_state;
}

static BENCH_INLINE uint64_t fnv1a64_ref(const uint8_t* p, size_t n) {
  uint64_t h = 1469598103934665603ull;
  for(size_t i=0;i<n;i++) {
    h ^= (uint64_t)p[i];
    h *= 1099511628211ull;
  }
  return h;
}

static BENCH_INLINE uint64_t splitmix64(uint64_t x) {
  x += 0x9E3779B97F4A7C15ull;
  x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
  x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
  x ^= (x >> 31);
  return x;
}

/* Optional: if you later integrate asm fastpath, provide this symbol and enable by define. */
#if defined(VITTE_BENCH_USE_ASM_HASH)
extern uint64_t vitte_fnv1a64_fast(const void* data, size_t n);
static BENCH_INLINE uint64_t hash64(const uint8_t* p, size_t n) {
  return vitte_fnv1a64_fast(p, n);
}
#else
static BENCH_INLINE uint64_t hash64(const uint8_t* p, size_t n) {
  return fnv1a64_ref(p, n);
}
#endif

/* Corpus buffers (static, deterministic). */
static uint8_t buf16[16];
static uint8_t buf64[64];
static uint8_t buf256[256];
static uint8_t buf1024[1024];
static uint8_t buf4096[4096];
static int corpus_init = 0;

static void init_corpus(void) {
  if(corpus_init) return;
  corpus_init = 1;

  /* Fill with deterministic pseudo-random bytes (stable across runs). */
  uint32_t s = 0x12345678u;
  #define FILL(BUF) do { \
    for(size_t i=0;i<sizeof(BUF);i++) { \
      s = (uint32_t)(s * 1103515245u + 12345u); \
      (BUF)[i] = (uint8_t)(s >> 16); \
    } \
  } while(0)

  FILL(buf16);
  FILL(buf64);
  FILL(buf256);
  FILL(buf1024);
  FILL(buf4096);

  #undef FILL
}

/* A micro-batch of hashes per call. */
void bm_hash(void* ctx) {
  (void)ctx;
  init_corpus();

  /* Alternate between sequential and pseudo-random selection. */
  const uint32_t r = rng_u32();

  const uint8_t* bufs[5] = { buf16, buf64, buf256, buf1024, buf4096 };
  const size_t lens[5] = { 16u, 64u, 256u, 1024u, 4096u };

  /* Do 8 hashes per call: mixture of sizes. */
  uint64_t acc = sink;

  for(int k=0;k<8;k++) {
    const uint32_t pick = (r + (uint32_t)k * 0x9E3779B9u);
    const int idx = (int)(pick % 5u);
    const uint8_t* p = bufs[idx];
    const size_t n = lens[idx];

    /* Random-ish offset within buffer for larger sizes (keeps cache behavior realistic). */
    size_t off = 0;
    if(n >= 256u) {
      off = (size_t)((pick >> 8) % (uint32_t)(n - 64u));
    }

    uint64_t h = hash64(p + off, (n >= 64u) ? 64u : n);
    h = splitmix64(h ^ (uint64_t)n ^ (uint64_t)off);
    acc ^= h;
  }

  /* Make it observable. */
  sink = acc;
}

void bench_register_micro_hash(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:hash", BENCH_MICRO, bm_hash, NULL);
}
