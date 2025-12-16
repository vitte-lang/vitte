// File: bench/src/macro/bm_json_parse.c
// Benchmark: JSON parsing (Vitte) — "max" benchmark TU (C17/C23).
//
// Objectif :
//   - Mesurer un parseur JSON (ex: vitte/json) sur plusieurs tailles de payloads.
//   - Prend en charge 2 modes :
//       (A) API réelle si disponible (define BENCH_HAVE_VITTE_JSON=1 + include + hooks)
//       (B) fallback benchmark (scan + validate minimal) si API absente
//
// Intégration :
//   - Enregistrement explicite recommandé via bench_register_std/bench_register_runtime/etc.
//   - Auto-reg possible si BENCH_ENABLE_AUTOREG=1 + GCC/Clang.
//
// IMPORTANT :
//   - Tu dois adapter les hooks "VITTE JSON API" ci-dessous à ton impl réelle.
//   - Les payloads sont générés en mémoire (pas I/O) pour bench stable.
//   - Le bench évite mallocs dans la boucle hot (state pré-alloué).

#include "bench/bench.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// ======================================================================================
// Vitte JSON API hook (à adapter)
// ======================================================================================
//
// Pour brancher un vrai parseur :
//   - définir BENCH_HAVE_VITTE_JSON=1
//   - fournir les fonctions suivantes, ou adapter les wrappers.
// Exemple d’API possible:
//   typedef struct vitte_json_doc vitte_json_doc_t;
//   bool vitte_json_parse(vitte_json_doc_t* out, const char* s, size_t n);
//   void vitte_json_doc_free(vitte_json_doc_t* doc);
//
// Ici, on définit une ABI minimale "wrapper" local.

#ifndef BENCH_HAVE_VITTE_JSON
  #define BENCH_HAVE_VITTE_JSON 0
#endif

#if BENCH_HAVE_VITTE_JSON
  // TODO: include tes headers réels ici.
  // #include "vitte/json/json.h"

  // ---- EXEMPLE WRAPPERS (à remplacer) ----
  typedef struct vitte_json_doc {
    void* _opaque;
  } vitte_json_doc_t;

  static bool vitte_json_parse_wrapper(vitte_json_doc_t* out, const char* s, size_t n) {
    (void)out; (void)s; (void)n;
    // return vitte_json_parse(out, s, n);
    return false;
  }

  static void vitte_json_doc_free_wrapper(vitte_json_doc_t* doc) {
    (void)doc;
    // vitte_json_doc_free(doc);
  }
#endif

// ======================================================================================
// Fallback parser (validation minimaliste) — stable & deterministic
// ======================================================================================
//
// But : approx CPU cost du "parse" si API réelle non branchée.
// On fait :
//   - scan des tokens JSON (strings, numbers, punctuation)
//   - validation basique des guillemets/escapes + stack profondeur {}
// (Ceci ne remplace pas un vrai parse, mais donne un bench reproductible.)

typedef struct json_scan_state {
  const char* s;
  size_t n;
  size_t i;
  int depth_obj;
  int depth_arr;
  uint64_t checksum;
} json_scan_state_t;

static inline bool is_ws(char c) {
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static inline bool is_digit(char c) { return (c >= '0' && c <= '9'); }

static bool scan_string(json_scan_state_t* st) {
  // st->s[st->i] == '"'
  st->i++; // skip "
  while (st->i < st->n) {
    char c = st->s[st->i++];
    st->checksum = (st->checksum * 1315423911u) ^ (uint8_t)c;

    if (c == '"') return true;
    if (c == '\\') {
      if (st->i >= st->n) return false;
      char e = st->s[st->i++];
      st->checksum ^= (uint8_t)e * 2654435761u;
      // minimal validation of escapes
      switch (e) {
        case '"': case '\\': case '/': case 'b': case 'f': case 'n': case 'r': case 't':
          break;
        case 'u': {
          // 4 hex digits
          for (int k = 0; k < 4; k++) {
            if (st->i >= st->n) return false;
            char h = st->s[st->i++];
            bool ok = (h >= '0' && h <= '9') || (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F');
            if (!ok) return false;
            st->checksum ^= (uint8_t)h + (uint64_t)k;
          }
        } break;
        default:
          return false;
      }
    }
  }
  return false;
}

static bool scan_number(json_scan_state_t* st) {
  // minimal number scan: -? (0|[1-9][0-9]*) (\.[0-9]+)? ([eE][+-]?[0-9]+)?
  size_t start = st->i;
  if (st->s[st->i] == '-') st->i++;
  if (st->i >= st->n) return false;

  if (st->s[st->i] == '0') {
    st->i++;
  } else {
    if (!is_digit(st->s[st->i])) return false;
    while (st->i < st->n && is_digit(st->s[st->i])) st->i++;
  }

  if (st->i < st->n && st->s[st->i] == '.') {
    st->i++;
    if (st->i >= st->n || !is_digit(st->s[st->i])) return false;
    while (st->i < st->n && is_digit(st->s[st->i])) st->i++;
  }

  if (st->i < st->n && (st->s[st->i] == 'e' || st->s[st->i] == 'E')) {
    st->i++;
    if (st->i < st->n && (st->s[st->i] == '+' || st->s[st->i] == '-')) st->i++;
    if (st->i >= st->n || !is_digit(st->s[st->i])) return false;
    while (st->i < st->n && is_digit(st->s[st->i])) st->i++;
  }

  // update checksum
  for (size_t k = start; k < st->i; k++) st->checksum = (st->checksum * 16777619u) ^ (uint8_t)st->s[k];
  return true;
}

static bool scan_literal(json_scan_state_t* st, const char* lit) {
  size_t m = strlen(lit);
  if (st->i + m > st->n) return false;
  if (memcmp(st->s + st->i, lit, m) != 0) return false;
  for (size_t k = 0; k < m; k++) st->checksum = (st->checksum * 1099511628211ULL) ^ (uint8_t)lit[k];
  st->i += m;
  return true;
}

static bool json_scan_validate(const char* s, size_t n, uint64_t* out_checksum) {
  json_scan_state_t st;
  st.s = s; st.n = n; st.i = 0;
  st.depth_obj = 0;
  st.depth_arr = 0;
  st.checksum = 1469598103934665603ULL;

  while (st.i < st.n) {
    char c = st.s[st.i];
    if (is_ws(c)) { st.i++; continue; }

    switch (c) {
      case '{': st.depth_obj++; st.checksum ^= 0xA1; st.i++; break;
      case '}': st.depth_obj--; st.checksum ^= 0xA2; st.i++; if (st.depth_obj < 0) return false; break;
      case '[': st.depth_arr++; st.checksum ^= 0xB1; st.i++; break;
      case ']': st.depth_arr--; st.checksum ^= 0xB2; st.i++; if (st.depth_arr < 0) return false; break;
      case ':': st.checksum ^= 0xC1; st.i++; break;
      case ',': st.checksum ^= 0xC2; st.i++; break;
      case '"':
        if (!scan_string(&st)) return false;
        break;
      case 't':
        if (!scan_literal(&st, "true")) return false;
        break;
      case 'f':
        if (!scan_literal(&st, "false")) return false;
        break;
      case 'n':
        if (!scan_literal(&st, "null")) return false;
        break;
      default:
        if (c == '-' || is_digit(c)) {
          if (!scan_number(&st)) return false;
        } else {
          return false;
        }
        break;
    }
  }

  if (st.depth_obj != 0 || st.depth_arr != 0) return false;
  if (out_checksum) *out_checksum = st.checksum;
  return true;
}

// ======================================================================================
// Payload generator (no I/O; deterministic)
// ======================================================================================

typedef struct bm_json_state {
  char* payload_small;
  size_t small_n;

  char* payload_med;
  size_t med_n;

  char* payload_large;
  size_t large_n;

#if BENCH_HAVE_VITTE_JSON
  vitte_json_doc_t doc;
#endif

  uint64_t sink; // blackhole
} bm_json_state_t;

static void append_str(char** buf, size_t* len, size_t* cap, const char* s) {
  size_t m = strlen(s);
  if (*len + m + 1 > *cap) {
    size_t nc = (*cap == 0) ? 1024 : *cap;
    while (*len + m + 1 > nc) nc *= 2;
    *buf = (char*)realloc(*buf, nc);
    if (!*buf) { fprintf(stderr, "bm_json_parse: OOM\n"); exit(1); }
    *cap = nc;
  }
  memcpy(*buf + *len, s, m);
  *len += m;
  (*buf)[*len] = '\0';
}

static char* build_payload(size_t items, size_t str_len, size_t* out_n) {
  char* buf = NULL;
  size_t len = 0, cap = 0;

  append_str(&buf, &len, &cap, "{ \"items\": [");

  for (size_t i = 0; i < items; i++) {
    char tmp[256];
#if defined(_MSC_VER)
    _snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "{\"id\":%llu,\"name\":\"", (unsigned long long)i);
#else
    snprintf(tmp, sizeof(tmp), "{\"id\":%llu,\"name\":\"", (unsigned long long)i);
#endif
    append_str(&buf, &len, &cap, tmp);

    // deterministic string content
    for (size_t k = 0; k < str_len; k++) {
      char c = (char)('a' + (int)((i + k) % 26));
      char one[2] = { c, 0 };
      append_str(&buf, &len, &cap, one);
    }

    append_str(&buf, &len, &cap, "\",\"ok\":true,\"v\":");
#if defined(_MSC_VER)
    _snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%llu", (unsigned long long)(i * 1315423911ULL));
#else
    snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)(i * 1315423911ULL));
#endif
    append_str(&buf, &len, &cap, tmp);

    append_str(&buf, &len, &cap, "}");

    if (i + 1 < items) append_str(&buf, &len, &cap, ",");
  }

  append_str(&buf, &len, &cap, "], \"meta\": {\"count\": ");
  {
    char tmp[64];
#if defined(_MSC_VER)
    _snprintf_s(tmp, sizeof(tmp), _TRUNCATE, "%llu", (unsigned long long)items);
#else
    snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)items);
#endif
    append_str(&buf, &len, &cap, tmp);
  }
  append_str(&buf, &len, &cap, ", \"tag\": \"bench\" } }");

  if (out_n) *out_n = len;
  return buf;
}

// ======================================================================================
// Bench setup/teardown
// ======================================================================================

static void* bm_json_setup(bench_ctx_t* ctx) {
  (void)ctx;
  bm_json_state_t* st = (bm_json_state_t*)calloc(1, sizeof(*st));
  if (!st) { fprintf(stderr, "bm_json_parse: OOM\n"); exit(1); }

  // Tune sizes to get representative overhead:
  // small: ~1-5KB, medium: ~50-150KB, large: ~1-3MB (depending on str_len)
  st->payload_small = build_payload(64, 16, &st->small_n);
  st->payload_med   = build_payload(2048, 16, &st->med_n);
  st->payload_large = build_payload(32768, 8, &st->large_n);

  // sanity: fallback validator should accept
  uint64_t cs = 0;
  if (!json_scan_validate(st->payload_small, st->small_n, &cs)) { fprintf(stderr, "bm_json_parse: small invalid\n"); exit(2); }
  if (!json_scan_validate(st->payload_med,   st->med_n,   &cs)) { fprintf(stderr, "bm_json_parse: med invalid\n"); exit(2); }
  if (!json_scan_validate(st->payload_large, st->large_n, &cs)) { fprintf(stderr, "bm_json_parse: large invalid\n"); exit(2); }

  st->sink = 0x12345678ULL ^ cs;
  return st;
}

static void bm_json_teardown(bench_ctx_t* ctx, void* state) {
  (void)ctx;
  bm_json_state_t* st = (bm_json_state_t*)state;
  if (!st) return;

#if BENCH_HAVE_VITTE_JSON
  vitte_json_doc_free_wrapper(&st->doc);
#endif

  free(st->payload_small);
  free(st->payload_med);
  free(st->payload_large);
  free(st);
}

// ======================================================================================
// Bench runs
// ======================================================================================

static inline void bench_blackhole_u64(volatile uint64_t x) { (void)x; }

static void bm_json_parse_small(bench_ctx_t* ctx, void* state, uint64_t iters) {
  (void)ctx;
  bm_json_state_t* st = (bm_json_state_t*)state;
  uint64_t acc = st->sink;

  for (uint64_t i = 0; i < iters; i++) {
#if BENCH_HAVE_VITTE_JSON
    bool ok = vitte_json_parse_wrapper(&st->doc, st->payload_small, st->small_n);
    acc ^= (uint64_t)ok + (acc << 1);
#else
    uint64_t cs = 0;
    bool ok = json_scan_validate(st->payload_small, st->small_n, &cs);
    acc ^= (uint64_t)ok + cs;
#endif
  }

  st->sink = acc;
  bench_blackhole_u64(acc);
}

static void bm_json_parse_med(bench_ctx_t* ctx, void* state, uint64_t iters) {
  (void)ctx;
  bm_json_state_t* st = (bm_json_state_t*)state;
  uint64_t acc = st->sink;

  for (uint64_t i = 0; i < iters; i++) {
#if BENCH_HAVE_VITTE_JSON
    bool ok = vitte_json_parse_wrapper(&st->doc, st->payload_med, st->med_n);
    acc ^= (uint64_t)ok + (acc << 1);
#else
    uint64_t cs = 0;
    bool ok = json_scan_validate(st->payload_med, st->med_n, &cs);
    acc ^= (uint64_t)ok + cs;
#endif
  }

  st->sink = acc;
  bench_blackhole_u64(acc);
}

static void bm_json_parse_large(bench_ctx_t* ctx, void* state, uint64_t iters) {
  (void)ctx;
  bm_json_state_t* st = (bm_json_state_t*)state;
  uint64_t acc = st->sink;

  for (uint64_t i = 0; i < iters; i++) {
#if BENCH_HAVE_VITTE_JSON
    bool ok = vitte_json_parse_wrapper(&st->doc, st->payload_large, st->large_n);
    acc ^= (uint64_t)ok + (acc << 1);
#else
    uint64_t cs = 0;
    bool ok = json_scan_validate(st->payload_large, st->large_n, &cs);
    acc ^= (uint64_t)ok + cs;
#endif
  }

  st->sink = acc;
  bench_blackhole_u64(acc);
}

// ======================================================================================
// Registration
// ======================================================================================
//
// Recommandé (portable): choisir un module hook existant, ex bench_register_std().
// Si tu préfères auto-reg: define BENCH_ENABLE_AUTOREG=1 (GCC/Clang) et remplace par BENCH_CASE macros.

void bench_register_std(bench_registry_t* r) {
  // JSON parse – small
  bench_register_case(r, (bench_case_t){
    .name = "macro.json_parse.small",
    .description = "Parse JSON small payload (~few KB)",
    .setup = bm_json_setup,
    .teardown = bm_json_teardown,
    .run = bm_json_parse_small,
    .flags = 0u,
  });

  // JSON parse – medium
  bench_register_case(r, (bench_case_t){
    .name = "macro.json_parse.medium",
    .description = "Parse JSON medium payload (~100 KB)",
    .setup = bm_json_setup,
    .teardown = bm_json_teardown,
    .run = bm_json_parse_med,
    .flags = 0u,
  });

  // JSON parse – large
  bench_register_case(r, (bench_case_t){
    .name = "macro.json_parse.large",
    .description = "Parse JSON large payload (~MBs)",
    .setup = bm_json_setup,
    .teardown = bm_json_teardown,
    .run = bm_json_parse_large,
    .flags = 0u,
  });
}
