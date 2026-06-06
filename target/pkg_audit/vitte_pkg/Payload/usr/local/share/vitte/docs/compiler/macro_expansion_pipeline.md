# Macro Expansion Pipeline (170 Foundation)

Date: May 22, 2026

## Objective

Integrate macro expansion without destabilizing the frontend.

Target order:

`source -> lexer -> macro expand -> parser -> AST`

## Current Foundation

- Frontend performs a pre-expansion lexing baseline and a post-expansion lexing pass.
- Macro expansion emits:
  - transformed source text
  - expansion trace
  - diagnostics
  - recursion-limit validity flag
- Frontend output stores:
  - macro trace
  - macro expanded flag
  - diagnostics mapped to frontend source spans

## Span and Diagnostics Contract

- Macro diagnostics carry `line`, `column`, `width`.
- Frontend translates these into standard diagnostic spans.
- Diagnostics are merged with lexer/parser diagnostics in a unified output stream.

## Recursion Safety Contract

- Expansion pass computes a recursion depth signal.
- If depth exceeds configured limit, expansion is marked invalid and a fatal diagnostic is emitted.
- Frontend validity includes macro validity in addition to parser/AST validity.

## Known Gaps (Next Steps)

- Token-level hygiene is currently foundational (string-level contract first).
- Expansion trace is linear and human-readable; structured machine trace can be added later.
- Recursion detection is heuristic; full macro call-graph limits can replace it in advanced phase.
