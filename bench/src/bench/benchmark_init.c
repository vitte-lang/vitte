// benchmark_init.c — optional benchmark runtime initialization (C17, max)
//
// Purpose:
//   Central place to perform one-time initialization for the benchmark runtime,
//   without forcing every suite to duplicate the logic.
//
// Provides:
//   - benchmark_runtime_init(): idempotent init (safe to call multiple times)
//   - benchmark_runtime_shutdown(): optional cleanup
//   - benchmark_pin_to_single_cpu(): best-effort process/thread affinity
//   - benchmark_set_high_priority(): best-effort priority bump
//   - benchmark_disable_turbo_best_effort(): placeholder hook (no-op by default)
//
// Notes:
//   - Turbo/boost disable is NOT portable; keep it as a hook only.
//   - CPU affinity differs between OSes; we implement best-effort on Windows and Linux.
//   - macOS thread affinity APIs are limited; we keep it as a no-op there.

#include <stdint.h>
#include <stddef.h>

#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#elif defined(__linux__)
  #include <sched.h>
  #include <unistd.h>
  #include <errno.h>
#elif defined(__APPLE__)
  #include <pthread.h>
  #include <unistd.h>
#else
  #include <unistd.h>
#endif

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

void benchmark_runtime_init(void);
void benchmark_runtime_shutdown(void);

int benchmark_pin_to_single_cpu(int cpu_index);
int benchmark_set_high_priority(void);
int benchmark_disable_turbo_best_effort(void);

// -----------------------------------------------------------------------------
// State
// -----------------------------------------------------------------------------

static int g_bench_runtime_inited = 0;

// -----------------------------------------------------------------------------
// Implementations
// -----------------------------------------------------------------------------

void benchmark_runtime_init(void) {
  if (g_bench_runtime_inited) return;
  g_bench_runtime_inited = 1;

  // Placeholders for future:
  //   - pre-fault allocator arenas
  //   - warm up timers
  //   - init registry
  //
  // Intentionally empty by default.
}

void benchmark_runtime_shutdown(void) {
  // Optional cleanup hook.
  (void)0;
}

int benchmark_set_high_priority(void) {
#if defined(_WIN32)
  // Process priority + current thread priority.
  HANDLE hproc = GetCurrentProcess();
  HANDLE hthr  = GetCurrentThread();
  int ok = 1;

  if (!SetPriorityClass(hproc, HIGH_PRIORITY_CLASS)) ok = 0;
  if (!SetThreadPriority(hthr, THREAD_PRIORITY_HIGHEST)) ok = 0;

  return ok ? 1 : 0;
#elif defined(__linux__)
  // On Linux, real-time priorities require CAP_SYS_NICE.
  // We keep this as a best-effort no-op (return 0 if cannot).
  return 0;
#else
  return 0;
#endif
}

int benchmark_pin_to_single_cpu(int cpu_index) {
  if (cpu_index < 0) cpu_index = 0;

#if defined(_WIN32)
  // Pin current thread to a single logical processor.
  DWORD_PTR mask = ((DWORD_PTR)1) << (DWORD_PTR)cpu_index;
  HANDLE hthr = GetCurrentThread();
  DWORD_PTR prev = SetThreadAffinityMask(hthr, mask);
  return (prev != 0) ? 1 : 0;

#elif defined(__linux__)
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(cpu_index, &set);
  const int rc = sched_setaffinity(0, sizeof(set), &set);
  return (rc == 0) ? 1 : 0;

#elif defined(__APPLE__)
  // macOS does not provide stable public APIs for strict CPU pinning.
  // Some projects use thread affinity tags which are only hints.
  (void)cpu_index;
  return 0;

#else
  (void)cpu_index;
  return 0;
#endif
}

int benchmark_disable_turbo_best_effort(void) {
  // Not portable: leave as hook.
  // Implementations could:
  //   - on Linux: write to sysfs (needs root)
  //   - on Windows: power profile changes
  //   - on macOS: not generally possible
  return 0;
}
