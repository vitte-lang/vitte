# mir

Path: `src/vitte/compiler/middle/mir`

## Purpose

Core project directory.

## Generic instances

- `MirFunction.generic_params` preserves the declaration order from typed HIR.
- `MirRvalue.generic_arguments` carries concrete call arguments in that same order.
- MIR validation substitutes call arguments into parameter types before checking the call contract.
- Monomorphization consumes this structured metadata; symbol spelling is not part of the generic identity.
