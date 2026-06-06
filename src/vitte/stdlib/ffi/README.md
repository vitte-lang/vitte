# ffi

Path: `src/vitte/stdlib/ffi`

## Purpose

ABI and foreign-function boundaries used when Vitte code must cross language or runtime edges.

## Architecture Role

Use `ffi` only when the program really needs a foreign boundary. This family should make coupling visible instead of hiding it.

## Main Responsibilities

- Document ABI-facing assumptions clearly.
- Keep unsafe or foreign boundaries distinct from pure library logic.
- Support interop without blurring ownership lines.

## Module Inventory

- `abi.vitl`
- `ffi.vitl`

## Complete Integration Story

- A system integration module can expose a narrow ABI-facing wrapper while the rest of the program stays pure.
- A runtime boundary should say exactly when the library stops and the foreign surface begins.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
