#include "bench/bench.h"
#include <string.h>
#include <math.h>

/*
  bm_math_compute.c (macro)

  Goal:
  - Macro benchmark for mathematical computations.
  - Tests trigonometry, logarithms, floating-point operations.
  - Measures numerical computation costs.

  Design:
  - Matrix operations (small 4x4 and larger 10x10).
  - Trigonometric functions over ranges.
  - Statistical calculations.
*/

/* Simple 4x4 matrix multiplication */
typedef struct {
  double m[4][4];
} Matrix4;

static Matrix4 matrix4_mult(const Matrix4* a, const Matrix4* b) {
  Matrix4 result = {0};
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      double sum = 0.0;
      for (int k = 0; k < 4; k++) {
        sum += a->m[i][k] * b->m[k][j];
      }
      result.m[i][j] = sum;
    }
  }
  return result;
}

/* Simple 10x10 matrix multiplication */
typedef struct {
  double m[10][10];
} Matrix10;

static Matrix10 matrix10_mult(const Matrix10* a, const Matrix10* b) {
  Matrix10 result = {0};
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      double sum = 0.0;
      for (int k = 0; k < 10; k++) {
        sum += a->m[i][k] * b->m[k][j];
      }
      result.m[i][j] = sum;
    }
  }
  return result;
}

/* Initialize matrix with pattern */
static void matrix4_init(Matrix4* m, int seed) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      m->m[i][j] = (double)((seed + i * 4 + j) % 100) / 10.0;
    }
  }
}

static void matrix10_init(Matrix10* m, int seed) {
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      m->m[i][j] = (double)((seed + i * 10 + j) % 100) / 10.0;
    }
  }
}

static int bm_math_matrix4(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  Matrix4 a, b;
  matrix4_init(&a, 1);
  matrix4_init(&b, 42);
  
  for (int iter = 0; iter < 1000; iter++) {
    Matrix4 result = matrix4_mult(&a, &b);
    b = result;  /* reuse result as next input */
  }
  return 0;
}

static int bm_math_matrix10(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  Matrix10 a, b;
  matrix10_init(&a, 1);
  matrix10_init(&b, 42);
  
  for (int iter = 0; iter < 100; iter++) {
    Matrix10 result = matrix10_mult(&a, &b);
    b = result;
  }
  return 0;
}

static int bm_math_trig(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  double sum = 0.0;
  for (int iter = 0; iter < 1000; iter++) {
    for (double angle = 0.0; angle < 2.0 * 3.14159265358979; angle += 0.01) {
      double s = sin(angle);
      double c = cos(angle);
      double t = tan(angle);
      sum += s * c + t;
    }
  }
  (void)sum;  /* avoid optimizer removing it */
  return 0;
}

static int bm_math_log_exp(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  double sum = 0.0;
  for (int iter = 0; iter < 1000; iter++) {
    for (double x = 0.1; x < 100.0; x += 0.1) {
      double l = log(x);
      double e = exp(l);
      sum += l + e;
    }
  }
  (void)sum;
  return 0;
}

static int bm_math_sqrt(void* ctx, int64_t iters) {
  (void)ctx;
  (void)iters;
  
  double sum = 0.0;
  for (int iter = 0; iter < 10000; iter++) {
    for (double x = 1.0; x < 10000.0; x += 1.0) {
      double r = sqrt(x);
      double r2 = r * r;
      sum += r + r2;
    }
  }
  (void)sum;
  return 0;
}

void bench_register_macro_math(void) {
  extern int bench_registry_add(const char* id, int kind, bench_fn_t fn, void* ctx);
  
  bench_registry_add("macro:matrix4", BENCH_MACRO, bm_math_matrix4, NULL);
  bench_registry_add("macro:matrix10", BENCH_MACRO, bm_math_matrix10, NULL);
  bench_registry_add("macro:trig", BENCH_MACRO, bm_math_trig, NULL);
  bench_registry_add("macro:log_exp", BENCH_MACRO, bm_math_log_exp, NULL);
  bench_registry_add("macro:sqrt", BENCH_MACRO, bm_math_sqrt, NULL);
}
