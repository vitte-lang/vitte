# Diagnostics Migration Plan (`dignostics` -> `diagnostics`)

## Goal
Use canonical `diagnostics` naming everywhere while preserving temporary compatibility during bootstrap.

## Phases
1. Freeze typo spread
- Enforce `make diagnostics-migration-gate`.
- Only files listed in `tools/dignostics_allowlist.txt` may still contain `dignostics`.

2. Introduce canonical modules
- Add canonical spaces/paths under `src/vitte/compiler/diagnostics`.
- Keep temporary forwarding layers from legacy modules.

3. Switch imports progressively
- Replace `use vitte/compiler/dignostics/...` with `use vitte/compiler/diagnostics/...` by subsystem.
- Keep snapshots updated after each subsystem move.

4. Remove compatibility
- Empty allowlist.
- Delete `src/vitte/compiler/diagnostics` compatibility shims that keep typo-era aliases.
- Keep gate strict so typo cannot reappear.

## Exit Criteria
- Zero `dignostics` references in `src`, `toolchain`, `tools`, `Makefile`.
- `make selfhost-hard-strict` green without migration exceptions.
