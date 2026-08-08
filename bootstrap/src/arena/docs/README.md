# Bootstrap Arena

The arena layer is a C17 monotonic allocator used by the bootstrap compiler.

Invariants:
- Allocations are monotonic inside linked blocks.
- Returned pointers satisfy the requested power-of-two alignment.
- Blocks are owned by `vitte_arena_t` and destroyed by `vitte_arena_destroy`.
- `vitte_arena_reset` either keeps the first block or releases all blocks according to policy.
- Checkpoint rollback releases blocks allocated after the checkpoint and restores the saved offset.
- Statistics track reserved bytes, used bytes, peak usage, allocation count, failed allocations, blocks, and resets.
- Errors use `bootstrap/src/api/error.h`; this layer does not depend on `runtime/error.h`.

The arena allocator adapter exposes a `vitte_allocator_t` view. `free` is a no-op and reallocating existing pointers is intentionally unsupported because individual arena allocations are not independently owned.
