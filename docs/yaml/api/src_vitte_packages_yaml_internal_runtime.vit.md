# src/vitte/packages/yaml/internal/runtime.vit

## usage
- use yaml/internal/runtime.vit as *_pkg

## contre-exemple
- avoid unsafe tags without allowlist

## symbols
pick YamlError {
pick YamlResult[T] {
form Number {
form Value {
form ObjectEntry {
form Object {
form Array {
form StreamState {
proc ok[T](value: T) -> YamlResult[T] {
proc err[T](code: string, message: string, context: string) -> YamlResult[T] {
proc starts_with(text: string, prefix: string) -> bool {
proc contains(text: string, needle: string) -> bool {
proc bytes_to_text(bytes: [int]) -> string {
proc tag_allowed(tag: string, allow_tags: [string]) -> bool {
proc validate_parse_input(text: string, max_bytes: int, max_depth: int, max_nodes: int, safe_load: bool, allow_tags: [string]) -> YamlResult[bool] {
proc parse_minimal(text: string, preserve_comments: bool) -> Value {
proc stringify_value(value: Value, pretty: bool, indent: int, canonical: bool, preserve_comments: bool, sort_keys: bool) -> string {
proc validate_schema(value: Value, mode: string, required_keys: [string], allow_extra: bool) -> YamlResult[bool] {
proc query_path_value(value: Value, path: string) -> YamlResult[Value] {
proc patch_value(base: Value, patch_value: Value) -> Value {
proc merge_value(base: Value, overlay: Value) -> Value {
proc diff_values(before: Value, after: Value) -> Array {
proc value_from_object(obj: Object) -> Value {
proc value_to_object_entries(value: Value) -> YamlResult[[ObjectEntry]] {
proc stream_ingest(chunk: string, state: StreamState, max_bytes: int) -> StreamState {
proc diag_message(code: string) -> string {
proc quickfix_for(code: string) -> string {
