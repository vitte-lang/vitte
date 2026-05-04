# Compiler Driver Alignment

This document tracks the Vitte-owned compiler driver surface after the host
driver migration.

Why the driver stays explicit:

- it defines the user-visible command surface
- it isolates option parsing and command dispatch from frontend/backend internals
- it gives CI a small, deterministic place to audit command and flag parity

## Current Surface

The Vitte package lives in:

- `src/vitte/packages/compiler/driver/mod.vit`
- `src/vitte/packages/compiler/driver/info.vit`
- `src/vitte/packages/compiler/driver/internal/command_catalog.vit`
- `src/vitte/packages/compiler/driver/internal/option_catalog.vit`
- `src/vitte/packages/compiler/driver/internal/normalize.vit`

This package now owns:

- command catalog metadata
- top-level command inventory
- grammar/mod subcommand inventory
- flag inventory parity against the Vitte driver catalogs
- normalization of help aliases
- normalization of selected command and flag aliases
- stage mapping for normalized commands

## Driver Implementation

The compiler driver implementation lives in:

- `src/vitte/compiler/driver/options.vit`
- `src/vitte/compiler/driver/compiler.vit`
- `src/vitte/compiler/driver/mod.vit`

## Audit And Bootstrap Commands

```sh
make driver-surface-audit
make driver-surface-parity
make selfhost-driver-bootstrap
```

## Next Step

Keep option value normalization aligned across the driver package and compiler
driver:

- `--template`
- `--lang`
- `--target`
- `--stage`
- `--stdlib-profile`
- `--syntax-profile`
- `--panic-budget`
- `--strict-recovery`

That keeps the command surface deterministic while the rest of the compiler
continues moving through the Vitte source tree.
