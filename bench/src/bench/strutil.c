#include "bench/strutil.h"
#include "bench/alloc.h"
#include <string.h>
#include <ctype.h>

/*
  strutil.c

  String utility functions implementation.
*/

char* bench_strdup(const char* str) {
  if (!str) return NULL;
  size_t len = strlen(str) + 1;
  char* dup = (char*)bench_malloc(len);
  if (dup) {
    memcpy(dup, str, len);
  }
  return dup;
}

char* bench_strndup(const char* str, size_t max_len) {
  if (!str) return NULL;
  size_t len = strlen(str);
  if (len > max_len) len = max_len;
  char* dup = (char*)bench_malloc(len + 1);
  if (dup) {
    memcpy(dup, str, len);
    dup[len] = '\0';
  }
  return dup;
}

char* bench_strcat_alloc(const char* a, const char* b) {
  if (!a || !b) return NULL;
  size_t len_a = strlen(a);
  size_t len_b = strlen(b);
  char* result = (char*)bench_malloc(len_a + len_b + 1);
  if (result) {
    memcpy(result, a, len_a);
    memcpy(result + len_a, b, len_b + 1);
  }
  return result;
}

char* bench_strtrim(char* str) {
  if (!str) return str;

  /* Trim leading */
  while (*str && isspace((unsigned char)*str)) {
    str++;
  }

  /* Trim trailing */
  char* end = str + strlen(str) - 1;
  while (end >= str && isspace((unsigned char)*end)) {
    *end = '\0';
    end--;
  }

  return str;
}

int bench_str_startswith(const char* str, const char* prefix) {
  if (!str || !prefix) return 0;
  return strncmp(str, prefix, strlen(prefix)) == 0;
}

int bench_str_endswith(const char* str, const char* suffix) {
  if (!str || !suffix) return 0;
  size_t str_len = strlen(str);
  size_t suffix_len = strlen(suffix);
  if (suffix_len > str_len) return 0;
  return strcmp(str + str_len - suffix_len, suffix) == 0;
}

int bench_strcasecmp(const char* a, const char* b) {
  if (!a || !b) return (a == b) ? 0 : 1;
  while (*a && *b) {
    int ca = tolower((unsigned char)*a);
    int cb = tolower((unsigned char)*b);
    if (ca != cb) return ca - cb;
    a++;
    b++;
  }
  return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

char* bench_str_replace(const char* str, const char* old, const char* new) {
  if (!str || !old || !new) return NULL;

  size_t old_len = strlen(old);
  size_t new_len = strlen(new);
  size_t str_len = strlen(str);

  /* Count occurrences */
  int count = 0;
  const char* pos = str;
  while ((pos = strstr(pos, old)) != NULL) {
    count++;
    pos += old_len;
  }

  if (count == 0) {
    return bench_strdup(str);
  }

  /* Allocate result */
  size_t result_len = str_len + count * (new_len - old_len) + 1;
  char* result = (char*)bench_malloc(result_len);
  if (!result) return NULL;

  /* Replace */
  char* dst = result;
  pos = str;
  while ((pos = strstr(pos, old)) != NULL) {
    size_t before = pos - str;
    memcpy(dst, str, before);
    dst += before;
    memcpy(dst, new, new_len);
    dst += new_len;
    str = pos + old_len;
    pos = str;
  }

  strcpy(dst, str);
  return result;
}
