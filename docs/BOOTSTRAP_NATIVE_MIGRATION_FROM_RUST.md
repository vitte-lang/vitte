# Vitte Native Bootstrap Migration (Rust-style)

Reference model: bootstrap builder graph semantics (step contracts, strict stage semantics).

## Target Architecture

1. `toolchain/stage0/src/stage0.vit`
- Global context (paths, host/target, budgets, report sinks).

2. `toolchain/stage1/src/stage1_pipeline.vit`
- Step contract (`name`, `deps`, `run`, `artifacts`, `invariants`).

3. `toolchain/stage1/src/stage1_driver.vit`
- Deterministic DAG executor with fail-fast semantics.

4. `toolchain/stage1/src/stage1_validation.vit`
- Reproducibility, interface contract, parity, corpus diagnostics, perf budgets.

5. `toolchain/stage1/src/stage1_statistics.vit`
- JSON report writer to `target/reports/bootstrap/*.json`.

6. `toolchain/stage0/src/main.vit`
- CLI entrypoint, routes to full `hard` gate or scoped runs.

## Migration Phases

1. Phase A (coexist)
- Keep current scripts and Make targets.
- Add `bootstrap-v2-hard-gate` target invoking strict hard gate.

2. Phase B (step parity)
- Re-encode existing stages as typed `Step` objects.
- Ensure exact behavioral parity with current `bootstrap-hard-gate`.

3. Phase C (ownership switch)
- Make `bootstrap-v2-hard-gate` the release gate.
- Keep legacy flow for rollback for 1 release cycle.

## Non-negotiable Invariants

- Fail-fast on first error.
- No silent fallback in release/hard mode.
- Stage artifacts hash-identical for reproducibility checks.
- Stage interface contract snapshots are diff-blocking.
- Bootstrap parity and corpus diagnostics are release-blocking.

## CI Matrix

- macOS: `x86_64`, `arm64`
- Linux: `x86_64`, `arm64`

Same invariants and JSON report shape on all runners.

## Entry Commands

- Local hard gate: `make bootstrap-hard-gate`
- Future native hard gate: `make bootstrap-v2-hard-gate`
- Reports: `target/reports/bootstrap/hard_gate.json`
