# llvm_bindings

Path: `src/vitte/compiler/backends/llvm_bindings`

## Purpose

Canonical LLVM adapter for `src/vitte/compiler/backend/*`: target config,
codegen, and object/link integration.

## Notes

- This is the only supported LLVM-facing surface for the production driver path.
- `src/vitte/compiler/backends/llvm_emit.vit` is legacy/experimental and not
  wired as the canonical backend.
- Keep target contracts explicit (ABI, endianness, object format).
- Avoid silent backend fallback in critical phases.
- Validate runtime payload/link artifacts in tests.

## Canonical values

The adapter emits identified LLVM types for Vitte forms and picks, concrete
`insertvalue` aggregate construction, typed calls and returns, and `declare`
entries for external functions. Arrays and tuples lower through
`%VitteAggregate = type { i64, i8* }` with block-local value storage.
