# Compiler Driver Alignment

This document tracks the Vitte-owned compiler driver surface after the host
driver migration.

## Overview

The driver remains a first-class subsystem because it defines the stable command
entrypoints that users, CI, bootstrap flows, and documentation all depend on.

| Concern | Current contract |
| --- | --- |
| Surface ownership | `src/vitte/packages/compiler/driver/*` catalogs |
| Runtime implementation | `src/vitte/compiler/driver/*` |
| Audit goals | parity, normalization, stage mapping |
| Enforcement | dedicated make targets and strict gates |

Why the driver stays explicit:

- it defines the user-visible command surface
- it isolates option parsing and command dispatch from frontend/backend internals
- it gives CI a small, deterministic place to audit command and flag parity

## Responsibilities

- Own the visible command and flag inventory.
- Normalize aliases and stage names consistently.
- Keep package metadata and runtime implementation aligned.

## Invariants

- Command catalogs and implementation must not drift silently.
- Normalization rules must stay deterministic.
- Strict gates must detect driver regressions before release-facing changes land.

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

## Data Flow

1. User input enters through the top-level driver command surface.
2. Package catalogs normalize commands, aliases, and selected option values.
3. The runtime driver maps the normalized request to compiler stages.
4. Strict audits verify that the exposed help and command behavior still match
   the owned catalogs.

## Driver Implementation

The compiler driver implementation is split between the public entry facade and
the runtime driver core:

- `src/vitte/compiler/main.vit`
- `src/vitte/compiler/driver/compile.vit`
- `src/vitte/compiler/driver/compiler.vit`
- `src/vitte/compiler/driver/mod.vit`

`src/vitte/compiler/main.vit` is the real compiler entrypoint used by bootstrap
and packaging flows. `src/vitte/compiler/driver/compiler.vit` remains the
runtime command dispatcher and stage-mapping core behind that facade.

## Audit And Bootstrap Commands

```sh
make driver-surface-audit
make driver-surface-parity
make selfhost-driver-bootstrap
```

## Pipeline

The driver sits ahead of the compiler pipeline proper. Its job is to convert CLI
surface into a precise compiler action before frontend, analysis, or backend
work begins.

## Examples

```sh
make driver-surface-audit
make driver-surface-parity
./bin/vitte --help
./bin/vitte check src/app.vit
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

## 2026-05-20 Strict Gate Update

Latest strict alignment pass added concrete enforcement around the driver path:

- `make compiler-entry-lock`
  - enforces `COMPILER_SOURCE_ROOT=src/vitte/compiler`
  - enforces `COMPILER_ENTRY_POINT=src/vitte/compiler/main.vit`
- `make compiler-reachability-audit`
  - checks transitive module reachability from `vitte/compiler/main`
  - allows temporary migration exceptions through a tracked allowlist
- `make compiler-max-gate-strict`
  - compiler-focused strict gate (frontend/analysis/middle/backend checks + strict smokes)
- `make selfhost-hard-strict`
  - end-to-end release gate chain for bootstrap + strict compiler path

The current migration objective remains unchanged:

1. reduce reachability allowlist to zero
2. complete diagnostics path migration (`dignostics` -> `diagnostics`)
3. keep strict gates green without hidden fallback behavior

## 2026-05-20 Stop-After And Repro Hardening

Additional strictness landed on the same date:

- Driver `--stop-after` is now treated as a hard runtime contract:
  - stage mapping is centralized in `src/vitte/compiler/driver/compiler.vit`
  - successful stop no longer rewrites final stage to `Done`
  - exact stop tests now cover `parse/hir/typeck/mir/ir/codegen/link/run`
- Driver diagnostic propagation now uses typed fields end-to-end in the
  driver mapping layer (`severity/code/file/line/column/message`) instead of
  fallback text-only wrappers.
- Structured stage parity check was added:
  - `tools/stage_parity_structured.sh`
  - writes machine-readable parity report and readable diff under
    `target/reports/stage_parity/`
  - executed by `make compiler-max-gate-strict`
- Bootstrap reproducibility drift gate was added:
  - `tools/bootstrap_selfhost_repro.sh`
  - compares hashes across repeated bootstrap/selfhost runs
  - exposed via `make bootstrap-selfhost-repro`
- The retired numbered-stage scripts and their controlled fallback have been
  removed. `toolchain/seed/vittec0.seed` is now the only bootstrap trust root.
