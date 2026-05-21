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

## 2026-05-20 Strict Gate Update

Latest strict alignment pass added concrete enforcement around the driver path:

- `make compiler-entry-lock`
  - enforces `COMPILER_SOURCE_ROOT=src/vitte/compiler`
  - enforces `COMPILER_ENTRY_POINT=src/vitte/compiler/driver/compiler.vit`
- `make compiler-reachability-audit`
  - checks transitive module reachability from `driver/compiler`
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
- `toolchain/scripts/bootstrap/stage2.sh` now supports explicit backend mode
  selection through `VITTE_BACKEND_MODE=shell|native` with controlled fallback.
