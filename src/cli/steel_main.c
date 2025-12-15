/*
  steel — build driver — scaffold
  - parses muffin.muf
  - validates mandatory blocks
  - prints build plan (and can execute toolchain later)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "steel/muf/muf_parse.h"
#include "steel/muf/muf_validate.h"
#include "steel/diag/diag.h"

static void usage(void) {
  puts("usage: steel <cmd> [args]\n"
       "  cmds: check | plan\n"
       "  check: parse+validate muffin.muf\n"
       "  plan : print packages + toolchain\n");
}

static const char* kv(const muf_block* b, const char* key) {
  for (size_t i = 0; i < b->kv_len; i++) if (strcmp(b->kvs[i].key, key) == 0) return b->kvs[i].val;
  return NULL;
}

int main(int argc, char** argv) {
  const char* cmd = (argc >= 2) ? argv[1] : NULL;
  if (!cmd) { usage(); return 2; }

  muf_program p;
  if (muf_parse_file("muffin.muf", &p) != 0) {
    return 1;
  }

  muf_validate_result vr = muf_validate(&p);
  if (!vr.ok) {
    steel_diag_emit(STEEL_DIAG_ERROR, (steel_span){0,0,0}, vr.error_msg);
    return 1;
  }

  if (strcmp(cmd, "check") == 0) {
    puts("muf: ok");
    return 0;
  }

  if (strcmp(cmd, "plan") == 0) {
    const muf_block* tc = NULL;
    for (size_t i = 0; i < p.len; i++) if (p.blocks[i].kind == MUF_BLOCK_TOOLCHAIN) { tc = &p.blocks[i]; break; }
    puts("== toolchain ==");
    if (tc) {
      printf("  cc      : %s\n", kv(tc, "cc"));
      printf("  backend : %s\n", kv(tc, "backend"));
      printf("  cflags  : %s\n", kv(tc, "cflags"));
    }
    puts("== packages ==");
    for (size_t i = 0; i < p.len; i++) {
      const muf_block* b = &p.blocks[i];
      if (b->kind != MUF_BLOCK_PACKAGE) continue;
      printf("  - %s (%s) src=%s\n", kv(b,"name"), kv(b,"kind"), kv(b,"src"));
    }
    puts("\nTODO: implement build graph + invocation.");
    return 0;
  }

  usage();
  return 2;
}
