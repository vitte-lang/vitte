#include "vitte/cpu.h"

#if defined(__x86_64__) || defined(_M_X64)
static void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
#if defined(_MSC_VER)
  int regs[4];
  __cpuidex(regs, (int)leaf, (int)subleaf);
  *a = (uint32_t)regs[0]; *b = (uint32_t)regs[1]; *c = (uint32_t)regs[2]; *d = (uint32_t)regs[3];
#else
  uint32_t ra, rb, rc, rd;
  __asm__ volatile("cpuid" : "=a"(ra), "=b"(rb), "=c"(rc), "=d"(rd) : "a"(leaf), "c"(subleaf));
  *a = ra; *b = rb; *c = rc; *d = rd;
#endif
}

static uint64_t xgetbv(uint32_t xcr) {
#if defined(_MSC_VER)
  return _xgetbv(xcr);
#else
  uint32_t eax, edx;
  __asm__ volatile(".byte 0x0f, 0x01, 0xd0" : "=a"(eax), "=d"(edx) : "c"(xcr));
  return ((uint64_t)edx << 32) | (uint64_t)eax;
#endif
}
#endif

vitte_cpu_features vitte_cpu_detect(void) {
  vitte_cpu_features f = {0};

#if defined(__aarch64__) || defined(_M_ARM64)
  /* arm64: assume NEON/ASIMD on most modern targets */
  f.has_neon = 1;

  /* Linux: refine via getauxval if available */
  #if defined(__linux__)
    #include <sys/auxv.h>
    #include <asm/hwcap.h>
    unsigned long hw = getauxval(AT_HWCAP);
    #ifdef HWCAP_ASIMD
    if(hw & HWCAP_ASIMD) f.has_neon = 1;
    #endif
    #ifdef HWCAP_CRC32
    if(hw & HWCAP_CRC32) f.has_crc32c = 1;
    #endif
  #endif

#elif defined(__x86_64__) || defined(_M_X64)
  uint32_t a,b,c,d;
  cpuid(1, 0, &a, &b, &c, &d);
  f.has_sse2 = (d >> 26) & 1;

  /* AVX/OSXSAVE check */
  int has_avx = (c >> 28) & 1;
  int has_osxsave = (c >> 27) & 1;

  if(has_avx && has_osxsave) {
    uint64_t xcr0 = xgetbv(0);
    /* XMM (bit1) + YMM (bit2) must be enabled by OS */
    if((xcr0 & 0x6) == 0x6) {
      cpuid(7, 0, &a, &b, &c, &d);
      f.has_avx2 = (b >> 5) & 1;
    }
  }
#endif

  return f;
}
