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

## Notes

- Construct `IrUnit` only through the canonical constructors in `ir.vit`.
- Keep target contracts explicit (ABI, endianness, object format).
- Avoid silent backend fallback in critical phases.
- Validate runtime payload/link artifacts in tests.
