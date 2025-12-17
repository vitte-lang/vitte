// C:\Users\vince\Documents\GitHub\vitte\fuzz\include\fuzz\fuzz_dict.h
// fuzz_dict.h — small helpers for fuzz dictionaries (AFL/libFuzzer)
//
// Provides:
//  - token container + builder helpers for in-memory dictionaries
//  - parsing of simple .dict format (one token per line, optionally quoted)
//  - deterministic hashing for dedup/tracing
//
// Notes:
//  - This is not a full AFL/libFuzzer dict spec parser; it is intentionally strict.
//  - Designed for fuzz harnesses and corpus tooling (load + feed mutator).
//
// C17.

#ifndef VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_DICT_H
#define VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_DICT_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fuzz_assert.h"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------

typedef struct fuzz_token {
  uint8_t* data;
  size_t size;
} fuzz_token;

typedef struct fuzz_dict {
  fuzz_token* toks;
  size_t len;
  size_t cap;
} fuzz_dict;

typedef struct fuzz_dict_view {
  const fuzz_token* toks;
  size_t len;
} fuzz_dict_view;

//------------------------------------------------------------------------------
// Hash (FNV-1a 64-bit) for stable token IDs
//------------------------------------------------------------------------------

FUZZ_INLINE static uint64_t
fuzz_fnv1a64(const void* data, size_t n) {
  const uint8_t* p = (const uint8_t*)data;
  uint64_t h = 1469598103934665603ull;
  for (size_t i = 0; i < n; ++i) {
    h ^= (uint64_t)p[i];
    h *= 1099511628211ull;
  }
  return h;
}

//------------------------------------------------------------------------------
// Allocation helpers
//------------------------------------------------------------------------------

FUZZ_INLINE static void
fuzz_dict_init(fuzz_dict* d) {
  FUZZ_ASSERT(d);
  d->toks = NULL;
  d->len = 0;
  d->cap = 0;
}

FUZZ_INLINE static void
fuzz_dict_free(fuzz_dict* d) {
  if (!d)
    return;
  for (size_t i = 0; i < d->len; ++i) {
    free(d->toks[i].data);
    d->toks[i].data = NULL;
    d->toks[i].size = 0;
  }
  free(d->toks);
  d->toks = NULL;
  d->len = 0;
  d->cap = 0;
}

FUZZ_INLINE static fuzz_dict_view
fuzz_dict_as_view(const fuzz_dict* d) {
  FUZZ_ASSERT(d);
  fuzz_dict_view v;
  v.toks = d->toks;
  v.len = d->len;
  return v;
}

FUZZ_INLINE static void
fuzz_dict__reserve(fuzz_dict* d, size_t want_cap) {
  FUZZ_ASSERT(d);
  if (d->cap >= want_cap)
    return;
  size_t new_cap = d->cap ? d->cap : 16;
  while (new_cap < want_cap)
    new_cap *= 2;
  fuzz_token* nt = (fuzz_token*)realloc(d->toks, new_cap * sizeof(fuzz_token));
  if (!nt)
    FUZZ_OOM("fuzz_dict: realloc failed");
  d->toks = nt;
  d->cap = new_cap;
}

FUZZ_INLINE static void
fuzz_dict_add_bytes(fuzz_dict* d, const void* bytes, size_t n) {
  FUZZ_ASSERT(d);
  FUZZ_ASSERT(bytes || n == 0);

  fuzz_dict__reserve(d, d->len + 1);

  uint8_t* buf = NULL;
  if (n) {
    buf = (uint8_t*)malloc(n);
    if (!buf)
      FUZZ_OOM("fuzz_dict: malloc failed");
    memcpy(buf, bytes, n);
  }

  d->toks[d->len].data = buf;
  d->toks[d->len].size = n;
  d->len += 1;
}

FUZZ_INLINE static void
fuzz_dict_add_cstr(fuzz_dict* d, const char* s) {
  FUZZ_ASSERT(d);
  FUZZ_ASSERT(s);
  fuzz_dict_add_bytes(d, s, strlen(s));
}

FUZZ_INLINE static uint64_t
fuzz_token_hash(const fuzz_token* t) {
  FUZZ_ASSERT(t);
  return fuzz_fnv1a64(t->data, t->size);
}

//------------------------------------------------------------------------------
// Simple .dict line parsing
//------------------------------------------------------------------------------
// Supported input lines:
//   - "token"      (double-quoted, with escapes)
//   - token        (bare token, trimmed)
//   - # comment    (ignored)
//   - empty lines  (ignored)
//
// Escapes supported inside quotes:
//   \\ \" \n \r \t \0
//   \xNN (2 hex digits)
//   \u{...} (hex codepoint up to 0x10FFFF, UTF-8 encoded)
//
// Everything else is treated as literal after the backslash (strict mode fails).
//------------------------------------------------------------------------------

static FUZZ_INLINE int
fuzz__is_space(unsigned char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static FUZZ_INLINE int
fuzz__is_hex(unsigned char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static FUZZ_INLINE uint32_t
fuzz__hex_val(unsigned char c) {
  if (c >= '0' && c <= '9')
    return (uint32_t)(c - '0');
  if (c >= 'a' && c <= 'f')
    return (uint32_t)(10 + (c - 'a'));
  return (uint32_t)(10 + (c - 'A'));
}

static FUZZ_INLINE void
fuzz__utf8_append(uint8_t* out, size_t* out_len, size_t out_cap, uint32_t cp) {
  FUZZ_ASSERT(out);
  FUZZ_ASSERT(out_len);

  // UTF-8 encoding for U+0000..U+10FFFF (excluding surrogate range)
  if (cp <= 0x7Fu) {
    FUZZ_ASSERT(*out_len + 1 <= out_cap);
    out[(*out_len)++] = (uint8_t)cp;
  } else if (cp <= 0x7FFu) {
    FUZZ_ASSERT(*out_len + 2 <= out_cap);
    out[(*out_len)++] = (uint8_t)(0xC0u | ((cp >> 6) & 0x1Fu));
    out[(*out_len)++] = (uint8_t)(0x80u | (cp & 0x3Fu));
  } else if (cp <= 0xFFFFu) {
    FUZZ_ASSERT(*out_len + 3 <= out_cap);
    out[(*out_len)++] = (uint8_t)(0xE0u | ((cp >> 12) & 0x0Fu));
    out[(*out_len)++] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
    out[(*out_len)++] = (uint8_t)(0x80u | (cp & 0x3Fu));
  } else {
    FUZZ_ASSERT(*out_len + 4 <= out_cap);
    out[(*out_len)++] = (uint8_t)(0xF0u | ((cp >> 18) & 0x07u));
    out[(*out_len)++] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
    out[(*out_len)++] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
    out[(*out_len)++] = (uint8_t)(0x80u | (cp & 0x3Fu));
  }
}

static FUZZ_INLINE int
fuzz__parse_u_braced_hex(const char* s, size_t n, size_t* i, uint32_t* out_cp) {
  // expects s[*i] == '{' on entry; parses until '}'
  FUZZ_ASSERT(s && i && out_cp);
  if (*i >= n || s[*i] != '{')
    return 0;
  (*i)++;

  uint32_t cp = 0;
  size_t digits = 0;
  while (*i < n && s[*i] != '}') {
    unsigned char c = (unsigned char)s[*i];
    if (!fuzz__is_hex(c))
      return 0;
    if (digits >= 6)
      return 0; // keep bounded
    cp = (cp << 4) | fuzz__hex_val(c);
    digits++;
    (*i)++;
  }
  if (*i >= n || s[*i] != '}')
    return 0;
  (*i)++;

  if (digits == 0)
    return 0;
  if (cp > 0x10FFFFu)
    return 0;
  if (cp >= 0xD800u && cp <= 0xDFFFu)
    return 0; // surrogate range
  *out_cp = cp;
  return 1;
}

typedef struct fuzz_dict_parse_opts {
  int strict;       // if 1, unknown escapes fail the line
  int keep_empty;   // if 1, allow empty tokens ("")
  size_t max_token; // 0 = unlimited; otherwise cap
} fuzz_dict_parse_opts;

FUZZ_INLINE static fuzz_dict_parse_opts
fuzz_dict_parse_opts_default(void) {
  fuzz_dict_parse_opts o;
  o.strict = 1;
  o.keep_empty = 0;
  o.max_token = 4096;
  return o;
}

static FUZZ_INLINE int
fuzz_dict__parse_line(const char* line, size_t n, const fuzz_dict_parse_opts* opts,
                      uint8_t** out_buf, size_t* out_len) {
  FUZZ_ASSERT(line);
  FUZZ_ASSERT(opts);
  FUZZ_ASSERT(out_buf);
  FUZZ_ASSERT(out_len);

  *out_buf = NULL;
  *out_len = 0;

  // Trim leading spaces
  size_t i = 0;
  while (i < n && fuzz__is_space((unsigned char)line[i]))
    i++;
  if (i >= n)
    return 0; // empty
  if (line[i] == '#')
    return 0; // comment

  // Trim trailing spaces/newlines
  size_t j = n;
  while (j > i && fuzz__is_space((unsigned char)line[j - 1]))
    j--;
  if (j <= i)
    return 0;

  // If quoted
  if (line[i] == '"') {
    i++;
    // allocate worst-case: j-i (escapes shrink) + 4x for utf8 expansion
    size_t cap = (j - i) * 4 + 8;
    if (opts->max_token && cap > opts->max_token * 4 + 8)
      cap = opts->max_token * 4 + 8;
    uint8_t* buf = (uint8_t*)malloc(cap ? cap : 1);
    if (!buf)
      FUZZ_OOM("fuzz_dict: malloc failed");

    size_t outp = 0;
    while (i < j) {
      char c = line[i++];

      if (c == '"') {
        // require only trailing spaces/comments
        while (i < j && fuzz__is_space((unsigned char)line[i]))
          i++;
        // allow inline comment after token
        if (i < j && line[i] == '#') {
          // ok: ignore rest
        } else if (i < j) {
          // garbage after quote
          free(buf);
          return -1;
        }
        break;
      }

      if (c != '\\') {
        if (opts->max_token && outp + 1 > opts->max_token) {
          free(buf);
          return -2;
        }
        if (outp + 1 > cap) {
          free(buf);
          return -3;
        }
        buf[outp++] = (uint8_t)c;
        continue;
      }

      if (i >= j) {
        free(buf);
        return -1;
      }
      char e = line[i++];

      if (e == '\\' || e == '"' || e == '\'') {
        if (opts->max_token && outp + 1 > opts->max_token) {
          free(buf);
          return -2;
        }
        buf[outp++] = (uint8_t)e;
      } else if (e == 'n') {
        if (opts->max_token && outp + 1 > opts->max_token) {
          free(buf);
          return -2;
        }
        buf[outp++] = (uint8_t)'\n';
      } else if (e == 'r') {
        if (opts->max_token && outp + 1 > opts->max_token) {
          free(buf);
          return -2;
        }
        buf[outp++] = (uint8_t)'\r';
      } else if (e == 't') {
        if (opts->max_token && outp + 1 > opts->max_token) {
          free(buf);
          return -2;
        }
        buf[outp++] = (uint8_t)'\t';
      } else if (e == '0') {
        if (opts->max_token && outp + 1 > opts->max_token) {
          free(buf);
          return -2;
        }
        buf[outp++] = (uint8_t)'\0';
      } else if (e == 'x') {
        if (i + 1 >= j) {
          free(buf);
          return -1;
        }
        unsigned char h1 = (unsigned char)line[i++];
        unsigned char h2 = (unsigned char)line[i++];
        if (!fuzz__is_hex(h1) || !fuzz__is_hex(h2)) {
          free(buf);
          return -1;
        }
        uint8_t v = (uint8_t)((fuzz__hex_val(h1) << 4) | fuzz__hex_val(h2));
        if (opts->max_token && outp + 1 > opts->max_token) {
          free(buf);
          return -2;
        }
        buf[outp++] = v;
      } else if (e == 'u') {
        // \u{...}
        if (i >= j || line[i] != '{') {
          free(buf);
          return -1;
        }
        uint32_t cp = 0;
        if (!fuzz__parse_u_braced_hex(line, j, &i, &cp)) {
          free(buf);
          return -1;
        }
        // worst-case append 4 bytes
        if (opts->max_token && outp + 4 > opts->max_token) {
          free(buf);
          return -2;
        }
        if (outp + 4 > cap) {
          free(buf);
          return -3;
        }
        fuzz__utf8_append(buf, &outp, cap, cp);
      } else {
        if (opts->strict) {
          free(buf);
          return -1;
        }
        // permissive: keep backslash + char
        if (opts->max_token && outp + 2 > opts->max_token) {
          free(buf);
          return -2;
        }
        if (outp + 2 > cap) {
          free(buf);
          return -3;
        }
        buf[outp++] = (uint8_t)'\\';
        buf[outp++] = (uint8_t)e;
      }
    }

    if (outp == 0 && !opts->keep_empty) {
      free(buf);
      return 0;
    }

    *out_buf = buf;
    *out_len = outp;
    return 1;
  }

  // Bare token: take trimmed substring [i, j)
  {
    size_t len = j - i;
    if (len == 0 && !opts->keep_empty)
      return 0;
    if (opts->max_token && len > opts->max_token)
      return -2;

    uint8_t* buf = (uint8_t*)malloc(len ? len : 1);
    if (!buf)
      FUZZ_OOM("fuzz_dict: malloc failed");
    if (len)
      memcpy(buf, line + i, len);

    *out_buf = buf;
    *out_len = len;
    return 1;
  }
}

//------------------------------------------------------------------------------
// Load .dict file from disk
//------------------------------------------------------------------------------

typedef struct fuzz_dict_load_result {
  size_t added;
  size_t skipped;
  size_t failed;
  size_t bytes_read;
} fuzz_dict_load_result;

FUZZ_INLINE static fuzz_dict_load_result
fuzz_dict_load_file_ex(fuzz_dict* d, const char* path, const fuzz_dict_parse_opts* opts) {
  FUZZ_ASSERT(d);
  FUZZ_ASSERT(path);
  FUZZ_ASSERT(opts);

  fuzz_dict_load_result r;
  r.added = 0;
  r.skipped = 0;
  r.failed = 0;
  r.bytes_read = 0;

  FILE* f = fopen(path, "rb");
  if (!f)
    FUZZ_PANIC("fuzz_dict_load_file_ex: fopen failed");

  // Read line-by-line (bounded) to avoid huge lines.
  char line[64 * 1024];
  while (fgets(line, (int)sizeof(line), f) != NULL) {
    size_t n = strlen(line);
    r.bytes_read += n;

    uint8_t* buf = NULL;
    size_t out_len = 0;
    int st = fuzz_dict__parse_line(line, n, opts, &buf, &out_len);

    if (st == 0) {
      r.skipped++;
      continue;
    }
    if (st < 0) {
      r.failed++;
      free(buf);
      continue; // keep going; fuzz infra likes partial loads
    }

    fuzz_dict_add_bytes(d, buf, out_len);
    free(buf);
    r.added++;
  }

  fclose(f);
  return r;
}

FUZZ_INLINE static fuzz_dict_load_result
fuzz_dict_load_file(fuzz_dict* d, const char* path) {
  fuzz_dict_parse_opts o = fuzz_dict_parse_opts_default();
  return fuzz_dict_load_file_ex(d, path, &o);
}

//------------------------------------------------------------------------------
// Debug / dump
//------------------------------------------------------------------------------

FUZZ_INLINE static void
fuzz_dict_dump(FILE* out, const fuzz_dict* d) {
  FUZZ_ASSERT(out);
  FUZZ_ASSERT(d);
  for (size_t i = 0; i < d->len; ++i) {
    const fuzz_token* t = &d->toks[i];
    fprintf(out, "\"");
    for (size_t j = 0; j < t->size; ++j) {
      unsigned char c = (unsigned char)t->data[j];
      if (c == '\\')
        fputs("\\\\", out);
      else if (c == '"')
        fputs("\\\"", out);
      else if (c == '\n')
        fputs("\\n", out);
      else if (c == '\r')
        fputs("\\r", out);
      else if (c == '\t')
        fputs("\\t", out);
      else if (c == '\0')
        fputs("\\0", out);
      else if (c < 0x20u || c >= 0x7Fu)
        fprintf(out, "\\x%02X", (unsigned)c);
      else
        fputc((int)c, out);
    }
    fprintf(out, "\"\n");
  }
}

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_FUZZ_INCLUDE_FUZZ_FUZZ_DICT_H
