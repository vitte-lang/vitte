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
