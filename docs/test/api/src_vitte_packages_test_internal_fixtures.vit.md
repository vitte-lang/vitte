# src/vitte/packages/test/internal/fixtures.vit

## usage
- use test/internal/fixtures.vit as *_pkg

## contre-exemple
- avoid side effects in test modules

## symbols
pick TestError {
pick TestResult[T] {
proc ok[T](value: T) -> TestResult[T] {
proc err[T](code: string, message: string, context: string, retryable: bool) -> TestResult[T] {
proc starts_with(text: string, prefix: string) -> bool {
proc load_fixture(path: string) -> TestResult[string] {
