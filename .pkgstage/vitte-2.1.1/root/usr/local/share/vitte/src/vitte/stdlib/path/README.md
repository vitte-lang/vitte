# path

Path: `src/vitte/stdlib/path`

## Purpose

Path manipulation, special locations, walkers, and globbing.

## Architecture Role

Use `path` when the program needs path semantics, traversal, or normalization. Keep it distinct from file contents and from business validation.

## Main Responsibilities

- Own path normalization and traversal logic.
- Clarify special directories and matching rules.
- Keep host path semantics explicit in the docs.

## Module Inventory

- `globbing.vitl`
- `manipulation.vitl`
- `special.vitl`
- `walker.vitl`

## Complete Integration Story

- A project root can be normalized before any file is read.
- A walker can enumerate source files while parsing and diagnostics remain elsewhere.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
