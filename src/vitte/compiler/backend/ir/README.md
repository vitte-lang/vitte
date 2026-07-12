# Canonical backend IR

Path: `src/vitte/compiler/backend/ir`

## Purpose

`vitte/compiler/backend/ir::IrUnit` is the only IR contract accepted by
production code generation and linking. MIR lowering owns construction;
backend consumers may inspect verified units but must not rebuild them with
form literals.

## Contract

- Dialect: `vitte.backend.ir`
- Schema version: `1`
- Producer: `middle/lower/mir_to_ir.vit`
- Validation boundary: `verify_unit`
- Consumers: canonical codegen, C lowering, LLVM bindings, and linker

Every unit carries its dialect and schema version. `verify_unit` rejects a
missing or unsupported contract with `unsupported-ir-contract`. Constructors
in `ir.vit` preserve this metadata across all state transitions.

The older `src/vitte/compiler/ir/*` tree contains bootstrap compatibility
fixtures. It is not a backend IR and production modules must not import it.

## Values and aggregates

- `Call` carries the callee, canonical argument start, typed operands, and a typed destination.
- Complex return expressions are materialized before `ReturnValue`; calls and computations are never hidden inside a return.
- `Aggregate` distinguishes `struct`, `array`, `tuple`, and generic aggregate initialization.
- `Discriminant` reads the tag of a `pick` value.
- External/imported functions remain in the function table for signature checking and are also listed as external symbols.

## Notes

- Construct `IrUnit` only through the canonical constructors in `ir.vit`.
- Keep target contracts explicit (ABI, endianness, object format).
- Avoid silent backend fallback in critical phases.
- Validate runtime payload/link artifacts in tests.
