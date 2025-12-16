#include "bench/csv.h"
#include "bench/alloc.h"
#include "bench/strutil.h"
#include <string.h>

/*
  csv.c

  CSV output generation utilities.
*/

bench_csv_writer_t* bench_csv_writer_new(const char* filename) {
  if (!filename) return NULL;

  FILE* f = fopen(filename, "w");
  if (!f) return NULL;

  bench_csv_writer_t* writer = (bench_csv_writer_t*)bench_malloc(sizeof(*writer));
  if (!writer) {
    fclose(f);
    return NULL;
  }

  writer->f = f;
  writer->header_written = 0;

  return writer;
}

void bench_csv_writer_free(bench_csv_writer_t* writer) {
  if (writer) {
    if (writer->f) {
      fclose(writer->f);
    }
    bench_free(writer);
  }
}

int bench_csv_write_header(bench_csv_writer_t* writer) {
  if (!writer || !writer->f) return 0;

  fprintf(writer->f,
          "benchmark_id,kind,mean_ns,median_ns,p95_ns,p99_ns,"
          "min_ns,max_ns,stddev_ns,samples,duration_s\n");

  writer->header_written = 1;
  return 1;
}

int bench_csv_write_result(bench_csv_writer_t* writer, const bench_result_t* result) {
  if (!writer || !writer->f || !result) return 0;

  if (!writer->header_written) {
    bench_csv_write_header(writer);
  }

  fprintf(writer->f, "%s,%s,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%lu,%.4f\n",
          result->case_info->id,
          result->case_info->kind == BENCH_MICRO ? "micro" : "macro",
          result->stats.mean,
          result->stats.median,
          result->stats.p95,
          result->stats.p99,
          result->stats.min,
          result->stats.max,
          result->stats.stddev,
          (unsigned long)result->samples,
          result->duration_ns / 1e9);

  return 1;
}

char* bench_csv_escape(const char* value) {
  if (!value) return NULL;

  int needs_quote = 0;
  const char* p = value;
  while (*p) {
    if (*p == '"' || *p == ',' || *p == '\n') {
      needs_quote = 1;
      break;
    }
    p++;
  }

  if (!needs_quote) {
    return bench_strdup(value);
  }

  /* Build escaped string */
  size_t len = strlen(value);
  char* escaped = (char*)bench_malloc(len * 2 + 3);
  if (!escaped) return NULL;

  char* dst = escaped;
  *dst++ = '"';

  p = value;
  while (*p) {
    if (*p == '"') {
      *dst++ = '"';
      *dst++ = '"';
    } else {
      *dst++ = *p;
    }
    p++;
  }

  *dst++ = '"';
  *dst = '\0';

  return escaped;
}
