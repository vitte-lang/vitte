#include "bench/bench.h"
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

/*
  bm_regex_match.c (macro)

  Goal:
  - Macro benchmark for regular expression matching on text patterns.
  - Simulates pattern matching workloads without external regex library.
  - Includes email validation, URL validation, simple glob matching.

  Design:
  - Multiple test cases of increasing complexity.
  - Matches both successful and failed patterns.
  - Measures real-world text processing costs.
*/

/* Simple regex-like matcher (limited but functional) */
static bool simple_glob_match(const char* pattern, const char* text) {
  while (*pattern && *text) {
    if (*pattern == '*') {
      /* '*' matches zero or more of any character */
      pattern++;
      if (!*pattern) return true;
      while (*text) {
        if (simple_glob_match(pattern, text))
          return true;
        text++;
      }
      return false;
    } else if (*pattern == '?') {
      /* '?' matches exactly one character */
      pattern++;
      text++;
    } else if (*pattern == *text) {
      pattern++;
      text++;
    } else {
      return false;
    }
  }
  return !*pattern && !*text;
}

/* Email validation (simple) */
static bool is_valid_email(const char* email) {
  const char* at = strchr(email, '@');
  if (!at || at == email) return false;
  
  const char* dot = strchr(at, '.');
  if (!dot || dot == at + 1 || !*(dot + 1)) return false;
  
  return true;
}

/* URL validation (simple) */
static bool is_valid_url(const char* url) {
  if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
    return false;
  }
  const char* slash = strchr(url + 8, '/');
  const char* domain_start = (strncmp(url, "https://", 8) == 0) ? url + 8 : url + 7;
  const char* domain_end = slash ? slash : url + strlen(url);
  
  if (domain_end - domain_start < 3) return false;
  
  const char* dot = strchr(domain_start, '.');
  if (!dot || dot >= domain_end || dot == domain_start) return false;
  
  return true;
}

/* Test data: various patterns and texts */
static const struct {
  const char* pattern;
  const char* text;
  bool should_match;
} test_patterns[] = {
  { "*.txt", "file.txt", true },
  { "*.txt", "file.doc", false },
  { "test_*.c", "test_bench.c", true },
  { "test_*.c", "bench_test.c", false },
  { "hello*world", "hello beautiful world", true },
  { "hello*world", "hello world", true },
  { "hello*world", "goodbye world", false },
  { "????.rs", "main.rs", true },
  { "????.rs", "lib.rs", false },
  { "*_test.go", "unit_test.go", true },
  { "*_test.go", "unit_test.rs", false },
};

static const char* test_emails[] = {
  "user@example.com",
  "alice+tag@mail.co.uk",
  "dev@localhost.local",
  "invalid@.com",
  "@nodomain.net",
  "noatsign.com",
  "user@",
};

static const char* test_urls[] = {
  "http://example.com",
  "https://api.github.com/repos/vitte",
  "http://localhost:8080/",
  "ftp://invalid.com",
  "https://",
  "no-scheme.com",
};

static void bm_regex_glob(void* ctx) {
  (void)ctx;
  
  /* Glob matching: 100 iterations per pattern */
  for (int iter = 0; iter < 100; iter++) {
    for (size_t i = 0; i < sizeof(test_patterns) / sizeof(test_patterns[0]); i++) {
      bool result = simple_glob_match(
        test_patterns[i].pattern,
        test_patterns[i].text
      );
      (void)result;
    }
  }
}

static void bm_regex_email(void* ctx) {
  (void)ctx;
  
  /* Email validation: 200 iterations per email */
  for (int iter = 0; iter < 200; iter++) {
    for (size_t i = 0; i < sizeof(test_emails) / sizeof(test_emails[0]); i++) {
      bool result = is_valid_email(test_emails[i]);
      (void)result;
    }
  }
}

static void bm_regex_url(void* ctx) {
  (void)ctx;
  
  /* URL validation: 200 iterations per URL */
  for (int iter = 0; iter < 200; iter++) {
    for (size_t i = 0; i < sizeof(test_urls) / sizeof(test_urls[0]); i++) {
      bool result = is_valid_url(test_urls[i]);
      (void)result;
    }
  }
}

void bench_register_macro_regex(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("macro:glob", BENCH_MACRO, bm_regex_glob, NULL);
  bench_registry_add("macro:email", BENCH_MACRO, bm_regex_email, NULL);
  bench_registry_add("macro:url", BENCH_MACRO, bm_regex_url, NULL);
}
