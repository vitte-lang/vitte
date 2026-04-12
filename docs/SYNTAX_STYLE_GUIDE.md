# Syntax Style Guide

This guide defines the recommended canonical writing style for Vitte code.
It is stricter than parser acceptance.

## Control Flow

- Prefer `while` as the default loop form.
- Avoid `loop` in new code.
- Use `for ... in` only where backend lowering is confirmed stable for your target.

Preferred:

```vit
let i: int = 0
while i < items.len {
  i += 1
}
```

Avoid:

```vit
loop {
  if i >= items.len { break }
  i += 1
}
```

## While Conditions

- Avoid `while !(cond)`.
- Rewrite to a positive inverse condition.

Preferred: `while i < n { ... }`
Avoid: `while !(i >= n) { ... }`

## Increments And Assignment

- Prefer compound assignment:
  - `+=`, `-=`, `*=`, `/=`
- Avoid verbose self-assignments (`x = x + 1`, etc.).

## Return Keyword

- `give` is the canonical return keyword in surface code.
- `return` is accepted for compatibility but should be treated as legacy style in libraries/facades.

## Canonical Recommendation Summary

1. Use `while` with positive conditions.
2. Use compound assignment for arithmetic updates.
3. Use `give` for returns.
4. Keep `loop` and legacy forms out of new code unless explicitly required.
