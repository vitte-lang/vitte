# src/vitte/packages/test/internal/fuzz_bridge.vit

## usage
- use test/internal/fuzz_bridge.vit as *_pkg

## contre-exemple
- avoid side effects in test modules

## symbols
pick TestError {
pick TestResult[T] {
form FuzzConfig {
proc ok[T](value: T) -> TestResult[T] {
proc err[T](code: string, message: string, context: string, retryable: bool) -> TestResult[T] {
proc run_fuzz(cfg: FuzzConfig) -> TestResult[string] {
