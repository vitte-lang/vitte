# target

Path: `src/vitte/compiler/backend/target`

## Purpose

Backend pipeline: target config, codegen, object/link integration.

## Notes

- `layout.vit` is the canonical source for primitive sizes, aggregate field
  offsets, stack alignment, and LLVM data-layout strings.
- `calling_convention.vit` owns register classes and argument/return locations
  for SysV64, Win64, and AAPCS64. Backends consume these profiles rather than
  duplicating register tables.
- Machine frames use explicit aligned storage. The SysV red zone is target
  metadata, not implicit local-variable storage.
- Keep target contracts explicit (ABI, endianness, object format).
- Avoid silent backend fallback in critical phases.
- Validate runtime payload/link artifacts in tests.
