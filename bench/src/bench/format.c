// format.c — tiny formatting helpers for benchmark output (C17, max)
//
// Goals:
//   - Provide consistent human-readable formatting for durations, sizes, rates.
//   - Avoid locale issues; use '.' decimal.
//   - Keep allocations out (caller provides buffers).
//   - Provide both fixed and adaptive units.
//
// This is deliberately not a full printf replacement.

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

// -----------------------------------------------------------------------------
// Public API (declare in format.h if needed)
// -----------------------------------------------------------------------------

// Format ns into an adaptive unit string: ns/us/ms/s.
// Example: 999ns, 1.234us, 12.345ms, 1.234s
char* bench_fmt_ns(char* buf, size_t cap, uint64_t ns);

// Format bytes into adaptive unit string: B/KiB/MiB/GiB/TiB.
char* bench_fmt_bytes(char* buf, size_t cap, uint64_t bytes);

// Format rate (ops/s) into adaptive unit string: ops/s, Kop/s, Mop/s, ...
char* bench_fmt_ops(char* buf, size_t cap, double ops_per_s);

// Format throughput in bytes/s into adaptive unit string: B/s, KiB/s, MiB/s, ...
char* bench_fmt_bps(char* buf, size_t cap, double bytes_per_s);

// Format a double with a maximum of `decimals` decimals, trimming trailing zeros.
char* bench_fmt_f64_trim(char* buf, size_t cap, double v, int decimals);

// Format a percentage (0..1) as xx.x%.
char* bench_fmt_pct(char* buf, size_t cap, double ratio);

// -----------------------------------------------------------------------------
// Internals
// -----------------------------------------------------------------------------

static char* bench_fmt_write_(char* buf, size_t cap, const char* s) {
  if (!buf || cap == 0) return buf;
  if (!s) s = "";
  (void)snprintf(buf, cap, "%s", s);
  return buf;
}

static char* bench_fmt_writef_(char* buf, size_t cap, const char* fmt, ...) {
  if (!buf || cap == 0) return buf;
  va_list ap;
  va_start(ap, fmt);
  (void)vsnprintf(buf, cap, fmt, ap);
  va_end(ap);
  return buf;
}

static void bench_trim_zeros_(char* s) {
  if (!s) return;
  char* dot = strchr(s, '.');
  if (!dot) return;

  char* end = s + strlen(s);
  while (end > dot + 1 && end[-1] == '0') {
    end[-1] = 0;
    --end;
  }
  if (end > dot && end[-1] == '.') {
    end[-1] = 0;
  }
}

char* bench_fmt_f64_trim(char* buf, size_t cap, double v, int decimals) {
  if (!buf || cap == 0) return buf;
  if (isnan(v)) return bench_fmt_write_(buf, cap, "nan");
  if (isinf(v)) return bench_fmt_write_(buf, cap, (v > 0.0) ? "inf" : "-inf");

  if (decimals < 0) decimals = 0;
  if (decimals > 12) decimals = 12;

  char tmp[128];
  (void)snprintf(tmp, sizeof(tmp), "%.*f", decimals, v);
  bench_trim_zeros_(tmp);
  return bench_fmt_write_(buf, cap, tmp);
}

char* bench_fmt_pct(char* buf, size_t cap, double ratio) {
  if (!buf || cap == 0) return buf;
  if (isnan(ratio)) return bench_fmt_write_(buf, cap, "nan%");
  const double pct = ratio * 100.0;
  char tmp[128];
  (void)snprintf(tmp, sizeof(tmp), "%.2f", pct);
  bench_trim_zeros_(tmp);
  char out[140];
  (void)snprintf(out, sizeof(out), "%s%%", tmp);
  return bench_fmt_write_(buf, cap, out);
}

// -----------------------------------------------------------------------------
// Duration
// -----------------------------------------------------------------------------

char* bench_fmt_ns(char* buf, size_t cap, uint64_t ns) {
  if (!buf || cap == 0) return buf;

  if (ns < 1000ULL) {
    (void)snprintf(buf, cap, "%lluns", (unsigned long long)ns);
    return buf;
  }

  const double d = (double)ns;
  if (ns < 1000000ULL) {
    char v[64];
    bench_fmt_f64_trim(v, sizeof(v), d / 1e3, 3);
    (void)snprintf(buf, cap, "%sus", v);
    return buf;
  }

  if (ns < 1000000000ULL) {
    char v[64];
    bench_fmt_f64_trim(v, sizeof(v), d / 1e6, 3);
    (void)snprintf(buf, cap, "%sms", v);
    return buf;
  }

  {
    char v[64];
    bench_fmt_f64_trim(v, sizeof(v), d / 1e9, 3);
    (void)snprintf(buf, cap, "%ss", v);
    return buf;
  }
}

// -----------------------------------------------------------------------------
// Bytes
// -----------------------------------------------------------------------------

char* bench_fmt_bytes(char* buf, size_t cap, uint64_t bytes) {
  if (!buf || cap == 0) return buf;

  static const char* units[] = { "B", "KiB", "MiB", "GiB", "TiB" };
  double v = (double)bytes;
  size_t u = 0;
  while (v >= 1024.0 && u + 1 < (sizeof(units)/sizeof(units[0]))) {
    v /= 1024.0;
    ++u;
  }

  if (u == 0) {
    (void)snprintf(buf, cap, "%lluB", (unsigned long long)bytes);
    return buf;
  }

  char n[64];
  bench_fmt_f64_trim(n, sizeof(n), v, 3);
  (void)snprintf(buf, cap, "%s%s", n, units[u]);
  return buf;
}

// -----------------------------------------------------------------------------
// Rates
// -----------------------------------------------------------------------------

char* bench_fmt_ops(char* buf, size_t cap, double ops_per_s) {
  if (!buf || cap == 0) return buf;

  if (isnan(ops_per_s)) return bench_fmt_write_(buf, cap, "nanops/s");
  if (isinf(ops_per_s)) return bench_fmt_write_(buf, cap, (ops_per_s > 0.0) ? "infops/s" : "-infops/s");

  static const char* units[] = { "ops/s", "Kop/s", "Mop/s", "Gop/s", "Top/s" };
  double v = ops_per_s;
  size_t u = 0;
  while (fabs(v) >= 1000.0 && u + 1 < (sizeof(units)/sizeof(units[0]))) {
    v /= 1000.0;
    ++u;
  }

  char n[64];
  bench_fmt_f64_trim(n, sizeof(n), v, 3);
  (void)snprintf(buf, cap, "%s%s", n, units[u]);
  return buf;
}

char* bench_fmt_bps(char* buf, size_t cap, double bytes_per_s) {
  if (!buf || cap == 0) return buf;

  if (isnan(bytes_per_s)) return bench_fmt_write_(buf, cap, "nanB/s");
  if (isinf(bytes_per_s)) return bench_fmt_write_(buf, cap, (bytes_per_s > 0.0) ? "infB/s" : "-infB/s");

  static const char* units[] = { "B/s", "KiB/s", "MiB/s", "GiB/s", "TiB/s" };
  double v = bytes_per_s;
  size_t u = 0;
  while (fabs(v) >= 1024.0 && u + 1 < (sizeof(units)/sizeof(units[0]))) {
    v /= 1024.0;
    ++u;
  }

  char n[64];
  bench_fmt_f64_trim(n, sizeof(n), v, 3);
  (void)snprintf(buf, cap, "%s%s", n, units[u]);
  return buf;
}

// -----------------------------------------------------------------------------
// Optional self-test
// -----------------------------------------------------------------------------

#if defined(BENCH_FORMAT_TEST)

int main(void) {
  char b[64];

  puts(bench_fmt_ns(b, sizeof(b), 999));
  puts(bench_fmt_ns(b, sizeof(b), 1234));
  puts(bench_fmt_ns(b, sizeof(b), 1234567));
  puts(bench_fmt_ns(b, sizeof(b), 1234567890ULL));

  puts(bench_fmt_bytes(b, sizeof(b), 12));
  puts(bench_fmt_bytes(b, sizeof(b), 4096));
  puts(bench_fmt_bytes(b, sizeof(b), 10ULL * 1024ULL * 1024ULL));

  puts(bench_fmt_ops(b, sizeof(b), 12));
  puts(bench_fmt_ops(b, sizeof(b), 12345));
  puts(bench_fmt_ops(b, sizeof(b), 12e6));

  puts(bench_fmt_bps(b, sizeof(b), 512));
  puts(bench_fmt_bps(b, sizeof(b), 1024*1024));

  puts(bench_fmt_pct(b, sizeof(b), 0.1234));

  return 0;
}

#endif
