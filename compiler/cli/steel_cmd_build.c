/* TODO */

// steel_cmd_build.c
//
// steelc build command (workspace build driver).
//
// This module implements a deterministic, best-effort "build" command that:
// - Locates a workspace root (explicit --workspace or upward search).
// - Locates a Muffin manifest (mod.muf) at the root.
// - Performs a minimal validation of the header "muf <version>".
// - Delegates compilation of a single entrypoint unit using steel_compiler_compile().
//
// This is a bootstrap-friendly implementation: it does not attempt to resolve
// full dependency graphs yet. It exists to provide a stable CLI surface.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>

#include "steel_compiler.h"

// -----------------------------------------------------------------------------
// Small utilities
// -----------------------------------------------------------------------------

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

static size_t s_len(const char* s) {
  return s ? strlen(s) : 0u;
}

static void path_join2(char* out, size_t out_cap, const char* a, const char* b) {
  if (!out || out_cap == 0) return;
  out[0] = '\0';
  if (!a) a = "";
  if (!b) b = "";

  size_t na = strlen(a);
  bool need_slash = (na > 0 && a[na - 1] != '/');

  if (need_slash) {
    snprintf(out, out_cap, "%s/%s", a, b);
  } else {
    snprintf(out, out_cap, "%s%s", a, b);
  }
  out[out_cap - 1] = '\0';
}

static bool file_exists(const char* path) {
  FILE* f = fopen(path, "rb");
  if (!f) return false;
  fclose(f);
  return true;
}

static bool read_first_line(const char* path, char* out, size_t cap) {
  if (!out || cap == 0) return false;
  out[0] = '\0';

  FILE* f = fopen(path, "rb");
  if (!f) return false;

  if (!fgets(out, (int)cap, f)) {
    fclose(f);
    return false;
  }
  fclose(f);

  // Strip CR/LF
  size_t n = strlen(out);
  while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r')) {
    out[--n] = '\0';
  }
  return true;
}

static bool is_sep(char c) {
  return c == '/' || c == '\\';
}

static void path_dirname_inplace(char* path) {
  if (!path) return;
  size_t n = strlen(path);
  if (n == 0) return;

  // Strip trailing slashes
  while (n > 0 && is_sep(path[n - 1])) path[--n] = '\0';
  if (n == 0) { path[0] = '/'; path[1] = '\0'; return; }

  char* last = NULL;
  for (size_t i = 0; i < n; i++) {
    if (is_sep(path[i])) last = &path[i];
  }

  if (!last) {
    // No separator => current dir
    path[0] = '.';
    path[1] = '\0';
    return;
  }

  // Root?
  if (last == path) {
    path[1] = '\0';
    return;
  }

  *last = '\0';
}

static bool get_cwd(char* out, size_t cap) {
  if (!out || cap == 0) return false;
#if defined(_WIN32)
  // Minimal fallback; Windows support can be added later.
  (void)cap;
  out[0] = '.';
  out[1] = '\0';
  return true;
#else
  // POSIX
  extern char* getcwd(char*, size_t);
  return getcwd(out, cap) != NULL;
#endif
}

// Find workspace root:
// - If explicit is provided: use it.
// - Else: search upward from cwd for "mod.muf".
static bool find_workspace_root(const char* explicit_root, char* out_root, size_t out_cap) {
  if (!out_root || out_cap == 0) return false;
  out_root[0] = '\0';

  if (explicit_root && *explicit_root) {
    snprintf(out_root, out_cap, "%s", explicit_root);
    out_root[out_cap - 1] = '\0';
    return true;
  }

  char cur[1024];
  if (!get_cwd(cur, sizeof(cur))) return false;

  for (int depth = 0; depth < 64; depth++) {
    char mf[1200];
    path_join2(mf, sizeof(mf), cur, "mod.muf");
    if (file_exists(mf)) {
      snprintf(out_root, out_cap, "%s", cur);
      out_root[out_cap - 1] = '\0';
      return true;
    }

    // Stop at filesystem root
    if (streq(cur, "/") || streq(cur, ".") || streq(cur, "")) break;

    path_dirname_inplace(cur);
    if (streq(cur, "/") && file_exists("/mod.muf")) {
      snprintf(out_root, out_cap, "/");
      return true;
    }
  }

  return false;
}

static bool validate_muf_header(const char* mod_muf_path, int* out_ver) {
  if (out_ver) *out_ver = 0;

  char line[256];
  if (!read_first_line(mod_muf_path, line, sizeof(line))) return false;

  // Expected: "muf <int>" (allow leading/trailing spaces)
  const char* p = line;
  while (*p == ' ' || *p == '\t') p++;
  if (!starts_with(p, "muf")) return false;
  p += 3;
  while (*p == ' ' || *p == '\t') p++;
  if (*p < '0' || *p > '9') return false;

  int v = 0;
  while (*p >= '0' && *p <= '9') {
    v = (v * 10) + (*p - '0');
    p++;
  }

  if (out_ver) *out_ver = v;
  return true;
}

// -----------------------------------------------------------------------------
// Public entry
// -----------------------------------------------------------------------------

// Build command contract:
// - input: optional entrypoint unit (defaults to "src/main.vitte" if exists)
// - workspace_root: optional explicit root
// - output: optional artifact path (backend-specific)
// - emit: ir|c|asm|obj
// - opt/debug/werror/json/verbose forwarded

int steel_cmd_build_run(const steel_compile_opts* base_opts,
                        const char* maybe_input,
                        steel_compile_diag* diag) {
  if (diag) { diag->code = 0; diag->message[0] = '\0'; }
  if (!base_opts) {
    if (diag) {
      diag->code = 2;
      snprintf(diag->message, sizeof(diag->message), "internal: base_opts is null");
    }
    return 2;
  }

  // Resolve workspace
  char root[1024];
  if (!find_workspace_root(base_opts->workspace_root, root, sizeof(root))) {
    if (diag) {
      diag->code = 2;
      snprintf(diag->message, sizeof(diag->message),
               "workspace root not found (pass --workspace or place mod.muf in root)");
      diag->message[sizeof(diag->message) - 1] = '\0';
    }
    return 2;
  }

  // Validate mod.muf
  char mod_muf[1200];
  path_join2(mod_muf, sizeof(mod_muf), root, "mod.muf");
  if (!file_exists(mod_muf)) {
    if (diag) {
      diag->code = 2;
      snprintf(diag->message, sizeof(diag->message), "mod.muf not found in workspace: %s", root);
      diag->message[sizeof(diag->message) - 1] = '\0';
    }
    return 2;
  }

  int muf_ver = 0;
  if (!validate_muf_header(mod_muf, &muf_ver)) {
    if (diag) {
      diag->code = 2;
      snprintf(diag->message, sizeof(diag->message), "invalid mod.muf header (expected: 'muf <version>'): %s", mod_muf);
      diag->message[sizeof(diag->message) - 1] = '\0';
    }
    return 2;
  }

  (void)muf_ver; // reserved for future compatibility checks

  // Determine input
  char def_in[1200];
  const char* in = maybe_input;
  if (!in || !*in) {
    path_join2(def_in, sizeof(def_in), root, "src/main.vitte");
    if (file_exists(def_in)) {
      in = def_in;
    } else {
      // fallback: root/main.vitte
      path_join2(def_in, sizeof(def_in), root, "main.vitte");
      if (file_exists(def_in)) {
        in = def_in;
      }
    }
  }

  if (!in || !*in) {
    if (diag) {
      diag->code = 2;
      snprintf(diag->message, sizeof(diag->message),
               "no input specified and default entrypoint not found (src/main.vitte or main.vitte)");
      diag->message[sizeof(diag->message) - 1] = '\0';
    }
    return 2;
  }

  // Compose opts
  steel_compile_opts opts = *base_opts;
  opts.workspace_root = root; // resolved root
  opts.input_path = in;

  // Delegate
  return steel_compiler_compile(&opts, diag);
}