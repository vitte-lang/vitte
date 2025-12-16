#pragma once

/*
  csv.h

  CSV output generation utilities.
*/

#ifndef VITTE_BENCH_CSV_H
#define VITTE_BENCH_CSV_H

#include <stdio.h>
#include "bench/types.h"

/* CSV writer state */
typedef struct {
  FILE* f;
  int header_written;
} bench_csv_writer_t;

/* Create CSV writer */
bench_csv_writer_t* bench_csv_writer_new(const char* filename);

/* Free CSV writer */
void bench_csv_writer_free(bench_csv_writer_t* writer);

/* Write CSV header */
int bench_csv_write_header(bench_csv_writer_t* writer);

/* Write benchmark result as CSV row */
int bench_csv_write_result(bench_csv_writer_t* writer, const bench_result_t* result);

/* Escape CSV field value */
char* bench_csv_escape(const char* value);

#endif /* VITTE_BENCH_CSV_H */
