# vitte/log API

Stable facade for `vitte/log` with diagnostics namespace `VITTE-L0001..L0099`.

## usage
- `use vitte/log as log_pkg`
- create: `logger_new(LogLevel.Info)`
- enrich context: `with_field`, `with_module`, `with_trace`
- emit: `trace/debug/info/warn/error/fatal`

## contre-exemple
- no import-time side effects (`entry` forbidden in package files)
- avoid exposing `vitte/log/internal/*` through public facade
