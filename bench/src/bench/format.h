#pragma once

/*
  format.h

  String formatting utilities.
*/

#ifndef VITTE_BENCH_FORMAT_H
#define VITTE_BENCH_FORMAT_H

#include <stddef.h>
#include <stdio.h>

/* Format nanoseconds to human-readable form */
int bench_format_ns(char* buf, size_t bufsz, double ns);

/* Format bytes to human-readable form */
int bench_format_bytes(char* buf, size_t bufsz, size_t bytes);

/* Format percentage */
int bench_format_percent(char* buf, size_t bufsz, double value);

/* Format throughput (ops/sec) */
int bench_format_ops_per_sec(char* buf, size_t bufsz, double ops_per_sec);

/* Safe printf to buffer */
int bench_snprintf(char* buf, size_t bufsz, const char* fmt, ...);

#endif /* VITTE_BENCH_FORMAT_H */
