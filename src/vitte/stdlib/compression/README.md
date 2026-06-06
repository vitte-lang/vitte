# compression

Path: `src/vitte/stdlib/compression`

## Purpose

Algorithms and interfaces for compacting data: huffman, lz, deflate, brotli, stats, and shared compression interfaces.

## Architecture Role

Use `compression` when compactness is a first-class requirement and the program must explain which algorithmic boundary owns that transformation.

## Main Responsibilities

- Keep compression algorithms explicit and isolated from business code.
- Document algorithm families and shared interfaces in one place.
- Clarify tradeoffs between compactness, speed, and complexity.

## Module Inventory

- `algorithms.vitl`
- `brotli.vitl`
- `deflate.vitl`
- `huffman.vitl`
- `interface.vitl`
- `lz.vitl`
- `stats.vitl`
- `tests/smoke.vitl`

## Complete Integration Story

- A report archive can be built in memory, then compressed before emission.
- A transport layer can separate serialization from compression instead of mixing both in one procedure.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
