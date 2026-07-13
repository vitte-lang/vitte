# Type System Implementation

The production type checker consumes resolved HIR through `run_production_typeck_hir`. The normative rules are defined in `docs/spec/type_system.md` and indexed by `schemas/type_system/rules.json`.

Implementation ownership:

- `checker.vit`: module environments, declarations, statements, returns, impl coherence and match exhaustiveness.
- `infer.vit`: expression typing, local generic inference, substitutions, calls, members and casts.
- `unify.vit`: common-type computation for numeric, reference, pointer and branch types.
- `coercion.vit`: directional implicit conversions and explicit-cast admissibility.
- `traits.vit`: primitive type classes, truthiness, copy/drop properties and trait facts.
- `errors.vit`: structured diagnostics and mandatory cause-chain enforcement.

`tools/check_type_system_rules.py` rejects a formal rule whose implementation, production test, run-list entry or diagnostic registry contract is missing.
