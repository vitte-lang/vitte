# Vitte 1.x Breaking Migration Guide

This guide covers breaking syntax normalization in Vitte 1.x and the fully automated migration path.

## Breaking Changes

1. `loop` removed from stable grammar.
2. `otherwise` removed from stable grammar.
3. `for ... in` finalized end-to-end (parser -> HIR lowering -> backends).
4. Import model normalized around `use` as principal keyword.
5. `pull` and `share` marked deprecated (`--syntax-strict` forbids them).

## Diagnostics

- `E1101`: `loop` removed. Quickfix: `while true { ... }`
- `E1102`: `otherwise` removed. Quickfix: `else`
- `E1103`: `return` removed. Quickfix: `give`
- `E1104`: `pull` removed. Quickfix: `use`
- `E1105`: `share` removed. Quickfix: remove declaration
- `W1106` / `E1106`: symbolic `!` deprecated / strict-forbidden. Quickfix: `not`
- `W1107` / `E1107`: symbolic `&&` deprecated / strict-forbidden. Quickfix: `and`
- `W1108` / `E1108`: symbolic `||` deprecated / strict-forbidden. Quickfix: `or`
- `E1109`: `for-in` scalar literal iterable rejected

## 100% Auto Migration Flow

1. Repository-wide codemod:

```bash
bin/vitte mod migrate-imports --write --roots src,tests,examples
```

2. File-level doctor autofix (optional for focused migration):

```bash
bin/vitte mod doctor --fix --write path/to/file.vit
```

3. Strict verification pass:

```bash
bin/vitte check path/to/root.vit --syntax-strict
```

## Codemod Coverage

The migration tooling rewrites:

- `while !(a >= b)` -> `while a < b`
- `loop { ... }` -> `while true { ... }`
- `otherwise` -> `else`
- `return` -> `give`
- `pull ...` -> `use ...`
- `share ...` -> removed line
- `&&` -> `and`, `||` -> `or`, `!x` -> `not x`
- legacy import paths -> canonical import paths

## Recommended Rollout

1. Run codemod on `tests` and `examples` first.
2. Run codemod on `src`.
3. Run `make build`.
4. Run `make grammar-test` and `make modules-tests`.
5. Enable `--syntax-strict` in CI.
