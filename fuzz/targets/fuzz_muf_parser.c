// fuzz_muf_parser.c
// Fuzz target: MUF (muffin manifest) parser placeholder.

#include <stddef.h>
#include <stdint.h>

#include "fuzz/fuzz.h"
#include "fuzz/targets/fuzz_target_scaffold.h"

static int muf_placeholder(const uint8_t* data, size_t size) {
  fuzz_reader r = fuzz_target_reader(data, size);
  uint8_t mode = fuzz_target_mode(&r);

  size_t max = fuzz_reader_remaining(&r);
  if (max > (1u << 20))
    max = (1u << 20);

  fuzz_bytes_view all;
  if (!fuzz_reader_read_bytes_view(&r, max, &all))
    return 0;

  const uint8_t* p = all.data;
  const uint8_t* end = all.data + all.size;
  uint32_t lines = 0;
  while (p < end && lines < 20000) {
    const uint8_t* line = p;
    while (p < end && *p != '\n')
      p++;
    size_t len = (size_t)(p - line);
    if (p < end && *p == '\n')
      p++;
    lines++;

    if (len == 0)
      continue;
    uint32_t h = fuzz_fnv1a32(line, len);
    FUZZ_TRACE_TAG_U32("muf.line", h);
    if ((mode & 1u) && line[0] == '[') {
      FUZZ_TRACE_TAG("muf.section");
    } else if (line[0] == '#') {
      FUZZ_TRACE_TAG("muf.comment");
    }
  }
  return 0;
}

FUZZ_TARGET(fuzz_muf_parser_target) {
  return muf_placeholder(data, size);
}

#define FUZZ_DRIVER_TARGET fuzz_muf_parser_target
#include "fuzz/fuzz_driver.h"
