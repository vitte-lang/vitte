# Seed Diagnostics Surface

This page documents the current diagnostics contract implemented by the stage0
bootstrap seed in both forms:

- executable shell seed: `toolchain/seed/vittec0.seed`
- canonical readable Vitte source: `toolchain/seed/src/main.vit`

## Required Phases

Every seed diagnostic must declare a phase. The supported phase names are:

- `lexer`
- `parser`
- `ast_validation`
- `module_resolution`
- `symbol_resolution`
- `sema`
- `typeck`
- `borrowck`
- `mir_lowering`
- `mir_verification`
- `backend`
- `linker`
- `runtime_execution`

Phase-less bootstrap messages are not part of the accepted diagnostics surface.

## Lexer Coverage

The seed surface currently covers these lexer errors:

- `LEX_E_INVALID_CHAR`
- `LEX_E_UNTERMINATED_STRING`
- `LEX_E_INVALID_CHAR_LITERAL`
- `LEX_E_INVALID_ESCAPE`
- `LEX_E_INVALID_UNICODE`
- `LEX_E_INVALID_NUMBER`
- `LEX_E_UNTERMINATED_COMMENT`
- `LEX_E_TOKEN_TOO_LARGE`

Expected payload quality:

- exact failing character or token
- line and column
- byte-oriented hint when the shell surface can compute it
- direct correction help

## Parser Coverage

The bootstrap subset currently reports:

- `PARSE_E_TOPLEVEL_DECL_EXPECTED`
- `PARSE_E_INCOMPLETE_EXPR`
- `PARSE_E_UNCLOSED_BLOCK`
- `PARSE_E_MISSING_RPAREN`
- `PARSE_E_MISSING_COMMA`
- `PARSE_E_TYPE_EXPECTED`
- `PARSE_E_PATTERN_EXPECTED`
- `PARSE_E_BLOCK_EXPECTED`

Expected payload quality:

- expected token
- found token
- syntax context
- recovery applied or not

## Module Resolution Coverage

The bootstrap subset currently reports:

- `MOD_E_MODULE_NOT_FOUND`
- `MOD_E_IMPORT_NOT_FOUND`
- `MOD_E_IMPORT_CYCLE`
- `MOD_E_SYMBOL_NOT_EXPORTED`
- `MOD_E_PACKAGE_MISSING`
- `MOD_E_STDLIB_MISSING`
- `MOD_E_AMBIGUOUS_MODULE`

Expected payload quality:

- import chain
- tested paths
- requester module
- closest-name or closest-path hint

## Symbol Resolution Coverage

The bootstrap subset currently reports:

- `SEMA_E_UNKNOWN_IDENTIFIER`
- `SEMA_E_AMBIGUOUS_SYMBOL`
- `SEMA_E_DUPLICATE_SYMBOL`
- `SEMA_E_SHADOWING_FORBIDDEN`
- `SEMA_E_UNKNOWN_FIELD`
- `SEMA_E_UNKNOWN_VARIANT`
- `SEMA_E_UNKNOWN_FUNCTION`

Expected payload quality:

- current scope
- closest symbol
- original declaration for duplicates
- active imports

## Surfaces

### Shell seed

`toolchain/seed/vittec0.seed` exposes:

- human-readable text diagnostics
- JSON diagnostics with `schema = "vitte.compiler.surface"`
- `pipeline_failed_at`
- `primary_report`
- `phase_reports`

### Vitte seed

`toolchain/seed/src/main.vit` exposes:

- `diagnostics_text_for_source(source_name, source)`
- `diagnostics_json_for_source(source_name, source)`
- CLI-like entry points via `run_seed_cli(args)`
- `diag <file>`
- `diag-json <file>`
- `check <file>`

The Vitte surface is aligned structurally with the shell seed and is intended to
converge toward identical codes and messages for every covered bootstrap case.

