#include "bench/bench.h"

/*
  bench_registry.c (max)

  Objectif:
  - Registry simple, stable, triable par id côté outils, extensible sans casser le build.
  - Les nouveaux benches peuvent être ajoutés via les sections guardées ci-dessous.

  Conventions:
  - id: "micro:<name>" ou "macro:<name>"
  - ctx: NULL par défaut (paramétrisation possible plus tard)
*/

/* ----------------------
 * Declarations (existing)
 * ---------------------- */

/* micro */
void bm_add(void* ctx);
void bm_hash(void* ctx);
void bm_memcpy(void* ctx);

/* macro */
void bm_json_parse(void* ctx);

/* ----------------------
 * Optional extension points
 * ----------------------
 *
 * Activer avec:
 *   -DVITTE_BENCH_EXTRA=1
 *
 * Puis fournir les implémentations bm_* correspondantes.
 */
#if defined(VITTE_BENCH_EXTRA)
/* micro extra (provide implementations when enabling the macro) */
/* void bm_strlen(void* ctx); */
/* void bm_memchr(void* ctx); */
/* void bm_branch(void* ctx); */

/* macro extra */
/* void bm_io_read(void* ctx); */
#endif

/*
 * Experimental (opt-in):
 *   -DVITTE_BENCH_EXPERIMENTAL=1
 */
#if defined(VITTE_BENCH_EXPERIMENTAL)
/* void bm_vm_dispatch(void* ctx); */
#endif

/* ----------------------
 * Registry macros
 * ---------------------- */

#define BENCH_MICRO_CASE(ID, FN) { (ID), BENCH_MICRO, (FN), NULL }
#define BENCH_MACRO_CASE(ID, FN) { (ID), BENCH_MACRO, (FN), NULL }

/* ----------------------
 * Registry
 * ---------------------- */

static bench_case g_cases[] = {
  /* core micro */
  BENCH_MICRO_CASE("micro:add",    bm_add),
  BENCH_MICRO_CASE("micro:hash",   bm_hash),
  BENCH_MICRO_CASE("micro:memcpy", bm_memcpy),

  /* core macro */
  BENCH_MACRO_CASE("macro:json_parse", bm_json_parse),

#if defined(VITTE_BENCH_EXTRA)
  /*
    EXTRAS (opt-in):
    - Ajoute ici tes nouveaux cases, une fois leurs fonctions présentes.

    Exemple:
      BENCH_MICRO_CASE("micro:strlen", bm_strlen),
      BENCH_MACRO_CASE("macro:io_read", bm_io_read),
  */
#endif

#if defined(VITTE_BENCH_EXPERIMENTAL)
  /*
    EXPERIMENTAL (opt-in):
    - Bench VM dispatch, decode, etc.
  */
#endif
};

const bench_case* bench_registry_all(int* out_count) {
  if(out_count) *out_count = (int)(sizeof(g_cases) / sizeof(g_cases[0]));
  return g_cases;
}

#undef BENCH_MICRO_CASE
#undef BENCH_MACRO_CASE
