# src/vitte/packages/test/internal/bench_bridge.vit

## usage
- use test/internal/bench_bridge.vit as *_pkg

## contre-exemple
- avoid side effects in test modules

## symbols
pick TestError {
pick TestResult[T] {
form BenchConfig {
proc ok[T](value: T) -> TestResult[T] {
proc err[T](code: string, message: string, context: string, retryable: bool) -> TestResult[T] {
proc run_bench(cfg: BenchConfig) -> TestResult[string] {
