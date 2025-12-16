#include "bench/bench.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/*
  bm_memcpy.c (complet max)

  Goal:
  - Micro benchmark for memcpy throughput/latency across realistic sizes.

  Design:
  - Uses a fixed corpus of sizes: 8, 16, 32, 64, 128, 256, 512, 1024, 4096
  - Mixes alignments and offsets to represent real-world copy patterns.
  - Performs a small batch per call to keep overhead bounded.

  Optional:
  - Define VITTE_BENCH_USE_ASM_MEMCPY and provide `vitte_memcpy_fast` symbol
    (from your asm dispatch layer) to benchmark the asm fastpath.
*/

static volatile uint64_t sink = 0;
static uint32_t rng_state = 0xC001D00Du;

static BENCH_INLINE uint32_t rng_u32(void) {
  rng_state = (uint32_t)(rng_state * 1664525u + 1013904223u);
  return rng_state;
}

#if defined(VITTE_BENCH_USE_ASM_MEMCPY)
extern void* vitte_memcpy_fast(void* dst, const void* src, size_t n);
static BENCH_INLINE void* do_memcpy(void* d, const void* s, size_t n) {
  return vitte_memcpy_fast(d, s, n);
}
#else
static BENCH_INLINE void* do_memcpy(void* d, const void* s, size_t n) {
  return memcpy(d, s, n);
}
#endif

static uint8_t src_buf[8192 + 64];
static uint8_t dst_buf[8192 + 64];
static int inited = 0;

static void init_buffers(void) {
  if(inited) return;
  inited = 1;

  uint32_t s = 0xBADC0DEu;
  for(size_t i=0;i<sizeof(src_buf);i++) {
    s = (uint32_t)(s * 1103515245u + 12345u);
    src_buf[i] = (uint8_t)(s >> 16);
  }
  memset(dst_buf, 0, sizeof(dst_buf));
}

static BENCH_INLINE uint64_t fold8(const uint8_t* p, size_t n) {
  /* Fold a few bytes to force reading from dst (anti DCE). */
  uint64_t x = 0;
  if(n == 0) return 0;
  x ^= (uint64_t)p[0] << 1;
  x ^= (uint64_t)p[n/2] << 3;
  x ^= (uint64_t)p[n-1] << 5;
  return x;
}

void bm_memcpy(void* ctx) {
  (void)ctx;
  init_buffers();

  static const size_t sizes[] = { 8,16,32,64,128,256,512,1024,4096 };

  const uint32_t r = rng_u32();
  uint64_t acc = sink;

  /* 6 copies per call: mix sizes, alignments, offsets */
  for(int k=0;k<6;k++) {
    const uint32_t pick = r + (uint32_t)k * 0x9E3779B9u;
    const size_t n = sizes[pick % (sizeof(sizes)/sizeof(sizes[0]))];

    /* offsets within 0..63 to vary alignment */
    const size_t so = (size_t)((pick >> 1) & 63u);
    const size_t doff = (size_t)((pick >> 7) & 63u);

    uint8_t* d = dst_buf + doff;
    const uint8_t* s = src_buf + so;

    do_memcpy(d, s, n);

    /* Fold a few bytes from dst to prevent elision */
    acc ^= fold8(d, n) + (uint64_t)n;

    /* Small perturbation to avoid perfect repetition */
    dst_buf[(doff + (n ? (n-1) : 0)) & (sizeof(dst_buf)-1)] ^= (uint8_t)pick;
  }

  sink = acc;
}

void bench_register_micro_memcpy(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("micro:memcpy", BENCH_MICRO, bm_memcpy, NULL);
}
