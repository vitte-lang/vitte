# src/vitte/packages/fs/internal/runtime.vit

## usage
- import with: use fs/internal/runtime.vit as *_pkg

## contre-exemple
- avoid side effects in module scope

## symbols
proc normalize_separators(path: string) -> string {
proc collapse_double_slash(path: string) -> string {
proc redact_sensitive_path(path: string) -> string {
proc is_sensitive(path: string) -> bool {
proc sink_allowed_for_profile(profile: string, op_name: string) -> bool {
proc diag_message(code: string) -> string {
proc quickfix_for(code: string) -> string {
proc normalize_path(raw: string) -> string {
proc relative_path(base_raw: string, target_raw: string) -> string {
proc is_subpath_path(parent_raw: string, child_raw: string) -> bool {
proc walk_index_add(index: [string], path_raw: string) -> [string] {
