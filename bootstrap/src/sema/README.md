# bootstrap/src/sema

The semantic analyzer validates the parsed AST before backend emission.

## Contract

- No dependency on `runtime/*`.
- Input is an initialized AST rooted at a module node.
- Type resolution uses `bootstrap/src/type`.
- Bindings use `bootstrap/src/symbol` and `bootstrap/src/scope`.
- Builtin operators and functions come from `bootstrap/src/builtin`.
- Optional constant evaluation uses `bootstrap/src/constant_fold`.
- Diagnostics go through `bootstrap/src/diagnostic`.
- Errors use `bootstrap/src/api/error.h`.

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
- `if` condition type validity
- callability, call arity, and procedure argument type compatibility
- builtin function argument compatibility
- operator operand compatibility
- canonical logical operator semantics for `and`, `or`, `&&`, and `||`

## Current Limits

- no user-defined aggregate types
- no mutation analysis
- no unreachable-code analysis
- export clauses are local-only in bootstrap; re-exporting imported module paths still belongs to a later stage
- imported modules are analyzed for bootstrap visibility and declaration validity only

## Driver Integration

The bootstrap driver now runs semantic analysis after AST validation and before
backend/codegen. Syntax-only success is no longer enough to reach C17 emission.
