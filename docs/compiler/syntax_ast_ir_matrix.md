# Syntaxe -> AST -> IR Matrix (Core)

Source of truth:
- grammar: `src/vitte/grammar/vitte.ebnf`
- parser AST kinds: `src/compiler/frontend/ast.hpp`
- lowering: `src/compiler/frontend/lower_hir.cpp`

| Syntax construct | AST kind(s) | HIR lowering note |
| --- | --- | --- |
| `space ...` | `SpaceDecl` | module metadata only |
| `pull ...` | `PullDecl` | module metadata only |
| `use ...` | `UseDecl` | import bindings |
| `share ...` | `ShareDecl` | export surface metadata |
| `const name = expr` | `ConstDecl` | lowered as value declaration |
| `type T = U` | `TypeAliasDecl` | resolved in type phase |
| `form T { ... }` | `FormDecl` | constructor/type symbol |
| `pick T { ... }` | `PickDecl` | variant/type symbol |
| `proc f(...) { ... }` | `ProcDecl` | lowered to HIR proc |
| `entry main at ... { ... }` | `EntryDecl` | lowered as entry proc |
| `let name = expr` | `LetStmt` | HIR let statement |
| `asm("...")` | `AsmStmt` | lowered as intrinsic call |
| `unsafe { ... }` | `UnsafeStmt` | lowered as unsafe markers + block |
| `make name = expr` | `MakeStmt` | mutable binding |
| `set name = expr` | `SetStmt` | assignment |
| `give expr` | `GiveStmt` | return-with-value equivalent |
| `emit expr` | `EmitStmt` | side-effect emission |
| `if cond { ... } else { ... }` | `IfStmt` | conditional control-flow |
| `loop { ... }` | `LoopStmt` | loop control-flow |
| `while cond { ... }` | `LoopStmt` + guard `IfStmt` | loop control-flow |
| `for x in expr { ... }` | `ForStmt` | iteration lowering |
| `break` | `BreakStmt` | loop exit |
| `continue` | `ContinueStmt` | loop continue |
| `select expr when Pattern { ... } otherwise { ... }` | `SelectStmt` + `WhenStmt` | select lowering |
| `match expr { case ... }` | `SelectStmt` + `WhenStmt` | match/select normalized form |
| `when expr is pattern { ... }` | `SelectStmt` + `WhenStmt` | single-arm select form |
| `return` / `return expr` | `ReturnStmt` | function return |
| `lhs op rhs` | `BinaryExpr` | op-dependent lowering |
| unary `-expr`, `not expr` | `UnaryExpr` | unary lowering |
| call `f(args)` | `InvokeExpr` | call lowering |
| member/index `a.b`, `a[i]` | `MemberExpr`, `IndexExpr` | resolved then lowered |
| cast/pattern test `expr as T`, `expr is P` | `AsExpr`, `IsExpr` | lowered to HIR/MIR; `is` currently behaves as a boolean pattern test expression |
| literals (`int`, `bool`, `string`, list) | `LiteralExpr`, `ListExpr` | constant lowering |
| patterns | `IdentPattern`, `CtorPattern` | match lowering |
| types (`proc`, pointer, slice, generic) | `ProcType`, `PointerType`, `SliceType`, `GenericType`, `NamedType`, `BuiltinType` | lowered to HIR/MIR; pointers flow as `ptr<T>` internally |
