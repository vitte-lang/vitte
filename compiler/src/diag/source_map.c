#include "vittec/diag/source_map.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void vittec_source_map_init(vittec_source_map_t* sm) {
  if (!sm) return;
  sm->files = NULL;
  sm->len = 0;
  sm->cap = 0;
}

static void vittec_source_map_free_file(vittec_source_file_t* f) {
  if (!f) return;
  if (f->owns_data && f->data) free((void*)f->data);
  free(f->line_starts);
  f->data = NULL;
  f->line_starts = NULL;
  f->line_count = 0;
  f->len = 0;
  f->owns_data = 0;
}

void vittec_source_map_free(vittec_source_map_t* sm) {
  if (!sm) return;
  for (uint32_t i = 0; i < sm->len; i++) {
    vittec_source_map_free_file(&sm->files[i]);
  }
  free(sm->files);
  sm->files = NULL;
  sm->len = 0;
  sm->cap = 0;
}

static int vittec_source_map_reserve(vittec_source_map_t* sm, uint32_t add) {
  if (!sm) return 0;
  uint32_t need = sm->len + add;
  if (need <= sm->cap) return 1;
  uint32_t cap = sm->cap ? sm->cap * 2u : 4u;
  if (cap < need) cap = need;
  vittec_source_file_t* files = (vittec_source_file_t*)realloc(sm->files, (size_t)cap * sizeof(*files));
  if (!files) return 0;
  sm->files = files;
  sm->cap = cap;
  return 1;
}

static uint32_t* vittec_build_line_starts(const uint8_t* data, uint32_t len, uint32_t* out_count) {
  if (out_count) *out_count = 0;
  if (!data && len) return NULL;
  uint32_t capacity = 16;
  uint32_t count = 0;
  uint32_t* starts = (uint32_t*)malloc((size_t)capacity * sizeof(*starts));
  if (!starts) return NULL;
  starts[count++] = 0;
  for (uint32_t i = 0; i < len; i++) {
    if (data[i] == '\n') {
      if (count == capacity) {
        capacity *= 2u;
        uint32_t* tmp = (uint32_t*)realloc(starts, (size_t)capacity * sizeof(*tmp));
        if (!tmp) {
          free(starts);
          return NULL;
        }
        starts = tmp;
      }
      starts[count++] = i + 1u;
    } else if (data[i] == '\r') {
      if (i + 1 < len && data[i + 1] == '\n') continue;
      if (count == capacity) {
        capacity *= 2u;
        uint32_t* tmp = (uint32_t*)realloc(starts, (size_t)capacity * sizeof(*tmp));
        if (!tmp) {
          free(starts);
          return NULL;
        }
        starts = tmp;
      }
      starts[count++] = i + 1u;
    }
  }
  if (out_count) *out_count = count;
  return starts;
}

static int vittec_source_map_push_file(
  vittec_source_map_t* sm,
  vittec_sv_t path,
  const uint8_t* data,
  uint32_t len,
  int owns_data,
  vittec_file_id_t* out_id
) {
  if (!sm || (!data && len)) return VITTEC_SM_EINVAL;
  if (!vittec_source_map_reserve(sm, 1u)) return VITTEC_SM_EOOM;
  vittec_source_file_t* file = &sm->files[sm->len];
  memset(file, 0, sizeof(*file));
  file->path = path;
  file->data = data;
  file->len = len;
  file->owns_data = owns_data ? 1u : 0u;
  file->line_starts = vittec_build_line_starts(data, len, &file->line_count);
  if (len && !file->line_starts) {
    if (owns_data && data) free((void*)data);
    return VITTEC_SM_EOOM;
  }
  vittec_file_id_t id = sm->len;
  sm->len++;
  if (out_id) *out_id = id;
  return VITTEC_SM_OK;
}

int vittec_source_map_add_memory(
  vittec_source_map_t* sm,
  vittec_sv_t path,
  const void* data,
  size_t len,
  int copy,
  vittec_file_id_t* out_id
) {
  if (!sm) return VITTEC_SM_EINVAL;
  const uint8_t* bytes = (const uint8_t*)data;
  uint8_t* owned = NULL;
  if (copy && len) {
    owned = (uint8_t*)malloc(len);
    if (!owned) return VITTEC_SM_EOOM;
    memcpy(owned, bytes, len);
    bytes = owned;
  }
  return vittec_source_map_push_file(sm, path, bytes, (uint32_t)len, copy, out_id);
}

int vittec_source_map_add_path(vittec_source_map_t* sm, const char* path, vittec_file_id_t* out_id) {
  if (!sm || !path) return VITTEC_SM_EINVAL;
  FILE* f = fopen(path, "rb");
  if (!f) return VITTEC_SM_EIO;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  if (n < 0) { fclose(f); return VITTEC_SM_EIO; }
  fseek(f, 0, SEEK_SET);
  uint8_t* data = NULL;
  if (n > 0) {
    data = (uint8_t*)malloc((size_t)n);
    if (!data) { fclose(f); return VITTEC_SM_EOOM; }
    size_t rd = fread(data, 1, (size_t)n, f);
    if (rd != (size_t)n) {
      free(data);
      fclose(f);
      return VITTEC_SM_EIO;
    }
  }
  fclose(f);
  vittec_sv_t path_sv = vittec_sv(path, (uint64_t)strlen(path));
  return vittec_source_map_push_file(sm, path_sv, data, (uint32_t)n, 1, out_id);
}

const vittec_source_file_t* vittec_source_map_get_file(const vittec_source_map_t* sm, vittec_file_id_t file) {
  if (!sm || file >= sm->len) return NULL;
  return &sm->files[file];
}

vittec_sv_t vittec_source_map_file_path(const vittec_source_map_t* sm, vittec_file_id_t file) {
  const vittec_source_file_t* f = vittec_source_map_get_file(sm, file);
  if (!f) return vittec_sv(NULL, 0);
  return f->path;
}

const uint8_t* vittec_source_map_file_data(const vittec_source_map_t* sm, vittec_file_id_t file, uint32_t* out_len) {
  const vittec_source_file_t* f = vittec_source_map_get_file(sm, file);
  if (!f) {
    if (out_len) *out_len = 0;
    return NULL;
  }
  if (out_len) *out_len = f->len;
  return f->data;
}

static uint32_t vittec_source_map_find_line(const vittec_source_file_t* f, uint32_t offset) {
  if (!f || f->line_count == 0) return 0;
  uint32_t lo = 0;
  uint32_t hi = f->line_count;
  while (lo + 1 < hi) {
    uint32_t mid = lo + (hi - lo) / 2u;
    if (f->line_starts[mid] <= offset) {
      lo = mid;
    } else {
      hi = mid;
    }
  }
  return lo;
}

vittec_line_col_t vittec_source_map_line_col(const vittec_source_map_t* sm, vittec_file_id_t file, uint32_t offset) {
  vittec_line_col_t lc = {1u, 1u};
  const vittec_source_file_t* f = vittec_source_map_get_file(sm, file);
  if (!f) return lc;
  if (offset > f->len) offset = f->len;
  if (f->line_count == 0) return lc;
  uint32_t line_idx = vittec_source_map_find_line(f, offset);
  lc.line = line_idx + 1u;
  uint32_t line_start = f->line_starts[line_idx];
  lc.col = (offset - line_start) + 1u;
  return lc;
}

uint32_t vittec_source_map_line_index(const vittec_source_map_t* sm, vittec_file_id_t file, uint32_t offset) {
  const vittec_source_file_t* f = vittec_source_map_get_file(sm, file);
  if (!f) return UINT32_MAX;
  if (offset > f->len) offset = f->len;
  if (f->line_count == 0) return 0;
  return vittec_source_map_find_line(f, offset);
}

int vittec_source_map_line_span(const vittec_source_map_t* sm, vittec_file_id_t file, uint32_t line_index, vittec_span_t* out_span) {
  if (!out_span) return 0;
  const vittec_source_file_t* f = vittec_source_map_get_file(sm, file);
  if (!f || line_index >= f->line_count) return 0;
  uint32_t start = f->line_starts[line_index];
  uint32_t end = (line_index + 1u < f->line_count) ? f->line_starts[line_index + 1u] : f->len;
  while (end > start && (f->data[end - 1] == '\n' || f->data[end - 1] == '\r')) {
    end--;
  }
  *out_span = vittec_span_with_file_id(file, start, end);
  return 1;
}

vittec_sv_t vittec_source_map_line_text(const vittec_source_map_t* sm, vittec_file_id_t file, uint32_t line_index) {
  vittec_span_t sp;
  if (!vittec_source_map_line_span(sm, file, line_index, &sp)) {
    return vittec_sv(NULL, 0);
  }
  const vittec_source_file_t* f = vittec_source_map_get_file(sm, file);
  if (!f) return vittec_sv(NULL, 0);
  return vittec_sv((const char*)f->data + sp.lo, (uint64_t)(sp.hi - sp.lo));
}

int vittec_source_map_build_snippet(
  const vittec_source_map_t* sm,
  vittec_span_t sp,
  uint32_t context_lines,
  vittec_snippet_t* out
) {
  if (!sm || !out) return 0;
  const vittec_source_file_t* f = vittec_source_map_get_file(sm, sp.file);
  if (!f || f->line_count == 0) return 0;
  uint32_t idx = vittec_source_map_find_line(f, sp.lo);
  uint32_t lo = (idx > context_lines) ? (idx - context_lines) : 0u;
  uint32_t hi = idx + context_lines + 1u;
  if (hi > f->line_count) hi = f->line_count;
  out->file = sp.file;
  out->line_lo = lo;
  out->line_hi = hi;
  out->primary_span = sp;
  out->primary = vittec_source_map_line_col(sm, sp.file, sp.lo);
  return 1;
}
