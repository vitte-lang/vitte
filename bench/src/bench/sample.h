#pragma once

/*
  sample.h

  Sample collection and analysis utilities.
*/

#ifndef VITTE_BENCH_SAMPLE_H
#define VITTE_BENCH_SAMPLE_H

#include <stdint.h>
#include <stddef.h>

/* Sample buffer for storing benchmark measurements */
typedef struct {
  double* data;
  size_t capacity;
  size_t count;
} bench_sample_buffer_t;

/* Allocate sample buffer */
bench_sample_buffer_t* bench_sample_buffer_new(size_t capacity);

/* Free sample buffer */
void bench_sample_buffer_free(bench_sample_buffer_t* buf);

/* Add sample to buffer */
int bench_sample_buffer_add(bench_sample_buffer_t* buf, double value);

/* Get all samples */
const double* bench_sample_buffer_data(const bench_sample_buffer_t* buf);

/* Get sample count */
size_t bench_sample_buffer_count(const bench_sample_buffer_t* buf);

/* Clear buffer */
void bench_sample_buffer_clear(bench_sample_buffer_t* buf);

/* Check if buffer is full */
int bench_sample_buffer_full(const bench_sample_buffer_t* buf);

#endif /* VITTE_BENCH_SAMPLE_H */
