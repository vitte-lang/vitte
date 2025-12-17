// strutil.h - small string utilities for vitte/bench (C17)
//
// SPDX-License-Identifier: MIT

#ifndef VITTE_BENCH_STRUTIL_H
#define VITTE_BENCH_STRUTIL_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

// Duplicate a NUL-terminated string. Returns NULL on OOM.
char* bench_strdup(const char* s);

// Duplicate up to N bytes (stops early on NUL). Always NUL-terminates.
// Returns NULL on OOM.
char* bench_strndup(const char* s, size_t n);

// ASCII-only, case-insensitive compare.
// Returns <0, 0, >0 like strcmp.
int bench_stricmp(const char* a, const char* b);

// ASCII-only, case-insensitive compare for first N chars.
int bench_strnicmp(const char* a, const char* b, size_t n);

// Prefix/suffix helpers.
bool bench_strstarts(const char* s, const char* prefix);
bool bench_strends(const char* s, const char* suffix);

// Trim ASCII whitespace in-place. Returns pointer to first non-ws char
// within the same buffer.
char* bench_strtrim_ascii(char* s);

// Tiny glob matcher supporting '*' and '?' only. Case-sensitive.
// Returns true if `text` matches `pattern`.
bool bench_glob_match(const char* pattern, const char* text);

#if defined(__cplusplus)
} // extern "C"
#endif

#endif // VITTE_BENCH_STRUTIL_H
