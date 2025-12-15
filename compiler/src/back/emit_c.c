
#include "vittec/back/emit_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* -------------------------------------------------------------------------
 * Small utilities
 * ------------------------------------------------------------------------- */

static int vittec__ends_with(const char* s, const char* suf) {
  if(!s || !suf) return 0;
  size_t n = strlen(s);
  size_t m = strlen(suf);
  if(m > n) return 0;
  return 0 == memcmp(s + (n - m), suf, m);
}

static char* vittec__strdup(const char* s) {
  if(!s) return NULL;
  size_t n = strlen(s);
  char* p = (char*)malloc(n + 1);
  if(!p) return NULL;
  memcpy(p, s, n);
  p[n] = 0;
  return p;
}

static char* vittec__path_without_ext(const char* path) {
  /* Remove last ".c" or ".h" if present; otherwise duplicate. */
  if(!path) return NULL;
  size_t n = strlen(path);
  if(n >= 2 && vittec__ends_with(path, ".c")) {
    char* p = (char*)malloc(n - 1);
    if(!p) return NULL;
    memcpy(p, path, n - 2);
    p[n - 2] = 0;
    return p;
  }
  if(n >= 2 && vittec__ends_with(path, ".h")) {
    char* p = (char*)malloc(n - 1);
    if(!p) return NULL;
    memcpy(p, path, n - 2);
    p[n - 2] = 0;
    return p;
  }
  return vittec__strdup(path);
}

static char* vittec__path_with_ext(const char* base, const char* ext) {
  if(!base || !ext) return NULL;
  size_t n = strlen(base);
  size_t m = strlen(ext);
  char* p = (char*)malloc(n + m + 1);
  if(!p) return NULL;
  memcpy(p, base, n);
  memcpy(p + n, ext, m);
  p[n + m] = 0;
  return p;
}

static uint64_t vittec__fnv1a64(const void* data, size_t n) {
  const unsigned char* p = (const unsigned char*)data;
  uint64_t h = 1469598103934665603ull;
  for(size_t i = 0; i < n; i++) {
    h ^= (uint64_t)p[i];
    h *= 1099511628211ull;
  }
  return h;
}

static void vittec__hex8(uint32_t v, char out[9]) {
  static const char* H = "0123456789abcdef";
  for(int i = 7; i >= 0; i--) {
    out[i] = H[v & 0xF];
    v >>= 4;
  }
  out[8] = 0;
}

static void vittec__sanitize_ident(const char* in, char* out, size_t cap) {
  if(!out || cap == 0) return;
  if(!in) {
    out[0] = '_';
    out[1] = 0;
    return;
  }

  size_t j = 0;
  /* Ensure first char is valid identifier start. */
  unsigned char c0 = (unsigned char)in[0];
  if(!(isalpha(c0) || c0 == '_')) {
    out[j++] = '_';
  }

  for(size_t i = 0; in[i] && j + 1 < cap; i++) {
    unsigned char c = (unsigned char)in[i];
    if(isalnum(c) || c == '_') {
      out[j++] = (char)c;
    } else {
      out[j++] = '_';
    }
  }
  out[j] = 0;
}

static void vittec__sv_to_cstr(vittec_sv_t sv, char* out, size_t cap) {
  if(!out || cap == 0) return;
  size_t n = sv.len;
  if(n >= cap) n = cap - 1;
  if(sv.data && n) memcpy(out, sv.data, n);
  out[n] = 0;
}

/* -------------------------------------------------------------------------
 * Simple dynamic buffer (for callback sink)
 * ------------------------------------------------------------------------- */

typedef struct vittec__buf {
  char* data;
  size_t len;
  size_t cap;
} vittec__buf_t;

static void vittec__buf_init(vittec__buf_t* b) {
  b->data = NULL;
  b->len = 0;
  b->cap = 0;
}

static void vittec__buf_free(vittec__buf_t* b) {
  if(b && b->data) free(b->data);
  if(b) { b->data = NULL; b->len = 0; b->cap = 0; }
}

static int vittec__buf_reserve(vittec__buf_t* b, size_t add) {
  if(!b) return 0;
  size_t need = b->len + add;
  if(need <= b->cap) return 1;
  size_t newcap = (b->cap == 0) ? 4096 : b->cap;
  while(newcap < need) newcap *= 2;
  char* p = (char*)realloc(b->data, newcap);
  if(!p) return 0;
  b->data = p;
  b->cap = newcap;
  return 1;
}

static int vittec__buf_write(vittec__buf_t* b, const char* s, size_t n) {
  if(!b || (!s && n)) return 0;
  if(!vittec__buf_reserve(b, n)) return 0;
  if(n) memcpy(b->data + b->len, s, n);
  b->len += n;
  return 1;
}

static int vittec__buf_printf(vittec__buf_t* b, const char* fmt, ...) {
  if(!b || !fmt) return 0;
  va_list ap;
  va_start(ap, fmt);
  char tmp[1024];
  int k = vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);
  if(k < 0) return 0;
  if((size_t)k < sizeof(tmp)) return vittec__buf_write(b, tmp, (size_t)k);

  /* slow path */
  size_t need = (size_t)k + 1;
  char* big = (char*)malloc(need);
  if(!big) return 0;
  va_start(ap, fmt);
  vsnprintf(big, need, fmt, ap);
  va_end(ap);
  int ok = vittec__buf_write(b, big, (size_t)k);
  free(big);
  return ok;
}

/* -------------------------------------------------------------------------
 * Options
 * ------------------------------------------------------------------------- */

void vittec_emit_c_options_init(vittec_emit_c_options_t* opt) {
  if(!opt) return;
  memset(opt, 0, sizeof(*opt));
  opt->size = (uint32_t)sizeof(*opt);
  opt->mode = VITTEC_EMIT_C_MODE_STUBS;
  opt->mangle = VITTEC_EMIT_C_MANGLE_SIMPLE;
  opt->runtime = VITTEC_EMIT_C_RUNTIME_NONE;
  opt->emit_header = 0;
  opt->emit_line_directives = 0;
  opt->emit_debug_comments = 0;
  opt->namespace_prefix = "vitte_";
  opt->header_guard_prefix = "VITTE_";
  opt->runtime_header = NULL;
  opt->write_file = NULL;
  opt->write_file_user = NULL;
}

static vittec_emit_c_options_t vittec__opt_or_default(const vittec_emit_c_options_t* in) {
  vittec_emit_c_options_t o;
  vittec_emit_c_options_init(&o);
  if(in) {
    /* Copy only known fields if caller uses a different size. */
    uint32_t n = in->size ? in->size : (uint32_t)sizeof(*in);
    if(n > (uint32_t)sizeof(o)) n = (uint32_t)sizeof(o);
    memcpy(&o, in, n);

    /* Ensure defaults for critical pointers if absent. */
    if(!o.namespace_prefix) o.namespace_prefix = "vitte_";
    if(!o.header_guard_prefix) o.header_guard_prefix = "VITTE_";
  }
  return o;
}

/* -------------------------------------------------------------------------
 * Deterministic name mangling
 * ------------------------------------------------------------------------- */

static void vittec__mangle(
  char* out,
  size_t cap,
  const vittec_emit_c_options_t* opt,
  vittec_sv_t module_name,
  vittec_sv_t fn_name
) {
  if(!out || cap == 0) return;
  out[0] = 0;

  char mod[256];
  char name[256];
  vittec__sv_to_cstr(module_name, mod, sizeof(mod));
  vittec__sv_to_cstr(fn_name, name, sizeof(name));

  char smod[256];
  char sname[256];
  vittec__sanitize_ident(mod, smod, sizeof(smod));
  vittec__sanitize_ident(name, sname, sizeof(sname));

  const char* pre = (opt && opt->namespace_prefix) ? opt->namespace_prefix : "vitte_";

  if(opt && opt->mangle == VITTEC_EMIT_C_MANGLE_HASHED) {
    /* hashed = <pre><mod>_<name>__<hash8> */
    char full[600];
    int k = snprintf(full, sizeof(full), "%s%s::%s", mod, (mod[0] ? "" : ""), name);
    if(k < 0) k = 0;
    uint64_t h = vittec__fnv1a64(full, (size_t)k);
    uint32_t h32 = (uint32_t)(h ^ (h >> 32));
    char hx[9];
    vittec__hex8(h32, hx);

    if(smod[0]) {
      snprintf(out, cap, "%s%s_%s__%s", pre, smod, sname, hx);
    } else {
      snprintf(out, cap, "%s%s__%s", pre, sname, hx);
    }
  } else {
    /* simple = <pre><mod>_<name> */
    if(smod[0]) {
      snprintf(out, cap, "%s%s_%s", pre, smod, sname);
    } else {
      snprintf(out, cap, "%s%s", pre, sname);
    }
  }
}

/* -------------------------------------------------------------------------
 * Emission helpers
 * ------------------------------------------------------------------------- */

static int vittec__write_file_or_path(
  const vittec_emit_c_options_t* opt,
  const char* path,
  const void* data,
  size_t size
) {
  if(!path || !data) return VITTEC_EMIT_C_EINVAL;

  if(opt && opt->write_file) {
    int rc = opt->write_file(opt->write_file_user, path, data, size);
    return (rc == 0) ? VITTEC_EMIT_C_OK : VITTEC_EMIT_C_EIO;
  }

  FILE* f = fopen(path, "wb");
  if(!f) return VITTEC_EMIT_C_EIO;
  if(size && fwrite(data, 1, size, f) != size) {
    fclose(f);
    return VITTEC_EMIT_C_EIO;
  }
  fclose(f);
  return VITTEC_EMIT_C_OK;
}

static void vittec__emit_preamble_buf(vittec__buf_t* b, const vittec_emit_c_options_t* opt) {
  vittec__buf_printf(b, "/* generated by vittec (C emitter) */\n");

  if(opt && opt->emit_debug_comments) {
    vittec__buf_printf(b, "/* mode=%s mangle=%s runtime=%s */\n",
      (opt->mode == VITTEC_EMIT_C_MODE_FULL) ? "full" : "stubs",
      (opt->mangle == VITTEC_EMIT_C_MANGLE_HASHED) ? "hashed" : "simple",
      (opt->runtime == VITTEC_EMIT_C_RUNTIME_VITTE) ? "vitte" : "none");
  }

  vittec__buf_printf(b, "#include <stdint.h>\n");
  vittec__buf_printf(b, "#include <stddef.h>\n");

  if(opt && opt->runtime == VITTEC_EMIT_C_RUNTIME_VITTE && opt->runtime_header && opt->runtime_header[0]) {
    vittec__buf_printf(b, "#include \"%s\"\n", opt->runtime_header);
  }

  vittec__buf_printf(b, "\n");
}

static void vittec__emit_header_buf(
  vittec__buf_t* b,
  const vittec_emit_c_options_t* opt,
  const char* guard,
  const vittec_parse_unit_t* u
) {
  vittec__buf_printf(b, "#ifndef %s\n#define %s\n\n", guard, guard);
  vittec__buf_printf(b, "#include <stdint.h>\n#include <stddef.h>\n\n");
  vittec__buf_printf(b, "#ifdef __cplusplus\nextern \"C\" {\n#endif\n\n");

  if(opt && opt->emit_debug_comments) {
    vittec__buf_printf(b, "/* generated header: bootstrap prototypes */\n\n");
  }

  if(u && u->fns && u->fns_len) {
    for(uint32_t i=0; i<u->fns_len; i++) {
      char cname[512];
      vittec__mangle(cname, sizeof(cname), opt, u->module_name, u->fns[i].name);
      /* Bootstrap: signatures are not lowered yet, default to int(void). */
      vittec__buf_printf(b, "int %s(void);\n", cname);
    }
  }

  vittec__buf_printf(b, "\n#ifdef __cplusplus\n} /* extern \"C\" */\n#endif\n\n");
  vittec__buf_printf(b, "#endif /* %s */\n", guard);
}

static void vittec__emit_functions_buf(vittec__buf_t* b, const vittec_emit_c_options_t* opt, const vittec_parse_unit_t* u) {
  if(!u) {
    vittec__buf_printf(b, "/* (no parse unit) */\n\n");
    return;
  }

  if(opt && opt->emit_debug_comments) {
    char mod[256];
    vittec__sv_to_cstr(u->module_name, mod, sizeof(mod));
    vittec__buf_printf(b, "/* module: %s */\n", mod[0] ? mod : "(none)");
    vittec__buf_printf(b, "/* functions: %u */\n\n", (unsigned)u->fns_len);
  }

  if(u->fns && u->fns_len) {
    for(uint32_t i=0; i<u->fns_len; i++) {
      const vittec_fn_decl_t* fn = &u->fns[i];

      char cname[512];
      vittec__mangle(cname, sizeof(cname), opt, u->module_name, fn->name);

      if(opt && opt->emit_line_directives) {
        /* We only have byte spans at this stage; emit a comment placeholder. */
#if defined(_MSC_VER)
        vittec__buf_printf(b, "/* line: (byte offsets %u..%u) */\n", (unsigned)fn->sig_span.lo, (unsigned)fn->sig_span.hi);
#else
        vittec__buf_printf(b, "/* line: (byte offsets %u..%u) */\n", (unsigned)fn->sig_span.lo, (unsigned)fn->sig_span.hi);
#endif
      }

      if(opt && opt->emit_debug_comments) {
        vittec__buf_printf(b, "/* fn name span: %u..%u */\n", (unsigned)fn->name_span.lo, (unsigned)fn->name_span.hi);
        vittec__buf_printf(b, "/* fn sig span:  %u..%u */\n", (unsigned)fn->sig_span.lo, (unsigned)fn->sig_span.hi);
        vittec__buf_printf(b, "/* fn body span: %u..%u (has_body=%u) */\n", (unsigned)fn->body_span.lo, (unsigned)fn->body_span.hi, (unsigned)fn->has_body);
      }

      /* Bootstrap signature: int(void). */
      vittec__buf_printf(b, "int %s(void) {\n", cname);
      if(opt && opt->mode == VITTEC_EMIT_C_MODE_FULL) {
        vittec__buf_printf(b, "  /* TODO(full): lower Vitte body to C */\n");
      } else {
        vittec__buf_printf(b, "  /* TODO(stubs): lower Vitte body to C */\n");
      }
      vittec__buf_printf(b, "  (void)0;\n");
      vittec__buf_printf(b, "  return 0;\n");
      vittec__buf_printf(b, "}\n\n");
    }
  }
}

static void vittec__emit_main_buf(vittec__buf_t* b, const vittec_emit_c_options_t* opt, const vittec_parse_unit_t* u) {
  (void)opt;

  if(u && u->has_main) {
    /* Attempt to call lowered main if present. */
    /* Note: the bootstrap parser flags has_main when it sees `fn main`. */

    char cname[512];
    vittec_sv_t mod = u->module_name;
    vittec_sv_t name;
    name.data = "main";
    name.len = 4;
    vittec__mangle(cname, sizeof(cname), opt, mod, name);

    vittec__buf_printf(b, "int main(void) {\n");
    vittec__buf_printf(b, "  /* bootstrap: call lowered main stub */\n");
    vittec__buf_printf(b, "  return %s();\n", cname);
    vittec__buf_printf(b, "}\n");
  } else {
    vittec__buf_printf(b, "int main(void) {\n");
    vittec__buf_printf(b, "  /* bootstrap: no `fn main` detected */\n");
    vittec__buf_printf(b, "  return 0;\n");
    vittec__buf_printf(b, "}\n");
  }
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

int vittec_emit_c_file(const vittec_parse_unit_t* u, const char* out_path) {
  return vittec_emit_c_file_ex(u, out_path, NULL);
}

int vittec_emit_c_file_ex(
  const vittec_parse_unit_t* u,
  const char* out_path,
  const vittec_emit_c_options_t* opt_in
) {
  if(!out_path || !out_path[0]) return VITTEC_EMIT_C_EINVAL;

  vittec_emit_c_options_t opt = vittec__opt_or_default(opt_in);

  /* Resolve output paths. */
  char* base = NULL;
  char* c_path = NULL;
  char* h_path = NULL;

  if(opt.emit_header) {
    base = vittec__path_without_ext(out_path);
    if(!base) return VITTEC_EMIT_C_EINTERNAL;
    c_path = vittec__path_with_ext(base, ".c");
    h_path = vittec__path_with_ext(base, ".h");
    if(!c_path || !h_path) {
      free(base);
      if(c_path) free(c_path);
      if(h_path) free(h_path);
      return VITTEC_EMIT_C_EINTERNAL;
    }
  } else {
    c_path = vittec__strdup(out_path);
    if(!c_path) return VITTEC_EMIT_C_EINTERNAL;
  }

  /* Build C file content (buffer). */
  vittec__buf_t cbuf;
  vittec__buf_init(&cbuf);

  vittec__emit_preamble_buf(&cbuf, &opt);

  if(opt.emit_header) {
    /* include generated header */
    const char* inc = h_path;
    const char* leaf = inc;
    for(const char* p = inc; *p; p++) {
      if(*p == '/' || *p == '\\') leaf = p + 1;
    }
    vittec__buf_printf(&cbuf, "#include \"%s\"\n\n", leaf);
  }

  vittec__emit_functions_buf(&cbuf, &opt, u);
  vittec__emit_main_buf(&cbuf, &opt, u);

  /* Write C file. */
  int rc = vittec__write_file_or_path(&opt, c_path, cbuf.data ? cbuf.data : "", cbuf.len);

  /* Optional header emission. */
  if(rc == VITTEC_EMIT_C_OK && opt.emit_header) {
    vittec__buf_t hbuf;
    vittec__buf_init(&hbuf);

    /* guard derived from base name */
    const char* pre = opt.header_guard_prefix ? opt.header_guard_prefix : "VITTE_";

    char base_leaf[256];
    base_leaf[0] = 0;
    if(base) {
      const char* leaf = base;
      for(const char* p = base; *p; p++) {
        if(*p == '/' || *p == '\\') leaf = p + 1;
      }
      vittec__sanitize_ident(leaf, base_leaf, sizeof(base_leaf));
    } else {
      vittec__sanitize_ident("generated", base_leaf, sizeof(base_leaf));
    }

    /* to upper */
    for(size_t i=0; base_leaf[i]; i++) {
      base_leaf[i] = (char)toupper((unsigned char)base_leaf[i]);
    }

    char guard[512];
    snprintf(guard, sizeof(guard), "%s%s_H", pre, base_leaf[0] ? base_leaf : "GENERATED");

    vittec__emit_header_buf(&hbuf, &opt, guard, u);

    rc = vittec__write_file_or_path(&opt, h_path, hbuf.data ? hbuf.data : "", hbuf.len);

    vittec__buf_free(&hbuf);
  }

  vittec__buf_free(&cbuf);

  if(base) free(base);
  if(c_path) free(c_path);
  if(h_path) free(h_path);

  return rc;
}

