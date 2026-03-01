# vitte/fs API

Stable facade for `vitte/fs` with diagnostics namespace `VITTE-F0001..F0099`.

## usage
- `use vitte/fs as fs_pkg`
- path: `join/normalize/relative/absolute/is_subpath`
- io: `read_text/read_bytes/write_text/write_bytes/append`
- safety: policy + allowlist + profile gates

## contre-exemple
- no import-time side effects in package files
- no public leakage of `vitte/fs/internal/*`
