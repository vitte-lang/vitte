# src/vitte/packages/lint/internal/engine.vit

## usage
- use lint/internal/engine.vit as *_pkg

## contre-exemple
- avoid non-deterministic lint output

## symbols
pick LintSeverity {
pick LintError {
pick LintResult[T] {
form LintRule {
form LintIssue {
form LintConfig {
form LintReport {
proc ok[T](value: T) -> LintResult[T] {
proc err[T](code: string, message: string, context: string) -> LintResult[T] {
proc contains(text: string, needle: string) -> bool {
proc make_issue(code: string, severity: LintSeverity, file: string, line: int, message: string, rule_id: string, quickfix: string) -> LintIssue {
proc issues_from_text(file: string, text: string, rules: [LintRule], cfg: LintConfig) -> [LintIssue] {
proc build_report(issues: [LintIssue], latency_ms: int) -> LintReport {
proc lint_file(path: string, text: string, rules: [LintRule], cfg: LintConfig) -> LintResult[LintReport] {
proc lint_package(pkg: string, files: [string], rules: [LintRule], cfg: LintConfig) -> LintResult[LintReport] {
proc lint_workspace(files: [string], rules: [LintRule], cfg: LintConfig) -> LintResult[LintReport] {
proc diag_message(code: string) -> string {
proc quickfix_for(code: string) -> string {
