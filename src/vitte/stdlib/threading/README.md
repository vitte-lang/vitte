# threading

Path: `src/vitte/stdlib/threading`

## Purpose

Thread, mutex, and pool-based concurrency helpers.

## Architecture Role

Use `threading` when the program needs explicit concurrency coordination rather than single-threaded transformation.

## Main Responsibilities

- Own thread-based coordination surfaces.
- Keep shared-state synchronization visible.
- Document how threading differs from lighter async orchestration.

## Module Inventory

- `mutex.vitl`
- `thread.vitl`
- `threadpool.vitl`

## Complete Integration Story

- A worker pool can process tasks in parallel while leaving task definition and result aggregation elsewhere.

## Documentation Rule

- Explain this family by responsibility first.
- Use complete scenarios, not only tiny snippets.
- Separate pure transformation, host interaction, and runtime boundaries.
- Keep failure paths explicit in examples and contracts.
