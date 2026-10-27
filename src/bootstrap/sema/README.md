# compiler/src/sema

The semantic analyzer validates the parsed AST before backend emission.

## Contract

- No dependency on `runtime/*`.
- Input is an initialized AST rooted at a module node.
- Type resolution uses `compiler/src/type`.
- Bindings use `compiler/src/symbol` and `compiler/src/scope`.
- Builtin operators and functions come from `compiler/src/builtin`.
- Optional constant evaluation uses `compiler/src/constant_fold`.
- Diagnostics go through `compiler/src/diagnostic`.
- Errors use `compiler/src/api/error.h`.

## Current Checks

- duplicate import declarations and conflicting visible import names
- explicit export targets must resolve to local top-level declarations
- explicit export names must not collide with other exported names
- imported symbol visibility for direct, glob, and module-alias usage
- duplicate top-level `proc` and `const`
- unknown identifiers
- unknown type names
- return value compatibility
- `let` initializer compatibility
- exact and safe conversions are accepted across assignments, arguments,
  fields, returns, pointers and references; lossy conversions require casts
- integer and floating literals are context-typed only when their value is
  representable in the requested destination type
- generic parameter scopes for type, trait, impl, form and procedure bodies
- generic type arguments are resolved in the importing scope, checked for
  arity, interned as nominal instances, and substituted into form fields and
  member access; imported nominal identities remain distinct
- tuple/list declaration and `for` binding patterns
- `if` condition type validity
- callability, call arity, and procedure argument type compatibility
- builtin function argument compatibility
- operator operand compatibility
- canonical logical operator semantics for `and` and `or`

## Current Limits

- generic procedure instantiation and trait-bound enforcement remain a later
  pass; nominal type arguments and their field/member substitutions are
  already validated here
- match exhaustiveness and constructor pattern typing remain a later pass
- export clauses are local-only in compiler; re-exporting imported module paths still belongs to a later stage
- imported modules are analyzed for compiler visibility and declaration validity only

## Driver Integration

The compiler driver now runs semantic analysis after AST validation and before
backend/codegen. Syntax-only success is no longer enough to reach C17 emission.
