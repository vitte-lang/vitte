# Vitte Stdlib API Stability

Public stdlib modules listed in `src/vitte/stdlib/stdlib_modules.json` are
treated as stable contracts.

Compatibility rules:

- public names are not removed without a migration alias;
- public signatures are documented in the generated API index;
- each public module must have a test reference;
- each public module must have an example reference;
- public `panic` usage must be documented or guarded by validation.

## Module Stability Classes

- `core`: stable no_std surface; no allocation, OS, or platform dependency.
- `alloc`: stable allocation surface; memory invariants must be tested.
- `std`: stable portable surface; OS behavior routes through platform backends.
- `platform`: stable boundary surface for posix, windows, wasm, and embedded.
- `experimental`: unstable opt-in surface.

## Panic Documentation Contract

Public functions that can panic must either return a structured diagnostic/result
or be listed in this document with the cause, location, and recovery guidance.
Undocumented public panic usage fails the stdlib gate.
