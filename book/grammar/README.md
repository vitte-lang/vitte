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
make core-language-test
make core-semantic-success
make grammar-docs
make grammar-gate
make core-language-gate
bin/vitte grammar check
```

## Maturity

Grammar coverage and implementation surface are broader than the stabilized language center.

- `stable`: behavior protected by the core language policy and core gate
- `experimental`: implemented or documented, but not yet protected as part of the language center
- `internal`: implementation detail, not a public language contract

References:

- `docs/LANGUAGE_CORE.md`
- `docs/LANGUAGE_CORE_CHECKLIST.md`
- `docs/LANGUAGE_CORE_COMPATIBILITY.md`
- `docs/LANGUAGE_CORE_GUARANTEES.md`
- `docs/LANGUAGE_CORE_TEST_PLAN.md`
- `docs/API_LIFECYCLE.md`

Focused semantic gate:

- `make core-semantic-success`
- `make core-semantic-snapshots`
- `tests/core_semantic_success_manifest.txt`
- `tests/diag_snapshots/core_semantic_manifest.txt`

## Change Policy

- Breaking grammar change: updates accepted syntax or parse tree shape.
- Non-breaking grammar change: refactor/factorization with same accepted surface.
- Diagnostic-only change: grammar same, parse error wording/code changed.

Record every grammar change in `book/grammar/grammar-changelog.md`.

## PR Checklist (Grammar)

- Rule changed in `src/vitte/grammar/vitte.ebnf` only.
- `make grammar-sync` executed.
- `make core-language-test` is green if the change touches the language center.
- Valid/invalid corpus updated under `tests/grammar`.
- Expected diagnostics updated in `book/grammar/diagnostics/expected`.
- Snapshots refreshed if required.
- `make grammar-gate` is green.
