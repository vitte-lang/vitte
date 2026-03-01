# src/vitte/packages/lint/internal/rules.vit

## usage
- use lint/internal/rules.vit as *_pkg

## contre-exemple
- avoid non-deterministic lint output

## symbols
form LintRule {
proc default_rules() -> [LintRule] {
proc enable_rule(rules: [LintRule], id: string) -> [LintRule] {
proc disable_rule(rules: [LintRule], id: string) -> [LintRule] {
