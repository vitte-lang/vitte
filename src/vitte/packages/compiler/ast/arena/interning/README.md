# interning

Path: `src/vitte/packages/compiler/ast/arena/interning`

## Purpose

Package-local AST interning contract used by package import smoke tests.

The current bootstrap surface is intentionally small and deterministic:

- create an `AstInternPool`
- intern one or two identifier strings
- reuse a previous handle when the same identifier is interned again
- read strings back through stable handles
- validate pool shape and generation metadata

## Notes

- Keep this directory focused on one responsibility.
- Add reproducible commands and examples.
- Document invariants and contracts near code.
