# Vitte compiler arena

The implementation in `src/compiler/arena/*.vitte` is written in Vitte and
uses the native `vitte::core::memory` interface for raw storage. It does not
call the C arena implementation. The C sources under `src/bootstrap/arena/`
remain the bootstrap counterpart while the compiler is brought up in Vitte.

The modules provide:

- `block`: aligned physical blocks and block-level accounting.
- `allocator`: monotonic allocation, marks, rewind, reset, trimming, and
  ownership checks.
- `arena`: compiler-facing limits, string helpers, checkpoints, and
  generation statistics.
- `checkpoint`: nested transactional checkpoints over an arena context.
- `pool`: reusable fixed-size slots with capacity limits and validation.
- `reset`: coordinated arena and pool reset operations with reports.
- `stats`: snapshots, deltas, aggregate statistics, and validation.

Arena allocations are reclaimed together through rewind, reset, trimming, or
destruction. Pool slots can be individually returned and reused. Arena and
pool mutation requires external synchronization when shared between threads.
