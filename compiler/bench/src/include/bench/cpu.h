/*
  bench/cpu.h

  CPU / platform probing utilities for the Vitte benchmark harness.

  Responsibilities
  - Best-effort detection of CPU topology and capabilities.
  - Best-effort jitter reduction helpers:
      - pin current thread to a CPU
      - elevate priority
      - query CPU frequency / invariant TSC presence
  - Expose stable, testable data models for reporting.

  Notes
  - All probes are best-effort and may fail silently depending on permissions.
  - This header is C17 and C++ friendly.
  - Implementation is header-only and uses platform #ifdefs.

  Supported platforms (best effort)
  - Windows (Win32)
  - Linux (glibc/musl)
  - macOS (Darwin)
  - FreeBSD / OpenBSD / NetBSD (partial)
  - Solaris/Illumos (partial)

  Dependencies
  - bench/detail/compat.h for basic portability wrappers
  - bench/compiler.h for intrinsic helpers
*/

#pragma once
#ifndef VITTE_BENCH_CPU_H
#define VITTE_BENCH_CPU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "bench/compiler.h"
#include "bench/detail/compat.h"

/* -------------------------------------------------------------------------- */
/* OS detection                                                                */
/* -------------------------------------------------------------------------- */

#if defined(_WIN32) || defined(_WIN64)
  #define VITTE_BENCH_OS_WINDOWS 1
#else
  #define VITTE_BENCH_OS_WINDOWS 0
#endif

#if defined(__linux__)
  #define VITTE_BENCH_OS_LINUX 1
#else
  #define VITTE_BENCH_OS_LINUX 0
#endif

#if defined(__APPLE__) && defined(__MACH__)
  #define VITTE_BENCH_OS_DARWIN 1
#else
  #define VITTE_BENCH_OS_DARWIN 0
#endif

#if defined(__FreeBSD__)
  #define VITTE_BENCH_OS_FREEBSD 1
#else
  #define VITTE_BENCH_OS_FREEBSD 0
#endif

#if defined(__NetBSD__)
  #define VITTE_BENCH_OS_NETBSD 1
#else
  #define VITTE_BENCH_OS_NETBSD 0
#endif

#if defined(__OpenBSD__)
  #define VITTE_BENCH_OS_OPENBSD 1
#else
  #define VITTE_BENCH_OS_OPENBSD 0
#endif

#if defined(__sun) && defined(__SVR4)
  #define VITTE_BENCH_OS_SOLARIS 1
#else
  #define VITTE_BENCH_OS_SOLARIS 0
#endif

#if defined(__unix__) || defined(__unix)
  #define VITTE_BENCH_OS_UNIX 1
#else
  #define VITTE_BENCH_OS_UNIX 0
#endif

/* -------------------------------------------------------------------------- */
/* Data model                                                                  */
/* -------------------------------------------------------------------------- */

typedef struct vitte_cpu_features {
    /* ISA */
    bool sse2;
    bool avx2;
    bool avx512f;
    bool neon;

    /* timing */
    bool invariant_tsc;
    bool nonstop_tsc;

    /* cache hints */
    uint32_t cache_line_bytes;

    /* misc */
    bool hyperthreading;
    bool turbo_boost; /* best effort */
} vitte_cpu_features;

typedef struct vitte_cpu_topology {
    uint32_t logical_cores;
    uint32_t physical_cores;
    uint32_t packages;

    /* best-effort NUMA nodes count */
    uint32_t numa_nodes;

    /* best-effort */
    uint32_t l1d_kb;
    uint32_t l2_kb;
    uint32_t l3_kb;
} vitte_cpu_topology;

typedef struct vitte_cpu_info {
    char vendor[32];
    char brand[96];

    vitte_cpu_features features;
    vitte_cpu_topology topo;

    /* nominal / max frequency (MHz) best-effort */
    uint32_t mhz_nominal;
    uint32_t mhz_max;

    /* OS and arch (stringy, stable) */
    char os[24];
    char arch[24];
} vitte_cpu_info;

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */

/*
  Fill `out` with best-effort CPU info.
  Returns true if at least vendor/arch/os were populated.
*/
bool vitte_cpu_query(vitte_cpu_info* out);

/*
  Best-effort pinning of current thread to a CPU index in [0, logical_cores).
  Returns true if the OS call succeeded.
*/
bool vitte_cpu_pin_thread(uint32_t cpu_index);

/* Unpin thread / reset affinity to default. Best-effort. */
void vitte_cpu_unpin_thread(void);

/*
  Best-effort priority adjustments.
  - level: 0=normal, 1=high, 2=realtime-ish (may require privileges)
*/
bool vitte_cpu_set_priority(int level);

/* -------------------------------------------------------------------------- */
/* Implementation (header-only)                                                */
/* -------------------------------------------------------------------------- */

/* Helpers: safe string copy */
static inline void vitte_cpu__strcpy(char* dst, size_t cap, const char* src)
{
    if (!dst || cap == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    size_t i = 0;
    for (; i + 1 < cap && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static inline void vitte_cpu__set_os_arch(vitte_cpu_info* out)
{
    /* os */
#if VITTE_BENCH_OS_WINDOWS
    vitte_cpu__strcpy(out->os, sizeof(out->os), "windows");
#elif VITTE_BENCH_OS_DARWIN
    vitte_cpu__strcpy(out->os, sizeof(out->os), "darwin");
#elif VITTE_BENCH_OS_LINUX
    vitte_cpu__strcpy(out->os, sizeof(out->os), "linux");
#elif VITTE_BENCH_OS_FREEBSD
    vitte_cpu__strcpy(out->os, sizeof(out->os), "freebsd");
#elif VITTE_BENCH_OS_OPENBSD
    vitte_cpu__strcpy(out->os, sizeof(out->os), "openbsd");
#elif VITTE_BENCH_OS_NETBSD
    vitte_cpu__strcpy(out->os, sizeof(out->os), "netbsd");
#elif VITTE_BENCH_OS_SOLARIS
    vitte_cpu__strcpy(out->os, sizeof(out->os), "solaris");
#else
    vitte_cpu__strcpy(out->os, sizeof(out->os), "unknown");
#endif

    /* arch */
#if VITTE_BENCH_ARCH_X64
    vitte_cpu__strcpy(out->arch, sizeof(out->arch), "x86_64");
#elif VITTE_BENCH_ARCH_X86
    vitte_cpu__strcpy(out->arch, sizeof(out->arch), "x86");
#elif VITTE_BENCH_ARCH_ARM64
    vitte_cpu__strcpy(out->arch, sizeof(out->arch), "arm64");
#elif VITTE_BENCH_ARCH_ARM
    vitte_cpu__strcpy(out->arch, sizeof(out->arch), "arm");
#else
    vitte_cpu__strcpy(out->arch, sizeof(out->arch), "unknown");
#endif
}

/* ------------------------------ CPUID (x86) -------------------------------- */

#if (VITTE_BENCH_ARCH_X86 || VITTE_BENCH_ARCH_X64)

static inline void vitte_cpu__cpuid(uint32_t leaf, uint32_t subleaf, uint32_t out_abcd[4])
{
#if VITTE_BENCH_CC_MSVC
    #include <intrin.h>
    int regs[4];
    __cpuidex(regs, (int)leaf, (int)subleaf);
    out_abcd[0] = (uint32_t)regs[0];
    out_abcd[1] = (uint32_t)regs[1];
    out_abcd[2] = (uint32_t)regs[2];
    out_abcd[3] = (uint32_t)regs[3];
#elif (VITTE_BENCH_CC_CLANG || VITTE_BENCH_CC_GCC)
    uint32_t a, b, c, d;
    __asm__ __volatile__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(leaf), "c"(subleaf));
    out_abcd[0] = a;
    out_abcd[1] = b;
    out_abcd[2] = c;
    out_abcd[3] = d;
#else
    out_abcd[0] = out_abcd[1] = out_abcd[2] = out_abcd[3] = 0;
    (void)leaf; (void)subleaf;
#endif
}

static inline void vitte_cpu__x86_vendor(char out_vendor[32])
{
    uint32_t r[4];
    vitte_cpu__cpuid(0u, 0u, r);
    /* vendor: EBX EDX ECX */
    char v[13];
    ((uint32_t*)v)[0] = r[1];
    ((uint32_t*)v)[1] = r[3];
    ((uint32_t*)v)[2] = r[2];
    v[12] = '\0';

    /* copy */
    vitte_cpu__strcpy(out_vendor, 32, v);
}

static inline void vitte_cpu__x86_brand(char out_brand[96])
{
    uint32_t r[4];
    uint32_t max_ext;

    vitte_cpu__cpuid(0x80000000u, 0u, r);
    max_ext = r[0];

    if (max_ext < 0x80000004u) {
        vitte_cpu__strcpy(out_brand, 96, "");
        return;
    }

    char b[49];
    uint32_t* w = (uint32_t*)b;
    vitte_cpu__cpuid(0x80000002u, 0u, r); w[0]=r[0]; w[1]=r[1]; w[2]=r[2]; w[3]=r[3];
    vitte_cpu__cpuid(0x80000003u, 0u, r); w[4]=r[0]; w[5]=r[1]; w[6]=r[2]; w[7]=r[3];
    vitte_cpu__cpuid(0x80000004u, 0u, r); w[8]=r[0]; w[9]=r[1]; w[10]=r[2]; w[11]=r[3];
    b[48] = '\0';

    /* trim leading spaces */
    const char* s = b;
    while (*s == ' ') s++;
    vitte_cpu__strcpy(out_brand, 96, s);
}

static inline void vitte_cpu__x86_features(vitte_cpu_features* f)
{
    uint32_t r1[4];
    uint32_t r7[4];
    uint32_t re[4];

    vitte_cpu__cpuid(1u, 0u, r1);
    vitte_cpu__cpuid(7u, 0u, r7);
    vitte_cpu__cpuid(0x80000007u, 0u, re);

    /* leaf 1 */
    const uint32_t edx = r1[3];
    const uint32_t ecx = r1[2];

    f->sse2 = ((edx >> 26u) & 1u) != 0u;
    (void)ecx;

    /* leaf 7 */
    const uint32_t ebx = r7[1];
    f->avx2 = ((ebx >> 5u) & 1u) != 0u;
    f->avx512f = ((ebx >> 16u) & 1u) != 0u;

    /* extended leaf 0x80000007: EDX[8] invariant TSC */
    const uint32_t edxe = re[3];
    f->invariant_tsc = ((edxe >> 8u) & 1u) != 0u;
    f->nonstop_tsc = f->invariant_tsc;

    /* cache line size: CPUID(1).EBX[15:8] * 8 */
    {
        uint32_t ebx1 = r1[1];
        uint32_t cl = (ebx1 >> 8u) & 0xffu;
        if (cl) {
            f->cache_line_bytes = cl * 8u;
        }
    }

    /* hyperthreading: CPUID(1).EDX[28] */
    f->hyperthreading = ((edx >> 28u) & 1u) != 0u;

    /* turbo_boost best effort: unknown */
    f->turbo_boost = false;
}

#endif /* x86 */

/* ------------------------------ sysconf (POSIX) ---------------------------- */

static inline uint32_t vitte_cpu__logical_cores_best_effort(void)
{
#if VITTE_BENCH_OS_LINUX || VITTE_BENCH_OS_DARWIN || VITTE_BENCH_OS_FREEBSD || VITTE_BENCH_OS_OPENBSD || VITTE_BENCH_OS_NETBSD || VITTE_BENCH_OS_SOLARIS
    #include <unistd.h>
    long n = -1;
    #if defined(_SC_NPROCESSORS_ONLN)
      n = sysconf(_SC_NPROCESSORS_ONLN);
    #endif
    if (n > 0) {
        return (uint32_t)n;
    }
#endif
#if VITTE_BENCH_OS_WINDOWS
    #include <windows.h>
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    if (si.dwNumberOfProcessors > 0) {
        return (uint32_t)si.dwNumberOfProcessors;
    }
#endif
    return 1u;
}

/* ------------------------------ sysctl (BSD/Darwin) ------------------------ */

static inline bool vitte_cpu__sysctl_u32(const char* name, uint32_t* out)
{
#if VITTE_BENCH_OS_DARWIN || VITTE_BENCH_OS_FREEBSD || VITTE_BENCH_OS_OPENBSD || VITTE_BENCH_OS_NETBSD
    #include <sys/types.h>
    #include <sys/sysctl.h>
    size_t len = sizeof(uint32_t);
    if (sysctlbyname(name, out, &len, NULL, 0) == 0 && len == sizeof(uint32_t)) {
        return true;
    }
    return false;
#else
    (void)name; (void)out;
    return false;
#endif
}

static inline bool vitte_cpu__sysctl_str(const char* name, char* out, size_t cap)
{
#if VITTE_BENCH_OS_DARWIN || VITTE_BENCH_OS_FREEBSD || VITTE_BENCH_OS_OPENBSD || VITTE_BENCH_OS_NETBSD
    #include <sys/types.h>
    #include <sys/sysctl.h>
    size_t len = cap;
    if (sysctlbyname(name, out, &len, NULL, 0) == 0) {
        if (len >= cap) {
            out[cap - 1] = '\0';
        } else {
            out[len] = '\0';
        }
        return true;
    }
    return false;
#else
    (void)name;
    vitte_cpu__strcpy(out, cap, "");
    return false;
#endif
}

/* ------------------------------ Linux /proc best effort -------------------- */

static inline void vitte_cpu__linux_brand_best_effort(char* out, size_t cap)
{
#if VITTE_BENCH_OS_LINUX
    #include <stdio.h>
    FILE* f = fopen("/proc/cpuinfo", "rb");
    if (!f) {
        vitte_cpu__strcpy(out, cap, "");
        return;
    }

    char line[512];
    while (fgets(line, (int)sizeof(line), f)) {
        /* model name\t: ... */
        const char* k = "model name";
        const char* p = line;
        while (*k && *p && *k == *p) { k++; p++; }
        if (*k == '\0') {
            const char* c = line;
            while (*c && *c != ':') c++;
            if (*c == ':') c++;
            while (*c == ' ' || *c == '\t') c++;
            /* strip trailing newline */
            char tmp[512];
            size_t n = 0;
            while (c[n] && c[n] != '\n' && c[n] != '\r' && n + 1 < sizeof(tmp)) {
                tmp[n] = c[n];
                n++;
            }
            tmp[n] = '\0';
            vitte_cpu__strcpy(out, cap, tmp);
            fclose(f);
            return;
        }
    }

    fclose(f);
    vitte_cpu__strcpy(out, cap, "");
#else
    vitte_cpu__strcpy(out, cap, "");
#endif
}

/* ------------------------------ Public implementations --------------------- */

bool vitte_cpu_query(vitte_cpu_info* out)
{
    if (!out) {
        return false;
    }

    /* zero */
    {
        vitte_cpu_info z;
        /* manual zero to avoid memset dependency */
        uint8_t* p = (uint8_t*)&z;
        for (size_t i = 0; i < sizeof(z); i++) p[i] = 0;
        *out = z;
    }

    vitte_cpu__set_os_arch(out);

    /* features defaults from compile-time macros */
    out->features.sse2 = (bool)VITTE_BENCH_HAS_SSE2;
    out->features.avx2 = (bool)VITTE_BENCH_HAS_AVX2;
    out->features.avx512f = (bool)VITTE_BENCH_HAS_AVX512F;
    out->features.neon = (bool)VITTE_BENCH_HAS_NEON;
    out->features.cache_line_bytes = 64u;

    /* topology best effort */
    out->topo.logical_cores = vitte_cpu__logical_cores_best_effort();
    out->topo.physical_cores = 0;
    out->topo.packages = 0;
    out->topo.numa_nodes = 0;

#if VITTE_BENCH_OS_DARWIN
    (void)vitte_cpu__sysctl_u32("hw.physicalcpu", &out->topo.physical_cores);
    (void)vitte_cpu__sysctl_u32("hw.packages", &out->topo.packages);
    (void)vitte_cpu__sysctl_u32("hw.l1dcachesize", &out->topo.l1d_kb);
    (void)vitte_cpu__sysctl_u32("hw.l2cachesize", &out->topo.l2_kb);
    (void)vitte_cpu__sysctl_u32("hw.l3cachesize", &out->topo.l3_kb);

    /* sizes are bytes; convert to KB where available */
    out->topo.l1d_kb /= 1024u;
    out->topo.l2_kb /= 1024u;
    out->topo.l3_kb /= 1024u;

    (void)vitte_cpu__sysctl_str("machdep.cpu.vendor", out->vendor, sizeof(out->vendor));
    (void)vitte_cpu__sysctl_str("machdep.cpu.brand_string", out->brand, sizeof(out->brand));

    (void)vitte_cpu__sysctl_u32("hw.cpufrequency", &out->mhz_nominal);
    (void)vitte_cpu__sysctl_u32("hw.cpufrequency_max", &out->mhz_max);
    if (out->mhz_nominal) out->mhz_nominal = out->mhz_nominal / 1000000u;
    if (out->mhz_max) out->mhz_max = out->mhz_max / 1000000u;
#endif

#if VITTE_BENCH_OS_FREEBSD || VITTE_BENCH_OS_OPENBSD || VITTE_BENCH_OS_NETBSD
    (void)vitte_cpu__sysctl_str("hw.model", out->brand, sizeof(out->brand));
    (void)vitte_cpu__sysctl_u32("hw.ncpu", &out->topo.logical_cores);
#endif

#if VITTE_BENCH_OS_LINUX
    vitte_cpu__linux_brand_best_effort(out->brand, sizeof(out->brand));
#endif

#if (VITTE_BENCH_ARCH_X86 || VITTE_BENCH_ARCH_X64)
    vitte_cpu__x86_vendor(out->vendor);
    vitte_cpu__x86_brand(out->brand);
    vitte_cpu__x86_features(&out->features);
#endif

#if (VITTE_BENCH_ARCH_ARM || VITTE_BENCH_ARCH_ARM64)
    /* vendor/brand are platform-specific; best-effort */
    if (out->brand[0] == '\0') {
        vitte_cpu__strcpy(out->brand, sizeof(out->brand), "arm");
    }
    if (out->vendor[0] == '\0') {
        vitte_cpu__strcpy(out->vendor, sizeof(out->vendor), "");
    }
    out->features.invariant_tsc = false;
    out->features.nonstop_tsc = false;
#endif

    /* normalize unknowns */
    if (out->topo.physical_cores == 0) {
        out->topo.physical_cores = out->topo.logical_cores;
    }
    if (out->topo.packages == 0) {
        out->topo.packages = 1u;
    }

    return out->os[0] != '\0' && out->arch[0] != '\0';
}

bool vitte_cpu_pin_thread(uint32_t cpu_index)
{
#if VITTE_BENCH_OS_WINDOWS
    #include <windows.h>
    DWORD_PTR mask = 0;
    if (cpu_index >= (uint32_t)(8u * sizeof(DWORD_PTR))) {
        return false;
    }
    mask = ((DWORD_PTR)1u) << cpu_index;
    return SetThreadAffinityMask(GetCurrentThread(), mask) != 0;
#elif VITTE_BENCH_OS_LINUX
    #include <sched.h>
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET((int)cpu_index, &set);
    return sched_setaffinity(0, sizeof(set), &set) == 0;
#elif VITTE_BENCH_OS_DARWIN
    /* macOS does not support strict CPU affinity for threads in a public API.
       We return false to signal that pinning is unavailable.
    */
    (void)cpu_index;
    return false;
#elif VITTE_BENCH_OS_FREEBSD
    #include <sys/param.h>
    #include <sys/cpuset.h>
    cpuset_t set;
    CPU_ZERO(&set);
    CPU_SET((int)cpu_index, &set);
    return cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_TID, -1, sizeof(set), &set) == 0;
#elif VITTE_BENCH_OS_NETBSD
    /* NetBSD affinity API varies; best-effort not provided here. */
    (void)cpu_index;
    return false;
#elif VITTE_BENCH_OS_OPENBSD
    (void)cpu_index;
    return false;
#elif VITTE_BENCH_OS_SOLARIS
    #include <sys/types.h>
    #include <sys/processor.h>
    #include <sys/procset.h>
    return processor_bind(P_LWPID, P_MYID, (processorid_t)cpu_index, NULL) == 0;
#else
    (void)cpu_index;
    return false;
#endif
}

void vitte_cpu_unpin_thread(void)
{
#if VITTE_BENCH_OS_WINDOWS
    #include <windows.h>
    /* reset to all CPUs */
    (void)SetThreadAffinityMask(GetCurrentThread(), (DWORD_PTR)(~(DWORD_PTR)0));
#elif VITTE_BENCH_OS_LINUX
    #include <sched.h>
    cpu_set_t set;
    CPU_ZERO(&set);
    /* best effort: enable all online CPUs */
    uint32_t n = vitte_cpu__logical_cores_best_effort();
    for (uint32_t i = 0; i < n && i < (uint32_t)CPU_SETSIZE; i++) {
        CPU_SET((int)i, &set);
    }
    (void)sched_setaffinity(0, sizeof(set), &set);
#elif VITTE_BENCH_OS_SOLARIS
    #include <sys/types.h>
    #include <sys/processor.h>
    #include <sys/procset.h>
    (void)processor_bind(P_LWPID, P_MYID, PBIND_NONE, NULL);
#else
    /* no-op */
#endif
}

bool vitte_cpu_set_priority(int level)
{
    /* 0=normal, 1=high, 2=realtime-ish */
#if VITTE_BENCH_OS_WINDOWS
    #include <windows.h>
    HANDLE th = GetCurrentThread();
    int ok = 0;
    if (level <= 0) {
        ok = SetThreadPriority(th, THREAD_PRIORITY_NORMAL);
    } else if (level == 1) {
        ok = SetThreadPriority(th, THREAD_PRIORITY_HIGHEST);
    } else {
        ok = SetThreadPriority(th, THREAD_PRIORITY_TIME_CRITICAL);
    }
    return ok != 0;
#elif VITTE_BENCH_OS_LINUX || VITTE_BENCH_OS_FREEBSD || VITTE_BENCH_OS_OPENBSD || VITTE_BENCH_OS_NETBSD || VITTE_BENCH_OS_SOLARIS
    #include <errno.h>
    #include <pthread.h>
    #include <sched.h>

    pthread_t th = pthread_self();

    if (level <= 0) {
        /* reset to SCHED_OTHER */
        struct sched_param sp;
        sp.sched_priority = 0;
        return pthread_setschedparam(th, SCHED_OTHER, &sp) == 0;
    }

    int policy = (level == 1) ? SCHED_RR : SCHED_FIFO;
    int maxp = sched_get_priority_max(policy);
    if (maxp <= 0) {
        return false;
    }

    struct sched_param sp;
    sp.sched_priority = (level == 1) ? (maxp / 2) : (maxp - 1);
    if (pthread_setschedparam(th, policy, &sp) == 0) {
        return true;
    }

    /* fallback: try nice */
    (void)errno;
    return false;
#elif VITTE_BENCH_OS_DARWIN
    /* Thread priority changes are constrained; best-effort not exposed here. */
    (void)level;
    return false;
#else
    (void)level;
    return false;
#endif
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* VITTE_BENCH_CPU_H */
