# const_eval

Path: `src/vitte/compiler/analysis/const_eval`

## Purpose

Semantic analysis phases: resolve, sema, typeck, borrowck, lint.

## Notes

- Keep phase boundaries explicit and testable.
- Emit structured diagnostics with stable codes.
- Ensure fatal conditions stop pipeline immediately.
