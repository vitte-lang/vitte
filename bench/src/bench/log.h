#pragma once

/*
  log.h

  Logging utilities for benchmarking.
*/

#ifndef VITTE_BENCH_LOG_H
#define VITTE_BENCH_LOG_H

#include <stdio.h>

typedef enum {
  BENCH_LOG_DEBUG = 0,
  BENCH_LOG_INFO = 1,
  BENCH_LOG_WARN = 2,
  BENCH_LOG_ERROR = 3,
} bench_log_level_t;

/* Set log level */
void bench_log_set_level(bench_log_level_t level);

/* Get current log level */
bench_log_level_t bench_log_get_level(void);

/* Log message */
void bench_log(bench_log_level_t level, const char* fmt, ...);

/* Debug log */
#define bench_debug(...) bench_log(BENCH_LOG_DEBUG, __VA_ARGS__)

/* Info log */
#define bench_info(...) bench_log(BENCH_LOG_INFO, __VA_ARGS__)

/* Warning log */
#define bench_warn(...) bench_log(BENCH_LOG_WARN, __VA_ARGS__)

/* Error log */
#define bench_error(...) bench_log(BENCH_LOG_ERROR, __VA_ARGS__)

#endif /* VITTE_BENCH_LOG_H */
