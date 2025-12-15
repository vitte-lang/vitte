

// source_map.c
// -----------------------------------------------------------------------------
// Diagnostic source mapping utilities.
//
// Goals:
//   - Map a byte offset into (line, column) with stable semantics.
//   - Provide cheap access to line slices for error printing.
//   - Provide a snippet renderer (line(s) + caret) that higher-level diagnostics
//     can use directly.
//
// Conventions:
//   - line/column are 1-based.
//   - offsets are 0-based byte offsets into the source buffer.
//   - column is counted in UTF-8 codepoints (best-effort; invalid bytes count as 1).
//   - tabs can be expanded in snippet rendering (default tab width configurable).
//
// Notes:
//   - This file is self-contained. If you already have a matching header,
//     keep it and align the typedef/function names as needed.
// -----------------------------------------------------------------------------

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef VITTE_DIAG_SM_MALLOC
#define VITTE_DIAG_SM_MALLOC  malloc
#define VITTE_DIAG_SM_REALLOC realloc
#define VITTE_DIAG_SM_FREE    free
#endif

// -----------------------------
// Public type
// -----------------------------

typedef struct vitte_source_map_t {
  char   *path;        // optional (owned; may be NULL)
  char   *src;         // source buffer (owned iff owns_src)
  size_t  len;         // length in bytes
  bool    owns_src;

  // line_starts[i] = byte offset of the first char of line i (0-based).
  // line_count     = number of lines.
  // invariant: line_starts[0] == 0 if len>0, and a sentinel at [line_count]
  // equals len for convenience.
  size_t *line_starts;
  size_t  line_count;
  size_t  line_cap;

  bool    indexed;     // whether line index is built
} vitte_source_map_t;

// -----------------------------
// Small helpers
// -----------------------------

static char *sm_strdup(const char *s) {
  if (!s) return NULL;
  size_t n = strlen(s);
  char *out = (char *)VITTE_DIAG_SM_MALLOC(n + 1);
  if (!out) return NULL;
  memcpy(out, s, n);
  out[n] = '\0';
  return out;
}

static bool sm_vec_reserve(vitte_source_map_t *sm, size_t want) {
  if (sm->line_cap >= want) return true;
  size_t new_cap = sm->line_cap ? sm->line_cap : 16;
  while (new_cap < want) {
    // grow ~1.5x with overflow guards
    size_t next = new_cap + (new_cap >> 1) + 8;
    if (next <= new_cap) { // overflow
      new_cap = want;
      break;
    }
    new_cap = next;
  }
  size_t *p = (size_t *)VITTE_DIAG_SM_REALLOC(sm->line_starts, new_cap * sizeof(size_t));
  if (!p) return false;
  sm->line_starts = p;
  sm->line_cap = new_cap;
  return true;
}

static void sm_vec_push(vitte_source_map_t *sm, size_t v) {
  if (!sm_vec_reserve(sm, sm->line_count + 1)) {
    // hard OOM: diagnostics must remain safe; fall back to minimal state.
    return;
  }
  sm->line_starts[sm->line_count++] = v;
}

static size_t sm_clamp_offset(const vitte_source_map_t *sm, size_t off) {
  if (!sm) return 0;
  if (off > sm->len) return sm->len;
  return off;
}

// Best-effort UTF-8 codepoint count from [p, p+bytes).
// Invalid sequences are treated as a single byte codepoint.
static uint32_t sm_utf8_count_codepoints(const unsigned char *p, size_t bytes) {
  uint32_t count = 0;
  size_t i = 0;
  while (i < bytes) {
    unsigned char c = p[i];
    if (c < 0x80) {
      i += 1;
    } else if ((c & 0xE0) == 0xC0 && i + 1 < bytes) {
      unsigned char c1 = p[i + 1];
      if ((c1 & 0xC0) == 0x80) {
        i += 2;
      } else {
        i += 1;
      }
    } else if ((c & 0xF0) == 0xE0 && i + 2 < bytes) {
      unsigned char c1 = p[i + 1];
      unsigned char c2 = p[i + 2];
      if (((c1 & 0xC0) == 0x80) && ((c2 & 0xC0) == 0x80)) {
        i += 3;
      } else {
        i += 1;
      }
    } else if ((c & 0xF8) == 0xF0 && i + 3 < bytes) {
      unsigned char c1 = p[i + 1];
      unsigned char c2 = p[i + 2];
      unsigned char c3 = p[i + 3];
      if (((c1 & 0xC0) == 0x80) && ((c2 & 0xC0) == 0x80) && ((c3 & 0xC0) == 0x80)) {
        i += 4;
      } else {
        i += 1;
      }
    } else {
      i += 1;
    }
    count += 1;
  }
  return count;
}

// -----------------------------
// Indexing
// -----------------------------

static void vitte_source_map_build_index(vitte_source_map_t *sm) {
  if (!sm || sm->indexed) return;

  sm->line_count = 0;

  // Always provide at least one line.
  sm_vec_push(sm, 0);

  const char *s = sm->src ? sm->src : "";
  size_t n = sm->len;

  for (size_t i = 0; i < n; i++) {
    if (s[i] == '\n') {
      // Next line starts after '\n'
      if (i + 1 <= n) sm_vec_push(sm, i + 1);
    }
  }

  // Sentinel end for convenience.
  sm_vec_push(sm, n);

  // line_count currently includes sentinel; store logical count separately.
  // We'll keep sentinel at line_starts[line_count-1].
  sm->indexed = true;
}

static size_t vitte_source_map_logical_line_count(const vitte_source_map_t *sm) {
  if (!sm || !sm->indexed || sm->line_count == 0) return 0;
  // minus sentinel
  return sm->line_count - 1;
}

// Binary search: returns the 0-based line index containing 'off'.
// Requires index built.
static size_t sm_find_line_index(const vitte_source_map_t *sm, size_t off) {
  assert(sm && sm->indexed);

  size_t logical = vitte_source_map_logical_line_count(sm);
  if (logical == 0) return 0;

  // Clamp to len.
  if (off > sm->len) off = sm->len;

  // line_starts is sorted; find last start <= off.
  size_t lo = 0;
  size_t hi = logical; // exclusive
  while (lo + 1 < hi) {
    size_t mid = lo + (hi - lo) / 2;
    size_t start = sm->line_starts[mid];
    if (start <= off) lo = mid;
    else hi = mid;
  }
  return lo;
}

// -----------------------------
// Public API
// -----------------------------

// Initialize a source map.
// If copy_source is true, the source buffer is copied and owned by the map.
// Otherwise the map borrows the source pointer (caller must keep it alive).
void vitte_source_map_init(
  vitte_source_map_t *sm,
  const char *path,
  const char *source,
  size_t source_len,
  bool copy_source
) {
  if (!sm) return;
  memset(sm, 0, sizeof(*sm));

  sm->path = sm_strdup(path);

  sm->len = source ? source_len : 0;
  sm->owns_src = copy_source;

  if (copy_source) {
    if (source && source_len) {
      sm->src = (char *)VITTE_DIAG_SM_MALLOC(source_len + 1);
      if (sm->src) {
        memcpy(sm->src, source, source_len);
        sm->src[source_len] = '\0';
      } else {
        // OOM: fall back to empty.
        sm->len = 0;
        sm->src = NULL;
        sm->owns_src = false;
      }
    } else {
      sm->src = NULL;
    }
  } else {
    sm->src = (char *)(uintptr_t)source; // borrowed; keep as char*
  }

  sm->indexed = false;
  vitte_source_map_build_index(sm);
}

void vitte_source_map_destroy(vitte_source_map_t *sm) {
  if (!sm) return;
  if (sm->path) VITTE_DIAG_SM_FREE(sm->path);
  if (sm->owns_src && sm->src) VITTE_DIAG_SM_FREE(sm->src);
  if (sm->line_starts) VITTE_DIAG_SM_FREE(sm->line_starts);
  memset(sm, 0, sizeof(*sm));
}

const char *vitte_source_map_path(const vitte_source_map_t *sm) {
  return sm ? sm->path : NULL;
}

const char *vitte_source_map_source(const vitte_source_map_t *sm) {
  return sm ? sm->src : NULL;
}

size_t vitte_source_map_length(const vitte_source_map_t *sm) {
  return sm ? sm->len : 0;
}

// Return number of logical lines (>=1).
size_t vitte_source_map_line_count(const vitte_source_map_t *sm) {
  if (!sm) return 0;
  if (!sm->indexed) return 0;
  size_t n = vitte_source_map_logical_line_count(sm);
  return (n == 0) ? 1 : n;
}

// Convert offset to 1-based (line, col). Column is UTF-8 codepoint count from line start.
void vitte_source_map_offset_to_line_col(
  const vitte_source_map_t *sm,
  size_t offset,
  uint32_t *out_line,
  uint32_t *out_col
) {
  if (out_line) *out_line = 1;
  if (out_col)  *out_col  = 1;
  if (!sm) return;

  if (!sm->indexed) {
    // best-effort
    if (out_line) *out_line = 1;
    if (out_col)  *out_col  = (uint32_t)(sm_clamp_offset(sm, offset) + 1);
    return;
  }

  size_t off = sm_clamp_offset(sm, offset);
  size_t li = sm_find_line_index(sm, off);
  size_t line_start = sm->line_starts[li];

  if (out_line) *out_line = (uint32_t)(li + 1);

  // Column as UTF-8 codepoints from line_start to off.
  if (out_col) {
    const unsigned char *p = (const unsigned char *)(sm->src ? sm->src : "");
    if (off < line_start) {
      *out_col = 1;
    } else {
      size_t bytes = off - line_start;
      uint32_t cps = sm_utf8_count_codepoints(p + line_start, bytes);
      *out_col = cps + 1;
    }
  }
}

// Get a view of a given 1-based line. The view excludes the trailing '\n'.
// If the source uses CRLF, a trailing '\r' is also excluded.
// Returns false if line is out of bounds.
bool vitte_source_map_get_line_view(
  const vitte_source_map_t *sm,
  uint32_t line1,
  const char **out_ptr,
  size_t *out_len
) {
  if (out_ptr) *out_ptr = NULL;
  if (out_len) *out_len = 0;
  if (!sm || !sm->indexed) return false;

  size_t logical = vitte_source_map_logical_line_count(sm);
  if (logical == 0) logical = 1;

  if (line1 == 0) return false;
  size_t li = (size_t)line1 - 1;
  if (li >= logical) return false;

  size_t start = sm->line_starts[li];
  size_t end   = sm->line_starts[li + 1];

  const char *s = sm->src ? sm->src : "";

  // Trim trailing newline.
  if (end > start && s[end - 1] == '\n') end--;
  // Trim possible CR.
  if (end > start && s[end - 1] == '\r') end--;

  if (out_ptr) *out_ptr = s + start;
  if (out_len) *out_len = (end >= start) ? (end - start) : 0;
  return true;
}

// -----------------------------
// Snippet rendering
// -----------------------------

typedef struct {
  char  *buf;
  size_t len;
  size_t cap;
} sm_str_t;

static void sm_str_init(sm_str_t *s) {
  s->buf = NULL;
  s->len = 0;
  s->cap = 0;
}

static void sm_str_free(sm_str_t *s) {
  if (s->buf) VITTE_DIAG_SM_FREE(s->buf);
  s->buf = NULL;
  s->len = 0;
  s->cap = 0;
}

static bool sm_str_reserve(sm_str_t *s, size_t add) {
  size_t want = s->len + add + 1;
  if (s->cap >= want) return true;
  size_t new_cap = s->cap ? s->cap : 128;
  while (new_cap < want) {
    size_t next = new_cap + (new_cap >> 1) + 32;
    if (next <= new_cap) { new_cap = want; break; }
    new_cap = next;
  }
  char *p = (char *)VITTE_DIAG_SM_REALLOC(s->buf, new_cap);
  if (!p) return false;
  s->buf = p;
  s->cap = new_cap;
  return true;
}

static void sm_str_append_n(sm_str_t *s, const char *p, size_t n) {
  if (!p || n == 0) return;
  if (!sm_str_reserve(s, n)) return;
  memcpy(s->buf + s->len, p, n);
  s->len += n;
  s->buf[s->len] = '\0';
}

static void sm_str_append_c(sm_str_t *s, char c) {
  if (!sm_str_reserve(s, 1)) return;
  s->buf[s->len++] = c;
  s->buf[s->len] = '\0';
}

static void sm_str_append_u32(sm_str_t *s, uint32_t v) {
  char tmp[32];
  int n = snprintf(tmp, sizeof(tmp), "%u", (unsigned)v);
  if (n > 0) sm_str_append_n(s, tmp, (size_t)n);
}

static uint32_t sm_digits_u32(uint32_t v) {
  uint32_t d = 1;
  while (v >= 10) { v /= 10; d++; }
  return d;
}

// Compute a visual column (1-based) within a line for a given byte offset,
// expanding tabs. This is used for caret positioning in snippets.
static uint32_t sm_visual_col_with_tabs(const char *line, size_t line_len, size_t byte_in_line, uint32_t tab_width) {
  if (tab_width == 0) tab_width = 4;
  if (!line) return 1;
  if (byte_in_line > line_len) byte_in_line = line_len;

  uint32_t col = 1;
  size_t i = 0;
  while (i < byte_in_line) {
    unsigned char c = (unsigned char)line[i];
    if (c == '\t') {
      uint32_t off = (col - 1) % tab_width;
      uint32_t step = tab_width - off;
      col += step;
      i += 1;
      continue;
    }

    // advance by one UTF-8 codepoint (best-effort)
    if (c < 0x80) {
      i += 1;
    } else if ((c & 0xE0) == 0xC0 && i + 1 < byte_in_line) {
      i += 2;
    } else if ((c & 0xF0) == 0xE0 && i + 2 < byte_in_line) {
      i += 3;
    } else if ((c & 0xF8) == 0xF0 && i + 3 < byte_in_line) {
      i += 4;
    } else {
      i += 1;
    }

    col += 1;
  }
  return col;
}

// Render a snippet around an offset.
// - context_lines: number of lines before/after to include.
// - span_len: length in bytes of the underline; if 0, a single caret is printed.
// - tab_width: tab expansion width for aligning the caret/underline.
//
// Returned string is heap-allocated; caller must free().
char *vitte_source_map_render_snippet(
  const vitte_source_map_t *sm,
  size_t offset,
  size_t span_len,
  uint32_t context_lines,
  uint32_t tab_width
) {
  if (!sm || !sm->indexed) return NULL;

  size_t off = sm_clamp_offset(sm, offset);
  size_t li = sm_find_line_index(sm, off);
  uint32_t line1 = (uint32_t)(li + 1);

  size_t logical = vitte_source_map_logical_line_count(sm);
  if (logical == 0) logical = 1;

  uint32_t first = (line1 > context_lines) ? (line1 - context_lines) : 1;
  uint32_t last  = line1 + context_lines;
  if (last > (uint32_t)logical) last = (uint32_t)logical;

  uint32_t width = sm_digits_u32(last);

  // Determine caret position within the main line.
  const char *line_ptr = NULL;
  size_t line_len = 0;
  (void)vitte_source_map_get_line_view(sm, line1, &line_ptr, &line_len);
  size_t line_start = sm->line_starts[li];
  size_t byte_in_line = (off >= line_start) ? (off - line_start) : 0;
  if (byte_in_line > line_len) byte_in_line = line_len;

  uint32_t caret_col = sm_visual_col_with_tabs(line_ptr, line_len, byte_in_line, tab_width);

  // Determine underline width (visual columns) for span_len.
  uint32_t underline_cols = 1;
  if (span_len > 0) {
    size_t end_off = sm_clamp_offset(sm, off + span_len);
    size_t end_in_line = (end_off >= line_start) ? (end_off - line_start) : 0;
    if (end_in_line > line_len) end_in_line = line_len;
    uint32_t end_col = sm_visual_col_with_tabs(line_ptr, line_len, end_in_line, tab_width);
    if (end_col > caret_col) underline_cols = end_col - caret_col;
    else underline_cols = 1;
  }

  sm_str_t out;
  sm_str_init(&out);

  for (uint32_t ln = first; ln <= last; ln++) {
    const char *p = NULL;
    size_t n = 0;
    if (!vitte_source_map_get_line_view(sm, ln, &p, &n)) {
      continue;
    }

    // Prefix: " <line>| "
    sm_str_append_c(&out, ' ');
    // left-pad line number
    uint32_t d = sm_digits_u32(ln);
    for (uint32_t k = d; k < width; k++) sm_str_append_c(&out, ' ');
    sm_str_append_u32(&out, ln);
    sm_str_append_n(&out, " | ", 3);

    // Line content
    sm_str_append_n(&out, p, n);
    sm_str_append_c(&out, '\n');

    // Caret line only for target line.
    if (ln == line1) {
      sm_str_append_c(&out, ' ');
      for (uint32_t k = 0; k < width; k++) sm_str_append_c(&out, ' ');
      sm_str_append_n(&out, " | ", 3);

      // spaces up to caret col (1-based)
      if (caret_col > 1) {
        for (uint32_t k = 1; k < caret_col; k++) sm_str_append_c(&out, ' ');
      }

      // caret/underline
      if (span_len == 0) {
        sm_str_append_c(&out, '^');
      } else {
        for (uint32_t k = 0; k < underline_cols; k++) sm_str_append_c(&out, '^');
      }

      sm_str_append_c(&out, '\n');
    }
  }

  // Finalize
  if (!out.buf) {
    sm_str_free(&out);
    return NULL;
  }

  // Caller frees
  return out.buf;
}

// -----------------------------
// Optional: basic self-test (compile-time disabled by default)
// -----------------------------

#ifdef VITTE_DIAG_SOURCE_MAP_TEST
static void sm_test_one(const char *src, size_t off) {
  vitte_source_map_t sm;
  vitte_source_map_init(&sm, "<test>", src, strlen(src), false);

  uint32_t line = 0, col = 0;
  vitte_source_map_offset_to_line_col(&sm, off, &line, &col);
  char *snip = vitte_source_map_render_snippet(&sm, off, 0, 1, 4);

  printf("off=%zu => line=%u col=%u\n", off, line, col);
  if (snip) {
    printf("%s", snip);
    free(snip);
  }

  vitte_source_map_destroy(&sm);
}

int main(void) {
  sm_test_one("a\n\tb\nccc\n", 0);
  sm_test_one("a\n\tb\nccc\n", 3);
  sm_test_one("a\n\tb\nccc\n", 4);
  sm_test_one("a\n\tb\nccc\n", 6);
  return 0;
}
#endif