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
| Syntax highlighting, snippets, and language-server completion | Tested compiler diagnostics, navigation, rename, and safe code actions | Build and run; tests only in the compiler repository | Optional experimental inline suggestions, off by default |

```text
Open a .vit file  →  Detect the toolchain  →  Edit with live feedback
                                  ↓
                         Build · Run · Test
```

## Quick start

Open the project folder and a `.vit` / `.vitl` file. The Vitte status item shows server readiness, the active compiler version and project diagnostics. Select it, or use **Vitte: Status and Project Health** in the Command Palette, to choose a compiler, inspect the project or set a build target. Then run **Vitte: Check**, **Vitte: Build** or **Vitte: Run**. Project roots are detected from `vitte.toml` or `.vitte/config`; see the [getting-started guide](https://github.com/vitte-lang/vitte/blob/main/vscode/VitteLangVsCode-main/docs/getting-started.md) for toolchain precedence and supported commands.

The extension adds no default keybindings and shows no startup command popup unless `vitte.notifications.showStartupCommands` is enabled. Status and project Quick Picks use native VS Code controls; the status item, project/toolchain selectors and diagnostic views can be set to French with `vitte.lang: "fr"`.

## Language support

- `.vit` — Vitte source
- `.vitl` — library and standard-library source
- `.vitte` — legacy compatibility
- TextMate highlighting, parser-checked snippets generated from the shared catalog, context-aware completion and signature help, source-based hover/navigation, compiler diagnostics and semantic tokens; document formatting is unavailable until a canonical formatter exists
- Vitte `//` and `/* */` comments, bracket matching, safe auto-closing/surrounding pairs, and native indentation for canonical brace blocks; no source rewrites run while typing
- Document outline and breadcrumbs, local/imported navigation, and symbol-aware rename with a multi-file preview, alias/shadowing checks and conservative refusal of incomplete workspace scans
- Bounded workspace symbol search is enabled; inlay hints and call/type hierarchies remain experimental and are not advertised because the compiler cannot provide verified relationships
- Experimental native DAP for Vitte sources and DWARF-built executables, available by explicit opt-in on Linux x86_64 / Ubuntu 24.04; external DAP stdio/TCP remains configurable

VS Code snippets are generated from the shared `editors/snippets/catalog.json` catalog. Their default expansions are checked against the active Vitte parser; the catalog covers declarations, control flow, tests, imports and module scaffolding.

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

The experimental native debugger is opt-in through `vitte.debug.nativeBackend`.
It uses the selected Vitte CLI with `--debug-info` and `#line` mappings, then
LLDB/DAP for source breakpoints, conditional breakpoints, stepping, continue,
pause, termination, call stacks, DWARF locals and watch expressions. It can
launch a Vitte source or an executable built with Vitte DWARF data. The current
platform candidate is compiler 0.1.1 on Linux x86_64 / Ubuntu 24.04 and requires
LLDB and Expect. The checked-in CI job still pins an older extension revision,
so it does not certify this pending 2.3.10 source snapshot; native debugging
remains experimental and unadvertised by default. When the compiler preserves
DWARF, its line directives map generated C back to Vitte source; structured
Vitte runtime diagnostics and generated multi-module source maps are not yet
available. Unsupported platforms or targets explain the limit and retain
Vitte: Build / Vitte: Run as the selected-CLI fallback. The selected folder's
compiler, project root, environment, module roots, manifest and compiler
arguments are preserved; the debugger never switches frontend silently.

The 2.3 release synchronizes editor support with the current parser grammar, including canonical `return`, procedure contracts (`requires` / `ensures`), `pick` variants, `match` arms, and external procedures. Shared lexical scanning keeps completion, navigation, indexing, and semantic highlighting consistent inside nested comments and strings. Version 2.3.2 adds source-scoped completions, actual filesystem module/import candidates, public export filtering, supported trait-method completion, constructor snippets, and signature help for local/imported procedures, supported methods and callable values. Version 2.3.3 adds source-derived hover details, scoped local and imported navigation, type and implementation navigation, bounded workspace symbols/references, and nested outlines for breadcrumbs. Version 2.3.4 adds conservative symbol-aware rename and checked file/module updates. Version 2.3.5 adds semantic token deltas, safe literal-type and unique-signature inlay hints, source-indexed reference lenses, lexical folding, nested selection ranges and unique project-local import links. Version 2.3.6 checks saved and unsaved buffers through structured compiler diagnostics, removes duplicate heuristic syntax lint and unsafe source rewrites, and disables document/range formatting until a canonical compatible formatter exists. Version 2.3.7 generates packaged snippets from the shared catalog, validates their expansions with the active parser, configures canonical comments/brackets/indentation, and keeps on-type source rewrites disabled. Version 2.3.8 detects CLI project files, isolates multi-root toolchains and module roots, adds check/build/run tasks and a compiler problem matcher, and explains unavailable standalone tests. Version 2.3.9 generates extension-owned task templates, aligns native debug compilation with folder-scoped CLI settings, and limits native launch templates to the evidenced platform. Version 2.3.10 adds VSIX contents and lifecycle checks across supported editor platforms, hardens Expect path quoting, and reduces remote suggestion context. Compiler 0.1.1 exposes no test metadata, so Test Explorer discovery and source-level test actions remain unavailable. These editor providers remain explicitly partial because compiler 0.1.1 has no editor semantic query API; import organization, return/alias hints and call/type hierarchies stay unavailable.

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

Native DAP is an explicit experimental feature. The extension offers its
current-file launch template only on Linux x86_64 / Ubuntu 24.04 with the
verified compiler version; it requires `lldb` and `expect`. On other platforms,
configure `vitte.debug.program` or `adapter` for an external DAP adapter. To
connect to an already running adapter, use **Vitte: Debug: Attach** or set
`request` to `attach` with a `host` and `port`; attach always uses TCP.

The extension uses its bundled TypeScript language server by default. Toolchain, compiler and language-server paths can be set explicitly when auto-detection is not suitable. `vitte.fmt.path` only selects the optional playground formatter and does not enable document formatting.

### Activation, trust, and installation

The extension activates for `.vit`, `.vitl`, and legacy `.vitte` documents, or when a workspace root contains `vitte.toml`, `.vitte/config`, `vitte.config.json`, or `.vitte/config.json`. Extension registration is lightweight; the language server starts only after a Vitte document or project is detected. In Restricted Mode, language services, compiler checks, build/run, debugging, project generation, Doctor, benchmarks, and project shell tools remain paused until the workspace is trusted. Document, range, and on-type formatting remain unavailable until a canonical compatible formatter is provided; VS Code's predictable block indentation remains configured.

Project detection follows the native CLI: starting at the active Vitte file (or selected workspace folder when no Vitte file is active), it walks upward and selects the nearest `vitte.toml` or `.vitte/config`. `vitte.project.root` overrides that result for its workspace folder; if there is no matching project file, the workspace folder is used. `Vitte: Detect Project` reports the selected folder, root, config file and rule in the **Vitte Project** Output channel. The extension can validate `vitte.config.json` and `.vitte/config.json` against its JSON schema and use them to activate support; compiler 0.1.1 does not load them as CLI build configuration.

The VSIX includes the compiled extension, TypeScript language server, production runtime dependencies, TextMate grammar, snippets, schemas, and icon. It does not include the Vitte compiler. Install from the Marketplace or use **Extensions: Install from VSIX...** with a locally available `.vsix`; after installation the bundled editor features work without a repository checkout or network access. Compiler-backed commands need a compatible local toolchain. Updating from the Marketplace requires network access; an offline update can be installed from a newer local VSIX.

Release checks validate the VSIX version, MIT license, changelog, production dependencies and contributed runtime resources. The platform matrix builds and installs the exact package in an isolated profile on Linux x64, macOS arm64 and Windows x64, then verifies activation and that install, update, rollback and uninstall preserve a workspace file, a user-profile setting and another installed extension. These rows remain partial until CI reports a green run. VS Code 1.105.0 is tested separately on Linux; the packaged activation checks do not claim compiler or native-debugger support on every editor platform. To run the package checks without downloading VS Code, set `VSCODE_EXECUTABLE_PATH` to an existing VS Code executable; CI uses the downloaded stable build and a minimum-engine build.

Toolchain discovery uses this precedence:

1. `vitte.compiler.path` for the compiler executable.
2. `vitte.toolchain.root` (or its legacy alias), looking under `bin/`.
3. Project-local bins such as `build/bin`, `.vitte/bin`, `vitte/bin`, `.bin`, or `tools/vitte/bin`.
4. `VITTE_BIN_DIR`, then `VITTE_TOOLCHAIN_ROOT`, `VITTE_ROOT`, or `VITTE_HOME`.
5. User-installed binaries found on `PATH` and standard OS install locations.

`vitte.serverPath` selects a custom JavaScript server module, then `vitte.lsp.path` selects an external executable with `vitte.lsp.args`; otherwise the packaged server runs. The selected language server must report a compatible `2.3.x` LSP version. Compiler-backed editor diagnostics, build/run tasks, and native debugging are verified against Vitte `0.1.1`; a missing or other compiler version is explained in the Vitte Language Server output with **Select Compiler** and settings actions. The extension does not silently substitute results from another compiler version.

Most `vitte.*` settings use resource scope, so they can be set at user, workspace, or workspace-folder level. Each workspace folder keeps its own compiler, project root, target, and module roots; compiler and module paths are resolved relative to that folder or its detected project root. Telemetry and cloud consent are application-scoped. Settings have manifest schemas for their type and accepted values. Use `vitte.trace.server` for LSP message tracing. Logs are available in **View: Output** under **Vitte Language Server**, **Vitte Project**, **Vitte Build**, and **Vitte Bench**. After changing toolchain or project-root settings, the extension restarts services automatically; **Vitte: Restart Language Server** is also available from the Command Palette.

LSP work is version-checked against unsaved buffers and workspace/configuration changes. Workspace symbol and reference scans stop at 256 files, 16 MiB, or 450 ms and report unavailable when that bound prevents a complete result. The server keeps at most 4,096 indexed documents and 64 queued LSP requests; style diagnostics are debounced and processed with event-loop yields. `vitte.trace.server` accepts `off`, `messages`, or `verbose`; source text is not logged by default. See the versioned capability matrix for the remaining platform and automated race-coverage limits.

**Vitte: Check**, **Vitte: Build**, **Vitte: Run**, and generated `vitte` tasks use the active file and folder-scoped settings. Generated task templates call the extension task provider, so compiler selection, project root, `vitte.build.env`, module roots, target, manifest and compiler arguments stay aligned with command-palette actions. In a compiler checkout, **Vitte Build** still compiles the active source through the selected CLI; the separate **Vitte Build Compiler Repository** task explicitly invokes Make. Make tasks report an error for CLI-only target/manifest/argument settings or a conflicting project config rather than ignoring them. Set `currentFileOnly` on a task to run the selected source through the CLI. The native CLI reads project configuration from the task working directory; the extension does not pass unsupported `--project-root`. The LSP does not infer generated-source or package mappings from manifests until `CAP-PROJECT-001` adds a compiler project query. `programArgs` are forwarded to `run` after `--`. **Test** is offered only when the compiler repository has a Make `test` target. The standalone CLI has no test metadata or structured result contract, so Vitte test discovery and Test Explorer run/debug actions are unavailable. Task output appears in the integrated terminal, Vitte command output records command and exit status, and the `$vitte` problem matcher sends terminal diagnostics to Problems.

## A compact command center

| Command | Use it to… |
| --- | --- |
| **Vitte: Quick** | Open the curated action menu and setup flow |
| **Vitte: Detect Project** | Explain the active folder's project root and CLI configuration file |
| **Vitte: Detect Toolchain / Select Compiler** | Inspect or set the compiler for the active workspace folder |
| **Vitte: Check** | Check the active Vitte file with the folder's compiler |
| **Vitte: Build / Run** | Build or run the active source with the folder's selected CLI |
| **Vitte: Test (compiler repository only)** | Run the compiler repository’s `make test` target; standalone `vitte test` is unavailable in compiler 0.1.1 |

The task picker also offers **Vitte Build Compiler Repository** when the active workspace is the compiler checkout and its Make `all` target exists.
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
| `vitte.project.root` | Project-root override for local binary discovery and CLI tasks |
| `vitte.project.manifestPath` | Optional CLI manifest selected for check/build/run tasks |
| `vitte.modulePaths` | Additional module roots for editor resolution and CLI commands; relative paths start at the detected project root and the setting is scoped to workspace/folder |
| `vitte.lsp.path` | Optional external language-server binary |
| `vitte.lsp.args` | Arguments for the external language-server executable |
| `vitte.serverPath` | Optional custom JavaScript language-server module |
| `vitte.trace.server` | LSP message tracing, shown in the Vitte Language Server Output channel |
| `vitte.features.signatureHelp` | Enable or disable procedure signature help |
| `vitte.debug.nativeBackend` | Explicitly opt into experimental native DAP (verified only on Ubuntu 24.04 x86_64) |
| `vitte.debug.program` | Optional external executable that implements DAP |
| `vitte.debug.adapterArgs` | Startup arguments for the DAP executable, separate from debuggee arguments |
| `vitte.debug.adapterTransport` | Default adapter transport for launch: `stdio` or an existing TCP server |
| `vitte.fmt.path` | Optional playground-only formatter using its stdin protocol; does not format editor documents |
| `vitte.lint.*` | Lightweight style lint settings |
| `vitte.build.args` | Extra compiler arguments for check/build/run CLI tasks |
| `vitte.build.env` | Environment overrides shared by CLI tasks and native debug, including `${workspaceFolder}` and `${env:NAME}` substitutions |
| `vitte.build.target` | Target passed as `--target`; cross-target compilation remains unsupported by the active backend |
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

- Extension: `VitteStudio.vitte-studio` · version `2.3.10`
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
npm run vsix:verify-artifact
VSCODE_EXECUTABLE_PATH=/path/to/code npm run vsix:smoke
```

`test:unit` builds the client and server, then checks lexical boundaries, grammar
vocabulary, TextMate tokenization, completion, hover, indexing,
signatures, semantic tokens, the capability contract, and a real stdio LSP
session without starting a graphical editor. It also checks argv/environment
preservation, managed-child shutdown and Tcl argument quoting.
`vsix:verify-artifact` checks metadata and required files inside the archive.
`vsix:smoke` installs the archive into a clean profile outside the checkout,
tests activation, simulates update and rollback, uninstalls it, and checks that
the workspace file, user-profile setting and other extension sentinel remain intact. It downloads VS Code unless
`VSCODE_EXECUTABLE_PATH` points to an existing executable; set that variable in
the shell before running the command for offline validation. The VS Code test
download honors `npm_config_proxy` and `npm_config_https_proxy`; the installed
extension downloads no language server or helper binary.
`npm test` additionally runs the VS Code integration suite, which needs a display
(on Linux CI, use `xvfb-run -a npm test`). Import paths are regenerated from this
repository's modules during the build; standalone checkouts retain the checked-in
catalog. Server output copying uses Node.js and works on Windows, macOS, and Linux.
