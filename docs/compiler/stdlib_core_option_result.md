# core.option and core.result

`vitte/stdlib/core/option` and `vitte/stdlib/core/result` define the standard
Vitte APIs for optional values and recoverable errors. Both modules belong to
`core`: they do not depend on the operating system and do not require dynamic
allocation.

## Option<T>

`Option<T>` is either `some(value)` or `none<T>()`.

The module provides:

- construction: `some`, `none`;
- predicates: `is_some`, `is_none`;
- extraction: `unwrap`, `expect`, `unwrap_or`, `unwrap_or_else`;
- transformation: `map`, `map_or`, `map_or_else`, `flatten`, `transpose`;
- chaining: `and`, `and_then`, `or`, `or_else`, `xor`, `filter`;
- in-place helpers: `take`, `replace`, `insert`, `get_or_insert`,
  `get_or_insert_with`;
- borrowing views: `as_ref`, `as_mut`;
- iteration: `option_iter`, `option_iter_next`.

`unwrap` and `expect` are explicit panic boundaries. Prefer `map`,
`and_then`, or `unwrap_or_else` when the absence of a value is expected.

## Result<T, E>

`Result<T, E>` is either `ok(value)` or `err(error)`.

The module provides:

- construction: `ok`, `err`;
- predicates: `is_ok`, `is_err`;
- extraction: `unwrap`, `unwrap_err`, `expect`, `expect_err`,
  `unwrap_or`, `unwrap_or_else`;
- transformation: `map`, `map_err`, `map_or`, `map_or_else`, `flatten`,
  `transpose`;
- chaining: `and`, `and_then`, `or`, `or_else`;
- borrowing views: `as_ref`, `as_mut`;
- iteration: `result_iter`, `result_iter_next`.

## Error Propagation

The official propagation helper is `propagate(result)`, also exposed as
`try_result(result)`. It returns the `ok` value and propagates the `err` value
through the language error path.

User-facing code should use `Result<T, E>` for recoverable failures and reserve
panic for violated invariants.
