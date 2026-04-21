# Grammar Changelog

## 2026-03-16

### non-breaking
- Documented explicit generic call surface syntax in EBNF as `foo[T](...)`.
- Added an ambiguity note for generic-call vs index-then-call parsing.

### breaking
- None.

### diagnostic-only
- None.

## 2026-03-03

### non-breaking
- Added generated `grammar-surface.ebnf` workflow with versioned metadata header.
- Added grammar corpus scaffold (`tests/grammar/valid`, `tests/grammar/invalid`).
- Added diagnostics contracts and snapshots for invalid grammar examples.

### breaking
- None.

### diagnostic-only
- Established wording/code snapshots for parse diagnostics in grammar corpus.
