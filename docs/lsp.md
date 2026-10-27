# Vitte LSP

Status: official Language Server Protocol surface for Vitte 0.1.0.

The LSP release gate covers these capabilities:

- references within the current document, other open documents, and indexed workspace files
- declaration, definition, implementation, and type navigation across indexed workspace files
- document highlights for symbol occurrences
- fix-it code actions backed by compiler diagnostics
- streaming diagnostics batches
- semantic token text snapshots
- workspace symbols
- real incremental document sync state
- multi-root workspaces
- large project stress fixtures
- VSCode and Neovim compatibility evidence

The VS Code workspace index is populated in the background, capped at 5,000
Vitte source files per scan and 30 seconds, and cached under `.vitte/`. Find
references searches indexed closed files in parallel with up to eight readers
and a 2.5-second default request budget. The response is capped at 20,000
locations; files outside the index, oversized files, and results beyond these
bounds are omitted to keep the language server responsive.

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
