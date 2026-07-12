# Type-checker architecture decision

Status: accepted

Date: 2026-07-12

## Context

Vitte carried two type-checking implementations: the production HIR checker and the AST-oriented `complete/*` checker. Keeping both behind the public API created two semantic authorities, two result models and a risk that commands could disagree about the same source.

## Decision

HIR is the sole canonical type-checker architecture.

The production contract is:

```text
resolved HirUnit -> run_production_typeck_hir -> TypeckResult + typed HirUnit
```

`analysis/pipeline.vit`, `middle/pipeline.vit` and `driver/compile.vit` must call that contract. No production module may call or import `typeck/complete`.

## Migration rule

`complete/*` is migration input, not a second pipeline. Its dedicated test may access the module root, but new behavior must be implemented in the HIR checker. Each migrated capability requires:

1. a lossless HIR representation;
2. implementation through the canonical type-checker modules;
3. positive and negative production tests;
4. structured diagnostics through `TypeckResult`;
5. deletion of the superseded `complete/*` behavior.

The migration is finished when `complete/*` and its dedicated test can be removed without reducing production coverage.

## Consequences

- Every compiler command observes one set of typing rules.
- Typed HIR remains the input to borrow checking and MIR lowering.
- Advanced AST-era behavior must be ported rather than wired in parallel.
- The typeck surface audit blocks reintroduction of a second production checker.
