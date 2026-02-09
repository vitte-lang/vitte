# Vitte CLI

Usage:
```
vitte [command] [options] <input>
```

Commands (recommended):
- `help`   Show help and common tasks
- `init [dir]`   Create a minimal project scaffold
- `explain <code>`  Explain a diagnostic (e.g. E0001)
- `doctor` Check toolchain prerequisites
- `parse`  Parse only (no backend)
- `check`  Parse + resolve only
- `emit`   Emit C++ only (no native compile)
- `build`  Full build (default)

Options:
- `--lang <code>`  Language for diagnostics (e.g. en, fr). Defaults to `LANG/LC_ALL`, then `en`.
- `--explain <code>`  Explain a diagnostic (e.g. E0001).
- `--stdout`  Emit C++ to stdout (implies `emit`).
- `--emit-obj`  Emit a native object file (`.o`).
- `--repro`  Enable reproducible object output flags.
- `--repro-strict`  Enforce strict deterministic IR lowering order.
- `--parse-modules`  Parse + load modules (no resolve/lowering).
- `--parse-silent`  Suppress parse-only informational logs.
- Diagnostics include stable error codes (e.g. `error[E0001]: ...`).

Examples:
```
vitte help
vitte init
vitte init app
vitte explain E0001
vitte doctor
vitte parse --lang=fr src/main.vit
```

Error Codes:
- Diagnostics use stable codes like `E0001`. These codes are defined in `src/compiler/frontend/diagnostics_messages.hpp`.
- The `.ftl` localization files can translate by code (preferred) or by message key.

Stdlib:
- See `docs/stdlib.md` for standard library modules and examples.

Legacy flags (still supported):
- `--parse-only`
- `--resolve-only`
- `--hir-only`
- `--mir-only`
- `--emit-cpp`
- `--strict-parse`

Notes:
- Commands set a default mode, but explicit flags still override.
- For strict parsing, use `parse --strict-parse <file>`.
