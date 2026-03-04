# VITTE Grammar Reference

Source of truth: `src/vitte/grammar/vitte.ebnf`.

Generated artifacts must not be edited manually:
- `book/grammar/grammar-surface.ebnf`
- `book/grammar/vitte.ebnf`
- `book/grammar-surface.ebnf`

## Standard Commands

```bash
make grammar-sync
make grammar-check
make grammar-test
make grammar-docs
make grammar-gate
bin/vitte grammar check
```

## Change Policy

- Breaking grammar change: updates accepted syntax or parse tree shape.
- Non-breaking grammar change: refactor/factorization with same accepted surface.
- Diagnostic-only change: grammar same, parse error wording/code changed.

Record every grammar change in `book/grammar/grammar-changelog.md`.

## PR Checklist (Grammar)

- Rule changed in `src/vitte/grammar/vitte.ebnf` only.
- `make grammar-sync` executed.
- Valid/invalid corpus updated under `tests/grammar`.
- Expected diagnostics updated in `book/grammar/diagnostics/expected`.
- Snapshots refreshed if required.
- `make grammar-gate` is green.
