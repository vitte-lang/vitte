# src/vitte/packages/lint/mod.vit

## usage
- use lint/mod.vit as *_pkg

## contre-exemple
- avoid non-deterministic lint output

## symbols
pick LintSeverity {
pick LintError {
pick LintResult[T] {
form LintRule {
form LintIssue {
form QuickFix {
form LintConfig {
form LintReport {
proc ok[T](value: T) -> LintResult[T] {
proc err[T](code: string, message: string, context: string) -> LintResult[T] {
proc diagnostics_message(code: string) -> string {
proc diagnostics_quickfix(code: string) -> string {
proc diagnostics_doc_url(code: string) -> string {
proc default_config() -> LintConfig {
proc list_rules() -> [LintRule] {
proc enable_rule(rules: [LintRule], id: string) -> [LintRule] {
proc disable_rule(rules: [LintRule], id: string) -> [LintRule] {
proc lint_file(path: string, text: string, cfg: LintConfig) -> LintResult[LintReport] {
proc lint_package(pkg: string, files: [string], cfg: LintConfig) -> LintResult[LintReport] {
proc lint_workspace(files: [string], cfg: LintConfig) -> LintResult[LintReport] {
proc report_json(report: LintReport) -> string {
proc report_sarif(report: LintReport) -> string {
proc report_md(report: LintReport) -> string {
proc quickfix_preview(path: string, text: string, cfg: LintConfig) -> LintResult[string] {
proc quickfix_apply(path: string, text: string, cfg: LintConfig) -> LintResult[string] {
proc policy_allows_ci_block(report: LintReport, cfg: LintConfig) -> bool {
proc api_version() -> string {
proc doctor_status() -> string {
proc ready() -> bool {
proc package_meta() -> string {
