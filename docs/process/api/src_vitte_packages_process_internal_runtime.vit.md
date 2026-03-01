# src/vitte/packages/process/internal/runtime.vit

## usage
- use process/internal/runtime.vit as *_pkg

## contre-exemple
- avoid unsafe shell/argv/env

## symbols
proc contains(text: string, needle: string) -> bool {
proc has_control_chars(text: string) -> bool {
proc starts_with(text: string, prefix: string) -> bool {
proc is_empty(text: string) -> bool {
proc normalize_program(program: string) -> string {
proc normalize_cwd(cwd: string) -> string {
proc is_profile_allowed(profile: string) -> bool {
proc command_allowlisted(program: string, allowlist: [string]) -> bool {
proc command_denylisted(program: string) -> bool {
proc shell_injection_pattern(args: [string]) -> bool {
proc shell_exec_pattern(program: string, args: [string]) -> bool {
proc env_valid(env: [string]) -> bool {
proc args_valid(args: [string]) -> bool {
proc cwd_valid(cwd: string) -> bool {
proc clamp_timeout(timeout_ms: int) -> int {
proc clamp_grace(grace_ms: int) -> int {
proc clamp_capture(limit: int) -> int {
proc diag_message(code: string) -> string {
proc quickfix_for(code: string) -> string {
proc spawn_pid(program: string, args_count: int) -> int {
proc policy_hash(program: string, profile: string, allow_shell: bool, allow_privileged: bool) -> int {
proc spawn_background_allowed(allow_background: bool, detached: bool) -> bool {
proc wait_elapsed(started_ms: int, timeout_ms: int) -> int {
proc wait_timed_out(timeout_ms: int, requested_timeout_ms: int) -> bool {
proc wait_success_exit() -> int {
proc wait_timeout_exit() -> int {
proc terminate_allowed(running: bool) -> bool {
proc kill_allowed(running: bool) -> bool {
proc capture_truncate(text: string, limit: int) -> string {
proc capture_bytes(text: string) -> int {
proc telemetry_update_p50(current: int, value: int) -> int {
proc telemetry_update_p95(current: int, value: int) -> int {
