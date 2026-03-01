# src/vitte/packages/lint/internal/policy.vit

## usage
- use lint/internal/policy.vit as *_pkg

## contre-exemple
- avoid non-deterministic lint output

## symbols
form LintConfig {
form LintReport {
proc policy_allows_ci_block(report: LintReport, cfg: LintConfig) -> bool {
