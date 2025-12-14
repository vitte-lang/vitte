# FS VFS Plugin (Vitte)

Ultra-complete skeleton layout for a Virtual File System plugin.

- `api/` public surface for users
- `core/` routing/handles/caches/locking/transactions
- `backends/` implementations (osfs, memfs, overlay, zipfs, tarfs, httpfs, tempfs)
- `host/` capabilities + sandbox policy + OS interop hooks
- `tooling/` debug / import-export utilities
- `tests/` and `benches/` stubs
