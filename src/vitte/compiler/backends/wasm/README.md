# wasm

Path: `src/vitte/compiler/backends/wasm`

## Purpose

Backend pipeline: target config, codegen, object/link integration.

## Notes

- Keep target contracts explicit (ABI, endianness, object format).
- Avoid silent backend fallback in critical phases.
- Validate runtime payload/link artifacts in tests.
