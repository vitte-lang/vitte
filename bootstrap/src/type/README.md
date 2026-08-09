# bootstrap/src/type

The type layer provides semantic type descriptors for the bootstrap compiler.
It resolves builtin names, represents procedure signatures, and centralizes
basic compatibility rules used by semantic analysis.

## Contract

- No dependency on `runtime/*`.
- Builtin names and kinds come from `bootstrap/src/builtin`.
- AST type references are resolved from `bootstrap/src/ast`.
- Errors use `bootstrap/src/api/error.h`.

## Supported Types

- builtin scalar types from the builtin registry
- procedure signatures with return type, arity, and parameter type list

Current compatibility rules are intentionally conservative:

- exact builtin matches always assign
- integer-to-integer assignment is accepted
- conditions accept `bool` and integer types

## Current Limits

- no user-defined struct, enum, or alias types
- no generic or pointer type constructors
