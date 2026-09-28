# Vitte Stdlib Boundaries

`core` is the allocation-free layer. It may use compiler intrinsics for layout,
iteration cursors, UTF-8 validation, and primitive operations, but it must not
depend on `alloc`, `std`, `platform`, OS handles, or dynamic allocation.

`alloc` owns heap-backed containers. It may depend on `core` and allocator
intrinsics, and it is responsible for growth, capacity, drop, and ownership
rules.

`std` is the OS-facing layer. It may depend on `core`, `alloc`, and `platform`.
Filesystem, process, environment, and clock APIs must flow through `platform`
or compiler-provided platform shims.

Public names are stable once added to `stdlib_modules.json`. Renames require a
compatibility alias or an explicit migration note in API documentation.
