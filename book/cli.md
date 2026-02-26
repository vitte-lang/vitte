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
- `check`  Parse + resolve + IR (no backend)
- `emit`   Emit C++ only (no native compile)
- `build`  Full build (default)
- `profile`  Full build with stage timing/memory profile summary
- `reduce` Reduce a failing file to a minimal reproducer
- `clean-cache`  Remove `.vitte-cache` stage cache files
- `mod graph` Show import graph, cycles, and module weights (LOC/exports/imports)
- `mod doctor` Check import hygiene (unused, aliasing, canonical paths, collisions)
- `mod contract-diff` Compare exported module contract between 2 entries (`--old` / `--new`)

Options:
- `--lang <code>`  Language for diagnostics (e.g. en, fr). Defaults to `LANG/LC_ALL`, then `en`.
- `--explain <code>`  Explain a diagnostic (e.g. E0001).
- `--stage <name>`  Stop at `parse|resolve|ir|backend`.
- `--diag-json`  Emit diagnostics as JSON.
- `--diag-json-pretty`  Emit pretty JSON diagnostics (includes `diag_schema`).
- `--diag-code-only`  Emit compact diagnostics (`file:line:col CODE`), useful for CI.
- `--deterministic`  Stable output ordering for diagnostics/tooling.
- `--cache-report`  Print parse/resolve/ir cache hit/miss report.
- `--runtime-profile <name>`  Restrict runtime/package imports: `core|system|desktop|arduino`.
- `--allow-experimental`  Allow imports from `experimental/...` modules.
- `--warn-experimental`  Downgrade experimental import denial to warning (migration mode).
- `--deny-internal`  Enforce internal module privacy (default behavior).
- `--allow-internal`  Disable internal module privacy check.
- `--strict-modules`  Forbid glob imports and enforce strict module import shape.
- `--dump-stdlib-map`  Print resolved package module -> exported symbols (legacy flag name).
- `--dump-module-index`  Dump full module index JSON (`modules`, `imports`, `exports`, `loc`).
- `--json`  For `mod graph`: emit JSON graph payload.
- `--from <module>`  For `mod graph`: restrict view to module-reachable subgraph.
- `--dump-ast`  Dump AST after parsing.
- `--dump-ir`  Dump IR (`--dump-mir` alias).
- `--strict-types`  Reject compatibility aliases (`integer`, `uint32`, etc.) and enforce canonical type names.
- `--strict-imports`  Require explicit import aliases, reject unused aliases, and forbid relative import paths.
- `--fix`  For `mod doctor`: print concrete rewrite suggestions.
- `--max-imports <N>`  For `mod doctor`: report modules with fan-out > N.
- `--old <file>` / `--new <file>`  For `mod contract-diff`.
- `--strict-bridge`  Alias of `--strict-imports` for native liaison policy.
- `--fail-on-warning`  Treat warnings as errors.
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
- Diagnostics use stable layer prefixes:
  - `E000x`: parse/grammar
  - `E100x`: resolve
  - `E1007`: invalid signed/unsigned cast
  - `E1010..E1020`: module/package import contract
  - `E200x`: IR/lowering
  - `E300x`: backend/toolchain
- Codes are defined in `src/compiler/frontend/diagnostics_messages.hpp`.
- The `.ftl` localization files can translate by code (preferred) or by message key.

Packages:
- See `book/stdlib.md` for package modules and examples.
- Contract and ABI surface: `book/compiler-stdlib-contract.md`, `book/stdlib_abi_surface_v1.txt`.

Legacy flags (still supported):
- `mod api-diff` (alias of `mod contract-diff`)
- `--stdlib-profile` (legacy alias of `--runtime-profile`)
  - mapping: `minimal->core`, `kernel->system`, `full->desktop`
- `--parse-only`
- `--resolve-only`
- `--hir-only`
- `--mir-only`
- `--emit-cpp`
- `--strict-parse`

Notes:
- Commands set a default mode, but explicit flags still override.
- For strict parsing, use `parse --strict-parse <file>`.
- For graph JSON in CI: `vitte mod graph --json --from __root__ <file>`.
