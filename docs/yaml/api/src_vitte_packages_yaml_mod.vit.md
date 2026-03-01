# src/vitte/packages/yaml/mod.vit

## usage
- use yaml/mod.vit as *_pkg

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
form Schema {
form ParseOptions {
form FormatOptions {
form StreamState {
proc ok[T](value: T) -> YamlResult[T] {
proc err[T](code: string, message: string, context: string) -> YamlResult[T] {
proc diagnostics_message(code: string) -> string {
proc diagnostics_quickfix(code: string) -> string {
proc diagnostics_doc_url(code: string) -> string {
proc default_parse_options() -> ParseOptions {
proc default_format_options() -> FormatOptions {
proc parse_text(text: string, options: ParseOptions) -> YamlResult[Value] {
proc parse_bytes(bytes: [int], options: ParseOptions) -> YamlResult[Value] {
proc stringify(value: Value, options: FormatOptions) -> YamlResult[string] {
proc validate(value: Value, schema: Schema) -> YamlResult[bool] {
proc format_pretty(text: string, options: FormatOptions) -> YamlResult[string] {
proc format_compact(text: string, options: FormatOptions) -> YamlResult[string] {
proc query_path(value: Value, path: string) -> YamlResult[Value] {
proc patch(base: Value, patch_value: Value) -> YamlResult[Value] {
proc merge(base: Value, overlay: Value) -> YamlResult[Value] {
proc diff(before: Value, after: Value) -> YamlResult[Array] {
proc from_record(entries: [ObjectEntry]) -> YamlResult[Value] {
proc to_record(value: Value) -> YamlResult[[ObjectEntry]] {
proc parse_stream(chunk: string, state: StreamState, options: ParseOptions) -> YamlResult[StreamState] {
proc write_stream(value: Value, state: StreamState, options: FormatOptions) -> YamlResult[string] {
proc api_version() -> string {
proc doctor_status() -> string {
proc quickfix_preview(payload: string) -> string {
proc quickfix_apply(payload: string) -> string {
proc ready() -> bool {
proc package_meta() -> string {
