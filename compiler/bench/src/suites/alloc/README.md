

# Suite: alloc

This suite benchmarks allocation-heavy micro workloads for the Vitte benchmark harness.

It contains two families of cases:

- **Arena-backed** cases using `bench_alloc()` (fast bump/arena allocator managed by the harness).
- **libc** cases using `malloc/free` to provide a baseline for the platform allocator.

The suite is designed to stress:

- small fixed-size allocation throughput
- mixed-size allocation behavior
- alignment handling
- pointer chasing (cache & TLB behavior)
- malloc/free churn patterns and reuse

---

## Cases

### Arena-backed

#### `arena_bump_16b_64k`

Allocates `64k` blocks of `16` bytes (~1 MiB) from the bench arena and touches memory.

Primary signals:

- bump allocator throughput
- arena bookkeeping overhead

#### `arena_bump_mixed_small`

Allocates `20k` blocks with sizes selected from:

`8, 16, 24, 32, 40, 48, 64, 96, 128, 192, 256`

Primary signals:

- mixed size allocation cost
- write-combine and cacheline interaction due to touches

#### `arena_bump_aligned_64`

Allocates `8k` blocks of `96` bytes aligned to `64` bytes.

Primary signals:

- alignment overhead
- cacheline alignment validation (alignment mask should be 0)

#### `arena_pointer_chase`

Builds a randomized linked list of `20k` nodes and performs pointer chasing for `n*4` steps.

Primary signals:

- pointer chasing cost (cache + branch prediction)
- layout randomness sensitivity

#### `arena_bulk_memcpy_4k`

Allocates `2048` blocks of `4096` bytes and performs `memcpy` chaining between blocks.

Primary signals:

- memcpy throughput
- cache eviction behavior

---

### libc malloc/free

#### `malloc_free_16b_64k`

Allocates `64k` blocks of `16` bytes via `malloc` and frees in-order.

Primary signals:

- small allocation throughput
- allocator fast-path behavior (tcache/fastbins where applicable)

#### `malloc_free_mixed_shuffle`

Allocates `20k` blocks with sizes selected from:

`8, 16, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024`

Then frees them in randomized order.

Primary signals:

- fragmentation sensitivity
- free list management and coalescing overhead

#### `malloc_reuse_pool_4k`

Allocates a pool of `4096` blocks of `4096` bytes.

Phases:

1. Allocate all
2. Free evens
3. Re-allocate evens
4. Checksum all
5. Free all

Primary signals:

- reuse behavior under churn
- page allocator interaction

---

## Notes

- Arena cases rely on harness lifecycle: memory is released between iterations via `bench_free_all()`.
- libc cases are intentionally heavier and may be affected by system allocator tuning.
- When comparing machines, prefer pinning CPU frequency governors and isolating cores.

---

## Extending

When adding new cases:

- Keep names stable (`snake_case`) and group by allocator family.
- Always touch memory to avoid dead-code elimination.
- Prefer deterministic pseudo-random patterns (seeded from harness RNG).
- Register the case in `bench_suite_alloc_init()`.