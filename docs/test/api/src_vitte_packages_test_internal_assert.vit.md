# src/vitte/packages/test/internal/assert.vit

## usage
- use test/internal/assert.vit as *_pkg

## contre-exemple
- avoid side effects in test modules

## symbols
pick TestError {
pick TestResult[T] {
form TestCase {
proc ok[T](value: T) -> TestResult[T] {
proc err[T](code: string, message: string, context: string, retryable: bool) -> TestResult[T] {
proc contains(text: string, needle: string) -> bool {
proc mk_case(name: string, passed: bool, message: string, code: string) -> TestCase {
proc assert_true(name: string, condition: bool, message: string) -> TestResult[TestCase] {
proc assert_eq(name: string, left: string, right: string) -> TestResult[TestCase] {
proc assert_ne(name: string, left: string, right: string) -> TestResult[TestCase] {
proc assert_match(name: string, text: string, needle: string) -> TestResult[TestCase] {
