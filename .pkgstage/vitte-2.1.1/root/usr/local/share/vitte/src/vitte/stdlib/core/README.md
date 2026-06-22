# core

Path: `src/vitte/stdlib/core`

## Purpose

Portable low-level building blocks: types, strings, memory helpers, panic/runtime-adjacent basics, and reusable utility routines.

## Architecture Role

Use `core` when the code should remain portable and unsurprising. It is the family you reach for before involving the filesystem, network, process table, or threading runtime.

## Main Responsibilities

- Own scalar and foundational helper surfaces.
- Expose utilities that higher-level families can safely compose.
- Stay small, explicit, and stable because most of the tree depends on it.

## Module Inventory

- `algorithms.vitl`
- `concurrency.vitl`
- `io_helpers.vitl`
- `memory.vitl`
- `panic.vitl`
- `strings.vitl`
- `types.vitl`
- `utils.vitl`

## Complete Integration Story

- A manifest validator stores names and counters with `core` types.
- A pure helper normalizes a string or integer without touching host state.
- The same helper can be reused in compiler code, stdlib code, and user code.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
