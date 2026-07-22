# Compatibility Matrix (Vitte Version Gate)

This matrix is the compatibility source for `make version-compatibility-gate`.

## Version Axes

- Language version: `0.2`
- Language semver: `0.2.0`
- Stdlib version: `2.0`
- Stdlib semver: `2.0.0`
- MIR version: `1.0`
- Diagnostics version: `1.1`
- Sysroot version: `2026.1`
- Package manifest version: `1.0`
- Minimum supported manifest version: `1.0`
- Supported editions: `2025`, `2026`

## Gate Policy

Build fails when one of these conditions happens:

- stdlib incompatible with requested language/version tuple
- sysroot incompatible with compiler declared sysroot version
- old cache not invalidated after MIR or diagnostics schema bump
- manifest version newer than supported parser contract
- diagnostics changes shipped without migration policy update
- language or stdlib semver omitted from the version contract

## Migration Policy Hooks

Any diagnostics schema or message contract bump must include:

- migration notes in `CHANGELOG.md`
- updated compatibility matrix entry
- migration tool behavior verification via `make migration-system-check`
