# Type checker

The canonical Vitte type checker is HIR-based.

Production callers use `run_production_typeck_hir` from `api.vit`. The API accepts a resolved `HirUnit` and returns `TypeckResult`; analysis, middle and driver pipelines must all use this entrypoint.

## Ownership

- `checker.vit` owns HIR traversal and production checking.
- `infer.vit`, `unify.vit`, `coercion.vit` and `traits.vit` own reusable typing rules.
- `errors.vit` owns structured type-checking results and diagnostics.
- `api.vit` is the only production entrypoint.

## Complete migration source

`complete/*` is an AST-era implementation retained only while its advanced capabilities are migrated into the HIR checker. It is not exported by `typeck/mod.vit` or `typeck/api.vit`, and production code must not import it.

`src/vitte/compiler/tests/typeck_complete_tests.vit` may import the `complete` module root to preserve migration coverage. A capability is considered migrated only after it has HIR representation, production checker coverage and positive and negative tests through `run_production_typeck_hir`.

The accepted decision and removal criteria are recorded in `ARCHITECTURE.md`.
