#include "bench/bench.h"
#include <stdint.h>
#include <string.h>

static const char* g_json =
  "{\"k\":123,\"arr\":[1,2,3,4,5],\"s\":\"hello\",\"b\":true,\"n\":null}";

static volatile uint64_t sink = 0;

void bm_json_parse(void* ctx) {
  (void)ctx;
  /* toy: count characters + cheap token scan */
  const char* p = g_json;
  uint64_t n = 0;
  while(*p) {
    char c = *p++;
    if(c == '{' || c == '}' || c == '[' || c == ']' || c == ':' || c == ',') n++;
    else if((unsigned char)c >= '0' && (unsigned char)c <= '9') n += 2;
  }
  sink ^= n;
}
