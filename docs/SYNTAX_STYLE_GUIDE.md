# Syntax Style Guide

This guide defines the recommended canonical writing style for Vitte code.
It is stricter than parser acceptance.

## Control Flow

- Prefer `while` as the default loop form.
- Avoid `loop` in new code.
- Treat `for ... in` as limited-use until lowering is fully stable on your target backend.
- Prefer `match/case` for value branching in new code.
- Keep `select/when` for compatibility-oriented contexts.

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
- Direct statement form is allowed and recommended for fluency: `x += 1` (no `set` required).
- Avoid verbose self-assignments (`x = x + 1`, etc.).
- In multiline calls/signatures/constructors, prefer trailing commas.

## Variable Role Clarity

- `let`: declaration (default).
- `let mut`: explicit mutable declaration (canonical style).
- `make`: compatibility mutable declaration form.
- `set`: reassignment.
- For long function types (`proc(...) -> ...`), extract named type aliases.
- For verbose generic/composite signatures, prefer alias-based compact forms.

Do not blur declaration and reassignment roles inside the same code path.

## String Interpolation

- Use standard interpolation form inside string literals: `"${name}"`.
- Current canonical interpolation scope is identifier/member-path interpolation (`${x}`, `${user.name}`).
- Keep interpolation expressions simple and explicit.

## Return Keyword

- `give` is the canonical return keyword in surface code.
- `return` is accepted for compatibility but should be treated as legacy style in libraries/facades.

## Block Syntax

- Use braces `{ ... }` as the single canonical block style.
- Avoid legacy terminators (`end`, `.end`) in new code.

## Branch Fallback Keyword

- Use `else` as canonical fallback keyword.
- `otherwise` remains compatibility syntax only.

## Pattern Matching

- Prefer `match/case` + optional guard for constrained branches (`case P if cond { ... }`).
- Use `_` as wildcard pattern when no binding is needed.
- Keep constructor destructuring homogeneous inside one arm.

## Logical Operators

- Prefer `not`, `and`, `or`.
- Avoid `&&` and `||` in canonical style.

## Operator Precedence

- Add parentheses whenever mixed operators could be ambiguous to a human reader.
- Do not rely on implicit precedence in critical or shared code.

## Canonical Recommendation Summary

1. Use `while` with positive conditions.
2. Treat `for` as constrained until lowering is stable for your targets.
3. Use `let`/`let mut`/`set` with explicit role separation (`make` in compatibility paths).
4. Prefer trailing commas in multiline lists.
5. Use braces and `else` in canonical source.
6. Prefer `match/case` over `select/when` for new value branching.
7. Prefer `use` as primary import primitive; keep `pull`/`share` minimal.
8. Prefer keyword logical operators and explicit parentheses.
9. Use compound assignments and `give` as canonical style.
