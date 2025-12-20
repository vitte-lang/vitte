#include <stdbool.h>

// Micro benchmark registrars
void bench_register_micro_array(void);
void bench_register_micro_bitops(void);
void bench_register_micro_branch(void);
void bench_register_micro_cache_line(void);
void bench_register_micro_cmov(void);
void bench_register_micro_dependency(void);
void bench_register_micro_division(void);
void bench_register_micro_float(void);
void bench_register_micro_calls(void);
void bench_register_micro_loop(void);
void bench_register_micro_memcpy(void);
void bench_register_micro_recursion(void);
void bench_register_micro_string_search(void);
void bench_register_micro_hash(void);
void bench_register_micro_fnv1a64(void);

// Macro benchmark registrars
void bench_register_macro_cache(void);
void bench_register_macro_compression(void);
void bench_register_macro_file_io(void);
void bench_register_macro_math(void);
void bench_register_macro_regex(void);
void bench_register_macro_sorts(void);
void bench_register_macro_strings(void);

// Optional modules may be absent; declare weakly when supported.
#if defined(__GNUC__) || defined(__clang__)
  #define BENCH_WEAK_SYMBOL __attribute__((weak))
#else
  #define BENCH_WEAK_SYMBOL
#endif

void bench_register_macro_json(void) BENCH_WEAK_SYMBOL;

void bench_register_builtin_suites(void) {
  static bool initialized = false;
  if (initialized) return;
  initialized = true;

  // Micro suites
  bench_register_micro_array();
  bench_register_micro_bitops();
  bench_register_micro_branch();
  bench_register_micro_cache_line();
  bench_register_micro_cmov();
  bench_register_micro_dependency();
  bench_register_micro_division();
  bench_register_micro_float();
  bench_register_micro_calls();
  bench_register_micro_loop();
  bench_register_micro_memcpy();
  bench_register_micro_recursion();
  bench_register_micro_string_search();
  bench_register_micro_hash();
  bench_register_micro_fnv1a64();

  // Macro suites
  bench_register_macro_cache();
  bench_register_macro_compression();
  bench_register_macro_file_io();
  bench_register_macro_math();
  bench_register_macro_regex();
  bench_register_macro_sorts();
  bench_register_macro_strings();
  if (bench_register_macro_json)
    bench_register_macro_json();
}
