# Getting Started with Vitte Language Support

This guide describes the prerequisites, installation workflow, and validation steps for the Vitte Language Support extension in Visual Studio Code.

## Prerequisites

- Visual Studio Code version **1.93.0** or later.
- Vitte toolchain binaries (compiler + runtime) available on your `PATH`.
- A workspace that includes at least one Vitte source file (`.vitte` or `.vit`).

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

Launch configurations for Vitte targets are generated automatically. To debug:

1. Set a breakpoint in the active `.vitte` document.
2. Press **F5** (Vitte: Launch current file) or select another configuration from the Run and Debug view.
3. Step through code, inspect scopes, and review the call stack as you would in any Visual Studio Code debugging session.

Define reusable launch recipes in `.vscode/launch.json` when you need additional control:

```json
{
  "type": "vitte",
  "request": "launch",
  "name": "Vitte: Launch current file",
  "program": "${file}",
  "cwd": "${workspaceFolder}",
  "stopOnEntry": true
}
```

Refer to the [`examples/`](../examples) directory for sample configurations.

## Troubleshooting

- **Language server does not start**: Run `Vitte: Show Server Log` to verify the resolved server path. Override the default via `vitte.serverPath` when bundling a custom binary.
- **Diagnostics do not appear**: Ensure the workspace is trusted and files use a supported Vitte/Vit extension.
- **macOS blocks tests or debugger**: Clear the quarantine flag from the Visual Studio Code test host with `xattr -dr com.apple.quarantine <Visual Studio Code.app>`.

If additional assistance is required, please open an issue on [GitHub](https://github.com/roussov/vittelang-vscode/issues).
