# ADR 0001: Boot Smoke Contract

## Status

Accepted

## Context

We need a fast validation loop for boot-facing changes without requiring full integration every time.

## Decision

Adopt a two-level smoke contract:

- Structural smoke in `vitteos/test/kernel_smoke.vit` (boot -> serial log -> halt).
- Runtime probe via `tools/vitteos_kernel_smoke_runtime.sh` when emulator/tooling is available.

## Consequences

- CI remains portable: runtime smoke gracefully skips when no emulator is available.
- Boot regressions are detected earlier through structural invariants.
