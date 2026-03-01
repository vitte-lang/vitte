# src/vitte/packages/db/internal/runtime.vit

## usage
- import with: use db/internal/runtime.vit as *_pkg

## contre-exemple
- avoid unprepared query paths

## symbols
proc redact_secret(value: string) -> string {
proc diag_message(code: string) -> string {
proc quickfix_for(code: string) -> string {
proc starts_with(s: string, p: string) -> bool {
proc contains(text: string, needle: string) -> bool {
proc normalize_dsn(dsn: string) -> string {
proc profile_allows(profile: string) -> bool {
proc is_safe_sql(sql: string) -> bool {
proc execute_prepared_error_code(connected: bool, prepared: bool, safe_sql: bool, timeout_ms: int, cancel_token: string, pool_in_use: int, pool_max: int) -> string {
proc is_select_sql(sql: string) -> bool {
proc is_mutation_sql(sql: string) -> bool {
proc tx_begin_error_code(connected: bool) -> string {
proc tx_transition_error_code(active: bool, committed: bool, rolled_back: bool) -> string {
