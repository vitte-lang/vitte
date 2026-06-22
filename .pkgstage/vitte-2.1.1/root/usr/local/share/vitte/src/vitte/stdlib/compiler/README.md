# compiler

Path: `src/vitte/stdlib/compiler`

## Purpose

Compiler-owned stdlib surfaces that support self-hosted compiler and driver integration.

## Architecture Role

This family is not general-purpose business code. It exists to support compiler-owned flows where the compiler and stdlib need a shared contract.

## Main Responsibilities

- Keep compiler-facing library helpers clearly scoped.
- Avoid leaking compiler-specific surfaces into the general-purpose families.
- Document any ownership boundary between compiler internals and shared helpers.

## Module Inventory

- `mod.vit`
- `backends/backend.vit`
- `backends/mod.vit`
- `driver/compiler.vit`
- `driver/mod.vit`
- `driver/options.vit`
- `driver/pipeline.vit`
- `frontend/ast.vit`
- `frontend/diagnostics.vit`
- `frontend/lexer.vit`
- `frontend/parser.vit`
- `frontend/token.vit`
- `ir/hir.vit`
- `ir/lower.vit`
- `ir/mir.vit`
- `tests/smoke.vit`

## Complete Integration Story

- A self-hosted compiler flow can reuse structured helpers without pretending they are general stdlib entry points.
- A driver surface can depend on this family while still keeping its public contract documented elsewhere.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
