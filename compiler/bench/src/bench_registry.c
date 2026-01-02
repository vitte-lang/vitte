\
/*
  bench_registry.c - registry for benchmark cases (explicit registration)
*/
#include "bench/bench.h"
#include <stdlib.h>
#include <string.h>

typedef struct bench_node {
  bench_case c;
  struct bench_node *next;
} bench_node;

static bench_node *g_head = NULL;
static size_t g_count = 0;

static bool same_case(const bench_case *a, const bench_case *b) {
  return a && b && a->suite && b->suite && a->name && b->name &&
         strcmp(a->suite, b->suite) == 0 && strcmp(a->name, b->name) == 0;
}

void bench_register(const bench_case *c) {
  if (!c || !c->name || !c->suite || !c->fn) return;

  for (bench_node *it = g_head; it; it = it->next) {
    if (same_case(&it->c, c)) return; /* idempotent */
  }

  bench_node *n = (bench_node *)malloc(sizeof(*n));
  if (!n) abort();
  n->c = *c;
  n->next = g_head;
  g_head = n;
  g_count++;
}

static int cmp_case(const void *a, const void *b) {
  const bench_case *A = (const bench_case *)a;
  const bench_case *B = (const bench_case *)b;
  int s = strcmp(A->suite, B->suite);
  if (s) return s;
  return strcmp(A->name, B->name);
}

const bench_case *bench_cases(size_t *count) {
  static bench_case *arr = NULL;
  static size_t arr_cap = 0;

  if (g_count == 0) {
    if (count) *count = 0;
    return NULL;
  }

  if (arr_cap < g_count) {
    free(arr);
    arr = (bench_case *)malloc(sizeof(bench_case) * g_count);
    if (!arr) abort();
    arr_cap = g_count;
  }

  size_t i = 0;
  for (bench_node *n = g_head; n; n = n->next) arr[i++] = n->c;

  qsort(arr, g_count, sizeof(bench_case), cmp_case);

  if (count) *count = g_count;
  return arr;
}
