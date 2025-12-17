// C:\Users\vince\Documents\GitHub\vitte\fuzz\include\fuzz\fuzz_trace.h
// fuzz_trace.h — lightweight tracing for fuzzing (C17)
//
// Goals:
//  - cheap, deterministic “breadcrumbs” for triage (without heavy logging)
//  - optional integration points for libFuzzer (trace-cmp / value profiling)
//  - counters + tags + hashing of events (stable across platforms)
//
// Design:
//  - FUZZ_TRACE_TAG("lexer:ident") style tags hashed to u32
//  - FUZZ_TRACE_U32(key, val) to record key/value pairs
//  - FUZZ_TRACE_HIT(id) for fixed sites
//
// By default, tracing is compiled in but mostly no-op unless enabled.
// Define FUZZ_TRACE_ENABLED=1 to keep counters, or FUZZ_TRACE_ENABLED=0 to drop.
//
// Notes:
//  - Do not print from hot paths; use counters. Printing can slow fuzzing.
//  - For libFuzzer, value profiling can be enabled via LLVMFuzzerTestOneInput;
//    but we keep this header toolchain-agnostic.

#ifndef VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_TRACE_H
#define VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_TRACE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include "fuzz_assert.h"
#include "fuzz_util.h"

//------------------------------------------------------------------------------
// Configuration
//------------------------------------------------------------------------------

#ifndef FUZZ_TRACE_ENABLED
// Default ON (cheap). Set to 0 for zero-cost builds.
#define FUZZ_TRACE_ENABLED 1
#endif

#ifndef FUZZ_TRACE_MAX_SITES
// Small fixed table; enough for typical per-target instrumentation.
#define FUZZ_TRACE_MAX_SITES 4096u
#endif

#ifndef FUZZ_TRACE_MAX_KV
#define FUZZ_TRACE_MAX_KV 1024u
#endif

//------------------------------------------------------------------------------
// Trace state
//------------------------------------------------------------------------------

typedef struct fuzz_trace_kv {
  uint32_t key;
  uint32_t value;
  uint32_t hits;
} fuzz_trace_kv;

typedef struct fuzz_trace_state {
#if FUZZ_TRACE_ENABLED
  uint32_t sites[FUZZ_TRACE_MAX_SITES]; // simple hit counters
  fuzz_trace_kv kv[FUZZ_TRACE_MAX_KV];  // small open-addressed kv table
#endif
  uint64_t total_hits;
} fuzz_trace_state;

FUZZ_INLINE static void
fuzz_trace_init(fuzz_trace_state* st) {
  FUZZ_ASSERT(st);
#if FUZZ_TRACE_ENABLED
  for (size_t i = 0; i < FUZZ_TRACE_MAX_SITES; ++i)
    st->sites[i] = 0;
  for (size_t i = 0; i < FUZZ_TRACE_MAX_KV; ++i) {
    st->kv[i].key = 0;
    st->kv[i].value = 0;
    st->kv[i].hits = 0;
  }
#endif
  st->total_hits = 0;
}

// Singleton default state (per-process). For per-thread, embed your own state.
FUZZ_INLINE static fuzz_trace_state*
fuzz_trace_global(void) {
  static fuzz_trace_state g;
  static int inited = 0;
  if (!inited) {
    fuzz_trace_init(&g);
    inited = 1;
  }
  return &g;
}

//------------------------------------------------------------------------------
// Sites: fixed hit counters
//------------------------------------------------------------------------------

FUZZ_INLINE static void
fuzz_trace_hit_u32(fuzz_trace_state* st, uint32_t id) {
  FUZZ_ASSERT(st);
  st->total_hits++;

#if FUZZ_TRACE_ENABLED
  // multiplicative hash to spread ids across table
  uint32_t idx = (uint32_t)((id * 2654435761u) % FUZZ_TRACE_MAX_SITES);
  st->sites[idx] += 1;
#else
  (void)id;
#endif
}

//------------------------------------------------------------------------------
// KV: record key/value patterns (e.g., token kinds, error codes)
//------------------------------------------------------------------------------

FUZZ_INLINE static void
fuzz_trace_kv_u32(fuzz_trace_state* st, uint32_t key, uint32_t value) {
  FUZZ_ASSERT(st);
  st->total_hits++;

#if FUZZ_TRACE_ENABLED
  // open addressing, bounded probes
  uint32_t h = key ^ (value * 16777619u);
  uint32_t idx = (uint32_t)((h * 2654435761u) % FUZZ_TRACE_MAX_KV);

  for (uint32_t probe = 0; probe < 16u; ++probe) {
    uint32_t i = (idx + probe) % FUZZ_TRACE_MAX_KV;

    if (st->kv[i].hits == 0) {
      st->kv[i].key = key ? key : 1u;
      st->kv[i].value = value;
      st->kv[i].hits = 1;
      return;
    }

    if (st->kv[i].key == (key ? key : 1u) && st->kv[i].value == value) {
      st->kv[i].hits += 1;
      return;
    }
  }
#else
  (void)key;
  (void)value;
#endif
}

FUZZ_INLINE static void
fuzz_trace_tag(fuzz_trace_state* st, const char* tag) {
  uint32_t id = fuzz_hash_cstr32(tag);
  fuzz_trace_hit_u32(st, id);
}

FUZZ_INLINE static void
fuzz_trace_tag_kv(fuzz_trace_state* st, const char* key_tag, uint32_t value) {
  uint32_t key = fuzz_hash_cstr32(key_tag);
  fuzz_trace_kv_u32(st, key, value);
}

//------------------------------------------------------------------------------
// Macros (convenient callsites)
//------------------------------------------------------------------------------

#define FUZZ_TRACE_HIT(id_u32)                                                                     \
  do {                                                                                             \
    fuzz_trace_hit_u32(fuzz_trace_global(), (uint32_t)(id_u32));                                   \
  } while (0)

#define FUZZ_TRACE_TAG(tag_cstr)                                                                   \
  do {                                                                                             \
    fuzz_trace_tag(fuzz_trace_global(), (tag_cstr));                                               \
  } while (0)

#define FUZZ_TRACE_U32(key_u32, val_u32)                                                           \
  do {                                                                                             \
    fuzz_trace_kv_u32(fuzz_trace_global(), (uint32_t)(key_u32), (uint32_t)(val_u32));              \
  } while (0)

#define FUZZ_TRACE_TAG_U32(key_tag, val_u32)                                                       \
  do {                                                                                             \
    fuzz_trace_tag_kv(fuzz_trace_global(), (key_tag), (uint32_t)(val_u32));                        \
  } while (0)

//------------------------------------------------------------------------------
// Optional: expose snapshot for debugging (no printing)
//------------------------------------------------------------------------------

typedef struct fuzz_trace_snapshot {
  uint64_t total_hits;
#if FUZZ_TRACE_ENABLED
  const uint32_t* sites;
  size_t sites_len;
  const fuzz_trace_kv* kv;
  size_t kv_len;
#endif
} fuzz_trace_snapshot;

FUZZ_INLINE static fuzz_trace_snapshot
fuzz_trace_snapshot_get(const fuzz_trace_state* st) {
  FUZZ_ASSERT(st);
  fuzz_trace_snapshot s;
  s.total_hits = st->total_hits;
#if FUZZ_TRACE_ENABLED
  s.sites = st->sites;
  s.sites_len = FUZZ_TRACE_MAX_SITES;
  s.kv = st->kv;
  s.kv_len = FUZZ_TRACE_MAX_KV;
#endif
  return s;
}

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_TRACE_H
