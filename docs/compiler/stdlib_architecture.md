# Vitte Standard Library Architecture

The current standard-library source root is `modules/`. The former `src/vitte/stdlib` tree and its documented JSON manifests are not present in the current repository. Treat the architecture below as a target model until it is reconciled with the module files and current build gates.

## Target levels

1. `core`
2. `alloc`
3. `std`
4. `platform`
5. `experimental`

Lower levels should not depend on higher levels. Stable layers should not import `experimental`.

## Target responsibilities

- `core`: portable primitive types and helpers without operating-system dependencies.
- `alloc`: allocator-aware memory and collections.
- `std`: portable higher-level modules such as math, strings, encoding, and JSON.
- `platform`: host, kernel, IO, path, process, network, threading, runtime, and FFI boundaries.
- `experimental`: unstable APIs and test-only surfaces.

For the files that currently exist, browse `modules/` and its module-level documentation.
