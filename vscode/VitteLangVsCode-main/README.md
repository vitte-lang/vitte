<p align="center">
  <img src="icons/vitte.png" alt="Vitte logo" width="112">
</p>

<h1 align="center">Vitte Studio for VS Code</h1>

<p align="center">
  Language support and a practical build, test, debug, and project workflow for Vitte.
</p>

<p align="center">
  <a href="https://marketplace.visualstudio.com/items?itemName=VitteStudio.vitte-studio"><img src="https://img.shields.io/visual-studio-marketplace/v/VitteStudio.vitte-studio?label=Marketplace&logo=visualstudiocode&color=007ACC" alt="Visual Studio Marketplace version"></a>
  <a href="https://marketplace.visualstudio.com/items?itemName=VitteStudio.vitte-studio"><img src="https://img.shields.io/visual-studio-marketplace/i/VitteStudio.vitte-studio?label=installs&color=2ea44f" alt="Marketplace installs"></a>
  <img src="https://img.shields.io/badge/VS%20Code-1.105%2B-007ACC?logo=visualstudiocode" alt="VS Code 1.105 or later">
  <img src="https://img.shields.io/badge/license-MIT-blue.svg" alt="MIT license">
</p>

<p align="center">
  <a href="https://marketplace.visualstudio.com/items?itemName=VitteStudio.vitte-studio">Install extension</a> ·
  <a href="https://vitte.netlify.app">Documentation</a> ·
  <a href="CHANGELOG.md">Changelog</a> ·
  <a href="https://github.com/vitte-lang/VitteLangVsCode/issues">Report an issue</a>
</p>

---

Write Vitte with the bundled language server and project commands close at hand. The extension supports canonical `.vit` and `.vitl` files, plus `.vitte` for legacy projects. See the [versioned capability matrix](../../docs/reference/capabilities.html) for the tested scope of each feature, compiler frontend, CLI command, and platform.

| ✍️ **Edit** | 🧭 **Understand** | ⚙️ **Build** | 🧠 **Suggestions** |
| --- | --- | --- | --- |
| Syntax highlighting, snippets, and language-server completion | Tested diagnostics, navigation, rename, formatting, and code actions | Build and run; tests only in the compiler repository | Optional experimental inline suggestions, off by default |

```text
Open a .vit file  →  Detect the toolchain  →  Edit with live feedback
                                  ↓
                         Build · Run · Test
```

## Language support

- `.vit` — Vitte source
- `.vitl` — library and standard-library source
- `.vitte` — legacy compatibility
- TextMate highlighting, snippets, context-aware completion and signature help, source-based hover/navigation, diagnostics, semantic tokens, and formatting
- Document outline and breadcrumbs, local/imported navigation, and symbol-aware rename with a multi-file preview, alias/shadowing checks and conservative refusal of incomplete workspace scans
- Bounded workspace symbol search is enabled; inlay hints and call/type hierarchies remain experimental and are not advertised because the compiler cannot provide verified relationships
- Native DAP launch for `.vit` sources, with external DAP stdio and TCP attach retained for advanced/native targets

### Capability scope

Editor capabilities use the bundled language server. Completion, hover, navigation and rename read source declarations, explicit imports and configured module roots; they do not query compiler symbol tables because Vitte 0.1.1 exposes no editor semantic API. Rename previews each affected file, validates the edits against current buffers, and refuses unresolved symbols, collisions, incomplete scans or ambiguous file-to-module mappings. Simple literal inference and explicit annotations are shown; overloads, generic substitutions and inferred receiver types remain unavailable.
Trait methods are offered only when a visible trait bound or implementation is
present in source. Direct inherent `impl` methods and type-qualified associated
functions are not offered because the active frontend does not validate them.
Signature help includes procedure declarations, callable values, supported trait
methods and named form fields. Named procedure-call arguments are not in the active
grammar. These are bounded source results, not compiler symbol or type-resolution
results. Navigation follows lexical scopes for local bindings and explicit imports for public symbols. References and workspace symbols scan at most 256 files and 16 MiB per request; larger workspaces may be incomplete. Unresolved and intrinsic-only targets produce an explicit information message or hover explanation. The server advertises only the LSP providers with automated evidence for their stated scope. Use **Vitte: Show Capabilities** for
live enablement, evidence, limitations, compiler frontend status, CLI commands,
platforms, and planned follow-up work. That view uses the bundled matrix when
the language server is offline and marks the data as a packaged baseline.
When an import root or explicitly typed receiver cannot be resolved, completion
shows a non-inserting explanation instead of inventing members.
Enterprise dead-code, documentation-coverage, and API-breakage reports are not
implemented; the plugin picker does not offer them. Follow-up: `CAP-EXT-002`.

The native debug path compiles with DWARF symbols and `#line` source mappings,
then exposes the debuggee lifecycle through a persistent LLDB-backed DAP
adapter. Breakpoints, source-level stepping, multi-frame stacks, scoped locals,
aggregate/pointer expansion, watch expressions, conditional breakpoints,
logpoints, and `setVariable` are resolved from the native debug information;
there is no compiler probe channel in this path. External adapters remain
supported through `adapter` or `vitte.debug.program`.

The 2.3 release synchronizes editor support with the current parser grammar, including `return` / `give`, procedure contracts (`requires` / `ensures`), `pick` variants, `match` arms, and external procedures. Shared lexical scanning keeps completion, navigation, indexing, and semantic highlighting consistent inside nested comments and strings. Version 2.3.2 adds source-scoped completions, actual filesystem module/import candidates, public export filtering, supported trait-method completion, constructor snippets, and signature help for local/imported procedures, supported methods and callable values. Version 2.3.3 adds source-derived hover details, scoped local and imported navigation, type and implementation navigation, bounded workspace symbols/references, and nested outlines for breadcrumbs. Version 2.3.4 adds conservative symbol-aware rename, checked multi-file previews, and unique module-path updates on file moves. Call/type hierarchy remain unavailable until the compiler exposes verified relationship data.

```vit
space hello

proc main() -> int {
    give 0;
}
```

## Get started

1. Install **Vitte Language Support** from the [Visual Studio Marketplace](https://marketplace.visualstudio.com/items?itemName=VitteStudio.vitte-studio), or install a `.vsix` with **Extensions: Install from VSIX...**.
2. Open a Vitte workspace and run **Vitte: Detect Toolchain** from the Command Palette (`Ctrl/Cmd+Shift+P`).
3. Run **Vitte: Quick** and choose **Run full setup**, or start with **Vitte: Build**.

Debugging uses the bundled native DAP backend by default. It compiles a `.vit`
source with `--debug-info --line-directives`, launches the resulting native
program, and delegates native inspection to LLDB/DWARF. The bundled native
adapter requires `lldb` and `expect` on the executable search path. Configure
`vitte.debug.program` or `adapter` to select an external DAP adapter when a
different target toolchain is required. To connect to an already running
adapter, use **Vitte: Debug: Attach** or set `request` to `attach` with a
`host` and `port`; attach always uses TCP.

The extension uses its bundled TypeScript language server by default. Toolchain, compiler, language-server, and formatter paths can be set explicitly when auto-detection is not suitable.

### Activation, trust, and installation

The extension activates for `.vit`, `.vitl`, and legacy `.vitte` documents, or when a workspace root contains `vitte.toml`, `vitte.config.json`, or `.vitte/config.json`. Extension registration is lightweight; the language server starts only after a Vitte document or project is detected. In Restricted Mode, language services, compiler checks, build/run, formatting, debugging, project generation, Doctor, benchmarks, and project shell tools remain paused until the workspace is trusted.

The VSIX includes the compiled extension, TypeScript language server, production runtime dependencies, TextMate grammar, snippets, schemas, and icon. It does not include the Vitte compiler. Install from the Marketplace or use **Extensions: Install from VSIX...** with a locally available `.vsix`; after installation the bundled editor features work without a repository checkout or network access. Compiler-backed commands need a compatible local toolchain. Updating from the Marketplace requires network access; an offline update can be installed from a newer local VSIX.

Toolchain discovery uses this precedence:

1. `vitte.compiler.path` for the compiler executable.
2. `vitte.toolchain.root` (or its legacy alias), looking under `bin/`.
3. Project-local bins such as `build/bin`, `.vitte/bin`, `vitte/bin`, `.bin`, or `tools/vitte/bin`.
4. `VITTE_BIN_DIR`, then `VITTE_TOOLCHAIN_ROOT`, `VITTE_ROOT`, or `VITTE_HOME`.
5. User-installed binaries found on `PATH` and standard OS install locations.

`vitte.serverPath` selects a custom JavaScript server module, then `vitte.lsp.path` selects an external executable with `vitte.lsp.args`; otherwise the packaged server runs. The selected language server must report a compatible `2.3.x` LSP version. Compiler-backed editor diagnostics, build/run tasks, and native debugging are verified against Vitte `0.1.1`; a missing or other compiler version is explained in the Vitte Language Server output with **Select Compiler** and settings actions. The extension does not silently substitute results from another compiler version.

Most `vitte.*` settings use resource scope, so they can be set at user, workspace, or workspace-folder level. Telemetry and cloud consent are application-scoped. Settings have manifest schemas for their type and accepted values; paths are resolved relative to the workspace when they are not absolute. Use `vitte.trace.server` for LSP message tracing. Logs are available in **View: Output** under **Vitte Language Server**, **Vitte Build**, and **Vitte Bench**. After changing toolchain or project-root settings, the extension restarts services automatically; **Vitte: Restart Language Server** is also available from the Command Palette.

## A compact command center

| Command | Use it to… |
| --- | --- |
| **Vitte: Quick** | Open the curated action menu and setup flow |
| **Vitte: Build / Run** | Compile or execute the workspace/source within the supported frontend subset |
| **Vitte: Test (compiler repository only)** | Run the compiler repository’s `make test` target; standalone `vitte test` is unavailable in compiler 0.1.1 |
| **Vitte: Test Current File (unavailable)** | Explain that the CLI has no current-file test command |
| **Vitte: Show Capabilities** | Inspect tested editor features, compiler frontends, CLI commands, platforms, and exclusions |
| **Vitte: Open Playground** | Try a small Vitte example in an isolated editor |
| **Vitte: Project Assistant / Doctor** | Create project files or diagnose setup problems |
| **Vitte: Show Server Log / Show Server Metrics** | Inspect language-server activity and performance |
| **Vitte: Restart Language Server** | Recover completion or diagnostics after a stalled session |
| **Vitte: Explain Offline / Copy Offline Report** | Understand connectivity state and collect a support report |

## Suggestions, local first

The local inline engine is experimental and disabled by default. It indexes workspace Vitte files and produces heuristic candidates that are not compiler-resolved. Standard completion comes from the language server; a completion timeout is reported and does not reuse cached or fabricated candidates.

The optional AI pipeline is off by default. Its privacy defaults are deliberately conservative:

| Setting | Default |
| --- | --- |
| Local inline suggestion engine | **Off** |
| Local-only suggestions | **On** |
| Cloud consent (`cloudOptIn`) | **Off** |
| AI pipeline and dedicated backend | **Off** |
| Trusted workspace required | **On** |
| Secret redaction | **On** |
| External training | **Off** |
| Requested data retention | **0 days** |

The suggestion diagnostics and profiling commands remain available for the experimental engines. **Vitte: Suggestions Cloud Opt-In** is required before a configured cloud backend can send requests. **Vitte: Suggestions Cloud Opt-Out (Local-Only)** restores local-only operation.

## Configure only what you need

Open **Preferences: Open Settings (UI)** and search for `vitte`.

| Setting | Purpose |
| --- | --- |
| `vitte.toolchain.root` | Toolchain installation root |
| `vitte.compiler.path` | Compiler binary used for live diagnostics, standalone build/run, and native debugging |
| `vitte.project.root` | Project-root override for local binary discovery and build/run tasks |
| `vitte.modulePaths` | Additional filesystem roots searched for module and import completions; relative paths start at the project/workspace root and the setting is scoped to workspace/folder |
| `vitte.lsp.path` | Optional external language-server binary |
| `vitte.lsp.args` | Arguments for the external language-server executable |
| `vitte.serverPath` | Optional custom JavaScript language-server module |
| `vitte.trace.server` | LSP message tracing, shown in the Vitte Language Server Output channel |
| `vitte.features.signatureHelp` | Enable or disable procedure signature help |
| `vitte.debug.nativeBackend` | Use the bundled native process DAP backend when no external adapter is configured |
| `vitte.debug.program` | Optional external executable that implements DAP |
| `vitte.debug.adapterArgs` | Startup arguments for the DAP executable, separate from debuggee arguments |
| `vitte.debug.adapterTransport` | Default adapter transport for launch: `stdio` or an existing TCP server |
| `vitte.fmt.path` | Optional external formatter |
| `vitte.format.onSave` | Format Vitte documents on save |
| `vitte.lint.*` | Lightweight style lint settings |
| `vitte.build.args` | Extra compiler arguments for standalone build/run tasks |
| `vitte.build.profile` | `dev`, `test`, `release`, or `bench` |
| `vitte.build.incremental` | Enable incremental builds when supported |
| `vitte.suggestions.localEngine.*` | Tune local indexing and suggestion behavior |
| `vitte.suggestions.aiPipeline.*` | Configure optional backend and privacy controls |

## When something feels off

1. Run **Vitte: Detect Toolchain** to check binary discovery.
2. Run **Vitte: Restart Language Server** if editor features have stopped refreshing.
3. Open **Vitte: Show Server Log** for technical details.
4. If the extension is offline, use **Vitte: Explain Offline**, then **Vitte: Copy Offline Report** when sharing diagnostics.

## Project

- Extension: `VitteStudio.vitte-studio` · version `2.3.4`
- Requires VS Code `1.105` or later
- License: [MIT](LICENSE)
- Release notes: [CHANGELOG.md](CHANGELOG.md)
- Repository: [vitte-lang/VitteLangVsCode](https://github.com/vitte-lang/VitteLangVsCode)

## Development and validation

Run these commands from `vscode/VitteLangVsCode-main` in the compiler repository,
or from the extension root in a standalone checkout:

```sh
npm ci
npm run test:unit
npm run lint
npm test
npm run vsix
```

`test:unit` builds the client and server, then checks lexical boundaries, grammar
vocabulary, TextMate tokenization, completion, hover, formatting, indexing,
signatures, semantic tokens, the capability contract, and a real stdio LSP
session without starting a graphical editor.
`npm test` additionally runs the VS Code integration suite, which needs a display
(on Linux CI, use `xvfb-run -a npm test`). Import paths are regenerated from this
repository's modules during the build; standalone checkouts retain the checked-in
catalog. Server output copying uses Node.js and works on Windows, macOS, and Linux.
