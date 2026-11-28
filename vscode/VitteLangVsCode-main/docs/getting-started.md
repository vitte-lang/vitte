# Getting Started with Vitte Language Support

This guide describes the prerequisites, installation workflow, and validation steps for the Vitte Language Support extension in Visual Studio Code.

## Prerequisites

- Visual Studio Code version **1.105.0** or later.
- The unified Vitte CLI (`vitte`) available on your `PATH`, or configured via `vitte.debug.program` / `vitte.compiler.path`.
- A workspace that includes at least one Vitte source file (`.vit`, `.vitl`, or legacy `.vitte`).

> ℹ️ Tip: Pin the **Vitte ▸ Diagnostics** and **Vitte ▸ Structure** views in the Activity Bar to monitor real-time diagnostics and module topology while you work.

## Installation

1. Install the extension from the Visual Studio Code Marketplace (search for **Vitte Language Support**) or run:
   ```bash
   code --install-extension VitteStudio.vitte-studio
   ```
2. Reload Visual Studio Code when prompted so the language server can initialize against your workspace.

To install from a locally built VSIX:

```bash
vsce package
code --install-extension vitte-studio-<version>.vsix
```

## Initial Verification

1. Open any Vitte/Vit source file; the extension activates automatically.
2. Confirm that syntax highlighting, code snippets, hover information, and diagnostics are available.
3. Observe the status bar item `$(rocket) Vitte`:
   - `$(gear)` indicates the server is starting.
   - `$(check)` confirms an active connection.
   - `$(pass-filled)` denotes a clean diagnostics pass, while `$(warning)` or `$(error)` signal outstanding issues.
   - `$(debug-stop)` appears if the client stops unexpectedly; review the output channel for details.
4. Browse the **Vitte ▸ Structure** view to inspect modules, structs, and functions annotated with diagnostics badges.
5. Use command palette shortcuts (`⇧⌘P`) such as `Vitte: Restart Server`, `Vitte: Show Info`, or `Vitte: Show Server Log` for routine maintenance.

## Kickstart with the Run Setup action

Prefer one-click onboarding? Run **Vitte: Quick Actions** and select **🚀 Setup**. The extension will automatically:

1. Open the local docs to highlight the walkthrough
2. Detect the toolchain on your machine
3. Trigger an initial build to warm caches and surface diagnostics

You can invoke this action from the status bar menu, the startup toast, or directly from the palette. It is safe to rerun whenever you change environments.

## Running the Debugger

The bundled native debugger is experimental and disabled by default. Enable
`vitte.debug.nativeBackend` in the active workspace folder's settings to expose
its launch templates. Linux x86_64 / Ubuntu 24.04 with Vitte CLI 0.1.1, LLDB
and Expect is the only platform candidate; CI still needs to validate this exact
extension revision. To debug:

1. Set a breakpoint in the active `.vit`, `.vitl`, or legacy `.vitte` document.
2. On that candidate platform, select **Vitte: Launch current file (native, experimental)** to compile with debug flags, or **Vitte: Debug built executable (DWARF)** to build the active file and launch its executable. The current extension revision still needs its own CI validation.
3. When DWARF is present, the adapter exposes source and conditional breakpoints, stepping, continue, pause, termination, locals, parameters, aggregate values, watches and call frames. Native signals map to Vitte lines only when line metadata exists; structured runtime diagnostics and generated multi-module mappings are unavailable.

Other platforms, non-default native targets, or executables without Vitte DWARF are unsupported by the bundled backend. Configure an external DAP adapter, or use **Vitte: Build** and **Vitte: Run** with the selected compiler and project settings. The extension never switches compiler frontend or project configuration silently.

Define reusable launch recipes in `.vscode/launch.json` when you need additional control:

```json
{
  "type": "vitte",
  "request": "launch",
  "name": "Vitte: Launch current file",
  "program": "${file}",
  "nativeBackend": true,
  "args": [],
  "stopOnEntry": true
}
```

This native launch recipe is verified only on Linux x86_64 / Ubuntu 24.04 with compiler 0.1.1. It uses the selected workspace folder's project root, compiler, `vitte.build.env`, `vitte.modulePaths`, `vitte.build.target`, manifest and compiler arguments. A custom `target`, `compilerArgs` or `manifestPath` in `launch.json` overrides its corresponding setting.

Compiler 0.1.1 does not expose reliable test discovery metadata or structured test results. The extension therefore does not populate the Test Explorer; **Vitte: Test Current File** explains this limit, while a compiler checkout's existing Make `test` target remains available as a task.

Use `tests/strict_ok.vit` in the main Vitte repository as a small smoke fixture when validating local toolchain behavior.

## Troubleshooting

- **Language server does not start**: Run `Vitte: Show Server Log` to verify the bundled server and optional external `vitte.lsp.path`.
- **Diagnostics do not appear**: Ensure the workspace is trusted and files use a supported Vitte/Vit extension.
- **macOS blocks tests or debugger**: Clear the quarantine flag from the Visual Studio Code test host with `xattr -dr com.apple.quarantine <Visual Studio Code.app>`.

If additional assistance is required, please open an issue on [GitHub](https://github.com/vitte-lang/vitte/issues).
