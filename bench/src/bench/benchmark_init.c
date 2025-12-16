/*
  benchmark_init.c

  Centralizes registration of all micro and macro benchmarks.
  
  Micro benchmarks (15):
  - Arithmetic: add, bitops, division
  - Memory: array_access, cache_line_effects, memcpy
  - Branches: branch_prediction, conditional_move
  - Floating-point: float, data_dependency
  - Function calls: calls, recursion
  - Loop optimization: loop_unroll
  - String: string_search
  - Hashing: hash

  Macro benchmarks (8):
  - JSON parsing
  - Regex/pattern matching
  - String operations
  - Sorting algorithms
  - Math computations
  - Compression
  - File I/O simulation
  - Cache effects (large working sets)
*/

#include "bench/bench.h"
#include "bench/registry.h"

/* ----------------------------------------
 * Micro benchmark registration functions
 * ---------------------------------------- */

extern void bench_register_micro_add(void);
extern void bench_register_micro_array(void);
extern void bench_register_micro_bitops(void);
extern void bench_register_micro_branch(void);
extern void bench_register_micro_cache_line(void);
extern void bench_register_micro_calls(void);
extern void bench_register_micro_cmov(void);
extern void bench_register_micro_dependency(void);
extern void bench_register_micro_division(void);
extern void bench_register_micro_float(void);
extern void bench_register_micro_hash(void);
extern void bench_register_micro_loop(void);
extern void bench_register_micro_memcpy(void);
extern void bench_register_micro_recursion(void);
extern void bench_register_micro_string_search(void);

/* ----------------------------------------
 * Macro benchmark registration functions
 * ---------------------------------------- */

extern void bench_register_macro_cache(void);
extern void bench_register_macro_compression(void);
extern void bench_register_macro_file_io(void);
extern void bench_register_macro_json(void);
extern void bench_register_macro_math(void);
extern void bench_register_macro_regex(void);
extern void bench_register_macro_sorts(void);
extern void bench_register_macro_strings(void);

/* ----------------------------------------
 * Initialization function
 * ---------------------------------------- */

void bench_init_all_benchmarks(void) {
  /* Register micro benchmarks */
  bench_register_micro_add();
  bench_register_micro_array();
  bench_register_micro_bitops();
  bench_register_micro_branch();
  bench_register_micro_cache_line();
  bench_register_micro_calls();
  bench_register_micro_cmov();
  bench_register_micro_dependency();
  bench_register_micro_division();
  bench_register_micro_float();
  bench_register_micro_hash();
  bench_register_micro_loop();
  bench_register_micro_memcpy();
  bench_register_micro_recursion();
  bench_register_micro_string_search();

  /* Register macro benchmarks */
  bench_register_macro_cache();
  bench_register_macro_compression();
  bench_register_macro_file_io();
  bench_register_macro_json();
  bench_register_macro_math();
  bench_register_macro_regex();
  bench_register_macro_sorts();
  bench_register_macro_strings();
}
