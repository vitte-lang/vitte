# Vitte C backend

Path: `src/vitte/compiler/backend/c`

## ABI contract

The production C backend implements `vitte-c-abi-v1` (schema version `1`).
Function ABI metadata is preserved from HIR to `backend/ir::IrFunction` and is
resolved by `c/abi.vit` before C declarations or definitions are emitted.

The x86_64 Linux C11 profile uses:

- C/System V AMD64 calling convention (`sysv64`)
- 64-bit pointers
- fixed-width `<stdint.h>` integers
- `stdbool.h::bool` for Vitte `bool`
- NUL-terminated `const char*` for Vitte `string`
- direct scalar values
- `const T*` for immutable borrows
- `T*` for mutable borrows
- terminal C variadics with at least one fixed parameter

The default Vitte convention and explicit `extern C` procedures both lower to
portable C declarations. Explicit `sysv64` is accepted by the x86_64 Linux
profile. A mismatched convention such as `win64` is rejected with
`CBACKEND_E_ABI` instead of being silently rewritten.

Generated headers define `VITTE_ABI_VITTE`, `VITTE_ABI_C`,
`VITTE_ABI_SYSV64`, and `VITTE_ABI_WIN64`. Every generated function
declaration and definition carries exactly one of these macros.

## Value lowering

- `form` and `class` values use C structs with designated initializers.
- Vitte unions use C unions and initialize one active field.
- `pick` values use a signed 32-bit tag plus a payload union.
- Arrays and tuples currently share the executable `VitteAggregate` boundary: a length and a block-scoped `uintptr_t` value array.
- External procedures emit a header declaration only; no synthetic C body is generated.
- A block that already returns a value does not receive a second bare `return`.

## Ownership

- `architecture.vit`: target profile and C type mapping
- `abi.vit`: versioned ABI contract and signature validation
- `lowering.vit`: canonical MIR/IR to C model lowering
- `emitter.vit`: declarations, definitions, and ABI preamble
- `linker.vit`: C toolchain invocation plan
