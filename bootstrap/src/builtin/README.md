# Bootstrap Builtin Registry

The builtin layer owns the static symbols that the bootstrap compiler can reason about before the full semantic/type system is implemented.

Invariants:
- The registry is static and deterministic.
- No allocation is required.
- Definition tables are immutable after initialization.
- Names are non-null, non-empty, and unique inside their table.
- Type kinds are stable and match `vitte_builtin_type_kind_t`.
- Function arity ranges are coherent.
- Operators declare kind, operand class, return type, precedence, and associativity.
- Errors use `bootstrap/src/api/error.h`.
- This layer does not depend on `runtime/*`, `sema`, `type`, `symbol`, or `scope`.

Lifecycle:
- Call `vitte_builtin_registry_init`.
- Optionally call `vitte_builtin_registry_validate`.
- Use lookup/count/index helpers.
- `vitte_builtin_registry_reset` restores the default static table bindings.

Builtin types:
- `void`
- `bool`
- `int`
- `i64`
- `string`
- `never`
- `error`

Builtin functions:
- `print(string) -> void`
- `println(string) -> void`
- `panic(string) -> never`
- `assert(bool) -> void`

Builtin operators:
- Numeric binary: `+`, `-`, `*`, `/`, `%`
- Comparison binary: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical binary: `&&`, `||`
- Logical unary: `!`

Lookup behavior:
- Exact string lookup is used.
- Missing symbols return `NULL` or a non-OK lookup result and set `last_error`.
- Successful lookup resets `last_error`.

Limitations:
- Lookup is linear, which is acceptable for bootstrap.
- Function signatures currently model one simple parameter type plus arity metadata.
- Operator result type for numeric arithmetic is resolved by the caller from operand type after `vitte_builtin_operator_accepts`.
- Future `sema`, `type`, `symbol`, and `scope` layers should consume this registry instead of duplicating builtin definitions.
