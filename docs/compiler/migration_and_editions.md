# Migration And Editions

## Migration System

Run:

- `make migration-system-check`
- or `vittec migrate project/ --from 0.1 --to 0.2`

Baseline behavior validated:

- obsolete edition markers are rewritten to current edition (`2026`)
- migration log is emitted in `build/logs/migrate_*.txt`
- before/after hashes are recorded

## Edition System

Run:

- `make edition-system-check`

Current baseline:

- edition key in manifest (`edition = "2025"|"2026"`)
- cross-edition parse/check smoke validated
- edition diagnostics and feature gates are policy-driven and tracked in release compatibility docs
