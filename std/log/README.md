# std.log

Bootstrap-friendly logging utilities for Vitte.

## Design
- No global logger state: you pass a `Logger` value to logging calls.
- Millisecond timestamps are intentionally not included (no formatting layer).
- Two modes:
  - `logger_log_best_effort`: ignores sys printing errors.
  - `logger_log_checked`: returns `(Bool, LogError)` and propagates sys errors when it actually prints.

## Minimal API
- `LogLevel` enum: Debug/Info/Warn/Error/Off
- `Logger` struct: `{ min_level, enabled }`
- `logger_new`, `logger_set_level`, `logger_set_enabled`
- `logger_debug/info/warn/error` wrappers
