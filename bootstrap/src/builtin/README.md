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
- Constants declare a stable name, type, and literal metadata.
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
- `u8`
- `u32`
- `u64`
- `usize`
- `int`
- `i64`
- `f32`
- `f64`
- `string`
- `never`
- `error`

Builtin constants:
- `true: bool`
- `false: bool`

Builtin functions:
- `print(string) -> void`
- `println(string) -> void`
- `eprint(string) -> void`
- `eprintln(string) -> void`
- `panic(string) -> never`
- `assert(bool) -> void`
- `len(string) -> usize`
- `to_string(int) -> string`
- `type_name(error) -> string`

Builtin operators:
- Numeric binary: `+`, `-`, `*`, `/`, `%`
- Integer binary: `&`, `|`, `^`, `<<`, `>>`
- Comparison binary: `==`, `!=`, `<`, `<=`, `>`, `>=`
- Logical binary: `&&`, `||`
- Unary: `!`, `+`, `-`, `~`

Lookup behavior:
- Exact string lookup is used.
- Missing symbols return `NULL` or a non-OK lookup result and set `last_error`.
- Successful lookup resets `last_error`.
- `vitte_builtin_operator_result_type` returns the declared result type or the operand type for arithmetic/integer operators.

Limitations:
- Lookup is linear, which is acceptable for bootstrap.
- Function signatures currently model one simple parameter type plus arity metadata; richer signatures belong in the later type/sema layer.
- Numeric operators require matching operand types in this bootstrap registry.
- Future `sema`, `type`, `symbol`, and `scope` layers should consume this registry instead of duplicating builtin definitions.
