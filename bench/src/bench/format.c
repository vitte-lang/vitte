#include "bench/format.h"
#include <stdarg.h>
#include <math.h>
#include <string.h>

/*
  format.c

  String formatting utilities implementation.
*/

int bench_format_ns(char* buf, size_t bufsz, double ns) {
  if (!buf || bufsz == 0) return 0;

  if (ns < 1000.0) {
    return snprintf(buf, bufsz, "%.2f ns", ns);
  } else if (ns < 1e6) {
    return snprintf(buf, bufsz, "%.2f us", ns / 1e3);
  } else if (ns < 1e9) {
    return snprintf(buf, bufsz, "%.2f ms", ns / 1e6);
  } else {
    return snprintf(buf, bufsz, "%.2f s", ns / 1e9);
  }
}

int bench_format_bytes(char* buf, size_t bufsz, size_t bytes) {
  if (!buf || bufsz == 0) return 0;

  if (bytes < 1024) {
    return snprintf(buf, bufsz, "%zu B", bytes);
  } else if (bytes < 1024 * 1024) {
    return snprintf(buf, bufsz, "%.2f KB", bytes / 1024.0);
  } else if (bytes < 1024 * 1024 * 1024) {
    return snprintf(buf, bufsz, "%.2f MB", bytes / (1024.0 * 1024.0));
  } else {
    return snprintf(buf, bufsz, "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
  }
}

int bench_format_percent(char* buf, size_t bufsz, double value) {
  if (!buf || bufsz == 0) return 0;
  if (value < 0.01) {
    return snprintf(buf, bufsz, "<0.01%%");
  } else if (value > 99.99) {
    return snprintf(buf, bufsz, ">99.99%%");
  } else {
    return snprintf(buf, bufsz, "%.2f%%", value);
  }
}

int bench_format_ops_per_sec(char* buf, size_t bufsz, double ops_per_sec) {
  if (!buf || bufsz == 0) return 0;

  if (ops_per_sec < 1e3) {
    return snprintf(buf, bufsz, "%.2f ops/s", ops_per_sec);
  } else if (ops_per_sec < 1e6) {
    return snprintf(buf, bufsz, "%.2f K ops/s", ops_per_sec / 1e3);
  } else if (ops_per_sec < 1e9) {
    return snprintf(buf, bufsz, "%.2f M ops/s", ops_per_sec / 1e6);
  } else {
    return snprintf(buf, bufsz, "%.2f G ops/s", ops_per_sec / 1e9);
  }
}

int bench_snprintf(char* buf, size_t bufsz, const char* fmt, ...) {
  if (!buf || bufsz == 0 || !fmt) return 0;

  va_list ap;
  va_start(ap, fmt);
  int result = vsnprintf(buf, bufsz, fmt, ap);
  va_end(ap);

  return result;
}
