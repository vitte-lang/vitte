# src/vitte/packages/process/mod.vit

## usage
- use process/mod.vit as *_pkg

## contre-exemple
- avoid unsafe shell/argv/env

## symbols
form ProcessError {
pick ProcessResult[T] {
pick ProcessSignal {
form ProcessPolicy {
pick ProcessStdio {
form ProcessConfig {
form ProcessHandle {
form ProcessOutput {
proc default_policy() -> ProcessPolicy {
proc default_config(program: string) -> ProcessConfig {
proc ok[T](value: T) -> ProcessResult[T] {
proc err[T](code: string, message: string, context: string, retryable: bool) -> ProcessResult[T] {
proc diagnostics_message(code: string) -> string {
proc diagnostics_quickfix(code: string) -> string {
proc diagnostics_doc_url(code: string) -> string {
proc with_timeout(cfg: ProcessConfig, timeout_ms: int, grace_ms: int) -> ProcessConfig {
proc with_cwd(cfg: ProcessConfig, cwd: string) -> ProcessConfig {
proc with_env(cfg: ProcessConfig, env: [string]) -> ProcessConfig {
proc with_policy(cfg: ProcessConfig, policy: ProcessPolicy) -> ProcessConfig {
proc validate_config(cfg: ProcessConfig) -> ProcessResult[bool] {
proc spawn(cfg: ProcessConfig) -> ProcessResult[ProcessHandle] {
proc wait(handle: ProcessHandle, timeout_ms: int) -> ProcessResult[ProcessOutput] {
proc terminate(handle: ProcessHandle) -> ProcessResult[bool] {
proc kill(handle: ProcessHandle) -> ProcessResult[bool] {
proc run(cfg: ProcessConfig) -> ProcessResult[ProcessOutput] {
proc analyze_report_json_stub() -> string {
proc api_version() -> string {
proc doctor_status() -> string {
proc quickfix_preview(payload: string) -> string {
proc quickfix_apply(payload: string) -> string {
proc ready() -> bool {
proc package_meta() -> string {
proc spec(path: string) -> process_compat_pkg.ProcSpec {
proc with_args(s: process_compat_pkg.ProcSpec, args: [string]) -> process_compat_pkg.ProcSpec {
proc with_env_count(s: process_compat_pkg.ProcSpec, env_count: int) -> process_compat_pkg.ProcSpec {
proc with_dry_run(s: process_compat_pkg.ProcSpec, enabled: bool) -> process_compat_pkg.ProcSpec {
proc run_legacy(s: process_compat_pkg.ProcSpec) -> process_compat_pkg.ProcHandle {
proc is_success(h: process_compat_pkg.ProcHandle) -> bool {
proc exit_code(h: process_compat_pkg.ProcHandle) -> int {
proc run_shell(cmdline: string) -> int {
