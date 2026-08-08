# bootstrap/src/symbol

The symbol layer stores semantic bindings for the bootstrap compiler.

## Contract

- No dependency on `runtime/*`.
- Symbol entries are fixed-capacity and stable after insertion.
- Names are borrowed, not copied.
- Procedure symbols own a local procedure type descriptor.
- Errors use `bootstrap/src/api/error.h`.

## Symbol Kinds

- top-level constants
- top-level procedures
- local bindings
- builtin constants
- builtin functions

## Notes

The symbol table does not enforce lexical scope rules by itself. Duplicate
rejection in the current scope is handled by `bootstrap/src/scope`.
