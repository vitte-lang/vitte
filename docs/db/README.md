# vitte/db API

Stable facade for `vitte/db` with diagnostics namespace `VITTE-D0001..D0099`.

## usage
- `use vitte/db as db_pkg`
- `connect/ping/close`
- prepared query flow: `prepare + bind_* + query_one/query_many/exec`
- tx flow: `begin/commit/rollback`

## contre-exemple
- no raw SQL concatenation for dynamic values
- no import-time side effects in package files
