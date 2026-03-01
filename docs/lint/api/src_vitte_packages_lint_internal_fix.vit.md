# src/vitte/packages/lint/internal/fix.vit

## usage
- use lint/internal/fix.vit as *_pkg

## contre-exemple
- avoid non-deterministic lint output

## symbols
pick LintError {
pick LintResult[T] {
form LintConfig {
proc ok[T](value: T) -> LintResult[T] {
proc err[T](code: string, message: string, context: string) -> LintResult[T] {
proc contains(text: string, needle: string) -> bool {
proc quickfix_preview(path: string, text: string, cfg: LintConfig) -> LintResult[string] {
proc quickfix_apply(path: string, text: string, cfg: LintConfig) -> LintResult[string] {
