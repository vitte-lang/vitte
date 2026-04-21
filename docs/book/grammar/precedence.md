# Operator Precedence (Vitte 1.0)

This table matches the current parser in `src/compiler/frontend/parser.cpp`.
Higher rows bind tighter.

1. Postfix: call `f(x)`, index `a[b]`, member `a.b`, deref postfix `a.*`
2. Unary: `not` / `!`, unary `-`, address `&`, deref `*`
3. Cast: `as`
4. Multiplicative: `*` `/` `%`
5. Additive: `+` `-`
6. Shift: `<<` `>>`
7. Relational: `<` `<=` `>` `>=`
8. Equality: `==` `!=` and pattern test `is`
9. Bitwise AND: `&`
10. Bitwise XOR: `^`
11. Bitwise OR: `|`
12. Logical AND: `and` `&&`
13. Logical OR: `or` `||`
14. Assignment: `=`

Notes:
- `as` is parsed before any binary operator, so it binds tighter than comparisons and arithmetic.
- `is` is handled at the same precedence level as equality.
- Assignment is right-associative (`a = b = c` parses as `a = (b = c)`).
