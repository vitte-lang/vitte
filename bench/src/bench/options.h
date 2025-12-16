#pragma once

/*
  options.h

  Command-line options parsing.
*/

#ifndef VITTE_BENCH_OPTIONS_H
#define VITTE_BENCH_OPTIONS_H

#include "runner.h"
#include <stdint.h>

/* Options from command line */
typedef struct {
  int show_help;
  int list_benchmarks;
  int list_full;
  int run_all;
  
  const char** case_names;
  int case_count;
  
  bench_runner_config_t runner_config;
  
  const char* csv_file;
  const char* json_file;
} bench_options_t;

/* Parse command-line arguments */
int bench_parse_options(int argc, char** argv, bench_options_t* opts);

/* Print usage information */
void bench_print_usage(const char* program);

#endif /* VITTE_BENCH_OPTIONS_H */
