# Vitte Language Support (Vitte Studio)

**Vitte Studio** is the official VS Code extension for the Vitte language. It delivers a complete developer experience: syntax highlighting, rich IntelliSense, hover docs, diagnostics, navigation, formatting, semantic tokens, debugging, tasks, and project tooling for real-world Vitte codebases.

## Badges

**Marketplace**

[![Version](https://img.shields.io/visual-studio-marketplace/v/VitteStudio.vitte-studio)](https://marketplace.visualstudio.com/items?itemName=VitteStudio.vitte-studio)
[![Installs](https://img.shields.io/visual-studio-marketplace/i/VitteStudio.vitte-studio)](https://marketplace.visualstudio.com/items?itemName=VitteStudio.vitte-studio)
[![Downloads](https://img.shields.io/visual-studio-marketplace/d/VitteStudio.vitte-studio)](https://marketplace.visualstudio.com/items?itemName=VitteStudio.vitte-studio)
[![Rating](https://img.shields.io/visual-studio-marketplace/r/VitteStudio.vitte-studio)](https://marketplace.visualstudio.com/items?itemName=VitteStudio.vitte-studio)
[![Last Updated](https://img.shields.io/visual-studio-marketplace/last-updated/VitteStudio.vitte-studio)](https://marketplace.visualstudio.com/items?itemName=VitteStudio.vitte-studio)

**GitHub**

[![CI](https://img.shields.io/github/actions/workflow/status/vitte-lang/VitteLangVsCode/ci.yml?branch=main)](https://github.com/vitte-lang/VitteLangVsCode/actions)
[![Webpack](https://img.shields.io/github/actions/workflow/status/vitte-lang/VitteLangVsCode/webpack.yml?branch=main)](https://github.com/vitte-lang/VitteLangVsCode/actions)
[![Release](https://img.shields.io/github/v/release/vitte-lang/VitteLangVsCode)](https://github.com/vitte-lang/VitteLangVsCode/releases)
[![License](https://img.shields.io/github/license/vitte-lang/VitteLangVsCode)](https://github.com/vitte-lang/VitteLangVsCode/blob/main/LICENSE)
[![Stars](https://img.shields.io/github/stars/vitte-lang/VitteLangVsCode)](https://github.com/vitte-lang/VitteLangVsCode)
[![Forks](https://img.shields.io/github/forks/vitte-lang/VitteLangVsCode)](https://github.com/vitte-lang/VitteLangVsCode)
[![Issues](https://img.shields.io/github/issues/vitte-lang/VitteLangVsCode)](https://github.com/vitte-lang/VitteLangVsCode/issues)
[![PRs](https://img.shields.io/github/issues-pr/vitte-lang/VitteLangVsCode)](https://github.com/vitte-lang/VitteLangVsCode/pulls)
[![Contributors](https://img.shields.io/github/contributors/vitte-lang/VitteLangVsCode)](https://github.com/vitte-lang/VitteLangVsCode/graphs/contributors)
[![Last Commit](https://img.shields.io/github/last-commit/vitte-lang/VitteLangVsCode)](https://github.com/vitte-lang/VitteLangVsCode/commits/main)
[![Repo Size](https://img.shields.io/github/repo-size/vitte-lang/VitteLangVsCode)](https://github.com/vitte-lang/VitteLangVsCode)

## Marketplace Descriptions

**Short**

Vitte Studio is the official VS Code extension for the Vitte language — syntax, IntelliSense, LSP, diagnostics, formatting, and tooling.

**Long**

Vitte Studio brings first‑class Vitte support to VS Code. You get rich completions, hover docs, diagnostics and linting, go‑to definition and references, formatting, and semantic tokens. The extension includes a built‑in language server, workspace indexing, and project tooling commands for build, run, test, and benchmarks. If you write Vitte, this is the all‑in‑one, modern IDE experience.

## Highlights

- Full language support for `.vitte` and `.vit`
- Rich completions (keywords, types, fields, methods, modules)
- Hover documentation (docstrings)
- Diagnostics and linting
- Go-to definition, references, rename
- Workspace-wide indexing and references
- Formatting
- Semantic tokens
- Built-in language server
- Debugger integration
- Tasks and quick actions for build/run/test/bench

## Supported Languages

- `vitte` (`.vitte`)
- `vit` (`.vit`)

## Core Features

- Syntax highlighting for Vitte and Vit
- Bracket/indentation rules and language configuration
- Snippets (including docstring helpers)
- IntelliSense completions (keywords, types, symbols, imports)
- Hover with documentation and type info
- Diagnostics (errors, warnings, style hints)
- Go-to definition and find references
- Rename symbols
- Document formatting
- Semantic tokens for precise coloring
- Workspace structure and module explorer

## Language Server

Vitte Studio ships with a built-in LSP server. It provides:

- Workspace indexing for fast navigation
- Cross-file references and symbol search
- Intelligent diagnostics and lint rules
- Completion and hover computed from project context
- Fast incremental updates on file change

## Views

The extension adds a Vitte activity bar with dedicated views.

- **Vitte Explorer**: project structure and modules
- **Vitte Metrics**: live diagnostics and performance data

## Commands

All commands are available from the Command Palette.

- `Vitte: Show Server Log`
- `Vitte: Show Server Metrics`
- `Vitte: Refresh Metrics View`
- `Vitte: Show Command Menu`
- `Vitte: Restart Language Server`
- `Vitte: Run Action`
- `Vitte: Run Action (Args)`
- `Vitte Debug: Run File`
- `Vitte Debug: Attach`
- `Vitte: Organize Imports`
- `Vitte: Fix All`
- `Vitte: Rename Symbol`
- `Vitte: Apply Sample Edit`
- `Vitte: Progress Sample`
- `Vitte: Show Info`
- `Vitte: Refresh Diagnostics`
- `Vitte: Open Diagnostic`
- `Vitte: Refresh Structure`
- `Vitte: Refresh Explorer`
- `Vitte: Reveal in Explorer`
- `Vitte: Open Docs`
- `Vitte: Open Playground`
- `Vitte: Detect Toolchain`
- `Vitte: Run Benchmarks`
- `Vitte: Run Benchmarks and Open Report`
- `Vitte: Open Bench Directory`
- `Vitte: Build`
- `Vitte: Clean`
- `Vitte: Run`
- `Vitte: Test`
- `Vitte: Test Current File`
- `Vitte: Switch Build Profile`
- `Vitte: Toggle Incremental Build`
- `Vitte: Refresh Tests`
- `Vitte: Run All Tests`
- `Vitte: Quick`
- `Vitte: Quick Export`
- `Vitte: Toggle Telemetry`

## Debugging

Vitte Studio registers a `vitte` debugger with launch configurations for:

- Launch current file
- Launch project entry
- Attach to server

## Configuration

You can configure the extension in VS Code settings.

- `vitte.trace.server`: `off | messages | verbose`
- `vitte.serverPath`: custom server binary (empty = bundled)
- `vitte.debug.program`: path to `vitte-runtime`
- `vitte.toolchain.root`: toolchain root directory
- `vitte.toolchainPath`: legacy fallback for toolchain
- `vitte.runtime.path`: runtime binary path
- `vitte.lsp.path`: LSP binary path
- `vitte.build.path`: build binary path
- `vitte.fmt.path`: formatter binary path
- `vitte.bench.path`: benchmark binary path
- `vitte.build.profile`: `dev | test | release | bench`
- `vitte.build.incremental`: enable incremental build
- `vitte.build.distributed`: enable distributed build
- `vitte.bench.reportDir`: benchmark report output
- `vitte.docs.include`: docs include glob
- `vitte.docs.exclude`: docs exclude glob
- `vitte.docs.indexLimit`: max indexed markdown files
- `vitte.telemetry.enabled`: local telemetry toggle
- `vitte.telemetry.sampleRate`: sampling rate
- `vitte.quickActions.sequences`: custom action chains
- `vitte.quickActions.defaults`: override built-in actions
- `vitte.commandShortcuts`: status bar shortcuts

## Quick Actions

The `Vitte: Quick` command provides curated, multi-step workflows. You can override defaults or add custom sequences using `vitte.quickActions.sequences` and `vitte.quickActions.defaults`.

## Tasks

Vitte Studio adds a `vitte` task definition for build tooling:

- `build`
- `run`
- `test`
- `clean`
- `bench`

## Getting Started

1. Install the extension from the Marketplace or via VSIX.
2. Open a Vitte project (or any `.vitte` / `.vit` file).
3. Run `Vitte: Detect Toolchain` to auto-detect your binaries.
4. Use `Vitte: Quick` for common workflows like build/run/test.
5. Use `Vitte: Restart Language Server` if IntelliSense feels stale.

## Installation (VSIX)

1. Open VS Code
2. Run `Extensions: Install from VSIX...`
3. Select the VSIX file, for example `vitte-studio-2.0.0.vsix`


## Changelog Highlights

### 2.0.0

- Richer completions (keywords, types, methods, modules)
- Hover documentation (docstrings)
- Improved diagnostics and linting
- Go-to definition and workspace references
- Formatting and semantic tokens
- Updated VSIX packaging and tooling

## Troubleshooting

- If completions or diagnostics are missing, run `Vitte: Restart Language Server`.
- If toolchain paths are custom, set `vitte.toolchain.root` and related binary paths.
- If workspace features are slow, reduce `vitte.docs.indexLimit` or refine include/exclude globs.

## License

MIT
