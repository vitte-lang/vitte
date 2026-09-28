# Vitte LSP

Status: official Language Server Protocol surface for Vitte 0.1.0.

The LSP release gate covers these capabilities:

- complete references for project symbols
- fix-it code actions backed by compiler diagnostics
- streaming diagnostics batches
- semantic token text snapshots
- workspace symbols
- real incremental document sync state
- multi-root workspaces
- large project stress fixtures
- VSCode and Neovim compatibility evidence

The protocol contract is generated under `target/lsp/`. Editors must consume stable JSON shapes and tolerate disabled code actions when a diagnostic cannot be fixed automatically.
