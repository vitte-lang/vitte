# Vitte CLI

Usage:
```
vitte [command] [options] <input>
```

Commands (recommended):
- `parse`  Parse only (no backend)
- `check`  Parse + resolve only
- `emit`   Emit C++ only (no native compile)
- `build`  Full build (default)

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
