# Diagnostics

The active compiler diagnostics surface is Vitte-owned.

## Source of truth

- Structured diagnostics types and renderers live in `src/vitte/compiler/diagnostics/diagnostic.vit`.
- The seed path documented in `docs/seed_diagnostics.md` only describes the bootstrap stage0 surface.
- Ad-hoc diagnostics are rejected: public errors should be emitted through the shared `Diagnostic` model.

## Main commands

- `vitte check <file>`
- `vitte build <file> -o <out>`
- `vitte check --diagnostics-json <file>`
- `vitte --dump-ast-json <file>`
- `vitte --dump-hir-json <file>`
- `vitte --dump-mir-json <file>`

## Output formats

- Human text:
  - `error[CODE] phase: message`
  - labels, notes, helps, and suggestions are rendered below the primary header.
- JSON envelope:
  - `schema = "vitte.compiler.surface"`
  - `surface = "diagnostics"`
  - top-level validity and pipeline summary fields
  - `primary_report.diagnostics[]` for the flattened report
  - `phase_reports.<phase>` for per-phase diagnostics

## Stable phases

- `lexer`
- `parser`
- `ast`
- `ast_validation`
- `module_resolution`
- `symbol_resolution`
- `sema`
- `hir`
- `typeck`
- `borrowck`
- `lifetime`
- `mir`
- `mir_lowering`
- `mir_verification`
- `ir`
- `backend`
- `linker`
- `runtime_execution`
- `driver`
- `internal`

## Operational notes

- `pipeline_failed_at` points to the first failing stage for the current request.
- Input-hardening failures use explicit limit codes such as:
  - `LIMIT_FILE_SIZE_MAX`
  - `LIMIT_TOKEN_SIZE_MAX`
  - `LIMIT_AST_DEPTH_MAX`
  - `LIMIT_EXPR_DEPTH_MAX`
  - `LIMIT_IMPORT_DEPTH_MAX`
  - `LIMIT_MODULE_COUNT_MAX`
  - `LIMIT_DIAGNOSTICS_MAX`
  - `LIMIT_SYMBOL_COUNT_MAX`
  - `LIMIT_PARSER_RECURSION_MAX`
  - `LIMIT_MACRO_EXPANSION_MAX`
- `InvalidInput` is reserved for hostile or malformed input; ICE/internal failures should remain compiler bugs only.
