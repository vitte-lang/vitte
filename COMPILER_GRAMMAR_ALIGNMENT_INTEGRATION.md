# Compiler-Grammar Alignment - Integration Guide

## Executive Summary

The compiler-grammar alignment is now integrated into the real frontend and CI surface.

Use these real modules:

- `src/vitte/compiler/frontend/ast/*.vit`
- `src/vitte/compiler/frontend/parse/parser.vit`
- `src/vitte/compiler/frontend/grammar_alignment_checker.vit`
- `src/vitte/compiler/ir/ast.vit`
- `tools/grammar_alignment_checker.py`

Do not add new detached `ast_extended.vit` or `parser_extended.vit` modules. Those names were part of an older plan and are now superseded.

## Current Architecture

```text
Lexer tokens
  -> frontend/parse/parser.vit
       -> ParseResult counters + diagnostics
       -> grammar-surface recovery
  -> frontend/pipeline.vit
       -> FrontendOutput
  -> driver/backend gates

AST surface definitions:
  frontend/ast/*.vit
  ir/ast.vit

Alignment enforcement:
  tools/grammar_alignment_checker.py
  make grammar-alignment-test
```

## Integration Points

### Frontend Module

`src/vitte/compiler/frontend/mod.vit` exports:

- input
- source_map
- lexer
- parse
- ast
- macros
- grammar_alignment_checker
- pipeline

### Parser Pipeline

`src/vitte/compiler/frontend/pipeline.vit` calls:

```vit
let tokens = scan_tokens(expanded);
let parsed = parse_source(source_path, tokens);
```

`parse_source` returns grammar version, AST root label, structural counters, diagnostics, and validity.

### Alignment Gate

Run:

```sh
make grammar-alignment-test
```

The gate is also part of:

```sh
make compiler-gate
```

The checker reports:

- declaration coverage
- statement coverage
- expression coverage
- type coverage
- pattern coverage

It exits non-zero if any tracked grammar rule is `NotStarted` or `Partial`.

## Adding a New Grammar Rule

When the grammar grows:

1. Add parser support in `src/vitte/compiler/frontend/parse/parser.vit`.
2. Add or extend AST kind definitions in `src/vitte/compiler/frontend/ast/*.vit` and/or `src/vitte/compiler/ir/ast.vit`.
3. Add a valid fixture in `tests/frontend_syntax/valid/`.
4. Add invalid fixtures in `tests/frontend_syntax/invalid/` when recovery behavior matters.
5. Add the rule to `tools/grammar_alignment_checker.py`.
6. Run:

```sh
make grammar-alignment-test
make frontend-syntax-test
make compiler-gate
```

## Current Next Implementation Step

The parser now has broad structural coverage. The next implementation step is rich AST materialization:

- `parse_decl` should create declaration nodes.
- `parse_stmt` should create statement nodes.
- `parse_expr` should create expression nodes.
- `parse_type_expr` should create type nodes.
- `parse_pattern` should create pattern nodes.

The current counter-based `ParseResult` should remain as a compatibility layer while typed AST construction is introduced.

## Test Strategy

Existing tests:

- `tests/frontend_syntax/valid/full_grammar_surface.vit`
- `tests/frontend_syntax/valid/compiler_surface.vit`
- `tests/frontend_syntax/valid/stmt_expr_type_pattern.vit`
- `tests/frontend_syntax/invalid/*.vit`
- `tests/driver_report_runtime/*.report.json`

Core commands:

```sh
make frontend-syntax-test
make driver-report-runtime-test
make grammar-alignment-test
make compiler-gate
```

## CI Notes

The driver runtime workflow watches frontend, driver, analysis, backend, grammar-alignment tool, syntax checker, and frontend syntax fixture paths.

If the grammar surface changes, update both:

- `tools/grammar_alignment_checker.py`
- `tests/frontend_syntax/valid/full_grammar_surface.vit`

This keeps docs, parser, AST declarations, and tests moving together.
