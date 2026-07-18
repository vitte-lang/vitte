# Toolchain Index

## Trust Root

- `seed/vittec0.seed`: sole executable bootstrap trust root.
- `seed/manifest.txt`: pinned hash, source path, and version.
- `seed/src/main.vit`: reviewed source identity.
- `bootstrap-config.json`: machine-readable one-seed contract.

## Orchestration

- `bootstrap.sh`: normal, quick, strict, dry-run, and prerequisite modes.
- `src/build.vit`: seed-rooted build-state model.
- `src/bootstrap_vitte/`: Vitte bootstrap hard-gate sources.
- `scripts/bootstrap/verify.sh`: deterministic seed output verification.
- `test_bootstrap_reproducibility.sh`: repeated seed installation audit.

## Validation

- `make seed-verify`
- `make bootstrap-source-of-truth`
- `make bootstrap-seed-root-check`
- `make bootstrap-native-snapshots`
- `make bootstrap-clean-checkout-gate`
- `make bootstrap-offline-gate`
- `make bootstrap-vitte-hard-gate`
- `make selfhost-completion-audit`

## Compiler Boundary

The canonical compiler entry is `src/vitte/compiler/main.vit`. Compiler
subsystems live below `src/vitte/compiler/{frontend,analysis,middle,ir,backend,
backends,driver,diagnostics,infrastructure}`. Bootstrap code must not introduce
another compiler entry constant or source root.

## Installation And Packaging

Install, package, CI, test, and platform helpers live under `scripts/` and
`toolchain/scripts/`. They consume the installed seed or active compiler
surfaces; they do not own bootstrap trust.
