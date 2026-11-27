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
- TextMate highlighting, snippets, language-server completion and signature help, hover, diagnostics, semantic tokens, and formatting
- Tested document symbols, definition, references, rename, and code actions
- Workspace symbols, inlay hints, and call/type hierarchies are experimental prototypes and are not advertised to VS Code
- Native DAP launch for `.vit` sources, with external DAP stdio and TCP attach retained for advanced/native targets

### Capability scope

Editor capabilities use the bundled language server. Source-indexed navigation,
completion, and diagnostics have documented limits; they are not compiler symbol
or type-resolution results. The server advertises only the LSP providers with
automated evidence for their stated scope. Use **Vitte: Show Capabilities** for
live enablement, evidence, limitations, compiler frontend status, CLI commands,
platforms, and planned follow-up work. That view uses the bundled matrix when
the language server is offline and marks the data as a packaged baseline.

The native debug path compiles with DWARF symbols and `#line` source mappings,
then exposes the debuggee lifecycle through a persistent LLDB-backed DAP
adapter. Breakpoints, source-level stepping, multi-frame stacks, scoped locals,
aggregate/pointer expansion, watch expressions, conditional breakpoints,
logpoints, and `setVariable` are resolved from the native debug information;
there is no compiler probe channel in this path. External adapters remain
supported through `adapter` or `vitte.debug.program`.

The 2.3 release synchronizes editor support with the current parser grammar, including `return` / `give`, procedure contracts (`requires` / `ensures`), `pick` variants, `match` arms, and external procedures. Shared lexical scanning keeps completion, navigation, indexing, and semantic highlighting consistent inside nested comments and strings.

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
| `vitte.compiler.path` | Compiler binary used for live diagnostics |
| `vitte.lsp.path` | Optional external language-server binary |
| `vitte.features.signatureHelp` | Enable or disable procedure signature help |
| `vitte.debug.nativeBackend` | Use the bundled native process DAP backend when no external adapter is configured |
| `vitte.debug.program` | Optional external executable that implements DAP |
| `vitte.debug.adapterArgs` | Startup arguments for the DAP executable, separate from debuggee arguments |
| `vitte.debug.adapterTransport` | Default adapter transport for launch: `stdio` or an existing TCP server |
| `vitte.fmt.path` | Optional external formatter |
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

- Extension: `VitteStudio.vitte-studio` · version `2.3.0`
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
