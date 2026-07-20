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

## Coverage state

IR coverage is tracked by `src/vitte/compiler/tests/ir_coverage_manifest.json` and checked by `tools/ir_coverage_check.py`.

The current gate requires every declared supported backend IR surface to have:

- a manifest entry;
- a `support_status`;
- an `owner_phase`;
- an evidence test;
- a concrete assertion for that evidence.

Generated reports are written to `target/reports/ir_coverage/`:

- `coverage.json` contains the machine-readable gate result;
- `coverage.md` contains the review table;
- `remaining.md` lists missing entries and hardening tasks;
- `fixtures.md` records required IR fixture artifacts and source fixtures.

## Test commands

- `make ir-fixtures` validates required IR fixture artifacts.
- `make ir-snapshots` validates coverage snapshots.
- `make ir-coverage` validates the manifest and regenerates reports.
- `make ir-gate` runs the full IR gate, including the backend IR contract audit and core IR golden snapshot.

`ir-gate` is the reference local command before changing backend IR forms, MIR-to-IR lowering, IR validation, codegen IR consumers or linker IR consumers.
