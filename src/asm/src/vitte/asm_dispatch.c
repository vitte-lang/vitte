#include "vitte/asm.h"
#include "vitte/cpu.h"
#include <string.h>

/* ----------------------------
 * C reference implementations
 * ---------------------------- */
void* vitte_memcpy_ref(void* dst, const void* src, size_t n) { return memcpy(dst, src, n); }
void* vitte_memset_ref(void* dst, int byte, size_t n) { return memset(dst, byte, n); }

uint64_t vitte_fnv1a64_ref(const void* data, size_t n) {
  const unsigned char* p = (const unsigned char*)data;
  uint64_t h = 1469598103934665603ull;
  for(size_t i=0;i<n;i++) { h ^= (uint64_t)p[i]; h *= 1099511628211ull; }
  return h;
}

/* Optional: if you don't provide it, asm utf8 stubs will still link if not used. */
__attribute__((weak)) int vitte_utf8_validate_ref(const uint8_t* p, size_t n) {
  /* Simple, correct UTF-8 validator (scalar). */
  size_t i = 0;
  while(i < n) {
    uint8_t c = p[i];
    if(c < 0x80) { i++; continue; }

    if((c & 0xE0) == 0xC0) {
      if(i+1 >= n) return 0;
      uint8_t c1 = p[i+1];
      if((c1 & 0xC0) != 0x80) return 0;
      uint32_t cp = ((uint32_t)(c & 0x1F) << 6) | (uint32_t)(c1 & 0x3F);
      if(cp < 0x80) return 0; /* overlong */
      i += 2; continue;
    }

    if((c & 0xF0) == 0xE0) {
      if(i+2 >= n) return 0;
      uint8_t c1 = p[i+1], c2 = p[i+2];
      if((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return 0;
      uint32_t cp = ((uint32_t)(c & 0x0F) << 12) | ((uint32_t)(c1 & 0x3F) << 6) | (uint32_t)(c2 & 0x3F);
      if(cp < 0x800) return 0; /* overlong */
      if(cp >= 0xD800 && cp <= 0xDFFF) return 0; /* surrogate */
      i += 3; continue;
    }

    if((c & 0xF8) == 0xF0) {
      if(i+3 >= n) return 0;
      uint8_t c1 = p[i+1], c2 = p[i+2], c3 = p[i+3];
      if((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return 0;
      uint32_t cp = ((uint32_t)(c & 0x07) << 18) | ((uint32_t)(c1 & 0x3F) << 12) | ((uint32_t)(c2 & 0x3F) << 6) | (uint32_t)(c3 & 0x3F);
      if(cp < 0x10000 || cp > 0x10FFFF) return 0; /* overlong / out of range */
      i += 4; continue;
    }

    return 0;
  }
  return 1;
}

/* ---------------------------------
 * External asm variant declarations
 * --------------------------------- */
#if defined(__x86_64__) || defined(_M_X64)
extern void* vitte_memcpy_baseline(void* dst, const void* src, size_t n);
extern void* vitte_memcpy_sse2(void* dst, const void* src, size_t n);
extern void* vitte_memcpy_avx2(void* dst, const void* src, size_t n);

extern void* vitte_memset_baseline(void* dst, int byte, size_t n);
extern void* vitte_memset_sse2(void* dst, int byte, size_t n);
extern void* vitte_memset_avx2(void* dst, int byte, size_t n);

extern uint64_t vitte_fnv1a64_asm(const void* data, size_t n);
extern int vitte_utf8_validate_asm(const uint8_t* p, size_t n);
#elif defined(__aarch64__) || defined(_M_ARM64)
extern void* vitte_memcpy_baseline(void* dst, const void* src, size_t n);
extern void* vitte_memcpy_neon(void* dst, const void* src, size_t n);

extern void* vitte_memset_baseline(void* dst, int byte, size_t n);
extern void* vitte_memset_neon(void* dst, int byte, size_t n);

extern uint64_t vitte_fnv1a64_asm(const void* data, size_t n);
extern int vitte_utf8_validate_asm(const uint8_t* p, size_t n);
#endif

/* ----------------------------
 * Dispatch via function pointers
 * ---------------------------- */
typedef void*    (*memcpy_fn)(void*, const void*, size_t);
typedef void*    (*memset_fn)(void*, int, size_t);
typedef uint64_t (*hash_fn)(const void*, size_t);
typedef int      (*utf8_fn)(const uint8_t*, size_t);

static memcpy_fn g_memcpy = 0;
static memset_fn g_memset = 0;
static hash_fn   g_hash   = 0;
static utf8_fn   g_utf8   = 0;

static void vitte_asm_init(void) {
  vitte_cpu_features f = vitte_cpu_detect();

#if defined(__x86_64__) || defined(_M_X64)
  g_memcpy = vitte_memcpy_baseline;
  g_memset = vitte_memset_baseline;
  if(f.has_avx2) {
    g_memcpy = vitte_memcpy_avx2;
    g_memset = vitte_memset_avx2;
  } else if(f.has_sse2) {
    g_memcpy = vitte_memcpy_sse2;
    g_memset = vitte_memset_sse2;
  }
  g_hash = vitte_fnv1a64_asm;
  g_utf8 = vitte_utf8_validate_asm;
#elif defined(__aarch64__) || defined(_M_ARM64)
  g_memcpy = vitte_memcpy_baseline;
  g_memset = vitte_memset_baseline;
  if(f.has_neon) {
    g_memcpy = vitte_memcpy_neon;
    g_memset = vitte_memset_neon;
  }
  g_hash = vitte_fnv1a64_asm;
  g_utf8 = vitte_utf8_validate_asm;
#else
  g_memcpy = vitte_memcpy_ref;
  g_memset = vitte_memset_ref;
  g_hash   = vitte_fnv1a64_ref;
  g_utf8   = vitte_utf8_validate_ref;
#endif
}

static inline void ensure_init(void) {
  if(!g_memcpy) vitte_asm_init();
}

void* vitte_memcpy_fast(void* dst, const void* src, size_t n) { ensure_init(); return g_memcpy(dst, src, n); }
void* vitte_memset_fast(void* dst, int byte, size_t n) { ensure_init(); return g_memset(dst, byte, n); }
uint64_t vitte_fnv1a64_fast(const void* data, size_t n) { ensure_init(); return g_hash(data, n); }
int vitte_utf8_validate_fast(const uint8_t* p, size_t n) { ensure_init(); return g_utf8(p, n); }
