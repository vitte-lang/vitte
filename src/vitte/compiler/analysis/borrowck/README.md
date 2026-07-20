# borrowck

Path: `src/vitte/compiler/analysis/borrowck`

## Purpose

Semantic analysis phases: resolve, sema, typeck, borrowck, lint.

## Notes

- Keep phase boundaries explicit and testable.
- Emit structured diagnostics with stable codes.
- Ensure fatal conditions stop pipeline immediately.

## Coverage state

Borrow checker coverage is tracked by `src/vitte/compiler/tests/borrowck_coverage_manifest.json` and checked by `tools/borrowck_coverage_check.py`.

The current gate distinguishes:

- `supported`: production analysis borrowck surfaces with a named evidence test;
- `planned`: older or transitional `middle/borrow` helper surfaces that are tracked but do not block supported coverage;
- `hir_only` and `mir_only`: reserved for future path-specific ownership checks;
- `not_supported`: explicit exclusions when a declared surface is intentionally unavailable.

Generated reports are written to `target/reports/borrowck_coverage/`:

- `coverage.json` contains the machine-readable gate result;
- `coverage.md` contains the review table;
- `remaining.md` lists missing entries and hardening tasks;
- `fixtures.md` records required diagnostic fixtures.

## Test commands

- `make borrowck-analysis-test` runs the executable borrow checker test suite.
- `make borrowck-fixtures` validates required diagnostic fixtures and snapshots are present.
- `make borrowck-snapshots` validates coverage snapshots.
- `make borrowck-coverage` validates the manifest and regenerates reports.
- `make borrowck-gate` runs the full borrow checker gate.

`borrowck-gate` is the reference local command before changing ownership, move, loan, lifetime, region or borrow diagnostics behavior.

## Remaining work

The supported analysis borrowck surface is manifested by the current gate. Remaining work is focused on stronger evidence:

- add compiler-emitted JSON snapshots for every borrowck diagnostic fixture;
- add real multi-file borrowck fixtures for imported ownership and borrowed parameters;
- add exact span assertions for move, use, borrow, write, drop and return diagnostics;
- compare HIR-only and HIR-to-MIR paths for every supported ownership rule;
- audit and either retire or clearly separate the older `middle/borrow` helper layer.

## Invariants

Ownership invariants:

- a non-Copy place is unavailable after a move until it is reinitialized;
- a Copy place can be copied without changing the source availability;
- a partial move only invalidates the moved projection and overlapping parent places;
- disjoint field projections may remain usable when their owner model proves no overlap.

Move invariants:

- move, use, return, drop and storage-dead events are recorded in source order;
- every conflict keeps the previous event and the current event for diagnostics;
- HIR-only and HIR-to-MIR paths must agree on validity and diagnostic codes for supported rules.

Loan invariants:

- any active mutable or unique loan conflicts with another active loan of the same owner;
- shared loans may coexist with shared loans;
- loan expiration is scope-based and must run before later writes in the parent scope;
- alias paths compare by root plus projection, not by raw text alone.

Lifetime invariants:

- returned aliases must not outlive their owner region;
- borrows that cross async/defer/drop boundaries require an explicit lifetime fact;
- diagnostics must explain the chronological chain from borrow or move to later invalid use.

## Middle borrow status

`src/vitte/compiler/middle/borrow` is still tracked in the manifest as `planned`. It is not the production borrow checker API. New production behavior belongs in `analysis/borrowck`; the middle helper layer should either become a clearly MIR-local helper or be retired after its useful forms are migrated.
