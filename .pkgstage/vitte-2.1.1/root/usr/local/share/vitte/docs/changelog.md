# Docs Changelog

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

See also:

- `docs/seed_diagnostics.md`
- `docs/bootstrap_seed.md`
- `docs/compiler/diagnostics.md`

