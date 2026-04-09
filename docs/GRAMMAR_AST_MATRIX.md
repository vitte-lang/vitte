# Grammar ↔ AST ↔ IR Matrix (Core v1)

This matrix documents `core-v1` syntax accepted by `src/vitte/grammar/vitte.ebnf`,
then maps each construct to AST and HIR shapes used by the compiler pipeline.

## Top-Level

| Syntax | AST | IR (HIR) |
|---|---|---|
| `space` | `SpaceDecl` | module metadata only |
| `pull` | `PullDecl` | import index only |
| `use` | `UseDecl` | import index only |
| `share` | `ShareDecl` | module export contract only |
| `const` | `ConstDecl` | global constant lowering |
| `type` | `TypeAliasDecl` | type table normalization |
| `form` | `FormDecl` | `FormDecl` |
| `pick` | `PickDecl` | `PickDecl` |
| `proc` | `ProcDecl` | `FnDecl` |
| `entry` | `EntryDecl` | `FnDecl` (entry wrapper) |

## Statements

| Syntax | AST | IR (HIR) |
|---|---|---|
| `let` | `LetStmt` | `LetStmt` |
| `asm("...")` | `AsmStmt` | lowered to runtime intrinsic call |
| `unsafe { ... }` | `UnsafeStmt` | lowered to unsafe boundary markers |
| `make` | `MakeStmt` | `LetStmt` (mutable lowering path) |
| `set` | `SetStmt` | `AssignStmt` |
| `give` | `GiveStmt` | `ReturnStmt` |
| `emit` | `EmitStmt` | side-effect statement |
| `if/else/otherwise` | `IfStmt` | `IfStmt` |
| `loop` | `LoopStmt` | `LoopStmt` |
| `while ...` | `LoopStmt` + guard `IfStmt` | `LoopStmt` |
| `for ... in ...` | `ForStmt` | `ForStmt` |
| `break` | `BreakStmt` | `BreakStmt` |
| `continue` | `ContinueStmt` | `ContinueStmt` |
| `select ... when ... otherwise ...` | `SelectStmt` + `WhenStmt` | `SelectStmt` |
| `match` | `SelectStmt` + `WhenStmt` | `SelectStmt` + lowered branches |
| `when <expr> is <pattern>` | `SelectStmt` + one `WhenStmt` | `SelectStmt` |
| `return` | `ReturnStmt` | `ReturnStmt` |
| expression statement | `ExprStmt` | `ExprStmt` |

## Expressions

| Syntax | AST | IR (HIR) |
|---|---|---|
| literals | `LiteralExpr` | `LiteralExpr` |
| identifier | `IdentExpr` | `VarExpr` |
| unary `not` / `!` / `-` | `UnaryExpr` | `UnaryExpr` |
| binary core operators | `BinaryExpr` | `BinaryExpr` |
| `x as T` | `AsExpr` | lowered through HIR/MIR |
| `x is P` | `IsExpr` | lowered as boolean pattern-test expression |
| call | `InvokeExpr` | `CallExpr` |
| member access | `MemberExpr` | `MemberExpr` |
| index access | `IndexExpr` | `IndexExpr` |
| list literal | `ListExpr` | lowered list construction call |

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

- `--syntax-profile core-v1` (or `--strict-core`) enables explicit rejection of non-core syntax (`E0014..E0016`).
- `vitte grammar diff` compares EBNF terminals with frontend keyword/operator tables.
- `make core-language-gate` enforces grammar sync, grammar tests, strict core corpus, core syntax lint, and core IR golden snapshots.
