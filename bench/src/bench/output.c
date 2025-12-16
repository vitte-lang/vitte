#include "bench/output.h"
#include "bench/types.h"
#include <stdio.h>
#include <math.h>

/*
  output.c

  Benchmark result formatting and output.
*/

void bench_format_time(char* buf, size_t bufsz, double ns) {
  const char* unit = "ns";
  double value = ns;

  if (ns >= 1e9) {
    value = ns / 1e9;
    unit = "s";
  } else if (ns >= 1e6) {
    value = ns / 1e6;
    unit = "ms";
  } else if (ns >= 1e3) {
    value = ns / 1e3;
    unit = "us";
  }

  snprintf(buf, bufsz, "%.2f %s", value, unit);
}

void bench_format_throughput(char* buf, size_t bufsz, double ops_per_sec) {
  if (ops_per_sec >= 1e9) {
    snprintf(buf, bufsz, "%.2f G ops/s", ops_per_sec / 1e9);
  } else if (ops_per_sec >= 1e6) {
    snprintf(buf, bufsz, "%.2f M ops/s", ops_per_sec / 1e6);
  } else if (ops_per_sec >= 1e3) {
    snprintf(buf, bufsz, "%.2f K ops/s", ops_per_sec / 1e3);
  } else {
    snprintf(buf, bufsz, "%.2f ops/s", ops_per_sec);
  }
}

void bench_print_header(void) {
  printf("%-20s %-12s %-12s %-12s %-12s %-12s\n",
         "Benchmark", "Mean", "Median", "p95", "Min", "Max");
  printf("%-20s %-12s %-12s %-12s %-12s %-12s\n",
         "---", "---", "---", "---", "---", "---");
}

void bench_print_footer(void) {
  printf("\n");
}

void bench_print_result(const bench_result_t* result) {
  char mean_buf[32], median_buf[32], p95_buf[32];
  char min_buf[32], max_buf[32];

  bench_format_time(mean_buf, sizeof(mean_buf), result->stats.mean);
  bench_format_time(median_buf, sizeof(median_buf), result->stats.median);
  bench_format_time(p95_buf, sizeof(p95_buf), result->stats.p95);
  bench_format_time(min_buf, sizeof(min_buf), result->stats.min);
  bench_format_time(max_buf, sizeof(max_buf), result->stats.max);

  printf("%-20s %-12s %-12s %-12s %-12s %-12s\n",
         result->case_info->id, mean_buf, median_buf, p95_buf, min_buf, max_buf);
}

void bench_csv_header(FILE* f) {
  fprintf(f, "benchmark_id,kind,mean_ns,median_ns,p95_ns,p99_ns,min_ns,max_ns,stddev_ns,samples,duration_s\n");
}

void bench_csv_row(FILE* f, const bench_result_t* result) {
  fprintf(f, "%s,%s,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%lu,%.4f\n",
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
}
