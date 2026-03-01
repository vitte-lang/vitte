# src/vitte/packages/fs/mod.vit

## usage
- import with: use fs/mod.vit as *_pkg

## contre-exemple
- avoid side effects in module scope

## symbols
pick SymlinkPolicy {
pick FsProfile {
pick FsError {
pick FsResult[T] {
form PathInfo {
form FsMetadata {
form FsEntry {
form FsWatcherConfig {
form FsCacheConfig {
form FsLimits {
form FsPolicy {
form FsTelemetry {
form FsRuntime {
proc ok[T](value: T) -> FsResult[T] {
proc err[T](code: string, message: string) -> FsResult[T] {
proc assert_or_err(condition: bool, code: string, message: string) -> FsResult[bool] {
proc panic_guard[T](r: FsResult[T], fallback: T) -> T {
proc checked_cast_int(value: int) -> FsResult[int] {
proc path(raw: string) -> PathInfo {
proc default_runtime() -> FsRuntime {
proc profile_name(profile: FsProfile) -> string {
proc symlink_policy_name(policy: SymlinkPolicy) -> string {
proc diagnostics_message(code: string) -> string {
proc diagnostics_quickfix(code: string) -> string {
proc diagnostics_doc_url(code: string) -> string {
proc normalize(p: PathInfo) -> PathInfo {
proc absolute(base: PathInfo, p: PathInfo) -> PathInfo {
proc relative(base: PathInfo, target: PathInfo) -> PathInfo {
proc is_subpath(parent: PathInfo, child: PathInfo) -> bool {
proc join(base: PathInfo, leaf: string) -> PathInfo {
proc dirname(p: PathInfo) -> string {
proc basename(p: PathInfo) -> string {
proc extname(p: PathInfo) -> string {
proc path_allowed(rt: FsRuntime, p: PathInfo) -> bool {
proc op_allowed_for_profile(rt: FsRuntime, op_name: string) -> bool {
proc open_backpressure_ok(rt: FsRuntime) -> bool {
proc upsert_entry(rt: FsRuntime, path_raw: string, text: string, bytes: [int], dir_flag: bool) -> FsRuntime {
proc exists(rt: FsRuntime, p: PathInfo) -> bool {
proc is_file(rt: FsRuntime, p: PathInfo) -> bool {
proc is_dir(rt: FsRuntime, p: PathInfo) -> bool {
proc create_dir_all(rt: FsRuntime, p: PathInfo) -> FsResult[FsRuntime] {
proc remove_tree(rt: FsRuntime, p: PathInfo) -> FsResult[FsRuntime] {
proc read_text(rt: FsRuntime, p: PathInfo) -> FsResult[string] {
proc read_bytes(rt: FsRuntime, p: PathInfo) -> FsResult[[int]] {
proc write_text(rt: FsRuntime, p: PathInfo, content: string) -> FsResult[FsRuntime] {
proc write_bytes(rt: FsRuntime, p: PathInfo, data: [int]) -> FsResult[FsRuntime] {
proc append(rt: FsRuntime, p: PathInfo, suffix: string) -> FsResult[FsRuntime] {
proc rename_atomic(rt: FsRuntime, src: PathInfo, dst: PathInfo) -> FsResult[FsRuntime] {
proc write_atomic(rt: FsRuntime, p: PathInfo, content: string) -> FsResult[FsRuntime] {
proc metadata(rt: FsRuntime, p: PathInfo) -> FsResult[FsMetadata] {
proc with_permissions(rt: FsRuntime, p: PathInfo, mode: string, owner: string) -> FsResult[FsRuntime] {
proc with_policy(rt: FsRuntime, policy: FsPolicy) -> FsRuntime {
proc with_watcher(rt: FsRuntime, watcher: FsWatcherConfig) -> FsResult[FsRuntime] {
proc with_cache(rt: FsRuntime, cache: FsCacheConfig) -> FsRuntime {
proc with_limits(rt: FsRuntime, limits: FsLimits) -> FsRuntime {
proc update_telemetry(rt: FsRuntime, stat_p50: int, stat_p95: int, read_p50: int, read_p95: int, write_p50: int, write_p95: int, walk_p50: int, walk_p95: int) -> FsRuntime {
proc build_search_index(rt: FsRuntime) -> FsRuntime {
proc analyze_report_json_stub() -> string {
proc api_version() -> string {
proc doctor_status() -> string {
proc quickfix_preview(payload: string) -> string {
proc quickfix_apply(payload: string) -> string {
proc ready() -> bool {
proc package_meta() -> string {
