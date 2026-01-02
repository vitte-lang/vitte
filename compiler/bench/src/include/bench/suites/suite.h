#pragma once
/*
  bench/suites/suite.h - helpers to define benchmark cases
*/
#include "bench/bench.h"

#define BENCH_CASE(SUITE, NAME, FN)   static void FN(bench_ctx *ctx);   static const bench_case FN##_case = { (SUITE), (NAME), (FN) };   static void FN(bench_ctx *ctx)

#define BENCH_REG(FN) bench_register(&FN##_case)
