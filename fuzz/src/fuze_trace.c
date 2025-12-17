// fuz_trace.c
// Implementation TU for fuzz tracing global state (C17).
//
// Use this if you want fuzz_trace_global() to be a real symbol (non-static inline),
// ensuring a single shared trace state across the binary.
//
// Include order expectation:
//   #include "fuzz/fuzz_trace.h"
//
// Build:
//   add this file once to your fuzz runtime library / target link.

#include "fuzz/fuzz_trace.h"

#if FUZZ_TRACE_ENABLED

static fuzz_trace_state g_fuzz_trace_state;
static int g_fuzz_trace_inited = 0;

fuzz_trace_state*
fuzz_trace_global(void) {
  if (!g_fuzz_trace_inited) {
    fuzz_trace_init(&g_fuzz_trace_state);
    g_fuzz_trace_inited = 1;
  }
  return &g_fuzz_trace_state;
}

#endif
