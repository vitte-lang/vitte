# src/vitte/packages/http/internal/runtime.vit

## usage
- use http/internal/runtime.vit as *_pkg

## contre-exemple
- avoid CRLF/header injection

## symbols
proc starts_with(s: string, p: string) -> bool {
proc contains(text: string, needle: string) -> bool {
proc is_space(ch: string) -> bool {
proc parse_method_token(line: string) -> string {
proc parse_path_token(line: string) -> string {
proc parse_version_token(line: string) -> string {
proc normalize_path(path: string) -> string {
proc method_known(method: string) -> bool {
proc is_private_host(host: string) -> bool {
proc host_from_url(url: string) -> string {
proc is_private_host_from_url(url: string) -> bool {
proc parse_query(path_or_url: string) -> string {
proc has_crlf(text: string) -> bool {
proc status_is_valid(status: int) -> bool {
proc content_length_matches(body: string, content_length: int) -> bool {
proc is_timeout_or_cancelled(cancel_token: string) -> bool {
proc timeout_config_valid(read_ms: int, write_ms: int, idle_ms: int, header_ms: int, body_ms: int) -> bool {
proc body_limit_exceeded(body_len: int, route_limit: int, global_limit: int) -> bool {
proc profile_allows_network(profile: string) -> bool {
proc cors_origin_allowed(allow_origin: string, origin: string) -> bool {
proc route_pattern_kind(pattern: string) -> int {
proc route_matches(pattern: string, path: string) -> bool {
proc route_rank(priority: int, pattern: string) -> int {
proc redact_sensitive(value: string) -> string {
proc body_limit_code() -> string {
proc timeout_code() -> string {
proc reason_phrase(status: int) -> string {
proc diag_message(code: string) -> string {
proc quickfix_for(code: string) -> string {
proc openapi_contract_diff_missing(old_snapshot: [string], new_snapshot: [string]) -> string {
