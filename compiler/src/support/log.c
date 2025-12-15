#include "vittec/support/log.h"
#include <stdio.h>
#include <stdarg.h>

static vittec_log_level_t g_level = VITTEC_LOG_INFO;

void vittec_log_set_level(vittec_log_level_t lvl) { g_level = lvl; }

void vittec_log(vittec_log_level_t lvl, const char* fmt, ...) {
  if (lvl > g_level) return;

  const char* p = "INFO";
  if (lvl == VITTEC_LOG_ERROR) p = "ERROR";
  else if (lvl == VITTEC_LOG_WARN) p = "WARN";
  else if (lvl == VITTEC_LOG_DEBUG) p = "DEBUG";

  fprintf(stderr, "[%s] ", p);
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
}
