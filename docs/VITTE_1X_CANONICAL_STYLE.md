# Vitte 1.x Canonical Style

This document defines the canonical source style for Vitte 1.x codebases.

## Scope

- Applies to `.vit` sources in `src/`, `tests/`, and `examples/`.
- Parser compatibility can keep legacy forms temporarily, but canonical style must be used in maintained code.

## Canonical Rules

1. Use `else`, not `otherwise`.
2. Prefer simplified loop guards:
   - Canonical: `while a < b { ... }`
   - Legacy (to rewrite): `while !(a >= b) { ... }`
3. Compound assignment is canonical in statement form:
   - `x += 1`, `x -= 1`, `x *= 2`, `x /= 2`, `x %= 2`
4. `set` remains accepted for compatibility:
   - `set x += 1` is valid, but plain statement form is preferred in new code.
5. Use `give` as the canonical return keyword:
   - Legacy `return` is removed from the stable grammar.
6. Use `use` as the canonical import primitive:
   - Legacy `pull` is removed from stable grammar and rewritten to `use`.
   - Legacy `share` declarations are removed; surface export is inferred.
7. Use keyword boolean operators:
   - Canonical: `not`, `and`, `or`
   - Legacy symbolic forms `!`, `&&`, `||` are deprecated (strict-forbidden).

## Auto-fix (Codemod)

Use:

```bash
bin/vitte mod migrate-imports --write --roots src,tests,examples
```

Current codemod rewrites:

- Legacy import paths to canonical import paths.
- `while !(a >= b)` to `while a < b`.
- `loop { ... }` to `while true { ... }`.
- `otherwise` to `else`.
- `return` to `give`.
- `pull ...` to `use ...`.
- `share ...` line removal.
- `&&` to `and`, `||` to `or`, `!x` to `not x`.

For single-file lint/fix:

```bash
bin/vitte mod doctor --fix --write path/to/file.vit
```

## Notes

- `loop` and `otherwise` are removed from the stable grammar and raise hard parser errors with quickfixes.
- `return` is removed from the stable grammar and raises hard parser errors with quickfixes.
- Canonical style is enforced operationally through `mod doctor` lint findings and codemod runs.
- Migration deprecations:
  - `E1101`: `loop` removed, quickfix `while true { ... }`
  - `E1102`: `otherwise` removed, quickfix `else`
  - `E1103`: `return` removed, quickfix `give <expr>`
  - `E1104`: `pull` removed, quickfix `use ...`
  - `E1105`: `share` removed, quickfix remove declaration
  - `W1106`/`E1106`: symbolic `!` deprecated / strict-forbidden, quickfix `not <expr>`
  - `W1107`/`E1107`: symbolic `&&` deprecated / strict-forbidden, quickfix `and`
  - `W1108`/`E1108`: symbolic `||` deprecated / strict-forbidden, quickfix `or`
  - `E1109`: `for-in` scalar literal iterable rejected (expects iterable expression)
