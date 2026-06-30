# lexer

Path: `src/vitte/compiler/frontend/lexer`

## Purpose

Frontend pipeline: lexer, parser, AST, and syntax diagnostics.

## Notes

- Prefer explicit grammar-driven behavior.
- Keep diagnostics span-accurate (FileId + Span).
- Add parser/lexer regression fixtures with each change.
- Treat `frontend/lexer/token.vit` as the canonical token model for scanner, parser, and driver consumers.
- Keep the active scanner aligned with `src/vitte/grammar/vitte.ebnf`, including char literals and the operator surface `~`, `&&`, `||`, `<<=`, `>>=`.
