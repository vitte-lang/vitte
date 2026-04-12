# prefer-give

Rule id: `prefer-give`

## Intent

Use `give` as the canonical return keyword in surface Vitte code.
`return` is legacy and removed from the stable grammar.

## Examples

Preferred:

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Legacy-compatible:

```vit
proc add(a: int, b: int) -> int {
  return a + b
}
```

## Quickfix

Replace `return expr` with `give expr` in non-entry code.
