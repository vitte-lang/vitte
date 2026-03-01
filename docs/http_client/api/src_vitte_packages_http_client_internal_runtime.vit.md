# src/vitte/packages/http_client/internal/runtime.vit

## usage
- use http_client/internal/runtime.vit as *_pkg

## contre-exemple
- avoid insecure redirects/tls

## symbols
proc starts_with(s: string, p: string) -> bool {
proc contains(text: string, needle: string) -> bool {
proc is_https(url: string) -> bool {
proc host(url: string) -> string {
proc is_private_host(host: string) -> bool {
proc redact_sensitive(value: string) -> string {
proc diag_message(code: string) -> string {
proc quickfix_for(code: string) -> string {
proc validate_request_code(url: string, strict_tls: bool, allow_private_hosts: bool, cancel_token: string, profile: string) -> string {
proc pool_checkout_next(in_use: int, max_per_host: int) -> int {
proc pool_release_next(in_use: int) -> int {
proc circuit_check_code(open: bool) -> string {
proc retry_decision(attempt: int, max_retries: int, idempotent_only: bool, request_is_idempotent: bool, status: int) -> bool {
proc next_retry_elapsed(elapsed_ms: int, base_delay_ms: int, jitter_ms: int) -> int {
proc observe_p50(prev: int, elapsed_ms: int) -> int {
proc observe_p95(prev: int, elapsed_ms: int) -> int {
proc observe_errors(prev: int, error_happened: bool) -> int {
proc circuit_open_from_errors(errors: int, threshold: int) -> bool {
