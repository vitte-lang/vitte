#include "vittec/vittec.h"
#include "vittec/version.h"
#include <stdio.h>
#include <string.h>

static void usage(void) {
  printf("vittec %s\\n", vittec_version_string());
  printf("usage: vittec [--tokens|--emit-c] <input.vitte> [-o out]\\n");
}

int main(int argc, char** argv) {
  if (argc < 2) { usage(); return 2; }

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
