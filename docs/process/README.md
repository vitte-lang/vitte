# vitte/process

## usage
- `use vitte/process as process_pkg`
- Configure with `default_config` + immutable `with_*` helpers.
- Execute via `spawn/wait/terminate/kill/run`.

## contre-exemple
- Do not allow shell expansion by default.
- Do not pass unsafe args/env with control characters.

## security notes
- Diagnostics: `VITTE-P0001..VITTE-P0099`.
- `core` profile denies process execution by default.
- Allowlist/denylist checks enforced in validation.

## profile matrix
- `core`: deny
- `desktop`: allow (policy gated)
- `system`: allow (policy gated)
