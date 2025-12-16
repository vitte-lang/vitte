// File: bench/src/micro/bm_hash.c
// Micro-benchmark: hashing baseline — "max" (C17/C23).
//
// Objectif :
//   - Mesurer le coût "plancher" d’un hash pur (sans alloc), sur différents sizes.
//   - Fournir plusieurs fonctions de hash :
//       * FNV-1a 64 (baseline)
//       * wyhash-inspired light mix (non-crypto, très rapide)
//       * siphash-like toy (plus lourd, baseline “secure-ish” sans prétention crypto)
//   - Bench sur buffers : 32B / 256B / 4KB / 64KB (générés en mémoire, déterministes).
//
// Notes :
//   - Ce fichier n’a pas de dépendance au runtime Vitte : c’est un micro pur.
//   - Résultats utiles pour comparer avec hashmap/regex/json parse etc.
//   - On empêche la DCE (dead code elimination) via sink volatile.
//
// Intégration :
//   - Enregistrement explicite via bench_register_std() ici.
//     Si tu as déjà d’autres micro dans bench_register_std, tu peux soit :
//       - fusionner les fonctions bench_register_std dans un seul TU,
//       - ou déplacer ce TU vers bench_register_vm/runtime/etc.
//     Sinon, si plusieurs TUs définissent bench_register_std, tu auras un duplicate symbol.
//
// Recommandation monorepo :
//   - Soit créer un TU unique bench/src/bench/bench_register_std.c qui appelle
//     bm_add_register(r), bm_hash_register(r), etc.
//   - Soit passer en BENCH_ENABLE_AUTOREG=1 sur GCC/Clang.
//
// Ici, pour rester compatible avec ta structure actuelle, on expose
//   void bm_hash_register(bench_registry_t* r)
// et on laisse bench_register_std (weak) optionnel appeler bm_hash_register.
// Tu peux supprimer bench_register_std si tu centralises ailleurs.

#include "bench/bench.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// ======================================================================================
// Blackhole
// ======================================================================================

static inline void bench_blackhole_u64(volatile uint64_t x) { (void)x; }

// ======================================================================================
// Data generation (deterministic)
// ======================================================================================

typedef struct bm_hash_state {
  uint8_t* buf32;
  uint8_t* buf256;
  uint8_t* buf4k;
  uint8_t* buf64k;

  uint64_t seed;
  uint64_t sink;
} bm_hash_state_t;

static uint64_t splitmix64(uint64_t* x) {
  uint64_t z = (*x += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

static void fill_random(uint8_t* p, size_t n, uint64_t seed) {
  uint64_t s = seed;
  size_t i = 0;
  while (i + 8 <= n) {
    uint64_t v = splitmix64(&s);
    memcpy(p + i, &v, 8);
    i += 8;
  }
  if (i < n) {
    uint64_t v = splitmix64(&s);
    memcpy(p + i, &v, n - i);
  }
}

static void* bm_hash_setup(bench_ctx_t* ctx) {
  bm_hash_state_t* st = (bm_hash_state_t*)calloc(1, sizeof(*st));
  if (!st) return NULL;

  uint64_t s = ctx ? ctx->seed : 1;
  st->seed = s ^ 0xD6E8FEB86659FD93ULL;
  st->sink = st->seed;

  st->buf32  = (uint8_t*)malloc(32);
  st->buf256 = (uint8_t*)malloc(256);
  st->buf4k  = (uint8_t*)malloc(4096);
  st->buf64k = (uint8_t*)malloc(65536);

  if (!st->buf32 || !st->buf256 || !st->buf4k || !st->buf64k) {
    free(st->buf32); free(st->buf256); free(st->buf4k); free(st->buf64k);
    free(st);
    return NULL;
  }

  fill_random(st->buf32,  32,    st->seed + 1);
  fill_random(st->buf256, 256,   st->seed + 2);
  fill_random(st->buf4k,  4096,  st->seed + 3);
  fill_random(st->buf64k, 65536, st->seed + 4);

  return st;
}

static void bm_hash_teardown(bench_ctx_t* ctx, void* state) {
  (void)ctx;
  bm_hash_state_t* st = (bm_hash_state_t*)state;
  if (!st) return;
  free(st->buf32);
  free(st->buf256);
  free(st->buf4k);
  free(st->buf64k);
  free(st);
}

// ======================================================================================
// Hash functions
// ======================================================================================

static inline uint64_t rotl64(uint64_t x, int r) {
  return (x << r) | (x >> (64 - r));
}

// FNV-1a 64
static uint64_t hash_fnv1a64(const void* data, size_t n) {
  const uint8_t* p = (const uint8_t*)data;
  uint64_t h = 1469598103934665603ULL;
  for (size_t i = 0; i < n; i++) {
    h ^= (uint64_t)p[i];
    h *= 1099511628211ULL;
  }
  return h;
}

// Lightweight mix (wyhash-inspired, non-crypto)
static inline uint64_t mum64(uint64_t a, uint64_t b) {
#if defined(__SIZEOF_INT128__)
  __uint128_t r = (__uint128_t)a * (__uint128_t)b;
  return (uint64_t)r ^ (uint64_t)(r >> 64);
#else
  // fallback: 64-bit mix approximation (not as good, but portable)
  uint64_t x = a ^ rotl64(b, 32);
  x *= 0x9E3779B97F4A7C15ULL;
  x ^= x >> 33;
  x *= 0xC2B2AE3D27D4EB4FULL;
  x ^= x >> 29;
  return x;
#endif
}

static uint64_t hash_mix64(const void* data, size_t n) {
  const uint8_t* p = (const uint8_t*)data;
  uint64_t seed = 0xA0761D6478BD642FULL ^ (uint64_t)n;

  // process 16 bytes per loop if possible
  size_t i = 0;
  while (i + 16 <= n) {
    uint64_t a, b;
    memcpy(&a, p + i, 8);
    memcpy(&b, p + i + 8, 8);
    seed = mum64(seed ^ a, 0xE7037ED1A0B428DBULL);
    seed = mum64(seed ^ b, 0x8EBC6AF09C88C6E3ULL);
    i += 16;
  }
  // tail
  uint64_t tail = 0;
  size_t rem = n - i;
  if (rem) {
    memcpy(&tail, p + i, rem);
    seed = mum64(seed ^ tail, 0xD6E8FEB86659FD93ULL);
  }
  // final avalanche
  seed ^= seed >> 32;
  seed *= 0x9FB21C651E98DF25ULL;
  seed ^= seed >> 29;
  seed *= 0x9E3779B97F4A7C15ULL;
  seed ^= seed >> 32;
  return seed;
}

// Siphash-like toy (not a compliant SipHash; only a “heavier” baseline)
static uint64_t hash_toy_sip(const void* data, size_t n) {
  const uint8_t* p = (const uint8_t*)data;

  uint64_t v0 = 0x736f6d6570736575ULL;
  uint64_t v1 = 0x646f72616e646f6dULL;
  uint64_t v2 = 0x6c7967656e657261ULL;
  uint64_t v3 = 0x7465646279746573ULL;

  uint64_t k0 = 0x0706050403020100ULL;
  uint64_t k1 = 0x0f0e0d0c0b0a0908ULL;
  v0 ^= k0; v1 ^= k1; v2 ^= k0; v3 ^= k1;

  auto void sip_round(void) {
    v0 += v1; v2 += v3;
    v1 = rotl64(v1, 13); v3 = rotl64(v3, 16);
    v1 ^= v0; v3 ^= v2;
    v0 = rotl64(v0, 32);
    v2 += v1; v0 += v3;
    v1 = rotl64(v1, 17); v3 = rotl64(v3, 21);
    v1 ^= v2; v3 ^= v0;
    v2 = rotl64(v2, 32);
  }

  size_t i = 0;
  while (i + 8 <= n) {
    uint64_t m;
    memcpy(&m, p + i, 8);
    v3 ^= m;
    sip_round(); sip_round(); // 2 rounds
    v0 ^= m;
    i += 8;
  }

  uint64_t b = (uint64_t)n << 56;
  size_t rem = n - i;
  for (size_t r = 0; r < rem; r++) b |= ((uint64_t)p[i + r]) << (8 * r);

  v3 ^= b;
  sip_round(); sip_round();
  v0 ^= b;

  v2 ^= 0xff;
  sip_round(); sip_round(); sip_round(); sip_round(); // 4 rounds
  return (v0 ^ v1) ^ (v2 ^ v3);
}

// ======================================================================================
// Bench runners (hash on fixed buffers)
// ======================================================================================

typedef uint64_t (*hash_fn_t)(const void*, size_t);

static void bm_hash_run(bench_ctx_t* ctx, void* state, uint64_t iters,
                        const uint8_t* buf, size_t n, hash_fn_t fn) {
  (void)ctx;
  bm_hash_state_t* st = (bm_hash_state_t*)state;
  uint64_t acc = st->sink;

  for (uint64_t i = 0; i < iters; i++) {
    acc ^= fn(buf, n) + 0x9E3779B97F4A7C15ULL * (i + 1);
    acc = rotl64(acc, 17);
  }

  st->sink = acc;
  bench_blackhole_u64(acc);
}

#define DEF_BENCH(NAME, BUF_FIELD, SIZE, FN)                                    \
  static void NAME(bench_ctx_t* ctx, void* state, uint64_t iters) {             \
    bm_hash_state_t* st = (bm_hash_state_t*)state;                              \
    bm_hash_run(ctx, state, iters, st->BUF_FIELD, (SIZE), (FN));                \
  }

DEF_BENCH(bm_hash_fnv_32,   buf32,  32,    hash_fnv1a64)
DEF_BENCH(bm_hash_fnv_256,  buf256, 256,   hash_fnv1a64)
DEF_BENCH(bm_hash_fnv_4k,   buf4k,  4096,  hash_fnv1a64)
DEF_BENCH(bm_hash_fnv_64k,  buf64k, 65536, hash_fnv1a64)

DEF_BENCH(bm_hash_mix_32,   buf32,  32,    hash_mix64)
DEF_BENCH(bm_hash_mix_256,  buf256, 256,   hash_mix64)
DEF_BENCH(bm_hash_mix_4k,   buf4k,  4096,  hash_mix64)
DEF_BENCH(bm_hash_mix_64k,  buf64k, 65536, hash_mix64)

DEF_BENCH(bm_hash_toy_32,   buf32,  32,    hash_toy_sip)
DEF_BENCH(bm_hash_toy_256,  buf256, 256,   hash_toy_sip)
DEF_BENCH(bm_hash_toy_4k,   buf4k,  4096,  hash_toy_sip)
DEF_BENCH(bm_hash_toy_64k,  buf64k, 65536, hash_toy_sip)

// ======================================================================================
// Registration strategy
// ======================================================================================
//
// Pour éviter les duplicates bench_register_std si tu as déjà d’autres TUs,
// on expose une fonction dédiée bm_hash_register().
// Tu peux l’appeler depuis un TU central.
//
// Si tu n’as PAS de TU central, tu peux définir BENCH_HASH_DEFINE_BENCH_REGISTER_STD=1
// pour que ce fichier fournisse bench_register_std().

void bm_hash_register(bench_registry_t* r) {
  // FNV-1a
  bench_register_case(r, (bench_case_t){
    .name="micro.hash.fnv1a.32B", .description="FNV-1a 64 on 32B buffer",
    .setup=bm_hash_setup, .teardown=bm_hash_teardown, .run=bm_hash_fnv_32, .flags=0u
  });
  bench_register_case(r, (bench_case_t){
    .name="micro.hash.fnv1a.256B", .description="FNV-1a 64 on 256B buffer",
    .setup=bm_hash_setup, .teardown=bm_hash_teardown, .run=bm_hash_fnv_256, .flags=0u
  });
  bench_register_case(r, (bench_case_t){
    .name="micro.hash.fnv1a.4KB", .description="FNV-1a 64 on 4KB buffer",
    .setup=bm_hash_setup, .teardown=bm_hash_teardown, .run=bm_hash_fnv_4k, .flags=0u
  });
  bench_register_case(r, (bench_case_t){
    .name="micro.hash.fnv1a.64KB", .description="FNV-1a 64 on 64KB buffer",
    .setup=bm_hash_setup, .teardown=bm_hash_teardown, .run=bm_hash_fnv_64k, .flags=0u
  });

  // Mix64 (fast)
  bench_register_case(r, (bench_case_t){
    .name="micro.hash.mix64.32B", .description="mix64 (wyhash-inspired) on 32B buffer",
    .setup=bm_hash_setup, .teardown=bm_hash_teardown, .run=bm_hash_mix_32, .flags=0u
  });
  bench_register_case(r, (bench_case_t){
    .name="micro.hash.mix64.256B", .description="mix64 (wyhash-inspired) on 256B buffer",
    .setup=bm_hash_setup, .teardown=bm_hash_teardown, .run=bm_hash_mix_256, .flags=0u
  });
  bench_register_case(r, (bench_case_t){
    .name="micro.hash.mix64.4KB", .description="mix64 (wyhash-inspired) on 4KB buffer",
    .setup=bm_hash_setup, .teardown=bm_hash_teardown, .run=bm_hash_mix_4k, .flags=0u
  });
  bench_register_case(r, (bench_case_t){
    .name="micro.hash.mix64.64KB", .description="mix64 (wyhash-inspired) on 64KB buffer",
    .setup=bm_hash_setup, .teardown=bm_hash_teardown, .run=bm_hash_mix_64k, .flags=0u
  });

  // Toy Sip-like (heavier)
  bench_register_case(r, (bench_case_t){
    .name="micro.hash.toy_sip.32B", .description="toy sip-like on 32B buffer",
    .setup=bm_hash_setup, .teardown=bm_hash_teardown, .run=bm_hash_toy_32, .flags=0u
  });
  bench_register_case(r, (bench_case_t){
    .name="micro.hash.toy_sip.256B", .description="toy sip-like on 256B buffer",
    .setup=bm_hash_setup, .teardown=bm_hash_teardown, .run=bm_hash_toy_256, .flags=0u
  });
  bench_register_case(r, (bench_case_t){
    .name="micro.hash.toy_sip.4KB", .description="toy sip-like on 4KB buffer",
    .setup=bm_hash_setup, .teardown=bm_hash_teardown, .run=bm_hash_toy_4k, .flags=0u
  });
  bench_register_case(r, (bench_case_t){
    .name="micro.hash.toy_sip.64KB", .description="toy sip-like on 64KB buffer",
    .setup=bm_hash_setup, .teardown=bm_hash_teardown, .run=bm_hash_toy_64k, .flags=0u
  });
}

#ifndef BENCH_HASH_DEFINE_BENCH_REGISTER_STD
  #define BENCH_HASH_DEFINE_BENCH_REGISTER_STD 0
#endif

#if BENCH_HASH_DEFINE_BENCH_REGISTER_STD
void bench_register_std(bench_registry_t* r) {
  bm_hash_register(r);
}
#endif
