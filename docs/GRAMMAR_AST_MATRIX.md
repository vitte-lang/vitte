# Grammar ↔ AST Matrix (Core)

This matrix documents the core surface accepted by `src/vitte/grammar/vitte.ebnf`
and the corresponding AST node families produced by the frontend parser.

## Top-Level

| Syntax | AST |
|---|---|
| `space` | `SpaceDecl` |
| `pull` | `PullDecl` |
| `use` | `UseDecl` |
| `share` | `ShareDecl` |
| `const` | `ConstDecl` |
| `type` | `TypeAliasDecl` |
| `form` | `FormDecl` |
| `pick` | `PickDecl` |
| `proc` | `ProcDecl` |
| `entry` | `EntryDecl` |

## Statements

| Syntax | AST |
|---|---|
| `let` | `LetStmt` |
| `make` | `MakeStmt` |
| `set` | `SetStmt` |
| `give` | `GiveStmt` |
| `emit` | `EmitStmt` |
| `if/else/otherwise` | `IfStmt` |
| `loop` | `LoopStmt` |
| `for ... in ...` | `ForStmt` |
| `break` | `BreakStmt` |
| `continue` | `ContinueStmt` |
| `match` | `SelectStmt` with `WhenStmt` branches |
| `when <expr> is <pattern>` | `SelectStmt` with one `WhenStmt` |
| `return` | `ReturnStmt` |
| expression statement | `ExprStmt` |

## Expressions

| Syntax | AST |
|---|---|
| literals | `LiteralExpr` |
| identifier | `IdentExpr` |
| unary `not` / `!` / `-` | `UnaryExpr` |
| binary core operators | `BinaryExpr` |
| `x as T` | `AsExpr` |
| `x is P` | `IsExpr` |
| call | `InvokeExpr` |
| member access | `MemberExpr` |
| index access | `IndexExpr` |
| list literal | `ListExpr` |

## Patterns and Types

| Syntax | AST |
|---|---|
| identifier pattern | `IdentPattern` |
| qualified constructor pattern | `CtorPattern` |
| named type | `NamedType` |
| generic type | `GenericType` |
| pointer type | `PointerType` |
| slice type | `SliceType` |
| proc type | `ProcType` |
| builtin type | `BuiltinType` |

## Guardrails

- `--strict-core` enables explicit rejection of non-core syntax (`E0014..E0016`).
- `vitte grammar diff` compares EBNF terminals with frontend keyword/operator tables.
- `make core-language-gate` enforces grammar sync, grammar tests, strict core corpus, and core syntax lint.

