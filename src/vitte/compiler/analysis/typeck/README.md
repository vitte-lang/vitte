# Type checker

The canonical Vitte type checker is HIR-based.

Production callers use `run_production_typeck_hir` from `api.vit`. The API accepts a resolved `HirUnit` and returns `TypeckResult`; analysis, middle and driver pipelines must all use this entrypoint.

## Ownership

- `checker.vit` owns HIR traversal and production checking.
- `infer.vit`, `unify.vit`, `coercion.vit` and `traits.vit` own reusable typing rules.
- `errors.vit` owns structured type-checking results and diagnostics.
- `api.vit` is the only production entrypoint.

## Retired architecture

The AST-era checker was removed after its remaining surface, declaration validation and procedure checks were migrated to typed HIR. There is no fallback checker or second result model. New typing behavior must be represented in HIR and tested through `run_production_typeck_hir`.

The accepted decision and completed migration are recorded in `ARCHITECTURE.md`.

## Coverage state

Typeck coverage is tracked by `src/vitte/compiler/tests/typeck_coverage_manifest.json` and checked by `tools/typeck_coverage_check.py`.

The current gate requires every declared supported Typeck surface to have:

- a manifest entry;
- a `support_status`;
- an `owner_phase`;
- an evidence test;
- a concrete assertion for that evidence.

Generated reports are written to `target/reports/typeck_coverage/`:

- `coverage.json` contains the machine-readable gate result;
- `coverage.md` contains the review table;
- `remaining.md` lists missing entries, planned work and hardening tasks.

## Test commands

- `make typeck-fixtures` validates executable Typeck fixture contracts.
- `make typeck-snapshots` validates diagnostic and summary snapshots.
- `make typeck-coverage` validates the manifest and regenerates reports.
- `make typeck-gate` runs the full Typeck gate.

`typeck-gate` is the reference local command before changing Typeck diagnostics, inference, unification, coercion or trait behavior.

## Remaining work

The supported Typeck surface is fully manifested by the current gate. Remaining work is focused on stronger evidence:

- replace synthetic fixture snapshots with compiler-emitted JSON as soon as the CLI exposes stable Typeck dumps;
- run invalid fixtures through the end-to-end compiler path and assert structured diagnostics;
- add real multi-file Typeck fixtures that resolve exported types through the module loader;
- compare diagnostics between `vitte check`, `vitte build` and LSP once all three use the stable diagnostics API;
- keep every new `supported` entry backed by a named test and an assertion.
