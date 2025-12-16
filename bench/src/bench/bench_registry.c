// File: bench/src/bench/bench_registry.c
// Vitte benchmark registry — "max" (C17/C23).
//
// Objectifs :
//   - API stable pour enregistrer des bench cases depuis n'importe quel module.
//   - Support de deux modes d'enregistrement :
//       (A) explicite (portable) via bench_register_<module>() appelées par bench_register_all()
//       (B) auto-registration (optionnel) via constructors (GCC/Clang) -> BENCH_ENABLE_AUTOREG=1
//   - Aucun header strictement requis, mais conçu pour matcher bench/src/bench/bench.h.
//
// Garanties :
//   - Pas de noms dupliqués (fail-fast).
//   - Capacité auto-growth (realloc).
//   - Auto-reg drain dans un ordre déterministe best-effort (reverse list).
//
// Notes MSVC :
//   - L’auto-reg via constructors n’est pas portable; ce TU l’annonce mais n’implémente pas
//     le système de sections MSVC (trop verbeux). Sur Windows, privilégier l'enregistrement explicite.
//
// Fichiers associés :
//   - bench.h         : API publique
//   - bench_main.c    : runner
//   - bench_time.c    : horloge/sleep/pin
//   - bench_stats.c   : stats/quantiles/outliers
//
// Build knobs :
//   -DBENCH_ENABLE_AUTOREG=1 : active auto-registration (GCC/Clang).
//   -DBENCH_NO_AUTOREG=1     : disable macro/auto-reg even if enabled.
//
// -----------------------------------------------------------------------------
// Minimal explicit usage (portable):
//
//   // bench_runtime.c
//   #include "bench/bench.h"
//
//   static void run(bench_ctx_t* ctx, void* st, uint64_t iters) { ... }
//
//   void bench_register_runtime(bench_registry_t* r) {
//     bench_register_case(r, (bench_case_t){ .name="runtime.foo", .run=run });
//   }
//
// Runner:
//   bench_registry_api_init(&reg);
//   bench_register_all(&reg);
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "bench/bench.h"

// ======================================================================================
// OOM helpers (fail-fast)
// ======================================================================================

static void* bench_xmalloc(size_t n) {
  void* p = malloc(n);
  if (!p) {
    fprintf(stderr, "bench_registry: OOM (%zu bytes)\n", n);
    exit(1);
  }
  return p;
}

static void* bench_xrealloc(void* p, size_t n) {
  void* q = realloc(p, n);
  if (!q) {
    fprintf(stderr, "bench_registry: OOM (%zu bytes)\n", n);
    exit(1);
  }
  return q;
}

static bool bench_streq(const char* a, const char* b) {
  if (a == b) return true;
  if (!a || !b) return false;
  return strcmp(a, b) == 0;
}

// ======================================================================================
// Registry core API
// ======================================================================================

void bench_registry_api_init(bench_registry_t* r) {
  if (!r) return;
  r->cases = NULL;
  r->len = 0;
  r->cap = 0;
}

void bench_registry_api_free(bench_registry_t* r) {
  if (!r) return;
  free(r->cases);
  r->cases = NULL;
  r->len = 0;
  r->cap = 0;
}

static void bench_registry_ensure_cap(bench_registry_t* r, size_t need) {
  if (need <= r->cap) return;
  size_t cap = (r->cap == 0) ? 32 : r->cap;
  while (cap < need) cap *= 2;
  r->cases = (bench_case_t*)bench_xrealloc(r->cases, cap * sizeof(bench_case_t));
  r->cap = cap;
}

void bench_registry_api_add(bench_registry_t* r, bench_case_t c) {
  if (!r) return;

  if (!c.name || c.name[0] == '\0') {
    fprintf(stderr, "bench_registry: invalid case: empty name\n");
    exit(2);
  }
  if (!c.run) {
    fprintf(stderr, "bench_registry: invalid case '%s': run is NULL\n", c.name);
    exit(2);
  }

  // duplicate guard
  for (size_t i = 0; i < r->len; i++) {
    if (bench_streq(r->cases[i].name, c.name)) {
      fprintf(stderr, "bench_registry: duplicate benchmark name: %s\n", c.name);
      exit(2);
    }
  }

  bench_registry_ensure_cap(r, r->len + 1);
  r->cases[r->len++] = c;
}

// ======================================================================================
// Auto-registration (optional)
// ======================================================================================

#ifndef BENCH_ENABLE_AUTOREG
  #define BENCH_ENABLE_AUTOREG 0
#endif

#ifndef BENCH_NO_AUTOREG
  #define BENCH_NO_AUTOREG 0
#endif

typedef struct bench_autoreg_node {
  bench_case_t c;
  struct bench_autoreg_node* next;
} bench_autoreg_node_t;

static bench_autoreg_node_t* g_autoreg_head = NULL;

static void bench_autoreg_push(bench_case_t c) {
  bench_autoreg_node_t* n = (bench_autoreg_node_t*)bench_xmalloc(sizeof(*n));
  n->c = c;
  n->next = g_autoreg_head;
  g_autoreg_head = n;
}

void bench_autoreg_register_case(const bench_case_t* c) {
  if (!c) return;
  // Shallow copy: strings/pointers are expected to live forever (static storage).
  bench_autoreg_push(*c);
}

static void bench_autoreg_drain_into(bench_registry_t* r) {
  // reverse list to stabilize order (best-effort deterministic)
  bench_autoreg_node_t* rev = NULL;
  while (g_autoreg_head) {
    bench_autoreg_node_t* n = g_autoreg_head;
    g_autoreg_head = n->next;
    n->next = rev;
    rev = n;
  }
  while (rev) {
    bench_autoreg_node_t* n = rev;
    rev = n->next;
    bench_registry_api_add(r, n->c);
    free(n);
  }
}

// ======================================================================================
// Weak module hooks (explicit registration; portable)
// ======================================================================================

#if defined(__GNUC__) || defined(__clang__)
  #define BENCH_WEAK __attribute__((weak))
#else
  #define BENCH_WEAK
#endif

// These are OPTIONAL. A module can provide any subset.
// bench_register_all() calls them if present.
BENCH_WEAK void bench_register_runtime(bench_registry_t* r);
BENCH_WEAK void bench_register_compiler(bench_registry_t* r);
BENCH_WEAK void bench_register_std(bench_registry_t* r);
BENCH_WEAK void bench_register_vm(bench_registry_t* r);
BENCH_WEAK void bench_register_muf(bench_registry_t* r);
BENCH_WEAK void bench_register_regex(bench_registry_t* r);
BENCH_WEAK void bench_register_unicode(bench_registry_t* r);

// ======================================================================================
// bench_register_all (symbol expected by runner)
// ======================================================================================

void bench_register_all(bench_registry_t* registry) {
  if (!registry) return;

  // Defensive init if user forgot (runner does init already).
  if (registry->cases == NULL && registry->len == 0 && registry->cap == 0) {
    bench_registry_api_init(registry);
  }

#if (BENCH_ENABLE_AUTOREG && !BENCH_NO_AUTOREG)
  bench_autoreg_drain_into(registry);
#endif

  if (bench_register_runtime)  bench_register_runtime(registry);
  if (bench_register_compiler) bench_register_compiler(registry);
  if (bench_register_std)      bench_register_std(registry);
  if (bench_register_vm)       bench_register_vm(registry);
  if (bench_register_muf)      bench_register_muf(registry);
  if (bench_register_regex)    bench_register_regex(registry);
  if (bench_register_unicode)  bench_register_unicode(registry);
}

// ======================================================================================
// Optional: validation / debug
// ======================================================================================

static bool bench_name_is_reasonable(const char* s) {
  // allow: [A-Za-z0-9._-] with at least one dot recommended (namespace.case)
  if (!s || !s[0]) return false;
  bool has_dot = false;
  for (const char* p = s; *p; p++) {
    char c = *p;
    if (c == '.') { has_dot = true; continue; }
    if (c == '_' || c == '-' ) continue;
    if (c >= '0' && c <= '9') continue;
    if (c >= 'A' && c <= 'Z') continue;
    if (c >= 'a' && c <= 'z') continue;
    return false;
  }
  return has_dot;
}

void bench_registry_debug_validate(const bench_registry_t* r) {
  if (!r) return;
  for (size_t i = 0; i < r->len; i++) {
    const bench_case_t* c = &r->cases[i];
    if (!c->name || !c->run) {
      fprintf(stderr, "bench_registry: invalid entry at %zu\n", i);
      abort();
    }
    if (!bench_name_is_reasonable(c->name)) {
      fprintf(stderr, "bench_registry: suspicious name: '%s' (expected namespace.case)\n", c->name);
    }
  }
}
