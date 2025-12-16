#include "bench/log.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

/*
  log.c

  Logging utilities implementation.
*/

static bench_log_level_t g_log_level = BENCH_LOG_INFO;

void bench_log_set_level(bench_log_level_t level) {
  g_log_level = level;
}

bench_log_level_t bench_log_get_level(void) {
  return g_log_level;
}

static const char* level_name(bench_log_level_t level) {
  switch (level) {
    case BENCH_LOG_DEBUG:
      return "DEBUG";
    case BENCH_LOG_INFO:
      return "INFO";
    case BENCH_LOG_WARN:
      return "WARN";
    case BENCH_LOG_ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

void bench_log(bench_log_level_t level, const char* fmt, ...) {
  if (level < g_log_level) {
    return;
  }

  if (!fmt) {
    return;
  }

  /* Get current time */
  time_t now = time(NULL);
  struct tm* tm_info = localtime(&now);
  char time_buf[32];
  strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

  /* Print with timestamp */
  fprintf(stderr, "[%s] [%s] ", time_buf, level_name(level));

  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  fprintf(stderr, "\n");
}
