# src/vitte/packages/json/mod.vit

## usage
- use json/mod.vit as *_pkg

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
form Schema {
form ParseOptions {
form FormatOptions {
form StreamState {
proc ok[T](value: T) -> JsonResult[T] {
proc err[T](code: string, message: string, context: string) -> JsonResult[T] {
proc diagnostics_message(code: string) -> string {
proc diagnostics_quickfix(code: string) -> string {
proc diagnostics_doc_url(code: string) -> string {
proc default_parse_options() -> ParseOptions {
proc default_format_options() -> FormatOptions {
proc parse_text(text: string, options: ParseOptions) -> JsonResult[Value] {
proc parse_bytes(bytes: [int], options: ParseOptions) -> JsonResult[Value] {
proc stringify(value: Value, options: FormatOptions) -> JsonResult[string] {
proc validate(value: Value, schema: Schema) -> JsonResult[bool] {
proc format_pretty(text: string, options: FormatOptions) -> JsonResult[string] {
proc format_compact(text: string, options: FormatOptions) -> JsonResult[string] {
proc query_path(value: Value, path: string) -> JsonResult[Value] {
proc patch(base: Value, patch_value: Value) -> JsonResult[Value] {
proc merge(base: Value, overlay: Value) -> JsonResult[Value] {
proc diff(before: Value, after: Value) -> JsonResult[Array] {
proc from_record(entries: [ObjectEntry]) -> JsonResult[Value] {
proc to_record(value: Value) -> JsonResult[[ObjectEntry]] {
proc parse_stream(chunk: string, state: StreamState, options: ParseOptions) -> JsonResult[StreamState] {
proc write_stream(value: Value, state: StreamState, options: FormatOptions) -> JsonResult[string] {
proc api_version() -> string {
proc doctor_status() -> string {
proc quickfix_preview(payload: string) -> string {
proc quickfix_apply(payload: string) -> string {
proc ready() -> bool {
proc package_meta() -> string {
