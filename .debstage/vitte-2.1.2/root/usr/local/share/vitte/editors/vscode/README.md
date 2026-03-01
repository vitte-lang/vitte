# Vitte Language Support (Vitte Studio)
Official VS Code extension for Vitte.
Vitte Studio gives you a complete daily workflow in VS Code: edit, navigate, diagnose, build, run, test, and debug from one place.
This README is intentionally practical.
It focuses on what you can do right now, why each command matters, and how to troubleshoot quickly when something breaks.
## 1. What the extension gives you
Vitte Studio supports both `.vitte` and `.vit` files.
You get syntax highlighting, autocomplete, hover details, diagnostics, symbol navigation, rename, formatting, semantic tokens, debug integration, and project commands.
In short: less manual setup, faster iteration, and better code feedback while you type.
## 2. Sidebar experience
The extension adds a dedicated Vitte sidebar.
`Vitte Documentation` loads official docs from `https://vitte.netlify.app`.
`Vitte Summary` gives chapter-based navigation from beginner to professional topics.
`Vitte Diagnostics` aggregates workspace issues.
`Vitte Modules` helps you inspect project structure.
`Vitte Metrics` exposes runtime/server metrics useful for performance checks.
`Vitte Offline` explains offline mode and gives recovery actions.
## 3. Installation
### Marketplace install
Open Extensions in VS Code, search `Vitte Language Support`, then click Install.
### VSIX install
1. Open VS Code.
2. Run `Extensions: Install from VSIX...`.
3. Select your `.vsix` file.
After installation, open a `.vitte` or `.vit` file to verify language features are active.
## 4. First 5-minute setup
Step 1: Run `Vitte: Detect Toolchain`.
This checks that the expected binaries are reachable.
Step 2: Run `Vitte: Open Docs`.

Keep docs visible while coding to reduce context switching.

Step 3: Run `Vitte: Quick`.

Use it as your daily launcher for build/run/test routines.

Step 4: If features feel stale, run `Vitte: Restart Language Server`.

## 5. Essential commands (what they really do)

`Vitte: Detect Toolchain` validates runtime/build/lsp paths and helps avoid hidden config drift.

`Vitte: Open Docs` opens the official documentation quickly, useful during syntax or API checks.

`Vitte: Open Playground` gives a safe space for quick experiments.

`Vitte: Quick` opens a curated action menu that saves time during repetitive workflows.

`Vitte: Build` compiles your project and confirms build health.

`Vitte: Run` builds and executes your app using current settings.

`Vitte: Test` runs the test suite for confidence before commit.

`Vitte: Restart Language Server` resets IntelliSense when completion/hover/diagnostics degrade.

`Vitte: Show Server Log` opens technical logs for root-cause analysis.

`Vitte: Show Server Metrics` helps identify slow requests and server pressure.

## 6. Secondary commands worth knowing

`Vitte: Refresh Diagnostics` forces diagnostic refresh.

`Vitte: Open Diagnostic` jumps directly to a selected issue.

`Vitte: Refresh Structure` reloads structural/module data.

`Vitte: Run Benchmarks` and `Vitte: Run Benchmarks and Open Report` support performance testing loops.

`Vitte: Open Bench Directory` opens report output quickly.

`Vitte: Clean` removes build artifacts when you need a fresh state.

`Vitte: Test Current File` runs file-focused tests for tighter feedback.

`Vitte: Switch Build Profile` swaps between `dev`, `test`, `release`, and `bench`.

`Vitte: Toggle Incremental Build` controls faster rebuild strategy.

`Vitte: Toggle Telemetry` and `Vitte: Quick Export` support local observability workflows.

### Module workflow (repo vitte)

For module-focused work in this repository, use workspace tasks from `.vscode/tasks.json`:

- `vitte: parse modules (current file)`
- `vitte: strict modules (current file)`
- `vitte: module graph (current file)`
- `vitte: module graph json (current file)`
- `vitte: module doctor (current file)`
- `vitte: dump module index (current file)`
- `vitte: modules tests`
- `vitte: modules snapshots`
- `vitte: module shape policy`
- `vitte: runtime matrix modules`
- `vitte: ci mod fast`

These map directly to Vitte module tooling (`mod graph`, `mod doctor`, strict module checks, and module CI targets).

## 7. Minimal configuration first

If auto-detection is enough, keep defaults.

If not, configure these first:

- `vitte.toolchain.root`
- `vitte.runtime.path`
- `vitte.lsp.path`
- `vitte.build.path`
- `vitte.fmt.path`

Then tune behavior:

- `vitte.build.profile`
- `vitte.build.incremental`
- `vitte.bench.reportDir`
- `vitte.telemetry.enabled`
- `vitte.telemetry.sampleRate`
- `vitte.quickActions.sequences`
- `vitte.quickActions.defaults`
- `vitte.commandShortcuts`

## 8. Recommended daily flow

Start with `Quick` to run common actions.

Use `Build` after major edits and `Test` before pushing changes.

Use docs and summary views while reviewing unfamiliar syntax.

When behavior looks inconsistent, restart the language server before changing settings.

## 9. Troubleshooting playbook

If autocomplete disappears:

1. Run `Vitte: Restart Language Server`.
2. Check `Vitte: Show Server Log`.

If toolchain detection fails:

1. Run `Vitte: Detect Toolchain` again.
2. Set explicit paths in settings.

If extension enters offline mode:

1. Run `Vitte: Explain Offline`.
2. Open `Vitte: Open Offline Log`.
3. Generate `Vitte: Copy Offline Report`.

## 10. Debug support

The extension provides debug type `vitte`.

Typical scenarios include launching the current file, launching project entry, and attaching to a running session.

## 11. Activation model

The extension keeps one explicit activation event: `onStartupFinished`.

Command, view, and language activation are inferred by VS Code from `contributes` in `package.json`.

## 12. Project links

Version history: `CHANGELOG.md`.

License: MIT.
