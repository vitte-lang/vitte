
/*
  vitte - Language Server Protocol (LSP) entrypoint
  -------------------------------------------------
  C17, single-file, stdio JSON-RPC 2.0 transport.

  This binary is intended to be wired into the rest of the compiler stack
  (parse/lower/sema/diag/fmt). In this repository snapshot it provides:
  - robust message framing (Content-Length)
  - minimal JSON field extraction (method/id)
  - correct LSP handshake (initialize/shutdown/exit)
  - stubs for common requests (completion/hover/definition)

  Build notes:
  - designed to compile as-is (only libc).
  - integrate real services by replacing the TODO sections.
*/

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#ifndef VITTE_LSP_VERSION
#define VITTE_LSP_VERSION "0.1.0"
#endif

/* ----------------------------- small utilities ---------------------------- */

static void *vitte_xmalloc(size_t n) {
  void *p = malloc(n ? n : 1);
  if (!p) {
    fprintf(stderr, "vitte-lsp: OOM (%zu bytes)\n", n);
    abort();
  }
  return p;
}

static void *vitte_xrealloc(void *p, size_t n) {
  void *q = realloc(p, n ? n : 1);
  if (!q) {
    fprintf(stderr, "vitte-lsp: OOM (realloc %zu bytes)\n", n);
    abort();
  }
  return q;
}

static char *vitte_xstrdup(const char *s) {
  size_t n = s ? strlen(s) : 0;
  char *p = (char *)vitte_xmalloc(n + 1);
  if (n) memcpy(p, s, n);
  p[n] = '\0';
  return p;
}

static char *vitte_strndup(const char *s, size_t n) {
  char *p = (char *)vitte_xmalloc(n + 1);
  if (n) memcpy(p, s, n);
  p[n] = '\0';
  return p;
}

static size_t vitte_min_size(size_t a, size_t b) { return (a < b) ? a : b; }

static void vitte_chomp_crlf(char *s) {
  if (!s) return;
  size_t n = strlen(s);
  while (n && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
    s[n - 1] = '\0';
    --n;
  }
}

/* --------------------------------- logging -------------------------------- */

typedef enum vitte_log_level {
  VITTE_LOG_ERROR = 0,
  VITTE_LOG_WARN  = 1,
  VITTE_LOG_INFO  = 2,
  VITTE_LOG_DEBUG = 3,
  VITTE_LOG_TRACE = 4,
} vitte_log_level;

static FILE *g_log_fp = NULL;
static vitte_log_level g_log_level = VITTE_LOG_INFO;

static const char *vitte_level_name(vitte_log_level lvl) {
  switch (lvl) {
  case VITTE_LOG_ERROR: return "ERROR";
  case VITTE_LOG_WARN:  return "WARN";
  case VITTE_LOG_INFO:  return "INFO";
  case VITTE_LOG_DEBUG: return "DEBUG";
  case VITTE_LOG_TRACE: return "TRACE";
  default:              return "?";
  }
}

static void vitte_log(vitte_log_level lvl, const char *fmt, ...) {
  if (!g_log_fp) return;
  if ((int)lvl > (int)g_log_level) return;

  time_t t = time(NULL);
  struct tm tmv;
#if defined(_WIN32)
  localtime_s(&tmv, &t);
#else
  localtime_r(&t, &tmv);
#endif
  char ts[32];
  (void)strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tmv);

  fprintf(g_log_fp, "%s [%s] ", ts, vitte_level_name(lvl));

  va_list ap;
  va_start(ap, fmt);
  vfprintf(g_log_fp, fmt, ap);
  va_end(ap);

  fputc('\n', g_log_fp);
  fflush(g_log_fp);
}

/* ------------------------------ dynamic buffer ----------------------------- */

typedef struct vitte_buf {
  char  *data;
  size_t len;
  size_t cap;
} vitte_buf;

static void vitte_buf_init(vitte_buf *b) {
  b->data = NULL;
  b->len = 0;
  b->cap = 0;
}

static void vitte_buf_free(vitte_buf *b) {
  free(b->data);
  b->data = NULL;
  b->len = 0;
  b->cap = 0;
}

static void vitte_buf_reserve(vitte_buf *b, size_t need) {
  if (need <= b->cap) return;
  size_t new_cap = b->cap ? b->cap : 256;
  while (new_cap < need) {
    new_cap = (new_cap < (SIZE_MAX / 2)) ? (new_cap * 2) : need;
    if (new_cap < need) new_cap = need;
  }
  b->data = (char *)vitte_xrealloc(b->data, new_cap);
  b->cap = new_cap;
}

static void vitte_buf_push_bytes(vitte_buf *b, const void *p, size_t n) {
  if (!n) return;
  vitte_buf_reserve(b, b->len + n + 1);
  memcpy(b->data + b->len, p, n);
  b->len += n;
  b->data[b->len] = '\0';
}

static void vitte_buf_push_cstr(vitte_buf *b, const char *s) {
  vitte_buf_push_bytes(b, s, s ? strlen(s) : 0);
}

static void vitte_buf_clear(vitte_buf *b) {
  b->len = 0;
  if (b->data) b->data[0] = '\0';
}

/* printf-like append (bounded intermediate) */
static void vitte_buf_printf(vitte_buf *b, const char *fmt, ...) {
  char tmp[2048];
  va_list ap;
  va_start(ap, fmt);
#if defined(_WIN32)
  int n = _vsnprintf_s(tmp, sizeof(tmp), _TRUNCATE, fmt, ap);
#else
  int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
#endif
  va_end(ap);
  if (n < 0) return;
  size_t nn = (size_t)n;
  if (nn >= sizeof(tmp)) nn = sizeof(tmp) - 1;
  vitte_buf_push_bytes(b, tmp, nn);
}

/* ----------------------------- JSON mini-extract --------------------------- */

/*
  This is not a full JSON parser.
  It extracts the first occurrence of a field at top-level-ish by scanning.
  Works for typical LSP messages.
*/

static const char *vitte_skip_ws(const char *p) {
  while (p && *p && isspace((unsigned char)*p)) ++p;
  return p;
}

static const char *vitte_find_key(const char *json, const char *key) {
  if (!json || !key) return NULL;
  size_t klen = strlen(key);

  /* Search for "key" */
  const char *p = json;
  while ((p = strstr(p, "\"")) != NULL) {
    p++; /* after opening quote */
    if (strncmp(p, key, klen) == 0 && p[klen] == '"') {
      /* Ensure preceding was quote, now find ':' */
      const char *q = p + klen + 1; /* after closing quote */
      q = vitte_skip_ws(q);
      if (*q == ':') return q + 1;
    }
    /* continue scanning */
    const char *next = strchr(p, '"');
    if (!next) break;
    p = next + 1;
  }
  return NULL;
}

static bool vitte_json_get_string(const char *json, const char *key, char **out) {
  *out = NULL;
  const char *p = vitte_find_key(json, key);
  if (!p) return false;
  p = vitte_skip_ws(p);
  if (*p != '"') return false;
  ++p;

  /* naive string scan with minimal escape handling */
  vitte_buf b;
  vitte_buf_init(&b);

  while (*p) {
    if (*p == '"') {
      *out = vitte_strndup(b.data ? b.data : "", b.len);
      vitte_buf_free(&b);
      return true;
    }
    if (*p == '\\') {
      ++p;
      if (!*p) break;
      /* keep escaped char as-is (best-effort) */
      vitte_buf_push_bytes(&b, p, 1);
      ++p;
      continue;
    }
    vitte_buf_push_bytes(&b, p, 1);
    ++p;
  }

  vitte_buf_free(&b);
  return false;
}

/* captures raw JSON value for "id" (number|string|null), preserving quotes if string */
static bool vitte_json_get_id_raw(const char *json, char **out_raw) {
  *out_raw = NULL;
  const char *p = vitte_find_key(json, "id");
  if (!p) return false;
  p = vitte_skip_ws(p);
  if (!*p) return false;

  if (*p == 'n') {
    /* null */
    if (strncmp(p, "null", 4) == 0) {
      *out_raw = vitte_xstrdup("null");
      return true;
    }
    return false;
  }

  if (*p == '"') {
    /* string id */
    const char *s = p;
    ++p;
    while (*p && *p != '"') {
      if (*p == '\\' && p[1]) p += 2;
      else ++p;
    }
    if (*p != '"') return false;
    ++p; /* include closing quote */
    *out_raw = vitte_strndup(s, (size_t)(p - s));
    return true;
  }

  /* number (int) */
  const char *s = p;
  if (*p == '-') ++p;
  if (!isdigit((unsigned char)*p)) return false;
  while (isdigit((unsigned char)*p)) ++p;
  *out_raw = vitte_strndup(s, (size_t)(p - s));
  return true;
}

/* ----------------------------- LSP wire protocol --------------------------- */

typedef struct vitte_lsp_msg {
  char  *json;          /* payload (NUL-terminated) */
  size_t json_len;      /* bytes, excluding NUL */
} vitte_lsp_msg;

static void vitte_lsp_msg_free(vitte_lsp_msg *m) {
  free(m->json);
  m->json = NULL;
  m->json_len = 0;
}

static bool vitte_read_exact(FILE *fp, void *buf, size_t n) {
  uint8_t *p = (uint8_t *)buf;
  size_t got = 0;
  while (got < n) {
    size_t r = fread(p + got, 1, n - got, fp);
    if (r == 0) {
      if (feof(fp)) return false;
      if (ferror(fp)) return false;
      continue;
    }
    got += r;
  }
  return true;
}

static bool vitte_lsp_read_message(FILE *in, vitte_lsp_msg *out) {
  out->json = NULL;
  out->json_len = 0;

  /* header lines */
  char line[2048];
  size_t content_len = 0;
  bool have_len = false;

  while (fgets(line, (int)sizeof(line), in)) {
    vitte_chomp_crlf(line);
    if (line[0] == '\0') break; /* end of headers */

    /* Content-Length: N */
    const char *k = "Content-Length:";
    size_t klen = strlen(k);
    if (strncmp(line, k, klen) == 0) {
      const char *p = line + klen;
      while (*p && isspace((unsigned char)*p)) ++p;
      errno = 0;
      unsigned long v = strtoul(p, NULL, 10);
      if (errno == 0 && v > 0) {
        content_len = (size_t)v;
        have_len = true;
      }
    }
  }

  if (!have_len) {
    if (feof(in)) return false;
    vitte_log(VITTE_LOG_WARN, "missing Content-Length header");
    return false;
  }

  char *payload = (char *)vitte_xmalloc(content_len + 1);
  if (!vitte_read_exact(in, payload, content_len)) {
    free(payload);
    return false;
  }
  payload[content_len] = '\0';

  out->json = payload;
  out->json_len = content_len;
  return true;
}

static bool vitte_write_all(FILE *fp, const void *buf, size_t n) {
  const uint8_t *p = (const uint8_t *)buf;
  size_t off = 0;
  while (off < n) {
    size_t w = fwrite(p + off, 1, n - off, fp);
    if (w == 0) {
      if (ferror(fp)) return false;
      continue;
    }
    off += w;
  }
  return true;
}

static bool vitte_lsp_send_json(FILE *out, const char *json) {
  if (!json) json = "";
  size_t n = strlen(json);
  char header[128];
  int hn = snprintf(header, sizeof(header), "Content-Length: %zu\r\n\r\n", n);
  if (hn <= 0) return false;
  if (!vitte_write_all(out, header, (size_t)hn)) return false;
  if (!vitte_write_all(out, json, n)) return false;
  fflush(out);
  return true;
}

/* ------------------------------ LSP responses ------------------------------ */

static void vitte_lsp_send_error(FILE *out, const char *id_raw, int code, const char *msg) {
  vitte_buf b;
  vitte_buf_init(&b);
  vitte_buf_printf(&b,
    "{\"jsonrpc\":\"2.0\",\"id\":%s,\"error\":{\"code\":%d,\"message\":\"",
    id_raw ? id_raw : "null", code);

  /* JSON-escape minimal */
  for (const char *p = msg ? msg : ""; *p; ++p) {
    if (*p == '"' || *p == '\\') {
      vitte_buf_push_cstr(&b, "\\");
      vitte_buf_push_bytes(&b, p, 1);
    } else if ((unsigned char)*p < 0x20) {
      vitte_buf_printf(&b, "\\u%04x", (unsigned char)*p);
    } else {
      vitte_buf_push_bytes(&b, p, 1);
    }
  }

  vitte_buf_push_cstr(&b, "\"}}\n");
  (void)vitte_lsp_send_json(out, b.data ? b.data : "{}");
  vitte_buf_free(&b);
}

static void vitte_lsp_send_result_raw(FILE *out, const char *id_raw, const char *result_json) {
  vitte_buf b;
  vitte_buf_init(&b);
  vitte_buf_printf(&b,
    "{\"jsonrpc\":\"2.0\",\"id\":%s,\"result\":%s}\n",
    id_raw ? id_raw : "null", result_json ? result_json : "null");
  (void)vitte_lsp_send_json(out, b.data ? b.data : "{}");
  vitte_buf_free(&b);
}

static void vitte_lsp_send_notification(FILE *out, const char *method, const char *params_json) {
  vitte_buf b;
  vitte_buf_init(&b);
  vitte_buf_printf(&b,
    "{\"jsonrpc\":\"2.0\",\"method\":\"%s\"",
    method ? method : "");
  if (params_json) {
    vitte_buf_printf(&b, ",\"params\":%s", params_json);
  }
  vitte_buf_push_cstr(&b, "}\n");
  (void)vitte_lsp_send_json(out, b.data ? b.data : "{}");
  vitte_buf_free(&b);
}

/* --------------------------- LSP method handlers --------------------------- */

typedef struct vitte_lsp_state {
  bool shutdown_requested;
  bool exit_received;
} vitte_lsp_state;

static const char *vitte_lsp_capabilities_json(void) {
  /*
    Minimal, broadly compatible capabilities.
    Extend with semanticTokens, inlayHints, etc once implemented.
  */
  return
    "{"
      "\"capabilities\":{"
        "\"textDocumentSync\":{\"openClose\":true,\"change\":2,\"save\":{\"includeText\":false}},"
        "\"completionProvider\":{\"resolveProvider\":false,\"triggerCharacters\":[\".\",\"::\"]},"
        "\"hoverProvider\":true,"
        "\"definitionProvider\":true,"
        "\"documentFormattingProvider\":true"
      "},"
      "\"serverInfo\":{\"name\":\"vitte-lsp\",\"version\":\"" VITTE_LSP_VERSION "\"}"
    "}";
}

static void vitte_handle_initialize(FILE *out, const char *id_raw) {
  vitte_log(VITTE_LOG_INFO, "initialize");
  vitte_lsp_send_result_raw(out, id_raw, vitte_lsp_capabilities_json());
}

static void vitte_handle_shutdown(vitte_lsp_state *st, FILE *out, const char *id_raw) {
  vitte_log(VITTE_LOG_INFO, "shutdown");
  st->shutdown_requested = true;
  vitte_lsp_send_result_raw(out, id_raw, "null");
}

static void vitte_handle_exit(vitte_lsp_state *st) {
  vitte_log(VITTE_LOG_INFO, "exit");
  st->exit_received = true;
}

static void vitte_handle_completion(FILE *out, const char *id_raw) {
  /* LSP CompletionList */
  const char *result = "{\"isIncomplete\":false,\"items\":[]}";
  vitte_lsp_send_result_raw(out, id_raw, result);
}

static void vitte_handle_hover(FILE *out, const char *id_raw) {
  /* null hover until semantic engine wired */
  vitte_lsp_send_result_raw(out, id_raw, "null");
}

static void vitte_handle_definition(FILE *out, const char *id_raw) {
  /* empty locations */
  vitte_lsp_send_result_raw(out, id_raw, "[]");
}

static void vitte_handle_formatting(FILE *out, const char *id_raw) {
  /* empty edits */
  vitte_lsp_send_result_raw(out, id_raw, "[]");
}

/* ------------------------------ main dispatch ------------------------------ */

static void vitte_dispatch(vitte_lsp_state *st, FILE *out, const char *json) {
  char *method = NULL;
  char *id_raw = NULL;

  (void)vitte_json_get_string(json, "method", &method);
  (void)vitte_json_get_id_raw(json, &id_raw);

  if (!method) {
    /* Not a method call; ignore (could be responses from client) */
    free(id_raw);
    return;
  }

  vitte_log(VITTE_LOG_DEBUG, "recv method=%s id=%s", method, id_raw ? id_raw : "(none)");

  /* notifications (no id) */
  bool is_request = (id_raw != NULL);

  if (strcmp(method, "initialize") == 0) {
    if (!is_request) {
      vitte_lsp_send_error(out, "null", -32600, "initialize must be a request");
    } else {
      vitte_handle_initialize(out, id_raw);
    }
  } else if (strcmp(method, "initialized") == 0) {
    /* optional: publish empty diagnostics on init */
    /* vitte_lsp_send_notification(out, "textDocument/publishDiagnostics", "{...}"); */
  } else if (strcmp(method, "shutdown") == 0) {
    if (!is_request) {
      vitte_lsp_send_error(out, "null", -32600, "shutdown must be a request");
    } else {
      vitte_handle_shutdown(st, out, id_raw);
    }
  } else if (strcmp(method, "exit") == 0) {
    vitte_handle_exit(st);
  } else if (strcmp(method, "textDocument/completion") == 0) {
    if (is_request) vitte_handle_completion(out, id_raw);
  } else if (strcmp(method, "textDocument/hover") == 0) {
    if (is_request) vitte_handle_hover(out, id_raw);
  } else if (strcmp(method, "textDocument/definition") == 0) {
    if (is_request) vitte_handle_definition(out, id_raw);
  } else if (strcmp(method, "textDocument/formatting") == 0) {
    if (is_request) vitte_handle_formatting(out, id_raw);
  } else if (strcmp(method, "workspace/executeCommand") == 0) {
    if (is_request) vitte_lsp_send_result_raw(out, id_raw, "null");
  } else {
    if (is_request) {
      vitte_lsp_send_error(out, id_raw, -32601, "Method not found");
    }
  }

  free(method);
  free(id_raw);
}

static void vitte_print_usage(FILE *fp, const char *argv0) {
  fprintf(fp,
          "vitte-lsp (C17)\n"
          "Usage: %s [options]\n\n"
          "Options:\n"
          "  --stdio              Use stdio transport (default)\n"
          "  --log-file <path>    Write logs to file\n"
          "  --log-level <lvl>    error|warn|info|debug|trace (default: info)\n"
          "  --version            Print version\n"
          "  -h, --help           Show help\n",
          argv0);
}

static bool vitte_parse_log_level(const char *s, vitte_log_level *out) {
  if (!s) return false;
  if (strcmp(s, "error") == 0) { *out = VITTE_LOG_ERROR; return true; }
  if (strcmp(s, "warn")  == 0) { *out = VITTE_LOG_WARN;  return true; }
  if (strcmp(s, "info")  == 0) { *out = VITTE_LOG_INFO;  return true; }
  if (strcmp(s, "debug") == 0) { *out = VITTE_LOG_DEBUG; return true; }
  if (strcmp(s, "trace") == 0) { *out = VITTE_LOG_TRACE; return true; }
  return false;
}

int main(int argc, char **argv) {
  const char *log_path = NULL;

  for (int i = 1; i < argc; ++i) {
    const char *a = argv[i];
    if (!a) continue;

    if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
      vitte_print_usage(stdout, argv[0]);
      return 0;
    }

    if (strcmp(a, "--version") == 0) {
      printf("%s\n", VITTE_LSP_VERSION);
      return 0;
    }

    if (strcmp(a, "--stdio") == 0) {
      continue;
    }

    if (strcmp(a, "--log-file") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "--log-file expects a path\n");
        return 2;
      }
      log_path = argv[++i];
      continue;
    }

    if (strcmp(a, "--log-level") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "--log-level expects a value\n");
        return 2;
      }
      vitte_log_level lvl;
      if (!vitte_parse_log_level(argv[++i], &lvl)) {
        fprintf(stderr, "invalid log level\n");
        return 2;
      }
      g_log_level = lvl;
      continue;
    }

    fprintf(stderr, "unknown argument: %s\n", a);
    vitte_print_usage(stderr, argv[0]);
    return 2;
  }

  if (log_path) {
    g_log_fp = fopen(log_path, "ab");
    if (!g_log_fp) {
      fprintf(stderr, "failed to open log file: %s\n", log_path);
      return 2;
    }
  }

  /* stdout must be unbuffered or line-buffered for LSP */
  setvbuf(stdout, NULL, _IONBF, 0);

  vitte_log(VITTE_LOG_INFO, "vitte-lsp starting version=%s", VITTE_LSP_VERSION);

  vitte_lsp_state st;
  st.shutdown_requested = false;
  st.exit_received = false;

  for (;;) {
    vitte_lsp_msg msg;
    if (!vitte_lsp_read_message(stdin, &msg)) {
      vitte_log(VITTE_LOG_INFO, "stdin closed (EOF)");
      break;
    }

    vitte_dispatch(&st, stdout, msg.json);
    vitte_lsp_msg_free(&msg);

    if (st.exit_received) {
      /* LSP spec: exit should terminate; exit code 0 if shutdown was requested, else 1 */
      int code = st.shutdown_requested ? 0 : 1;
      vitte_log(VITTE_LOG_INFO, "exiting code=%d", code);
      if (g_log_fp) fclose(g_log_fp);
      return code;
    }
  }

  if (g_log_fp) fclose(g_log_fp);
  return 0;
}
