# Diagnostics
This page is generated. Edit the registry and re-run the generator:
- Registry: `docs/diagnostics/registry.json`
- Regenerate: `python3 tools/scripts/gen_diagnostics_docs.py`
## Errors

| Code | Name | Message |
|---|---|---|
| `V0001` | `unexpected-eof` | file ended unexpectedly |
| `V0002` | `unmatched-end` | unmatched `.end` |
| `V0003` | `missing-end` | missing `.end` to close this block |
| `V0004` | `syntax` | syntax error |

### `V0001` `unexpected-eof`

- Message: file ended unexpectedly
- Help: Check for missing blocks/terminators (e.g. missing `.end`) or unterminated literals.
- Examples:

```vitte
fn main()
  say "hello
.end
```

```vitte
fn main()
  say "hello"
.end
```

### `V0002` `unmatched-end`

- Message: unmatched `.end`
- Help: Remove the extra `.end`, or add the matching opener.
- Examples:

```vitte
.end
```

```vitte
fn main()
  say "ok"
.end
```

### `V0003` `missing-end`

- Message: missing `.end` to close this block
- Help: Add a matching `.end` for the open block.
- Examples:

```vitte
fn main()
  say "ok"
```

```vitte
fn main()
  say "ok"
.end
```

### `V0004` `syntax`

- Message: syntax error
- Help: Check the syntax near this location.
- Examples:

```vitte
fn main()
  set x 1
.end
```

```vitte
fn main()
  set x = 1
.end
```

## Warnings

| Code | Name | Message |
|---|---|---|
| `V1001` | `unused-binding` | unused binding |
| `V1002` | `shadowing` | binding shadows an outer binding |
| `V1003` | `unreachable` | unreachable statement |

### `V1001` `unused-binding`

- Message: unused binding
- Help: Remove it, or use it (read it) to silence this warning.
- Examples:

```vitte
fn main()
  set x = 1
  ret
.end
```

```vitte
fn main()
  set x = 1
  say x
  ret
.end
```

### `V1002` `shadowing`

- Message: binding shadows an outer binding
- Help: Rename the inner binding to avoid confusion, or use the outer binding directly.
- Examples:

```vitte
fn f(x: i32)
  when true
    set x = 1
  .end
  ret
.end
```

```vitte
fn f(x: i32)
  when true
    set y = 1
    say y
  .end
  ret
.end
```

### `V1003` `unreachable`

- Message: unreachable statement
- Help: Remove the unreachable code, or restructure control-flow so it can execute.
- Examples:

```vitte
fn main()
  ret
  say "never"
.end
```

```vitte
fn main()
  say "ok"
  ret
.end
```
