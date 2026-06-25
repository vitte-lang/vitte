# typeck

Path: `src/vitte/compiler/analysis/typeck`

## Purpose

Type checking surfaces for the compiler analysis pipeline.

## Current Split

The repository currently carries two distinct type checking surfaces:

- `checker.vit`
  - HIR-oriented type checker used by the real compiler pipeline today
  - entrypoint: `run_typeck_hir`
  - consumed by `analysis/pipeline.vit`, `middle/pipeline.vit`, and
    `driver/compile.vit`
- `complete/api.vit`
  - richer AST-oriented surface for forms, picks, generics, procs, traits, and
    impls
  - entrypoint: `run_complete_typeck_frontend`
  - covered by dedicated tests, but not yet wired into the production pipeline

This split is intentional during convergence, but it is also a maintenance
risk. The contract must stay explicit:

- do not silently replace the production HIR checker with `complete/*`
- do not let `complete/*` drift without dedicated coverage
- keep the bridge point explicit when the real pipeline eventually migrates

## Current Production Entry

The real compiler path still uses:

- `src/vitte/compiler/analysis/pipeline.vit`
- `src/vitte/compiler/middle/pipeline.vit`
- `src/vitte/compiler/driver/compile.vit`

All three are expected to route through `run_typeck_hir`.

## Advanced Surface

The advanced surface currently lives under:

- `src/vitte/compiler/analysis/typeck/complete/api.vit`
- `src/vitte/compiler/analysis/typeck/complete/context.vit`
- `src/vitte/compiler/analysis/typeck/complete/exprs.vit`
- `src/vitte/compiler/analysis/typeck/complete/forms.vit`
- `src/vitte/compiler/analysis/typeck/complete/procs.vit`
- `src/vitte/compiler/analysis/typeck/complete/traits_impls.vit`

Its dedicated contract tests live in:

- `src/vitte/compiler/tests/typeck_complete_tests.vit`

## Notes

- Keep phase boundaries explicit and testable.
- Emit structured diagnostics with stable codes.
- Ensure fatal conditions stop pipeline immediately.
- Keep the HIR checker and the advanced AST checker from drifting silently.
