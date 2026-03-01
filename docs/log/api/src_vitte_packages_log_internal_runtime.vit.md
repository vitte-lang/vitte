# src/vitte/packages/log/internal/runtime.vit

## usage
- import with: use log/internal/runtime.vit as *_pkg

## contre-exemple
- avoid side effects in module scope

## symbols
proc redact_field_value(key: string, value: string) -> string {
proc safe_message(message: string) -> string {
proc sink_allowed_for_profile(profile: string, sink_name: string) -> bool {
proc fallback_sink_name() -> string {
proc level_name_from_rank(rank: int) -> string {
proc diag_message(code: string) -> string {
proc quickfix_for(code: string) -> string {
proc emit_enabled(
proc route_module_name(explicit_module: string, context_module: string) -> string {
proc dedup_key(level_name: string, module: string, message: string) -> string {
proc dedup_hit(key: string, last_key: string, last_repeat: int, dedup_window: int) -> bool {
