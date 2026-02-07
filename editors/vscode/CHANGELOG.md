# Changelog
All notable changes to **Vitte Language Support** are documented here.
This project follows the [Keep a Changelog](https://keepachangelog.com/en/1.0.0/) format
and adheres to [SemVer](https://semver.org/spec/v2.0.0.html).

---

## [1.9.0] — 2025-11-20
### Changed
- 📚 **Official grammar refresh**: TextMate, semantic tokens, and server language facts now mirror the published Vitte `.ebnf` (new module/import path rules, `union` items, `static` bindings, cleaned operator set, CLI tokens). Keywords such as `async/await`, `switch/case`, `try/catch`, `impl/trait`, `with/defer`, and `use/mod` were retired to match the spec (`syntaxes/vitte.tmLanguage.json`, `server/src/languageFacts.ts`, `server/src/semantic.ts`).
- 🧠 **Language server alignment**: completions, hovers, indexer, navigation, and snippets only expose the official constructs (module/import/struct/enum/union/type/fn/let/mut/const/static/match/loops). Workspace symbol extraction, hover texts, and semantic highlighting were simplified accordingly (`server/src/completion.ts`, `server/src/languageService.ts`, `server/src/indexer.ts`, `server/src/navigation.ts`).
- 🧷 **Documentation & fixtures**: snippets, README highlights, and colorization fixtures were rewritten to showcase the new syntax (module declarations end with `;`, imports use `import`, examples cover unions/CLI options) and duplicated fixtures under `test/` were kept in sync (`snippets/vitte.json`, `README.md`, `colorize-fixtures/**/*`, `test/colorize-fixtures/**/*`).
- ⚙️ **Language configuration**: added `//` line comments, updated block-introducing keywords, and improved operator detection to include `..=` while removing legacy modifiers (`language-configuration.json`, `syntaxes/vitte.tmLanguage.json`).
- 🧲 **Quick Actions overrides**: use `vitte.quickActions.defaults` to adjust any built-in action (change labels, swap commands, add args, or replace with custom step chains) without redefining whole sequences (`src/commands/quickActions.ts`, `package.json`, `README.md`).
- 🕒 **Quick Actions telemetry badge**: build/run/test/bench entries now surface a “🔁 Dernière exécution” indicator plus an “⟳ Exécuter à nouveau” hint when telemetry is enabled, helping users rerun their most common workflows directly from the palette (`src/commands/quickActions.ts`).
- 🖱️ **Command buttons & menu**: the status bar now exposes dedicated Build/Run/Test buttons plus a Vitte drop-down that lists the other commands, and the extension greets you at startup with these shortcuts so workflows are one click away (`src/extension.ts`, `package.json`, `README.md`).
- 🩺 **Diagnostics-aware startup suggestions**: the launch toast now highlights context-specific actions (e.g., ⭐ Run Tests when errors are present, ⭐ Refresh Diagnostics when stale) and respects the customizable shortcut list (`src/extension.ts`, `README.md`).
- 📈 **Server metrics channel**: every LSP handler records duration stats that can be queried via `Vitte: Show Server Metrics` or the new **Vitte Metrics** tree view (with auto-refresh and toolbar command), providing rolling averages/max to spot regressions without digging through logs (`server/src/server.ts`, `src/providers/metricsView.ts`, `src/extension.ts`, `package.json`, `README.md`).
- 📘 **Onboarding docs/tests**: README + Getting Started now call out the **🚀 Run Setup** Quick Action, and integration tests ensure the bench/diagnostics Quick Actions keep firing even as their logic evolves (`README.md`, `docs/getting-started.md`, `src/test/suite/extension.test.ts`).

### Fixed
- 🧪 CLI highlighting recognizes every documented option/command and number/identifier regexes forbid unsupported suffixes, ensuring tokenizer/tests align with the reference grammar (`syntaxes/vitte.tmLanguage.json`, `colorize-fixtures/**/*`).

---

## [1.8.0] — 2025-11-15
### Added
- 🧠 **Smarter hovers**: brand-new Markdown hover engine (doc comment extraction, signature previews, precise ranges) wired into the server for every position (`server/features/hover.ts`, `server/src/server.ts`).
- 🌈 **Semantic tokens refresh**: deterministic tokenizer covering keywords, numbers (bin/hex/oct), strings, multi-line comments, and modifiers (`declaration`, `readonly`, `async`, …) for richer themes (`server/features/semanticTokens.ts`).
- 🤖 **CI pipeline**: GitHub Actions workflow now runs lint + build + tests on every PR/commit (`.github/workflows/ci.yml`).
- ⚡ **Quick Actions palette**: `Vitte: Quick Actions` now bundles build/run/test/bench/diagnostics, auto-suggests *Test Active File* for `_test.vitte`, highlights `Vitte: Build` until the workspace is built once, hides bench entries when no `vitte.config.json` exists, shows a 3-item MRU, flags stale diagnostics, supports favorites, exposes inline toggles (*Next Profile*, *Toggle Incremental*), adds *Jump to Next Diagnostic*, a “🚀 Run full setup” (Docs → Toolchain → Build) button, fully configurable sequences via `vitte.quickActions.sequences`, and a new `Vitte: Export Quick Action Stats` command to dump telemetry data in JSON. With telemetry enabled, every action shows its run count and sequences like *Docs + Playground* share those insights too (`src/commands/quickActions.ts`, `package.json`, `README.md`).
- 🧭 **Welcome walkthrough**: a “Vitte Quickstart” walkthrough appears in the VS Code Welcome view to guide users through docs, toolchain detection, and the onboarding Quick Action (`package.json`, `README.md`).

### Changed
- ⚙️ **LSP completions**: completion handler now exposes `onCompletionResolve` and centralizes trigger characters to enrich items lazily, reducing traffic (`server/src/server.ts`).
- 🩺 **Diagnostics pipeline & views**: scheduling/publishing now flows through the LSP connection (no more silent `sendDiagnostics`), user preferences are validated before use, tooltips are richer, and the Diagnostics tree filters to `.vitte/.vit` files (`server/features/diagnostics.ts`, `src/diagnosticsView.ts`, `src/utils/diagnostics.ts`).
- 🛠️ **Bench/Build/Test tasks**: typed task providers, resilient `vitte.config.json` parsing, advanced arguments (profiles, `--targets`, exports, regression thresholds), and hardened output handling (`src/tasks/benchTasks.ts`, `src/tasks/buildTasks.ts`, `src/tasks/testTasks.ts`).
- 🧭 **Explorers**: the structural view is now `Vitte ▸ Modules`, keeps aggregated diagnostics, reuses packaged icons, and the Project tree groups sources per folder while surfacing targets (`src/moduleExplorerView.ts`, `src/providers/tree/projectTree.ts`).
- 🎨 **Icons & docs**: icon theme points to `media/icons/vitte-logo.svg`, recognizes `vitte.config.json`, and the docs capture the latest install/health workflows (`icons/vitte-icon-theme.json`, `README.md`).

### Fixed
- 🧹 **Diagnostics publishing**: debounce, clearing, and config-triggered revalidation now use the LSP `Connection`, preventing stuck or missing diagnostics (`server/features/diagnostics.ts`).
- 🧪 **Toolchain resolution**: resolver now normalizes env vars, handles PATH/HOME portably, and validates runtime detection more strictly (`src/utils/toolchain.ts`).
- 📈 **Bench/Test reports**: opening reports handles VS Code errors/Buffers gracefully and “run/test” tasks ignore non-Vitte files to avoid accidental execution (`src/tasks/benchTasks.ts`, `src/tasks/buildTasks.ts`).

---

## [1.7.1] — 2025-11-05
### Added
- 🧹 **Unified ESLint config** (`eslint.config.mjs`) using `typescript-eslint`, applied at the repo root to cover client, server, and tooling.
- 🔧 **Lint dependencies** updated (`typescript-eslint`) and `npm run lint` now scans the entire repo.

### Changed
- 🛡️ **LSP server**: hardening across modules (`commands`, `lsp`, `server`, `config`, `logger`, `navigation`, `semantic`, …) with typed imports, explicit error handling, and removal of unsafe access.
- 🪄 **Utilities** (`formatting`, `utils/eol`, `utils/text`): cleaned promises, normalized regexes, pervasive use of `??` and optional chains.
- 🧭 **Debug tooling** (`adapterFactory`, `configurationProvider`, `runtimeLocator`): refreshed to stabilize runtime resolution, env handling, and multi-platform detection.

### Fixed
- ✅ Eliminated most ESLint warnings (type-only imports, implicit `any`, floating promises, legacy index signatures).
- 🪛 Clarified server logs/error messages to avoid stray `await`s or `any` returns.

---

## [1.7.0] — 2025-11-02
### Added
- ⚙️ **Full LSP commands** (`vitte.formatDocument`, `vitte.organizeImports`, `vitte.fixAll`) wired across client/server with automatic `TextEdit` application.
- 🧠 **Extended linter** with new rules (`mixedIndent`, `blankLines`, `nonAscii`, `finalNewline`) and automatic LSP Quick Fix generation.
- 🪶 **Formatter upgrades**: range formatting support, minimal edit computation, indentation normalization.
- 🔍 **LSP indexer**: handles `impl` blocks, robust comments/strings parsing, plus a `reindexWorkspace()` API.

### Changed
- 🧰 `languageFacts.ts` refactored with a comprehensive catalog of keywords/types/operators/literals for completion & semantic tokens.
- 💡 `commands.ts` now talks to the server through `vitte/*` requests (no manual stubs).
- 🚀 Diagnostics, completion, and formatting now share the same set of rules for consistency.

### Fixed
- 🐞 Patched TypeScript errors (`Thenable.catch`, duplicate `TextDocumentSyncKind`, missing `languageService` import).
- 🔧 Reworked LSP handlers (`onCodeAction`, `onDocumentFormatting`, `onDocumentRangeFormatting`) to avoid unnecessary global replacements and improve performance.

---

## [1.6.0] — 2025-10-21
### Added
- 🐞 Dedicated **Vitte Diagnostics** view (Rust Analyzer style) listing errors/warnings per file with direct navigation and the command *Vitte: Refresh Diagnostics*.
- 🧭 Utility commands to open/refresh diagnostics and a “Vitte” Activity Bar view.

### Changed
- 🧰 Diagnostics view opens automatically to guide both beginners and power users through LSP/lint/debug errors.

---

## [1.5.0] — 2025-10-21
### Added
- ✅ Full support for Vitte 1.5 keywords (`async/await`, `switch/case/default`, `try/catch/finally/throw`, `defer`, `unsafe`, `with`, …) across grammar, semantic tokens, completion, and lint.
- ✍️ New snippets/completions (`async fn`, `switch`, `try/catch`, `defer`, `unsafe`, `with`) plus recognition of `pub async` functions, public structs/enums for suggestions and outline.
- 👀 Automatic watching of config files (`vitte.toml`, `.vitteconfig`) in addition to sources.

### Changed
- 🔄 LSP client now handles untitled documents and notebook cells while reusing watchers between restarts.
- 🧠 Indexers, navigation (outline/go-to-symbol), and completion now respect modifiers (`pub`, `async`, `unsafe`, `extern`) preceding declarations.

---

## [0.1.0] — 2025-08-21
### Added
- 🎨 Basic TextMate highlighting for **Vitte**:
  - keywords (`fn`, `let`, `const`, `mut`, `struct`, `enum`, `trait`, `impl`, `module`, `use`, `match`, `return`, `break`, `continue`)
  - strings, comments, function identifiers
- ✂️ Snippets:
  - public/private functions
  - modules + imports
  - structs, enums, impls, traits
  - loops (`for`, `while`)
  - `match` expressions
  - test templates
  - FFI (`C`, `Rust`)
  - data structures (`Vec`, `Map`, `Option`, `Result`)
- ⚙️ Language configuration:
  - auto-closing brackets `() [] {}`
  - auto-closing quotes `"" ''`
  - comments `//` and `/* */`
- 🖼️ Dedicated Marketplace icon (`icon.png`)
- 📦 Packaging + `vsce` scripts (build, publish)
- 📑 Initial, detailed README.md
- 🧩 User configuration support:
  - `vitte.enableLSP` (bool)
  - `vitte.trace.server` (LSP logs)

---

## [0.2.0] - 2025-08-29
### ✨ Added
- `.vitte` icon via `vitte-icon-theme.json` (theme-agnostic, unique logo).
- Richer syntax support:
  - New TextMate scopes for attributes `#[...]`, macros `name!`, raw strings `r#"..."#`.
  - Hex/bin/oct/float numbers with underscores.
  - Expanded keywords (mut, async/await, package, typedef, mov/jmp, etc.).
  - Detection of complex operators (`::`, `->`, `=>`, `==`, `<=`, `&&`, `||`, `<<`, `>>`, etc.).
- Improved LSP:
  - Clearer diagnostics for TODO/FIXME/??? and trailing spaces.
  - Heuristic detection of type identifiers (PascalCase).
  - Extended hover docs for many keywords.
  - Enriched semantic tokens (keywords, types, numbers, strings, comments).
- Updated snippets: `fn`, `struct`, `enum`, `trait`, `impl`, `match`, `for/while`, `main`.
- Better VS Code dev experience:
  - `npm run compile`, `npm run watch`, `npm run build:vsix`.
  - Debug configurations (Run Extension + Attach LSP).
  - Publisher aligned to `VitteStudio`.

### 🔧 Changed
- More explicit diagnostic messages.
- Consolidated icon paths and `package.json`.

### 🚫 Breaking
- No major breaking change. Validate overly strict custom themes (scopes are more specific).
