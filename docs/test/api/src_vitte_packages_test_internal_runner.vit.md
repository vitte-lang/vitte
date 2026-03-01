# src/vitte/packages/test/internal/runner.vit

## usage
- use test/internal/runner.vit as *_pkg

## contre-exemple
- avoid side effects in test modules

## symbols
pick TestMode {
pick TestError {
pick TestResult[T] {
form TestCase {
form TestSuite {
form TestContext {
form TestReport {
proc ok[T](value: T) -> TestResult[T] {
proc err[T](code: string, message: string, context: string, retryable: bool) -> TestResult[T] {
proc default_context() -> TestContext {
proc context_with_mode(ctx: TestContext, mode: TestMode) -> TestContext {
proc context_with_profile(ctx: TestContext, profile: string) -> TestContext {
proc context_with_timeout(ctx: TestContext, timeout_ms: int, suite_timeout_ms: int) -> TestContext {
proc is_profile_supported(profile: string) -> bool {
proc validate_context(ctx: TestContext) -> TestResult[bool] {
proc suite(name: string, mode: TestMode, profile: string, timeout_ms: int, max_retries: int) -> TestSuite {
proc case_new(name: string, passed: bool, message: string, duration_ms: int, retries: int, attempts: int, flaky: bool, timed_out: bool, last_failure_code: string, tags: [string]) -> TestCase {
proc suite_add_case(s: TestSuite, c: TestCase) -> TestSuite {
proc contains(text: string, needle: string) -> bool {
proc has_tag(c: TestCase, tags: [string]) -> bool {
proc report_for_suite(ctx: TestContext, s: TestSuite, chosen: [bool]) -> TestResult[TestReport] {
proc run_suite(ctx: TestContext, s: TestSuite) -> TestResult[TestReport] {
proc run_selected(ctx: TestContext, s: TestSuite, names: [string]) -> TestResult[TestReport] {
proc run_tagged(ctx: TestContext, s: TestSuite, tags: [string]) -> TestResult[TestReport] {
proc run_all(ctx: TestContext, suites: [TestSuite]) -> TestResult[[TestReport]] {
proc merge_reports(reports: [TestReport]) -> TestReport {
proc profile_matrix_check(s: TestSuite) -> TestResult[bool] {
proc mode_name(mode: TestMode) -> string {
proc report_json(r: TestReport) -> string {
proc report_md(r: TestReport) -> string {
proc report_compact(r: TestReport) -> string {
proc quickfix_preview(payload: string) -> string {
proc quickfix_apply(payload: string) -> string {
proc doc_url_for(code: string) -> string {
proc diag_message(code: string) -> string {
proc quickfix_for(code: string) -> string {
