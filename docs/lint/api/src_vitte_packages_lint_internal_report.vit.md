# src/vitte/packages/lint/internal/report.vit

## usage
- use lint/internal/report.vit as *_pkg

## contre-exemple
- avoid non-deterministic lint output

## symbols
pick LintSeverity {
form LintIssue {
form LintReport {
proc report_json(r: LintReport) -> string {
proc report_sarif(r: LintReport) -> string {
proc report_md(r: LintReport) -> string {
