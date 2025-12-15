#include "bench/bench.h"
#include <stdint.h>
#include <string.h>
#include "bench/bench.h"
#include <stdint.h>
#include <string.h>

/*
  bm_json_parse.c (max)

  Goal:
  - Macro benchmark that approximates real JSON parse costs without external deps.
  - Validates structure, parses strings (incl escapes), parses numbers (int/frac/exp),
    tracks nesting, and touches most bytes.

  Notes:
  - This is still a "toy" parser (no full Unicode validation), but it is far closer
    to a realistic workload than a delimiter counter.
  - The macro runner already loops for ~seconds, so this function should do
    one moderately sized parse per call.
*/

/* small */
static const char* g_json_s =
  "{\"k\":123,\"arr\":[1,2,3,4,5],\"s\":\"hello\",\"b\":true,\"n\":null}";

/* medium */
static const char* g_json_m =
  "{"
  "\"user\":{\"id\":42,\"name\":\"Vincent\\nDev\",\"tags\":[\"c\",\"rust\",\"vm\",\"ffi\"],\"active\":true},"
  "\"metrics\":[{\"t\":1,\"v\":0.25},{\"t\":2,\"v\":1.5e2},{\"t\":3,\"v\":-3.75}],"
  "\"cfg\":{\"opt\":{\"lto\":true,\"codegen_units\":1},\"arch\":\"arm64\"},"
  "\"note\":\"escapes: \\\"quote\\\" \\\\ backslash \\/ slash \\t tab\""
  "}";

/* large (constructed by repeating patterns) */
static const char* g_json_l =
  "{\n"
  "  \"items\": [\n"
  "    {\"id\":1,\"name\":\"alpha\",\"vals\":[1,2,3,4,5],\"ok\":true},\n"
  "    {\"id\":2,\"name\":\"beta\",\"vals\":[6,7,8,9,10],\"ok\":false},\n"
  "    {\"id\":3,\"name\":\"gamma\",\"vals\":[11,12,13,14,15],\"ok\":true},\n"
  "    {\"id\":4,\"name\":\"delta\",\"vals\":[16,17,18,19,20],\"ok\":true}\n"
  "  ],\n"
  "  \"meta\": {\"count\": 4, \"ver\": \"0.1.0\", \"desc\": \"bench json parse large\"},\n"
  "  \"floats\": [0.0, 1.0, -1.25, 3.14159, 2.99792458e8, -6.022e23],\n"
  "  \"nested\": {\"a\":{\"b\":{\"c\":{\"d\":[1,{\"e\":\"x\"}],\"f\":null}}}}\n"
  "}\n";

static const char* g_docs[] = { NULL, NULL, NULL };

static volatile uint64_t sink = 0;
static uint32_t rng_state = 0xC0FFEEu;

static BENCH_INLINE uint32_t rng_u32(void) {
  /* LCG: deterministic, cheap */
  rng_state = (uint32_t)(rng_state * 1664525u + 1013904223u);
  return rng_state;
}

static BENCH_INLINE int is_ws(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static BENCH_INLINE const char* skip_ws(const char* p) {
  while(is_ws(*p)) p++;
  return p;
}

static BENCH_INLINE int is_digit(char c) {
  return (unsigned char)c >= '0' && (unsigned char)c <= '9';
}

static const char* parse_string(const char* p, uint64_t* acc) {
  /* p points to opening '"' */
  if(*p != '"') return NULL;
  p++;
  uint64_t len = 0;
  for(;;) {
    char c = *p++;
    if(c == '\0') return NULL;
    if(c == '"') break;
    if(c == '\\') {
      /* escape */
      char e = *p++;
      if(e == '\0') return NULL;
      switch(e) {
        case '"': case '\\': case '/': case 'b': case 'f': case 'n': case 'r': case 't':
          len++;
          break;
        case 'u': {
          /* accept 4 hex digits (no surrogate checks) */
          for(int i=0;i<4;i++) {
            char h = *p++;
            if(h == '\0') return NULL;
            int ok = (h >= '0' && h <= '9') || (h >= 'a' && h <= 'f') || (h >= 'A' && h <= 'F');
            if(!ok) return NULL;
          }
          len++;
        } break;
        default:
          return NULL;
      }
    } else {
      /* disallow control chars */
      if((unsigned char)c < 0x20) return NULL;
      len++;
    }
  }
  *acc ^= (len * 1315423911ull);
  return p;
}

static const char* parse_number(const char* p, uint64_t* acc) {
  /* JSON number: -? int frac? exp? */
  const char* start = p;
  if(*p == '-') p++;

  if(*p == '0') {
    p++;
  } else {
    if(!is_digit(*p)) return NULL;
    while(is_digit(*p)) p++;
  }

  if(*p == '.') {
    p++;
    if(!is_digit(*p)) return NULL;
    while(is_digit(*p)) p++;
  }

  if(*p == 'e' || *p == 'E') {
    p++;
    if(*p == '+' || *p == '-') p++;
    if(!is_digit(*p)) return NULL;
    while(is_digit(*p)) p++;
  }

  /* fold a small hash of the number bytes */
  uint64_t h = 1469598103934665603ull;
  for(const char* q = start; q < p; q++) {
    h ^= (uint64_t)(unsigned char)*q;
    h *= 1099511628211ull;
  }
  *acc ^= h;
  return p;
}

static const char* parse_value(const char* p, uint64_t* acc, int depth);

static const char* parse_array(const char* p, uint64_t* acc, int depth) {
  if(*p != '[') return NULL;
  p = skip_ws(p + 1);

  if(*p == ']') return p + 1;

  for(;;) {
    p = parse_value(p, acc, depth + 1);
    if(!p) return NULL;
    p = skip_ws(p);

    if(*p == ',') {
      p = skip_ws(p + 1);
      continue;
    }
    if(*p == ']') return p + 1;
    return NULL;
  }
}

static const char* parse_object(const char* p, uint64_t* acc, int depth) {
  if(*p != '{') return NULL;
  p = skip_ws(p + 1);

  if(*p == '}') return p + 1;

  for(;;) {
    /* key */
    p = parse_string(p, acc);
    if(!p) return NULL;
    p = skip_ws(p);
    if(*p != ':') return NULL;
    p = skip_ws(p + 1);

    /* value */
    p = parse_value(p, acc, depth + 1);
    if(!p) return NULL;
    p = skip_ws(p);

    if(*p == ',') {
      p = skip_ws(p + 1);
      continue;
    }
    if(*p == '}') return p + 1;
    return NULL;
  }
}

static const char* parse_value(const char* p, uint64_t* acc, int depth) {
  if(depth > 64) return NULL;
  p = skip_ws(p);

  switch(*p) {
    case '"':
      return parse_string(p, acc);
    case '{':
      *acc += 17u;
      return parse_object(p, acc, depth);
    case '[':
      *acc += 31u;
      return parse_array(p, acc, depth);
    case 't':
      if(p[0]=='t' && p[1]=='r' && p[2]=='u' && p[3]=='e') { *acc ^= 0xA5A5u; return p + 4; }
      return NULL;
    case 'f':
      if(p[0]=='f' && p[1]=='a' && p[2]=='l' && p[3]=='s' && p[4]=='e') { *acc ^= 0x5A5Au; return p + 5; }
      return NULL;
    case 'n':
      if(p[0]=='n' && p[1]=='u' && p[2]=='l' && p[3]=='l') { *acc ^= 0xDEADu; return p + 4; }
      return NULL;
    default:
      if(*p == '-' || is_digit(*p)) return parse_number(p, acc);
      return NULL;
  }
}

void bm_json_parse(void* ctx) {
  (void)ctx;

  /* init doc table once */
  if(!g_docs[0]) {
    g_docs[0] = g_json_s;
    g_docs[1] = g_json_m;
    g_docs[2] = g_json_l;
  }

  /* rotate docs to avoid constant-only micro-optimizations */
  const uint32_t r = rng_u32();
  const char* doc = g_docs[(size_t)(r % 3u)];

  uint64_t acc = 0;
  const char* p = parse_value(doc, &acc, 0);
  if(!p) {
    /* should not happen: doc is valid; still mutate sink deterministically */
    sink ^= 0xBAD0BAD0ull;
    return;
  }
  p = skip_ws(p);
  if(*p != '\0') {
    /* trailing junk */
    sink ^= 0xBADC0FFEEull;
    return;
  }

  /* fold size and some bytes to ensure the loop touches memory */
  const size_t n = strlen(doc);
  acc ^= (uint64_t)n * 11400714819323198485ull;
  if(n > 0) {
    acc ^= (uint64_t)(unsigned char)doc[0] << 1;
    acc ^= (uint64_t)(unsigned char)doc[n/2] << 3;
    acc ^= (uint64_t)(unsigned char)doc[n-1] << 5;
  }

  sink ^= acc;
}
