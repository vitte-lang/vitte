# src/vitte/packages/log/mod.vit

## usage
- import with: use log/mod.vit as *_pkg

## contre-exemple
- avoid side effects in module scope

## symbols
pick LogLevel {
pick LogSink {
pick LogFormat {
pick DropPolicy {
form LogField {
form LogEvent {
pick LogError {
pick LogResult[T] {
form LogConfig {
form Logger {
proc ok[T](value: T) -> LogResult[T] {
proc err[T](code: string, message: string) -> LogResult[T] {
proc assert_or_err(condition: bool, code: string, message: string) -> LogResult[bool] {
proc panic_guard[T](r: LogResult[T], fallback: T) -> T {
proc checked_cast_int(value: int) -> LogResult[int] {
proc level_rank(level: LogLevel) -> int {
proc level_name(level: LogLevel) -> string {
proc sink_name(sink: LogSink) -> string {
proc default_config() -> LogConfig {
proc logger_new(min_level: LogLevel) -> Logger {
proc with_field(l: Logger, key: string, value: string) -> Logger {
proc with_module(l: Logger, module: string) -> Logger {
proc with_trace(l: Logger, trace_id: string, span_id: string) -> Logger {
proc with_min_level(l: Logger, min_level: LogLevel) -> Logger {
proc with_module_filter(l: Logger, module_filter: string) -> Logger {
proc with_sampling(l: Logger, rate_percent: int, burst: int) -> Logger {
proc with_rate_limit(l: Logger, per_window: int) -> Logger {
proc with_dedup_window(l: Logger, window_size: int) -> Logger {
proc with_buffering(l: Logger, async_enabled: bool, capacity: int, policy: DropPolicy) -> Logger {
proc with_perf_mode(l: Logger, enabled: bool) -> Logger {
proc with_lossless_debug(l: Logger, enabled: bool) -> Logger {
proc with_sink(l: Logger, sink: LogSink, fallback_sink: LogSink, format: LogFormat) -> LogResult[Logger] {
proc hot_reload_config(l: Logger, new_cfg: LogConfig) -> LogResult[Logger] {
proc enabled_for_emit(l: Logger, level: LogLevel, module: string) -> bool {
proc redacted_fields(fields: [LogField]) -> [LogField] {
proc merge_context_fields(base: [LogField], extra: [LogField]) -> [LogField] {
proc event(level: LogLevel, timestamp: int, message: string, module: string, fields: [LogField], trace_id: string, span_id: string) -> LogEvent {
proc event_key(e: LogEvent) -> string {
proc append_buffer(l: Logger, e: LogEvent) -> Logger {
proc emit_event(l: Logger, level: LogLevel, timestamp: int, message: string, module: string, fields: [LogField]) -> Logger {
proc trace(l: Logger, timestamp: int, message: string, module: string, fields: [LogField]) -> Logger {
proc debug(l: Logger, timestamp: int, message: string, module: string, fields: [LogField]) -> Logger {
proc info(l: Logger, timestamp: int, message: string, module: string, fields: [LogField]) -> Logger {
proc warn(l: Logger, timestamp: int, message: string, module: string, fields: [LogField]) -> Logger {
proc error(l: Logger, timestamp: int, message: string, module: string, fields: [LogField]) -> Logger {
proc fatal(l: Logger, timestamp: int, message: string, module: string, fields: [LogField]) -> Logger {
proc format_text(e: LogEvent) -> string {
proc format_json_line(e: LogEvent) -> string {
proc format_event(e: LogEvent, format: LogFormat) -> string {
proc open_sink(l: Logger) -> LogResult[Logger] {
proc write_sink(l: Logger, e: LogEvent) -> LogResult[Logger] {
proc flush(l: Logger) -> LogResult[Logger] {
proc flush_on_exit(l: Logger) -> LogResult[Logger] {
proc rotate(l: Logger, _max_size_mb: int, _retention: int, _compress: bool) -> LogResult[Logger] {
proc sink_allowed_for_profile(profile: string, sink: LogSink) -> bool {
proc diagnostics_message(code: string) -> string {
proc diagnostics_quickfix(code: string) -> string {
proc diagnostics_doc_url(code: string) -> string {
proc summary(l: Logger) -> string {
proc feature_std_debug() -> bool {
proc feature_std_strict() -> bool {
proc feature_std_compat() -> bool {
proc semantic_search_key(level: LogLevel, module: string, message: string) -> string {
proc analyze_report_json_stub() -> string {
proc api_version() -> string {
proc doctor_status() -> string {
proc quickfix_preview(payload: string) -> string {
proc quickfix_apply(payload: string) -> string {
proc ready() -> bool {
proc package_meta() -> string {
