# Type-checker architecture decision

Status: final

Date: 2026-07-12

## Context

Vitte carried two type-checking implementations: the production HIR checker and an AST-oriented checker. Keeping both created two semantic authorities, two result models and a risk that commands could disagree about the same source.

## Decision

HIR is the sole canonical type-checker architecture.

The production contract is:

```text
resolved HirUnit -> run_production_typeck_hir -> TypeckResult + typed HirUnit
```

`analysis/pipeline.vit`, `middle/pipeline.vit` and `driver/compile.vit` must call that contract. No second type-checking implementation is permitted.

## Completed migration

The AST-era implementation and its dedicated test were removed after every retained capability met these requirements:

1. a lossless HIR representation;
2. implementation through the canonical type-checker modules;
3. positive and negative production tests;
4. structured diagnostics through `TypeckResult`;
5. deletion of the superseded implementation.

## Consequences

- Every compiler command observes one set of typing rules.
- Typed HIR remains the input to borrow checking and MIR lowering.
- Type-checking behavior must be added directly to typed HIR rather than wired through a parallel AST path.
- The typeck surface audit blocks reintroduction of a second production checker.
