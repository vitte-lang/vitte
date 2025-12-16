#pragma once

/*
  output.h

  Benchmark result formatting and output.
*/

#ifndef VITTE_BENCH_OUTPUT_H
#define VITTE_BENCH_OUTPUT_H

#include "types.h"
#include <stdio.h>

/* Output format */
typedef enum {
  BENCH_OUT_TEXT,
  BENCH_OUT_CSV,
  BENCH_OUT_JSON,
  BENCH_OUT_HISTOGRAM,
} bench_output_format_t;

/* Print benchmark result to stdout */
void bench_print_result(const bench_result_t* result);

/* Print header for text output */
void bench_print_header(void);

/* Print footer for text output */
void bench_print_footer(void);

/* Write CSV header */
void bench_csv_header(FILE* f);

/* Write CSV row */
void bench_csv_row(FILE* f, const bench_result_t* result);

/* Format time with appropriate unit (ns/us/ms/s) */
void bench_format_time(char* buf, size_t bufsz, double ns);

/* Format throughput (ops/sec, MB/s, etc.) */
void bench_format_throughput(char* buf, size_t bufsz, double ops_per_sec);

#endif /* VITTE_BENCH_OUTPUT_H */
