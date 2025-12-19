

// csv.c — tiny CSV writer/parser helpers (C17, max)
//
// Goals:
//   - Provide minimal CSV emitting utilities for benchmark outputs.
//   - Correct RFC4180-ish quoting rules (commas, quotes, CR/LF).
//   - Optional small line parser (splits a CSV line into fields).
//   - No dynamic dependencies beyond libc.
//
// This module is designed for benchmark tooling where speed is not critical.

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// -----------------------------------------------------------------------------
// Public API (declare in csv.h if needed)
// -----------------------------------------------------------------------------

// Write one CSV field with correct escaping.
// Returns 1 on success, 0 on write error.
int csv_write_field(FILE* out, const char* s);

// Write a separator (comma) if not first column.
int csv_write_sep(FILE* out, int* inout_first);

// Write end-of-record (\n).
int csv_write_eol(FILE* out);

// Convenience: write a whole record from an array of C strings.
int csv_write_record(FILE* out, const char* const* fields, size_t nfields);

// -----------------------------------------------------------------------------
// Optional parser: split a single CSV line into fields.
//
// - The input line may contain quotes. CR/LF are allowed at end.
// - The function allocates an array of pointers + a copy buffer.
// - Caller frees with csv_fields_free().
//
// Returns 1 on success, 0 on parse/allocation failure.

typedef struct csv_fields {
  char**  v;      // field pointers (into buf)
  size_t  n;      // number of fields
  char*   buf;    // backing storage
} csv_fields;

int  csv_parse_line(const char* line, csv_fields* out);
void csv_fields_free(csv_fields* f);

// -----------------------------------------------------------------------------
// Writer
// -----------------------------------------------------------------------------

static int csv_fputc_(FILE* out, int ch) {
  return (fputc(ch, out) == EOF) ? 0 : 1;
}

static int csv_fputs_(FILE* out, const char* s) {
  if (!s) s = "";
  return (fputs(s, out) < 0) ? 0 : 1;
}

static int csv_needs_quotes(const char* s) {
  if (!s || !*s) return 0;
  for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
    if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') return 1;
  }
  // Leading/trailing spaces are sometimes problematic; keep strict minimal.
  return 0;
}

int csv_write_field(FILE* out, const char* s) {
  if (!out) return 0;
  if (!s) s = "";

  if (!csv_needs_quotes(s)) {
    return csv_fputs_(out, s);
  }

  // Quote and double inner quotes.
  if (!csv_fputc_(out, '"')) return 0;
  for (const unsigned char* p = (const unsigned char*)s; *p; ++p) {
    if (*p == '"') {
      if (!csv_fputc_(out, '"')) return 0;
      if (!csv_fputc_(out, '"')) return 0;
    } else {
      if (!csv_fputc_(out, (int)*p)) return 0;
    }
  }
  if (!csv_fputc_(out, '"')) return 0;
  return 1;
}

int csv_write_sep(FILE* out, int* inout_first) {
  if (!out || !inout_first) return 0;
  if (*inout_first) {
    *inout_first = 0;
    return 1;
  }
  return csv_fputc_(out, ',');
}

int csv_write_eol(FILE* out) {
  if (!out) return 0;
  return csv_fputc_(out, '\n');
}

int csv_write_record(FILE* out, const char* const* fields, size_t nfields) {
  if (!out) return 0;
  int first = 1;
  for (size_t i = 0; i < nfields; ++i) {
    if (!csv_write_sep(out, &first)) return 0;
    if (!csv_write_field(out, fields ? fields[i] : "")) return 0;
  }
  return csv_write_eol(out);
}

// -----------------------------------------------------------------------------
// Parser
// -----------------------------------------------------------------------------

static const char* csv_skip_eol_(const char* s) {
  if (!s) return s;
  // tolerate trailing \r\n or \n
  size_t n = strlen(s);
  while (n && (s[n-1] == '\n' || s[n-1] == '\r')) n--;
  // return pointer to trimmed end not needed; parser operates with length.
  return s;
}

static size_t csv_trim_eol_len_(const char* s) {
  if (!s) return 0;
  size_t n = strlen(s);
  while (n && (s[n-1] == '\n' || s[n-1] == '\r')) n--;
  return n;
}

static int csv_push_field_(csv_fields* out, char* field_start) {
  if (!out) return 0;
  if (out->n == (size_t)-1) return 0;

  // grow vector
  size_t cap = 0;
  if (out->v) {
    // we store cap in v[-1]? no. Keep simple: reallocate by doubling using n.
  }

  // naive growth: realloc each time (small n in bench outputs)
  char** nv = (char**)realloc(out->v, (out->n + 1) * sizeof(char*));
  if (!nv) return 0;
  out->v = nv;
  out->v[out->n++] = field_start;
  return 1;
}

int csv_parse_line(const char* line, csv_fields* out) {
  if (!out) return 0;
  out->v = NULL;
  out->n = 0;
  out->buf = NULL;

  if (!line) {
    out->buf = (char*)malloc(1);
    if (!out->buf) return 0;
    out->buf[0] = 0;
    return 1;
  }

  const size_t len = csv_trim_eol_len_(line);
  out->buf = (char*)malloc(len + 1);
  if (!out->buf) return 0;
  memcpy(out->buf, line, len);
  out->buf[len] = 0;

  (void)csv_skip_eol_(line);

  char* s = out->buf;
  char* w = out->buf;

  // parse state
  int in_quotes = 0;
  char* field_start = w;

  for (size_t i = 0; i < len; ++i) {
    const char ch = s[i];

    if (in_quotes) {
      if (ch == '"') {
        // either end quote or escaped quote
        if (i + 1 < len && s[i + 1] == '"') {
          *w++ = '"';
          ++i; // consume second quote
        } else {
          in_quotes = 0;
        }
      } else {
        *w++ = ch;
      }
      continue;
    }

    if (ch == ',') {
      *w++ = 0;
      if (!csv_push_field_(out, field_start)) { csv_fields_free(out); return 0; }
      field_start = w;
      continue;
    }

    if (ch == '"') {
      // start quoted field only if at field start
      if (w == field_start) {
        in_quotes = 1;
        continue;
      }
      // stray quote in unquoted field: treat as literal
      *w++ = ch;
      continue;
    }

    *w++ = ch;
  }

  // terminate last field
  *w++ = 0;
  if (!csv_push_field_(out, field_start)) { csv_fields_free(out); return 0; }

  // If line ended while in quotes, consider it parse failure.
  if (in_quotes) {
    csv_fields_free(out);
    return 0;
  }

  return 1;
}

void csv_fields_free(csv_fields* f) {
  if (!f) return;
  free(f->v);
  free(f->buf);
  f->v = NULL;
  f->buf = NULL;
  f->n = 0;
}

// -----------------------------------------------------------------------------
// Self-test
// -----------------------------------------------------------------------------

#if defined(CSV_TEST)

static void dump_fields(const csv_fields* f) {
  printf("n=%zu\n", f->n);
  for (size_t i = 0; i < f->n; ++i) {
    printf("  [%zu]=<%s>\n", i, f->v[i] ? f->v[i] : "(null)");
  }
}

int main(void) {
  const char* fields[] = { "a", "b,c", "d\"e", "x\ny" };
  csv_write_record(stdout, fields, 4);

  const char* line = "a,\"b,c\",\"d\"\"e\",\"x\ny\"\n";
  csv_fields f;
  if (!csv_parse_line(line, &f)) {
    fprintf(stderr, "parse failed\n");
    return 1;
  }
  dump_fields(&f);
  csv_fields_free(&f);
  return 0;
}

#endif
