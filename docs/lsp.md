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

## IntelliSense capability contract

The bundled server exposes `vitte/intellisenseCapabilities` so clients can
inspect the supported editor surface without inferring it from individual
LSP responses. The contract declares the current Microsoft C/C++-style
IntelliSense parity surface and explicitly reports the bundled native process
DAP backend plus the external-adapter fallback.

The compiler's debug profile emits DWARF symbols and source `#line` mappings.
The native DAP implementation drives a persistent LLDB session for process
lifecycle, source retrieval, line breakpoints, source stepping, multi-frame
stacks, scoped locals, aggregate/pointer values, watch expressions, conditional
breakpoints, logpoints, and `setVariable`. The native path does not depend on
compiler-injected probes. External adapters remain available for toolchains
with different native debug facilities.
