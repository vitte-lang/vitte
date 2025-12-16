#pragma once

/*
  strutil.h

  String utility functions.
*/

#ifndef VITTE_BENCH_STRUTIL_H
#define VITTE_BENCH_STRUTIL_H

#include <stddef.h>

/* String duplication */
char* bench_strdup(const char* str);

/* Safe string copy */
char* bench_strndup(const char* str, size_t max_len);

/* String concatenation */
char* bench_strcat_alloc(const char* a, const char* b);

/* Trim whitespace */
char* bench_strtrim(char* str);

/* String starts with */
int bench_str_startswith(const char* str, const char* prefix);

/* String ends with */
int bench_str_endswith(const char* str, const char* suffix);

/* Case-insensitive comparison */
int bench_strcasecmp(const char* a, const char* b);

/* Replace substring */
char* bench_str_replace(const char* str, const char* old, const char* new);

#endif /* VITTE_BENCH_STRUTIL_H */
