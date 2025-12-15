/*
  steelc — Vitte compiler (single-stage) — scaffold
  Supports: module? + fn main() -> i32 return <int>; .end
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "steel/vitte/parser_core.h"
#include "steel/compiler/backend_c.h"
#include "steel/diag/diag.h"

static char* slurp(const char* path, size_t* out_len) {
  FILE* f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (n < 0) { fclose(f); return NULL; }
  char* buf = (char*)malloc((size_t)n + 1);
  if (!buf) { fclose(f); return NULL; }
  size_t r = fread(buf, 1, (size_t)n, f);
  fclose(f);
  buf[r] = 0;
  if (out_len) *out_len = r;
  return buf;
}

static void usage(void) {
  puts("usage: steelc <input.vitte> -o <out.c>");
}

int main(int argc, char** argv) {
  if (argc < 2) { usage(); return 2; }

  const char* in = NULL;
  const char* out = "target/out.c";

  in = argv[1];
  for (int i = 2; i < argc; i++) {
    if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) { out = argv[++i]; continue; }
  }

  size_t n = 0;
  char* src = slurp(in, &n);
  if (!src) { fprintf(stderr, "steelc: cannot read %s\n", in); return 1; }

  vitte_parser p;
  vitte_parser_init(&p, src, (uint32_t)n, 0);

  ast_program prog;
  if (vitte_parse_program(&p, &prog) != 0) {
    free(src);
    return 1;
  }

  if (backend_c_emit_program(&prog, out) != 0) {
    steel_diag_emit(STEEL_DIAG_ERROR, (steel_span){0,0,0}, "backend_c_emit_program failed");
    free(src);
    return 1;
  }

  free(src);
  return 0;
}
