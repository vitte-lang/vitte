# mir

Path: `src/vitte/compiler/middle/mir`

## Purpose

Core project directory.

## Generic instances

- `MirFunction.generic_params` preserves the declaration order from typed HIR.
- `MirRvalue.generic_arguments` carries concrete call arguments in that same order.
- MIR validation substitutes call arguments into parameter types before checking the call contract.
- `lower_hir_to_mir_templates` exposes the transient template MIR for focused tests.
- `monomorphize_mir` deduplicates concrete instances, substitutes signatures and bodies, and removes templates before canonical MIR validation.
- Concrete symbols use deterministic `vitte_mono_*` names; source spelling is not the generic identity.
