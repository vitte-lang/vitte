# Official Language Spec (Seed 0.1 Subset)

This freezes the supported subset for current professional gate.

## Syntax
- module header: `space <path>`
- imports: `use`, `from ... import`, `import`
- decls: `const`, `proc`, `form`, `pick`
- stmts subset: `let`, `set`, `give`, `if`, `loop/for/while`, `match`

## Semantics
- explicit export surface required (`export *` or explicit list per module policy)
- unresolved imports are errors
- circular imports are errors
- symbol shadowing/duplicate checks apply

## Type Rules
- minimal primitive typing (`int/string/bool/void`)
- return type consistency required
- unsafe implicit casts rejected

## Ownership/Borrow
- use-after-move rejected
- mut+shared alias rejected
- borrow-outlives marker rejected

## Backend Support
- only C backend is production-functional in this subset.

## Exemples pédagogiques
### Hello World
```vit
space main

proc main() -> void {
  give "Hello, Vitte!"
}
```
Ce programme illustre la structure minimale d'un module Vitte et la procédure d'entrée `main`.

### Déclaration et flux
```vit
space math

const PI = 3.14

proc area(radius: int) -> int {
  let result = PI * radius * radius
  give result
}
```
Cet exemple montre une constante, une procédure, et le calcul d'une valeur retournée.

### Ownership / borrowing
Pour ce sous-ensemble, l'idée clé est le contrôle des transferts d'alias et l'interdiction de `use-after-move`.
Un code correct doit éviter de prêter mutablement et d'utiliser une valeur après qu'elle a été déplacée.
