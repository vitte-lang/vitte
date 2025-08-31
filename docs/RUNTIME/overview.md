# Runtime

- **Loader VITBC** (mmap possible), validation CRC.
- **I/O optionnels** (features), timers via `instant` si `no_std` ciblé.
- **REPL** (feature `repl`), **CLI** (feature `cli`).

Évite les surprises : pas d’unsafe, erreurs `thiserror/anyhow` selon contexte.
