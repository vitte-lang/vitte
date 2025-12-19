#include "bench/registry.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct bench_case_entry {
  bench_case_t c;
  char* owned_id;
} bench_case_entry_t;

static bench_case_entry_t* g_cases = NULL;
static int32_t g_count = 0;
static int32_t g_capacity = 0;

static void bench_registry__free_entry(bench_case_entry_t* e) {
  if (!e) return;
  free(e->owned_id);
  e->owned_id = NULL;
  e->c.id = NULL;
  e->c.fn = NULL;
  e->c.ctx = NULL;
  e->c.kind = 0;
}

static int bench_registry__ensure_capacity(int32_t min_cap) {
  if (min_cap <= g_capacity) return 0;
  int32_t new_cap = g_capacity ? g_capacity * 2 : 16;
  while (new_cap < min_cap) {
    if (new_cap > (INT32_MAX / 2)) {
      new_cap = min_cap;
      break;
    }
    new_cap *= 2;
  }

  bench_case_entry_t* new_entries =
      (bench_case_entry_t*)realloc(g_cases, (size_t)new_cap * sizeof(*new_entries));
  if (!new_entries) return -2;

  g_cases = new_entries;
  for (int32_t i = g_capacity; i < new_cap; ++i) {
    g_cases[i].owned_id = NULL;
    g_cases[i].c.id = NULL;
    g_cases[i].c.fn = NULL;
    g_cases[i].c.ctx = NULL;
    g_cases[i].c.kind = 0;
  }
  g_capacity = new_cap;
  return 0;
}

int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx) {
  if (!id || !*id || !fn) return -1;
  if (bench_registry_find(id) >= 0) return -3;

  char* dup = strdup(id);
  if (!dup) return -2;

  if (bench_registry__ensure_capacity(g_count + 1) != 0) {
    free(dup);
    return -2;
  }

  bench_case_entry_t* entry = &g_cases[g_count++];
  entry->owned_id = dup;
  entry->c.id = dup;
  entry->c.kind = kind;
  entry->c.fn = fn;
  entry->c.ctx = ctx;
  return 0;
}

int32_t bench_registry_count(void) {
  return g_count;
}

const bench_case_t* bench_registry_get(int32_t index) {
  if (index < 0 || index >= g_count) return NULL;
  return &g_cases[index].c;
}

int32_t bench_registry_find(const char* id) {
  if (!id) return -1;
  for (int32_t i = 0; i < g_count; ++i) {
    if (g_cases[i].c.id && strcmp(g_cases[i].c.id, id) == 0) {
      return i;
    }
  }
  return -1;
}

void bench_registry_clear(void) {
  for (int32_t i = 0; i < g_count; ++i) {
    bench_registry__free_entry(&g_cases[i]);
  }
  g_count = 0;
}

void bench_registry_shutdown(void) {
  bench_registry_clear();
  free(g_cases);
  g_cases = NULL;
  g_capacity = 0;
}

int bench_registry_foreach(bench_registry_iter_fn it, void* user) {
  if (!it) return -1;
  for (int32_t i = 0; i < g_count; ++i) {
    int rc = it(&g_cases[i].c, user);
    if (rc != 0) return rc;
  }
  return 0;
}
