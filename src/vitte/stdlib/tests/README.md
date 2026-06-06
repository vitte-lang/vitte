# tests

Path: `src/vitte/stdlib/tests`

## Purpose

Stdlib-focused smoke and integration validation surfaces.

## Architecture Role

This family exists to prove that the documented stdlib surfaces keep behaving as promised.

## Main Responsibilities

- Provide stable smoke checks for the stdlib tree.
- Keep proof artifacts close to the library surface they exercise.
- Document what the tests are supposed to guarantee.

## Module Inventory

- `smoke.vit`

## Complete Integration Story

- A smoke file should prove the library can still be loaded and exercised through its intended public paths.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
