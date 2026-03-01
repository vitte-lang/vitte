# src/vitte/packages/test/internal/snapshot.vit

## usage
- use test/internal/snapshot.vit as *_pkg

## contre-exemple
- avoid side effects in test modules

## symbols
pick TestError {
pick TestResult[T] {
form SnapshotConfig {
proc ok[T](value: T) -> TestResult[T] {
proc err[T](code: string, message: string, context: string, retryable: bool) -> TestResult[T] {
proc fake_hash(input: string) -> string {
proc normalize_nl(text: string, enabled: bool) -> string {
proc snapshot_diff(got: string, expected: string) -> string {
proc default_snapshot_config() -> SnapshotConfig {
proc snapshot_expect(name: string, got: string, expected: string, cfg: SnapshotConfig) -> TestResult[bool] {
proc snapshot_update(name: string, got: string, cfg: SnapshotConfig) -> TestResult[string] {
