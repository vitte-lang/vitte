# while-not

Rule id: `while-not`

## Intent

Avoid negated while guards such as `while !(cond) { ... }`.
Prefer a positive equivalent (`while cond_inverse { ... }`) to reduce cognitive load.

## Examples

Bad:

```vit
while !(i >= xs.len) {
  i += 1
}
```

Good:

```vit
while i < xs.len {
  i += 1
}
```

## Quickfix

Rewrite `while !(...)` using an equivalent positive condition.
