# Vitte Offline

Vitte is running in **offline mode**. The language server is not started and server-only features are disabled.

## Why am I offline?
- `vitte.server.offline` is enabled
- or **offline permanent** is enabled: `vitte.server.offlinePermanent`
- or the server module could not be found

## What still works
- Syntax highlighting
- Snippets
- Offline diagnostics (basic lint)
- File explorer / views not requiring the server

## Offline report
Run **Vitte: Copy Offline Report** to copy a summary (settings + log tail).

## What is disabled
- Completion / hover / definition / references
- Formatting / rename
- Metrics / server telemetry

## Logs
- Offline log: `.vitte/offline.log`
- Extension log: **Vitte Language Server** output channel

## Settings
Open settings and search for `vitte.server`.
