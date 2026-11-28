# Getting Started with Vitte Language Support

This guide describes the prerequisites, installation workflow, and validation steps for the Vitte Language Support extension in Visual Studio Code.

## Prerequisites

- Visual Studio Code version **1.105.0** or later.
- The Vitte CLI (`vitte`) available on your `PATH`, or selected with **Vitte: Select Compiler** / `vitte.compiler.path`.
- A workspace that includes at least one Vitte source file (`.vit`, `.vitl`, or legacy `.vitte`).

## Quick start

1. Install **Vitte Language Support** from the Marketplace or a VSIX, then open the project folder in VS Code.
2. Open a `.vit`, `.vitl` or `.vitte` file. The extension detects `vitte.toml` or `.vitte/config` by walking upward from that file. If neither exists, the workspace folder is used. Set `vitte.project.root` to override the root for that folder.
3. Read the Vitte status item: it shows language-server state, the selected CLI version and project diagnostics. Select it, or run **Vitte: Status and Project Health** from the Command Palette, for compiler, project, target, logs and capability actions.
4. If needed, run **Vitte: Select Compiler** and choose a detected CLI or browse for one. Discovery precedence is `vitte.compiler.path`, `vitte.toolchain.root`, project-local binaries, user/environment installs, then `PATH`.
5. Run **Vitte: Check**, **Vitte: Build** or **Vitte: Run** from the Command Palette. **Vitte: Project Actions** offers the same commands with project detection and scaffolding. **Vitte: Select Build Target** sets or clears `vitte.build.target`; the selected CLI decides whether a requested target is supported.

For multiple workspace folders, select a folder when prompted. Compiler, project-root, module-path, target, environment and build-argument settings are folder-scoped. The extension does not silently change the compiler frontend or project configuration.

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

Use the status item tooltip for full server, compiler and project health details. Error and warning colors use VS Code theme tokens, including high-contrast themes. Problems and navigation are exposed through VS Code's native editor views. Quick Picks support keyboard navigation and screen readers; the extension adds no default keybindings, so it does not capture common editor shortcuts. Use VS Code's built-in Command Palette shortcut (`Ctrl+Shift+P` on Windows/Linux, `Cmd+Shift+P` on macOS) and search for a `Vitte:` command.

Status and project actions are available without startup popups. To opt into the post-start command picker, enable `vitte.notifications.showStartupCommands`.

The status item, project/toolchain selectors and diagnostic views support English and French through `vitte.lang` (`en` or `fr`). Other command titles, project messages and compiler/server text may remain in English. Native VS Code controls inherit editor theme and accessibility behavior. Extension-owned status indicators use no continuous animation.

## Capabilities and configuration

Open **Vitte: Show Capabilities** for the versioned editor/compiler/platform support matrix. Editor features backed by the active compiler can be partial or unavailable; the matrix lists the tested scope and evidence. In particular, compiler 0.1.1 has no reliable Test Explorer metadata, and document/range formatting is unavailable without a compatible canonical formatter.

Useful folder-scoped settings include:

- `vitte.compiler.path` and `vitte.lsp.path` select the CLI and optional language server.
- `vitte.project.root`, `vitte.project.manifestPath` and `vitte.modulePaths` configure project and module roots.
- `vitte.build.args`, `vitte.build.env`, `vitte.build.profile` and `vitte.build.target` configure CLI commands.
- `vitte.lang` selects English or French for the supported extension UI; `vitte.notifications.showStartupCommands` enables the optional startup picker.

Settings descriptions in the VS Code Settings editor document their scope and limits. Run **Vitte: Detect Project**, **Vitte: Detect Toolchain**, **Vitte: Show Server Log** or **Vitte: Show Capabilities** for details. The server log is available in the **Vitte Language Server** Output channel.

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
