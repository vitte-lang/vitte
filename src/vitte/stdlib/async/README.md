# async

Path: `src/vitte/stdlib/async`

## Purpose

Future, channel, executor, and task orchestration helpers.

## Architecture Role

Use `async` when work should be coordinated as tasks rather than as direct thread ownership.

## Main Responsibilities

- Own task orchestration and message-passing surfaces.
- Clarify the difference between asynchronous coordination and thread control.
- Keep scheduling concepts visible in the docs.

## Module Inventory

- `async.vitl`
- `channel.vitl`
- `executor.vitl`
- `future.vitl`

## Complete Integration Story

- A pipeline can spawn tasks, exchange messages through channels, and join through the executor boundary.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
