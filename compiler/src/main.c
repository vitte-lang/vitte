#include "vittec/vittec.h"
#include "vittec/version.h"
#if VITTEC_ENABLE_RUST_API
  #include "vittec/muf.h"
  #include "vitte_rust_api.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) {
  printf("vittec %s\\n", vittec_version_string());
  printf("usage:\\n");
  printf("  vittec [--tokens|--emit-c] <input.vitte> [-o out]\\n");
#if VITTEC_ENABLE_RUST_API
  printf("  vittec muf fmt <file.muf>\\n");
#endif
  printf("\\n");
}

#if VITTEC_ENABLE_RUST_API
static int cmd_muf_fmt(const char* path) {
  FILE* f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "error: cannot open: %s\\n", path);
    return 2;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    fprintf(stderr, "error: cannot seek: %s\\n", path);
    fclose(f);
    return 2;
  }

  long n = ftell(f);
  if (n < 0) {
    fprintf(stderr, "error: cannot tell: %s\\n", path);
    fclose(f);
    return 2;
  }
  if (fseek(f, 0, SEEK_SET) != 0) {
    fprintf(stderr, "error: cannot seek: %s\\n", path);
    fclose(f);
    return 2;
  }

  size_t len = (size_t)n;
  char* buf = (char*)malloc(len + 1);
  if (!buf) {
    fprintf(stderr, "error: out of memory\\n");
    fclose(f);
    return 2;
  }

  size_t rd = fread(buf, 1, len, f);
  fclose(f);
  if (rd != len) {
    fprintf(stderr, "error: short read: %s\\n", path);
    free(buf);
    return 2;
  }
  buf[len] = '\0';

  size_t out_len = 0;
  size_t out_cap = (len * 2u) + 4096u;
  char* out = (char*)malloc(out_cap);
  if (!out) {
    fprintf(stderr, "error: out of memory\\n");
    free(buf);
    return 2;
  }

  vitte_err_code_t rc = vittec_muf_normalize(buf, len, out, out_cap, &out_len);
  if (rc != VITTE_ERR_OK) {
    fprintf(stderr, "error: muf normalize failed (code=%d)\\n", rc);
    free(out);
    free(buf);
    return 1;
  }

  fwrite(out, 1, out_len, stdout);
  free(out);
  free(buf);
  return 0;
}
#else
static int cmd_muf_fmt(const char* path) {
  (void)path;
  fprintf(stderr, "error: muf support requires a Rust-enabled build\\n");
  return 2;
}
#endif

int main(int argc, char** argv) {
#if VITTEC_ENABLE_RUST_API
  if (vitte_rust_api_abi_version() != VITTE_RUST_API_ABI_VERSION) {
    fprintf(stderr,
            "error: Rust API ABI mismatch; rebuild the Rust staticlib with the "
            "matching toolchain\\n");
    return 1;
  }
#endif
  if (argc < 2) { usage(); return 2; }

  if (!strcmp(argv[1], "muf")) {
    if (argc >= 3 && (!strcmp(argv[2], "-h") || !strcmp(argv[2], "--help"))) {
      usage();
      return 0;
    }
    if (argc == 4 && !strcmp(argv[2], "fmt")) {
      return cmd_muf_fmt(argv[3]);
    }
    fprintf(stderr, "error: usage: vittec muf fmt <file.muf>\\n");
    return 2;
  }

  vittec_compile_options_t opt;
  memset(&opt, 0, sizeof(opt));
  opt.emit_kind = VITTEC_EMIT_TOKENS;

  const char* in = NULL;
  const char* out = NULL;

  for (int i = 1; i < argc; i++) {
    const char* a = argv[i];
    if (!strcmp(a, "-h") || !strcmp(a, "--help")) { usage(); return 0; }
    if (!strcmp(a, "--version")) { printf("%s\\n", vittec_version_string()); return 0; }
    if (!strcmp(a, "--tokens")) { opt.emit_kind = VITTEC_EMIT_TOKENS; continue; }
    if (!strcmp(a, "--emit-c")) { opt.emit_kind = VITTEC_EMIT_C; continue; }
    if (!strcmp(a, "--json-diag")) { opt.json_diagnostics = 1; continue; }
    if (!strcmp(a, "-o")) {
      if (i + 1 >= argc) { fprintf(stderr, "error: -o requires a path\\n"); return 2; }
      out = argv[++i];
      continue;
    }
    if (!in) { in = a; continue; }
    fprintf(stderr, "error: unexpected arg: %s\\n", a);
    return 2;
  }

  if (!in) { usage(); return 2; }

  opt.input_path = in;
  opt.output_path = out;

  vittec_session_t* s = vittec_session_new();
  int rc = vittec_compile(s, &opt);
  vittec_session_free(s);
  return rc;
}
