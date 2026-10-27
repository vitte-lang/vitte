# compiler/src/type

The type layer provides semantic type descriptors for the compiler compiler.
It resolves builtin names, represents procedure signatures, and centralizes
basic compatibility rules used by semantic analysis.

## Contract

- No dependency on `runtime/*`.
- Builtin names and kinds come from `compiler/src/builtin`.
- AST type references are resolved from `compiler/src/ast`.
- Errors use `compiler/src/api/error.h`.

## Supported Types

- builtin scalar types from the builtin registry
- `f32` and `f64` floating-point types with explicit narrowing policy
- procedure signatures with return type, arity, and parameter type list

Implicit compatibility is classified centrally as exact, safe, lossy or
incompatible.  Assignments, arguments, field initializers and returns accept
only exact and safe conversions.  The policy covers integer widening,
floating-point widening, representable integer-to-float conversions,
mutable-to-immutable pointer/reference/slice weakening, null pointers,
fixed-array to slice views, nominal identity and generic parameters.

## Current Limits

- semantic declaration identity is supplied by the symbol layer for imported
  nominal types; the type layer remains independent of sema
- generic parameter identity is preserved for later substitution; checking
  trait bounds and materializing generic argument substitutions remains in sema
