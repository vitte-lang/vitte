# ast

Path: `src/vitte/compiler/frontend/ast`

## Purpose

Frontend pipeline: lexer, parser, AST, and syntax diagnostics.

## Notes

- Prefer explicit grammar-driven behavior.
- Keep diagnostics span-accurate (FileId + Span).
- Add parser/lexer regression fixtures with each change.
