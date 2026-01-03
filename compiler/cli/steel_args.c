/* TODO */

// steel_args.c
//
// Minimal, robust CLI argument parsing helpers for Vitte tools.
//
// Features:
// - Global flag parsing (short/long).
// - Positional capture.
// - Response files: arguments prefixed with '@' are expanded.
// - Diff-friendly usage rendering.
//
// Design constraints:
// - No heap leaks in normal operation.
// - Never crash on malformed inputs; return diagnostics.
// - Avoid shell-like complexity; response file tokenization is conservative.
//
// NOTE:
// This module is intentionally self-contained.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

// -----------------------------------------------------------------------------
// Public-ish types (kept here to avoid hard dependency on a header while this
// component is being bootstrapped). If you later add steel_args.h, move these
// declarations there and keep this .c in sync.
// -----------------------------------------------------------------------------

typedef enum steel_arg_kind {
  STEEL_ARG_BOOL = 0,   // --flag, -f
  STEEL_ARG_COUNT = 1,  // --verbose (repeatable)
  STEEL_ARG_STRING = 2, // --out <value>
  STEEL_ARG_INT = 3     // --jobs 8
} steel_arg_kind;

typedef struct steel_flag_spec {
  const char* long_name;   // "help" for --help
  char short_name;         // 'h' for -h, or 0
  steel_arg_kind kind;
  void* out;               // bool*, int*, const char**
  const char* value_name;  // e.g. "PATH" or "N" (for usage), optional
  const char* help;        // usage help text
} steel_flag_spec;

typedef struct steel_args_diag {
  int code;                // 0 = ok, non-zero = error
  char message[512];       // short diagnostic
} steel_args_diag;

typedef struct steel_args_result {
  const char* prog;        // argv[0]
  const char* command;     // first non-flag token (optional)
  int command_index;       // index in expanded argv, or -1
  int positional_count;
  const char** positionals;
} steel_args_result;

// -----------------------------------------------------------------------------
// Internal vector
// -----------------------------------------------------------------------------

typedef struct steel_vec {
  char** items;
  size_t len;
  size_t cap;
} steel_vec;

static void vec_init(steel_vec* v) {
  v->items = NULL;
  v->len = 0;
  v->cap = 0;
}

static void vec_free(steel_vec* v) {
  if (!v) return;
  for (size_t i = 0; i < v->len; i++) free(v->items[i]);
  free(v->items);
  v->items = NULL;
  v->len = 0;
  v->cap = 0;
}

static bool vec_reserve(steel_vec* v, size_t need) {
  if (v->cap >= need) return true;
  size_t nc = v->cap ? v->cap : 16;
  while (nc < need) nc *= 2;
  char** p = (char**)realloc(v->items, nc * sizeof(char*));
  if (!p) return false;
  v->items = p;
  v->cap = nc;
  return true;
}

static char* sdup(const char* s) {
  if (!s) s = "";
  size_t n = strlen(s);
  char* p = (char*)malloc(n + 1);
  if (!p) return NULL;
  memcpy(p, s, n + 1);
  return p;
}

static bool vec_push(steel_vec* v, const char* s) {
  if (!vec_reserve(v, v->len + 1)) return false;
  char* c = sdup(s);
  if (!c) return false;
  v->items[v->len++] = c;
  return true;
}

// -----------------------------------------------------------------------------
// Diagnostics
// -----------------------------------------------------------------------------

static void diag_set(steel_args_diag* d, int code, const char* fmt, ...) {
  if (!d) return;
  d->code = code;
  d->message[0] = '\0';

  va_list ap;
  va_start(ap, fmt);
  vsnprintf(d->message, sizeof(d->message), fmt ? fmt : "", ap);
  va_end(ap);

  d->message[sizeof(d->message) - 1] = '\0';
}

static bool streq(const char* a, const char* b) {
  if (a == b) return true;
  if (!a || !b) return false;
  return strcmp(a, b) == 0;
}

static bool starts_with(const char* s, const char* pref) {
  if (!s || !pref) return false;
  size_t n = strlen(pref);
  return strncmp(s, pref, n) == 0;
}

// -----------------------------------------------------------------------------
// Response file expansion (@file)
//
// Tokenizer:
// - Splits on whitespace.
// - Supports "..." and '...'.
// - Supports backslash escapes inside "...".
// - Does NOT do full shell parsing.
// -----------------------------------------------------------------------------

static bool read_entire_file(const char* path, char** out_buf, size_t* out_len) {
  if (out_buf) *out_buf = NULL;
  if (out_len) *out_len = 0;

  FILE* f = fopen(path, "rb");
  if (!f) return false;

  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
  long sz = ftell(f);
  if (sz < 0) { fclose(f); return false; }
  if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return false; }

  size_t n = (size_t)sz;
  char* buf = (char*)malloc(n + 1);
  if (!buf) { fclose(f); return false; }

  size_t got = fread(buf, 1, n, f);
  fclose(f);
  if (got != n) { free(buf); return false; }

  buf[n] = '\0';
  if (out_buf) *out_buf = buf;
  else free(buf);
  if (out_len) *out_len = n;
  return true;
}

static bool rsp_tokenize(const char* src, steel_vec* out_tokens) {
  const char* p = src ? src : "";

  while (*p) {
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) break;

    char quote = 0;
    if (*p == '"' || *p == '\'') {
      quote = *p;
      p++;
    }

    size_t cap = 64;
    size_t len = 0;
    char* tok = (char*)malloc(cap);
    if (!tok) return false;

    while (*p) {
      char ch = *p;

      if (quote) {
        if (ch == quote) {
          p++;
          break;
        }
        if (quote == '"' && ch == '\\' && p[1]) {
          char esc = p[1];
          if (esc == 'n') ch = '\n';
          else if (esc == 'r') ch = '\r';
          else if (esc == 't') ch = '\t';
          else ch = esc;
          p += 2;
        } else {
          p++;
        }
      } else {
        if (isspace((unsigned char)ch)) break;
        p++;
      }

      if (len + 2 > cap) {
        cap *= 2;
        char* nt = (char*)realloc(tok, cap);
        if (!nt) { free(tok); return false; }
        tok = nt;
      }
      tok[len++] = ch;
    }

    tok[len] = '\0';
    bool ok = vec_push(out_tokens, tok);
    free(tok);
    if (!ok) return false;

    while (*p && !quote && !isspace((unsigned char)*p)) p++;
  }

  return true;
}

static bool expand_one(const char* a, steel_vec* out, steel_args_diag* d, int depth) {
  if (!a) a = "";

  if (a[0] == '@' && a[1] != '\0') {
    if (depth > 16) {
      diag_set(d, 2, "response file nesting too deep");
      return false;
    }

    const char* path = a + 1;
    char* buf = NULL;
    if (!read_entire_file(path, &buf, NULL)) {
      diag_set(d, 2, "failed to read response file '%s': %s", path, strerror(errno));
      return false;
    }

    steel_vec toks;
    vec_init(&toks);
    bool ok = rsp_tokenize(buf, &toks);
    free(buf);
    if (!ok) {
      vec_free(&toks);
      diag_set(d, 2, "failed to parse response file '%s'", path);
      return false;
    }

    for (size_t i = 0; i < toks.len; i++) {
      if (!expand_one(toks.items[i], out, d, depth + 1)) {
        vec_free(&toks);
        return false;
      }
    }

    vec_free(&toks);
    return true;
  }

  if (!vec_push(out, a)) {
    diag_set(d, 2, "out of memory while expanding arguments");
    return false;
  }
  return true;
}

static bool expand_args(int argc, char** argv, steel_vec* out, steel_args_diag* d) {
  for (int i = 0; i < argc; i++) {
    if (!expand_one(argv[i], out, d, 0)) return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
// Flag lookup
// -----------------------------------------------------------------------------

static const steel_flag_spec* find_long(const steel_flag_spec* specs, size_t n, const char* name) {
  for (size_t i = 0; i < n; i++) {
    const char* ln = specs[i].long_name;
    if (ln && streq(ln, name)) return &specs[i];
  }
  return NULL;
}

static const steel_flag_spec* find_short(const steel_flag_spec* specs, size_t n, char ch) {
  for (size_t i = 0; i < n; i++) {
    if (specs[i].short_name == ch) return &specs[i];
  }
  return NULL;
}

static bool set_value(const steel_flag_spec* sp, const char* value, steel_args_diag* d) {
  if (!sp || !sp->out) return true;

  switch (sp->kind) {
    case STEEL_ARG_BOOL: {
      bool* b = (bool*)sp->out;
      *b = true;
      return true;
    }
    case STEEL_ARG_COUNT: {
      int* c = (int*)sp->out;
      (*c)++;
      return true;
    }
    case STEEL_ARG_STRING: {
      const char** s = (const char**)sp->out;
      *s = value ? value : "";
      return true;
    }
    case STEEL_ARG_INT: {
      int* x = (int*)sp->out;
      if (!value || !*value) {
        diag_set(d, 2, "missing value for --%s", sp->long_name ? sp->long_name : "(flag)");
        return false;
      }
      char* end = NULL;
      long v = strtol(value, &end, 10);
      if (!end || *end != '\0') {
        diag_set(d, 2, "invalid integer for --%s: '%s'", sp->long_name ? sp->long_name : "(flag)", value);
        return false;
      }
      if (v < INT32_MIN || v > INT32_MAX) {
        diag_set(d, 2, "integer out of range for --%s: '%s'", sp->long_name ? sp->long_name : "(flag)", value);
        return false;
      }
      *x = (int)v;
      return true;
    }
    default:
      diag_set(d, 2, "unknown flag kind for --%s", sp->long_name ? sp->long_name : "(flag)");
      return false;
  }
}

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

int steel_args_parse(int argc,
                     char** argv,
                     const steel_flag_spec* specs,
                     size_t specs_count,
                     steel_args_result* out,
                     steel_args_diag* d,
                     bool stop_at_nonflag) {
  if (d) { d->code = 0; d->message[0] = '\0'; }
  if (!out) {
    diag_set(d, 2, "internal: out result is null");
    return 2;
  }

  out->prog = (argc > 0 && argv && argv[0]) ? argv[0] : "";
  out->command = NULL;
  out->command_index = -1;
  out->positional_count = 0;
  out->positionals = NULL;

  steel_vec ex;
  vec_init(&ex);
  if (!expand_args(argc, argv, &ex, d)) {
    vec_free(&ex);
    if (!d || d->code == 0) diag_set(d, 2, "failed to expand arguments");
    return 2;
  }

  const char** pos = NULL;
  int pos_len = 0;
  int pos_cap = 0;

  #define POS_PUSH(val) do { \
    if (pos_len + 1 > pos_cap) { \
      int nc = pos_cap ? pos_cap * 2 : 16; \
      const char** np = (const char**)realloc((void*)pos, (size_t)nc * sizeof(const char*)); \
      if (!np) { \
        free((void*)pos); \
        vec_free(&ex); \
        diag_set(d, 2, "out of memory while parsing args"); \
        return 2; \
      } \
      pos = np; \
      pos_cap = nc; \
    } \
    pos[pos_len++] = (val); \
  } while (0)

  int i = 1;
  for (; i < (int)ex.len; i++) {
    const char* a = ex.items[i];
    if (!a) a = "";

    if (streq(a, "--")) {
      for (int j = i + 1; j < (int)ex.len; j++) POS_PUSH(ex.items[j]);
      i = (int)ex.len;
      break;
    }

    if (starts_with(a, "--")) {
      const char* name = a + 2;
      const char* eq = strchr(name, '=');
      char tmp[256];
      const char* val = NULL;

      if (eq) {
        size_t nn = (size_t)(eq - name);
        if (nn >= sizeof(tmp)) {
          free((void*)pos);
          vec_free(&ex);
          diag_set(d, 2, "flag name too long: %s", a);
          return 2;
        }
        memcpy(tmp, name, nn);
        tmp[nn] = '\0';
        name = tmp;
        val = eq + 1;
      }

      const steel_flag_spec* sp = find_long(specs, specs_count, name);
      if (!sp) {
        free((void*)pos);
        vec_free(&ex);
        diag_set(d, 2, "unknown flag: --%s", name);
        return 2;
      }

      if (sp->kind == STEEL_ARG_STRING || sp->kind == STEEL_ARG_INT) {
        if (!val) {
          if (i + 1 >= (int)ex.len) {
            free((void*)pos);
            vec_free(&ex);
            diag_set(d, 2, "missing value for --%s", name);
            return 2;
          }
          val = ex.items[++i];
        }
      }

      if (!set_value(sp, val, d)) {
        free((void*)pos);
        vec_free(&ex);
        if (!d || d->code == 0) diag_set(d, 2, "invalid value for --%s", name);
        return 2;
      }

      continue;
    }

    if (a[0] == '-' && a[1] != '\0') {
      const char* p = a + 1;
      while (*p) {
        char ch = *p++;
        const steel_flag_spec* sp = find_short(specs, specs_count, ch);
        if (!sp) {
          free((void*)pos);
          vec_free(&ex);
          diag_set(d, 2, "unknown flag: -%c", ch);
          return 2;
        }

        const char* val = NULL;
        if (sp->kind == STEEL_ARG_STRING || sp->kind == STEEL_ARG_INT) {
          if (*p) {
            val = p;
            p = "";
          } else {
            if (i + 1 >= (int)ex.len) {
              free((void*)pos);
              vec_free(&ex);
              diag_set(d, 2, "missing value for -%c", ch);
              return 2;
            }
            val = ex.items[++i];
          }
        }

        if (!set_value(sp, val, d)) {
          free((void*)pos);
          vec_free(&ex);
          if (!d || d->code == 0) diag_set(d, 2, "invalid value for -%c", ch);
          return 2;
        }
      }
      continue;
    }

    if (stop_at_nonflag) {
      out->command = a;
      out->command_index = i;
      break;
    }

    POS_PUSH(a);
  }

  if (out->command_index >= 0) {
    for (int j = out->command_index + 1; j < (int)ex.len; j++) POS_PUSH(ex.items[j]);
  }

  out->positional_count = pos_len;

  const char* cmd_dup = NULL;
  if (out->command) cmd_dup = sdup(out->command);

  const char** pos_dup = NULL;
  if (pos_len > 0) {
    pos_dup = (const char**)malloc((size_t)pos_len * sizeof(const char*));
    if (!pos_dup) {
      free((void*)pos);
      free((void*)cmd_dup);
      vec_free(&ex);
      diag_set(d, 2, "out of memory while finalizing parse result");
      return 2;
    }
    for (int k = 0; k < pos_len; k++) {
      pos_dup[k] = sdup(pos[k]);
      if (!pos_dup[k]) {
        for (int m = 0; m < k; m++) free((void*)pos_dup[m]);
        free((void*)pos_dup);
        free((void*)pos);
        free((void*)cmd_dup);
        vec_free(&ex);
        diag_set(d, 2, "out of memory while finalizing parse result");
        return 2;
      }
    }
  }

  free((void*)pos);
  vec_free(&ex);

  out->command = cmd_dup;
  out->positionals = pos_dup;

  return 0;
}

void steel_args_free_result(steel_args_result* out) {
  if (!out) return;
  if (out->command) {
    free((void*)out->command);
    out->command = NULL;
  }
  if (out->positionals) {
    for (int i = 0; i < out->positional_count; i++) {
      free((void*)out->positionals[i]);
    }
    free((void*)out->positionals);
    out->positionals = NULL;
  }
  out->positional_count = 0;
  out->command_index = -1;
}

static void usage_print_flag(FILE* f, const steel_flag_spec* sp) {
  if (!sp) return;
  if (sp->short_name) {
    fprintf(f, "  -%c", sp->short_name);
    if (sp->long_name) fprintf(f, ", --%s", sp->long_name);
  } else if (sp->long_name) {
    fprintf(f, "      --%s", sp->long_name);
  } else {
    fprintf(f, "      (unnamed)");
  }

  if (sp->kind == STEEL_ARG_STRING || sp->kind == STEEL_ARG_INT) {
    const char* vn = sp->value_name ? sp->value_name : "VALUE";
    fprintf(f, " <%s>", vn);
  }

  if (sp->help && *sp->help) {
    fprintf(f, "\n      %s", sp->help);
  }

  fputc('\n', f);
}

void steel_args_print_usage(FILE* f,
                            const char* prog,
                            const char* header,
                            const char* commands,
                            const steel_flag_spec* specs,
                            size_t specs_count,
                            const char* footer) {
  if (!f) f = stdout;
  if (!prog) prog = "<tool>";

  if (header && *header) {
    fprintf(f, "%s\n\n", header);
  }

  fprintf(f, "Usage:\n  %s [flags] <command> [args...]\n\n", prog);

  if (commands && *commands) {
    fprintf(f, "Commands:\n%s\n", commands);
  }

  if (specs && specs_count) {
    fprintf(f, "Flags:\n");
    for (size_t i = 0; i < specs_count; i++) {
      usage_print_flag(f, &specs[i]);
    }
    fputc('\n', f);
  }

  if (footer && *footer) {
    fprintf(f, "%s\n", footer);
  }
}

const char* steel_args_diag_cstr(const steel_args_diag* d) {
  if (!d) return "";
  return d->message;
}