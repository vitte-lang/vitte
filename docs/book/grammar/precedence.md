# Priorité des opérateurs (Vitte 1.0)

Cette table correspond au parser actuel dans `src/compiler/frontend/parser.cpp`.
Les lignes les plus hautes ont la liaison la plus forte.

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

Notes :
- `as` est analysé avant tout opérateur binaire, donc il lie plus fort que les comparaisons et l'arithmétique.
- `is` est traité au même niveau de priorité que l'égalité.
- L'affectation est associative à droite (`a = b = c` se parse comme `a = (b = c)`).
