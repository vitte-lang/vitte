# Bench Seeds

This directory contains small, hand-curated **Vitte** programs used as **seed corpora** for:

- lexer / parser smoke tests
- lowering + semantic analysis smoke tests
- formatter round-trip tests
- baseline benchmarks and regression reproduction

The seeds are intentionally portable: they avoid host IO dependencies and focus on syntax + semantics coverage.

## Conventions

- Blocks are delimited with `.end` (no `{}` braces).
- Keep programs deterministic.
- Prefer minimal dependencies (standalone files).

## Files

- `hello.vitte` — minimal end-to-end seed (strings, `let`, `if`, `ret`).
- `fib.vitte` — recursion + loops + arithmetic; good for codegen/runtime smoke.
- `exprs.vitte` — expression/precedence surface: operators, nesting, calls, indexing/dot (parser-only parts are commented).

## How seeds are used

Typical usage patterns:

- **Parser smoke**: run the compiler front-end over every `*.vitte` file.
- **Formatter**: format → parse → (optional) format idempotence check.
- **Sema**: ensure diagnostics are stable and no unexpected ICE occurs.

## Adding a new seed

Guidelines:

1. Keep it short (ideally < 200 LOC).
2. Maximize syntax/precedence coverage.
3. Avoid platform-specific features.
4. If a construct is not implemented yet, it can still be included as **parser-only** surface; comment out runtime calls.

## License

Part of the Vitte repository; see repository root license.
