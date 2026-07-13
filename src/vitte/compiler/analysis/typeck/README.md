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
