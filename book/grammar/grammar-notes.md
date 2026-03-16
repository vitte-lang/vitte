# Grammar Notes

## Known ambiguities and resolution

- `if` expression vs `if` statement:
  - Resolution: statement is parsed in block statement context, expression in expression context.
- `when` clause vs `when ... is` match guard:
  - Resolution: `when <expr> is <pattern>` is preferred when `is` token is present.
- `use` simple path vs grouped path:
  - Resolution: parser reads `.{` as group start and `.*` as glob import.
- generic call vs index-then-call:
  - Surface syntax allows `foo[T](...)`.
  - Resolution today: the parser only commits to a generic call when the bracket payload is an unambiguous type list; otherwise it stays on normal indexing followed by call.
  - Example: `id[int](1)` is a generic call, while `id[i](1)` stays `index` then `call`.

## Top-level vs Statement vs Expression matrix

| Construct | Top-level | Statement | Expression |
|---|---:|---:|---:|
| `proc` | yes | no | yes (`proc_expr`) |
| `entry` | yes | no | no |
| `if` | no | yes | yes |
| `match` | no | yes | no |
| `emit` | no | yes | no |
| call (`foo()`) | no | yes (`expr_stmt`) | yes |

## Reading guide (quick)

1. Start from `program` and `toplevel`.
2. Validate `stmt` boundaries before debugging expression precedence.
3. Use precedence table in `precedence.md` when operator parse looks wrong.
4. Reproduce with a minimal file in `tests/grammar`.
5. Confirm diagnostics contract (`book/grammar/diagnostics/expected`).
