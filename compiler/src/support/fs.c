#include "vittec/support/fs.h"
#include "vittec/support/assert.h"
#include <stdio.h>
#include <stdlib.h>

int vittec_read_entire_file(const char* path, vittec_file_buf_t* out) {
  out->data = NULL;
  out->len = 0;

  FILE* f = fopen(path, "rb");
  if (!f) return 1;

  if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 1; }
  long sz = ftell(f);
  if (sz < 0) { fclose(f); return 1; }
  if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 1; }

  char* buf = (char*)malloc((size_t)sz + 1);
  VITTEC_ASSERT(buf != NULL);

  size_t got = fread(buf, 1, (size_t)sz, f);
  fclose(f);

  if (got != (size_t)sz) { free(buf); return 1; }
  buf[sz] = 0;

  out->data = buf;
  out->len = (uint64_t)sz;
  return 0;
}

void vittec_free_file_buf(vittec_file_buf_t* b) {
  if (b->data) free(b->data);
  b->data = NULL;
  b->len = 0;
}
