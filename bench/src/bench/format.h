// format.h — tiny formatting helpers for benchmark output (C17)

#ifndef VITTE_BENCH_FORMAT_H
#define VITTE_BENCH_FORMAT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
} // extern "C"
#endif

#endif // VITTE_BENCH_FORMAT_H
