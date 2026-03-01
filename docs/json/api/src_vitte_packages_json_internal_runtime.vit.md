# src/vitte/packages/json/internal/runtime.vit

## usage
- use json/internal/runtime.vit as *_pkg

## contre-exemple
- avoid oversized/untrusted payload without limits

## symbols
pick JsonError {
pick JsonResult[T] {
form Number {
form Value {
form ObjectEntry {
form Object {
form Array {
form StreamState {
proc ok[T](value: T) -> JsonResult[T] {
proc err[T](code: string, message: string, context: string) -> JsonResult[T] {
proc starts_with(text: string, prefix: string) -> bool {
proc contains(text: string, needle: string) -> bool {
proc bytes_to_text(bytes: [int]) -> string {
proc validate_parse_input(text: string, max_bytes: int, max_depth: int, max_nodes: int, strict_utf8: bool, strict_escapes: bool) -> JsonResult[bool] {
proc parse_minimal(text: string) -> Value {
proc stringify_value(value: Value, pretty: bool, indent: int, canonical: bool, sort_keys: bool) -> string {
proc validate_schema(value: Value, mode: string, required_keys: [string], allow_extra: bool) -> JsonResult[bool] {
proc query_path_value(value: Value, path: string) -> JsonResult[Value] {
proc patch_value(base: Value, patch_value: Value) -> Value {
proc merge_value(base: Value, overlay: Value) -> Value {
proc diff_values(before: Value, after: Value) -> Array {
proc value_from_object(obj: Object) -> Value {
proc value_to_object_entries(value: Value) -> JsonResult[[ObjectEntry]] {
proc stream_ingest(chunk: string, state: StreamState, max_bytes: int) -> StreamState {
proc diag_message(code: string) -> string {
proc quickfix_for(code: string) -> string {
