# Docs Changelog

## 2026-06-26 · Strict Native Bootstrap and Compiler Surface Gates

- Enforced strict native bootstrap behavior so compiler outputs for the real
  compiler entry no longer rely on a bootstrap bridge sidecar.
- Added and stabilized the `real-native` compiler gate for
  `src/vitte/compiler/main.vit`, including no-sidecar verification on emitted
  executables.
- Promoted native JSON compiler surfaces for AST, HIR, MIR, and diagnostics as
  required gates with richer payload content checks instead of presence-only
  checks.
- Expanded the stable compiler test-suite parser/check gate and hardened the
  compiler test-suite bridge path to run through a real host compiler binary
  without assuming `bin/vitte` only.
- Hardened the seed bootstrap parser heuristics so embedded source snippets in
  string literals do not trigger false diagnostics during bootstrap analysis.
- Closed the stage2 compiler reachability audit from the real entrypoint so
  `make build` now passes end-to-end under the stricter bootstrap rails.

## 2026-05-25 · Seed Diagnostics Surface

- Stage0 seed shell `toolchain/seed/vittec0.seed` now emits phase-explicit diagnostics for:
  `lexer`, `parser`, `ast_validation`, `module_resolution`, `symbol_resolution`,
  `sema`, `typeck`, `borrowck`, `mir_lowering`, `mir_verification`, `backend`,
  `linker`, and `runtime_execution`.
- Generic phase-less bootstrap failures are no longer accepted on the diagnostics
  surface.
- Lexer diagnostics now cover invalid character, unterminated string, invalid
  char literal, invalid number, unterminated comment, invalid unicode escape,
  and oversized token, with source byte hints and correction help.
- Parser diagnostics now expose expected token, found token, syntax context, and
  recovery status for the current bootstrap subset.
- Module diagnostics now expose import chain, tested paths, requester module,
  and a closest-name hint for the current bootstrap subset.
- Symbol diagnostics now expose current scope, active imports, closest symbol,
  and original declaration information for the current bootstrap subset.
- Canonical readable seed source `toolchain/seed/src/main.vit` now mirrors the
  same phase-oriented diagnostic model and exposes:
  `diag <file>`, `diag-json <file>`, and `check <file>` entry points.
- The seed Vitte JSON surface now includes `pipeline_failed_at`,
  `primary_report`, and `phase_reports`, aligning with the shell seed
  diagnostics contract.

## 2026-06-23 · Diagnostics JSON output

- `vitte check --format json` now emits the diagnostics envelope as JSON.
- Useful for CI, language servers, and translator tooling that consumes diagnostics.

See also:

- `docs/seed_diagnostics.md`
- `docs/bootstrap_seed.md`
- `docs/compiler/diagnostics.md`
