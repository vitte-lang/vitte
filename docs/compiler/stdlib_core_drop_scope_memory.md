# core.drop, core.scope, and core.memory

These modules define destruction, scope-exit, and low-level memory contracts for
Vitte core code.

## core.drop

`core.drop` defines the destruction protocol with deterministic order.

- Local values are destroyed in reverse declaration order.
- Fields are destroyed in declaration order.
- Array elements are destroyed by ascending index.
- Partial destruction records the remaining live fields.
- Early exits still run required destruction.
- Destructor errors are reported through `DropResult`.
- Double destruction is forbidden and diagnosed with `double_drop_diagnostic`.

`drop(value, protocol)` runs the registered destructor when `needs_drop` is
true. `forget(value)` intentionally suppresses destruction and transfers
responsibility to the caller.

## core.scope

`core.scope` defines scope guards and `defer`.

`defer(name, action)` registers an action for scope exit. It must run for normal
exit, early return, break, continue, error propagation, and panic unwinding.

During panic, defer actions run in reverse registration order. If a defer action
panics while another panic is already unwinding, the process aborts.

## core.memory

`core.memory` exposes layout queries and value movement helpers:

- `size_of`
- `size_of_value`
- `align_of`
- `align_of_value`
- `offset_of`
- `swap`
- `replace`
- `take`
- `forget`
- `drop`

Unsafe operations are intentionally named with an `unsafe_` prefix:

- `unsafe_transmute`
- `unsafe_zeroed`
- `unsafe_uninitialized`

## Unsafe Invariants

`unsafe_transmute` requires identical size, compatible alignment, a valid target
bit pattern, and preserved ownership rules.

`unsafe_zeroed` requires that the all-zero bit pattern is valid for the target
type.

`unsafe_uninitialized` requires full initialization before any read, move,
borrow, or drop.

The module checks incompatible sizes with `detect_size_compatible` and
incompatible alignments with `detect_alignment_compatible`.
