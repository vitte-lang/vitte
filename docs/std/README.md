# vitte/std API

Stable API facade for `vitte/std`.

## Invariants
- `src/vitte/packages/std/mod.vit` is facade-only.
- No import-time side effects under `src/vitte/packages/std`.
- Public diagnostics namespace: `VITTE-S0001..VITTE-S0099`.
- Import aliases must use `*_pkg`.

## Layers
- `vitte/std/base`
- `vitte/std/io`
- `vitte/std/net`
- `vitte/std/data`
- `vitte/std/async`
