# Athens Design

Athens keeps runtime dependencies small and prefers explicit data structures.
The module returns plain hashes and arrays so package users can inspect and
serialize plans without an object system.

All helpers are deterministic except name generation, which includes process
and counter components to reduce collisions.
