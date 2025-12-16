#include "bench/sample.h"
#include <stdlib.h>
#include <string.h>

/*
  sample.c

  Sample collection and analysis utilities.
*/

bench_sample_buffer_t* bench_sample_buffer_new(size_t capacity) {
  if (capacity == 0) {
    return NULL;
  }

  bench_sample_buffer_t* buf = (bench_sample_buffer_t*)malloc(sizeof(*buf));
  if (!buf) {
    return NULL;
  }

  buf->data = (double*)malloc(capacity * sizeof(double));
  if (!buf->data) {
    free(buf);
    return NULL;
  }

  buf->capacity = capacity;
  buf->count = 0;

  return buf;
}

void bench_sample_buffer_free(bench_sample_buffer_t* buf) {
  if (buf) {
    if (buf->data) {
      free(buf->data);
    }
    free(buf);
  }
}

int bench_sample_buffer_add(bench_sample_buffer_t* buf, double value) {
  if (!buf || buf->count >= buf->capacity) {
    return 0;
  }

  buf->data[buf->count++] = value;
  return 1;
}

const double* bench_sample_buffer_data(const bench_sample_buffer_t* buf) {
  return buf ? buf->data : NULL;
}

size_t bench_sample_buffer_count(const bench_sample_buffer_t* buf) {
  return buf ? buf->count : 0;
}

void bench_sample_buffer_clear(bench_sample_buffer_t* buf) {
  if (buf) {
    buf->count = 0;
  }
}

int bench_sample_buffer_full(const bench_sample_buffer_t* buf) {
  return buf && (buf->count >= buf->capacity);
}
