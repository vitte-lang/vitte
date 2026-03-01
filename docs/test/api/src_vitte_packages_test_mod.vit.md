# src/vitte/packages/test/mod.vit

## usage
- use test/mod.vit as *_pkg

## contre-exemple
- avoid side effects in test modules

## symbols
pick TestMode {
pick TestError {
pick TestResult[T] {
form TestCase {
form TestSuite {
form TestContext {
form SnapshotConfig {
form FuzzConfig {
form BenchConfig {
form TestReport {
proc ok[T](value: T) -> TestResult[T] {
proc err[T](code: string, message: string, context: string, retryable: bool, doc_url: string) -> TestResult[T] {
proc diagnostics_message(code: string) -> string {
proc diagnostics_quickfix(code: string) -> string {
proc diagnostics_doc_url(code: string) -> string {
proc api_version() -> string {
proc doctor_status() -> string {
proc default_context() -> TestContext {
proc context_with_mode(ctx: TestContext, mode: TestMode) -> TestContext {
proc context_with_profile(ctx: TestContext, profile: string) -> TestContext {
proc context_with_timeout(ctx: TestContext, timeout_ms: int, suite_timeout_ms: int) -> TestContext {
proc validate_context(ctx: TestContext) -> TestResult[bool] {
proc default_snapshot_config() -> SnapshotConfig {
proc default_fuzz_config() -> FuzzConfig {
proc default_bench_config() -> BenchConfig {
proc assert_true(name: string, condition: bool, message: string) -> TestResult[TestCase] {
proc assert_eq(name: string, left: string, right: string) -> TestResult[TestCase] {
proc assert_ne(name: string, left: string, right: string) -> TestResult[TestCase] {
proc assert_match(name: string, text: string, needle: string) -> TestResult[TestCase] {
proc suite(name: string, mode: TestMode, profile: string, timeout_ms: int, max_retries: int) -> TestSuite {
proc case_new(name: string, passed: bool, message: string, duration_ms: int, retries: int, attempts: int, flaky: bool, timed_out: bool, last_failure_code: string, tags: [string]) -> TestCase {
proc suite_add_case(s: TestSuite, c: TestCase) -> TestSuite {
proc run_suite(ctx: TestContext, s: TestSuite) -> TestResult[TestReport] {
proc run_all(ctx: TestContext, suites: [TestSuite]) -> TestResult[[TestReport]] {
proc run_selected(ctx: TestContext, s: TestSuite, names: [string]) -> TestResult[TestReport] {
proc run_tagged(ctx: TestContext, s: TestSuite, tags: [string]) -> TestResult[TestReport] {
proc merge_reports(reports: [TestReport]) -> TestReport {
proc is_profile_supported(profile: string) -> bool {
proc profile_matrix_check(s: TestSuite) -> TestResult[bool] {
proc load_fixture(path: string) -> TestResult[string] {
proc snapshot_expect(name: string, got: string, expected: string, cfg: SnapshotConfig) -> TestResult[bool] {
proc snapshot_update(name: string, got: string, cfg: SnapshotConfig) -> TestResult[string] {
proc snapshot_diff(got: string, expected: string) -> string {
proc run_fuzz(cfg: FuzzConfig) -> TestResult[string] {
proc run_bench(cfg: BenchConfig) -> TestResult[string] {
proc report_json(report: TestReport) -> string {
proc report_md(report: TestReport) -> string {
proc report_compact(report: TestReport) -> string {
proc quickfix_preview(payload: string) -> string {
proc quickfix_apply(payload: string) -> string {
proc ready() -> bool {
proc package_meta() -> string {
