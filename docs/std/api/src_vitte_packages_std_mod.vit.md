# src/vitte/packages/std/mod.vit

## usage
- import with: use std/mod.vit as *_pkg

## contre-exemple
- avoid runtime side effects in module scope

## symbols
form StdEntry {
form StdIndex {
pick StdError {
pick StdResult[T] {
proc ok[T](value: T) -> StdResult[T] {
proc err[T](code: string, message: string) -> StdResult[T] {
proc assert_or_err(condition: bool, code: string, message: string) -> StdResult[bool] {
proc panic_guard[T](r: StdResult[T], fallback: T) -> T {
proc checked_cast_int(value: int) -> StdResult[int] {
proc std_index() -> StdIndex {
proc register(i: StdIndex, name: string, group: string, path: string) -> StdIndex {
proc find_group(i: StdIndex, group: string) -> [string] {
proc resolve_path(i: StdIndex, name: string) -> string {
proc size(i: StdIndex) -> int {
proc std_debug_enabled() -> bool {
proc std_strict_enabled() -> bool {
proc std_compat_enabled() -> bool {
proc export_allowed(profile: string, module: string) -> bool {
proc diagnostics_message(code: string) -> string {
proc diagnostics_quickfix(code: string) -> string {
proc diagnostics_doc_url(code: string) -> string {
proc api_version() -> string {
proc doctor_status() -> string {
proc quickfix_preview(payload: string) -> string {
proc quickfix_apply(payload: string) -> string {
proc ready() -> bool {
proc package_meta() -> string {
