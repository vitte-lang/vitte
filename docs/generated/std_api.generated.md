# Generated Std API (v1)

Source modules:
- vitte/std/base
- vitte/std/io
- vitte/std/net
- vitte/std/data
- vitte/std/async

## Highlights
- std/base: normalize_config, validate_or_err, profile_apply, diag
- std/io: read/write text+bytes, append, exists/remove/mkdirs, copy/move, list_dir, watch, atomic_write, path_* helpers
- std/net: request + get/post/put/delete, retry_policy, timeout_profile, parse_url/build_url, status helpers, headers_normalize
- std/data: json/toml/yaml facades, schema_validate, deep_merge, pick/set/remove, ttl cache, hash_sha256/hash_xx
- std/async: spawn/join/sleep/timeout, channel send/recv/close, select, debounce/throttle, task_cancel

## Diagnostics harmonized
- VITTE-S0001..0003
- VITTE-STDBASE0001..0003
- VITTE-STDIO0001..0003
- VITTE-STDNET0001..0003
- VITTE-STDDATA0001..0003
- VITTE-STDASYNC0001..0003

Generated from current signature surface (manual snapshot in repo).
