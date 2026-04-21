# Vitte Errors

This page is the stable reference for diagnostics and common fixes.

Layer map:
- `parse`: syntax and structure checks
- `resolve`: name, type, import, and declaration checks
- `IR`: lowering and intermediate representation checks
- `backend`: code generation and toolchain checks

Each diagnostic follows the same structure:
- Code
- Symptôme
- Cause probable
- Correction
- Exemple
- Voir aussi

<a id="e0001"></a>
## E0001 - expected identifier

- Code: `E0001`
- Couche: `parse`
- Symptôme: the parser expected a name for a variable, type, module, or declaration.
- Cause probable: a declaration is missing its identifier, or the token after a keyword is not a valid name.
- Correction: add a valid identifier where the parser points.
- Exemple:
```vit
proc main() -> i32 { return 0 }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0002"></a>
## E0002 - expected expression

- Code: `E0002`
- Couche: `parse`
- Symptôme: the parser expected an expression at this location.
- Cause probable: a value, call, or block expression is missing after a keyword or operator.
- Correction: provide an expression such as `1`, `name`, `call()`, or `{ ... }`.
- Exemple:
```vit
let x = 1
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0003"></a>
## E0003 - expected pattern

- Code: `E0003`
- Couche: `parse`
- Symptôme: the parser expected a pattern.
- Cause probable: a `when` or match-like construct received an incomplete pattern.
- Correction: use a pattern such as an identifier or constructor, for example `Some(x)`.
- Exemple:
```vit
when x is Option.Some { return 0 }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0004"></a>
## E0004 - expected type

- Code: `E0004`
- Couche: `parse`
- Symptôme: the parser expected a type name.
- Cause probable: a type annotation is present, but the right-hand side is missing or malformed.
- Correction: use a built-in type or a named type, for example `int` or `Option[T]`.
- Exemple:
```vit
proc id(x: int) -> int { return x }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0005"></a>
## E0005 - expected 'end'

- Code: `E0005`
- Couche: `parse`
- Symptôme: a block was opened but not closed with `end` or `.end`.
- Cause probable: a `proc`, `form`, or `pick` block is missing its terminator.
- Correction: add the missing terminator for the construct you opened.
- Exemple:
```vit
form Point
  field x as int
.end
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0006"></a>
## E0006 - expected proc after attribute

- Code: `E0006`
- Couche: `parse`
- Symptôme: an attribute must be followed by a proc declaration.
- Cause probable: the attribute is attached to the wrong item or stands alone.
- Correction: place the attribute directly above a `proc`.
- Exemple:
```vit
#[inline]
proc add(a: int, b: int) -> int { return a + b }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0007"></a>
## E0007 - expected top-level declaration

- Code: `E0007`
- Couche: `parse`
- Symptôme: the parser expected a top-level declaration.
- Cause probable: an item appears at the file root, but it is not one of the supported declarations.
- Correction: keep top-level items to `space`, `use`, `form`, `pick`, `type`, `const`, `proc`, and `entry`.
- Exemple:
```vit
space my/app
proc main() -> int { return 0 }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0008"></a>
## E0008 - duplicate pattern binding

- Code: `E0008`
- Couche: `parse`
- Symptôme: a pattern bound the same name more than once.
- Cause probable: the pattern reuses one binding in two positions.
- Correction: give each binding a distinct name.
- Exemple:
```vit
when Pair(x, x) { return 0 }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0009"></a>
## E0009 - unknown type

- Code: `E0009`
- Couche: `resolve`
- Symptôme: a referenced type name was not found.
- Cause probable: the type is misspelled or not imported.
- Correction: check spelling or import the type with `use` or `pull`.
- Exemple:
```vit
use std/core/option.Option
proc f(x: Option[int]) -> int { return 0 }
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0010"></a>
## E0010 - unknown generic base type

- Code: `E0010`
- Couche: `resolve`
- Symptôme: the base type of a generic was not found.
- Cause probable: the generic base type is misspelled or not imported.
- Correction: check spelling or import the base type with `use` or `pull`.
- Exemple:
```vit
use std/core/option.Option
let x: Option[int] = Option.None
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0011"></a>
## E0011 - generic type requires at least one argument

- Code: `E0011`
- Couche: `resolve`
- Symptôme: a generic type was written without any type arguments.
- Cause probable: the type application is incomplete.
- Correction: provide one or more type arguments inside `[ ]`.
- Exemple:
```vit
let x: Option = Option.None
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0012"></a>
## E0012 - unsupported type

- Code: `E0012`
- Couche: `resolve`
- Symptôme: this type form is not supported yet.
- Cause probable: the type syntax is valid enough to parse, but the compiler does not accept this form yet.
- Correction: use a supported type form such as built-ins, named types, pointers, slices, or proc types.
- Exemple:
```vit
let p: *int = &value
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0013"></a>
## E0013 - unknown identifier

- Code: `E0013`
- Couche: `resolve`
- Symptôme: a referenced name was not found in the current scope.
- Cause probable: the symbol is misspelled, out of scope, or not imported.
- Correction: check spelling, or import it from a module with `use` or `pull`.
- Exemple:
```vit
use std/bridge/print.print
proc main() -> int { print("hi"); return 0 }
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0014"></a>
## E0014 - invoke has no callee

- Code: `E0014`
- Couche: `parse`
- Symptôme: an invocation is missing its callee.
- Cause probable: the call expression starts with arguments instead of a function or proc name.
- Correction: provide a function or proc name before the arguments.
- Exemple:
```vit
print("hi")
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0015"></a>
## E0015 - unsupported expression in HIR

- Code: `E0015`
- Couche: `IR`
- Symptôme: this expression is not supported by the HIR lowering yet.
- Cause probable: the expression shape reaches lowering, but the lowering path does not implement it yet.
- Correction: rewrite the expression using supported constructs.
- Exemple:
```vit
let x = value
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0016"></a>
## E0016 - unsupported pattern in HIR

- Code: `E0016`
- Couche: `IR`
- Symptôme: this pattern is not supported by the HIR lowering yet.
- Cause probable: the pattern reaches lowering, but the lowering path does not implement it yet.
- Correction: rewrite the pattern using supported constructs.
- Exemple:
```vit
when x is Option.Some { return 0 }
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0017"></a>
## E0017 - unsupported statement in HIR

- Code: `E0017`
- Couche: `IR`
- Symptôme: this statement is not supported by the HIR lowering yet.
- Cause probable: the statement shape reaches lowering, but the lowering path does not implement it yet.
- Correction: rewrite the statement using supported constructs.
- Exemple:
```vit
return 0
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0018"></a>
## E0018 - extern proc cannot have a body

- Code: `E0018`
- Couche: `resolve`
- Symptôme: an extern procedure cannot define a body.
- Cause probable: `#[extern]` and a body were combined on the same proc.
- Correction: remove the body or drop `#[extern]` if you want to implement it here.
- Exemple:
```vit
#[extern]
proc puts(s: string) -> int
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0019"></a>
## E0019 - proc requires a body unless marked #[extern]

- Code: `E0019`
- Couche: `resolve`
- Symptôme: a procedure must have a body unless marked `#[extern]`.
- Cause probable: the declaration ends without a body or the extern marker is missing.
- Correction: add a body with `{ ... }` or mark the proc `#[extern]`.
- Exemple:
```vit
proc add(a: int, b: int) -> int { return a + b }
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0020"></a>
## E0020 - type alias requires a target type

- Code: `E0020`
- Couche: `resolve`
- Symptôme: a type alias must specify a target type.
- Cause probable: the alias definition stops after the name.
- Correction: provide the right-hand side of the alias.
- Exemple:
```vit
type Size = int
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0021"></a>
## E0021 - generic type requires at least one type argument

- Code: `E0021`
- Couche: `resolve`
- Symptôme: a generic type needs at least one type argument.
- Cause probable: the generic application is empty.
- Correction: provide type arguments inside `[ ]`.
- Exemple:
```vit
let xs: List = List.empty()
```
- Voir aussi: `docs/book/compiler-stdlib-contract.md`, `docs/book/cli.md`

<a id="e0022"></a>
## E0022 - unexpected HIR type kind

- Code: `E0022`
- Couche: `IR`
- Symptôme: the compiler encountered an unexpected HIR type kind.
- Cause probable: lowering reached a type shape that is not handled correctly.
- Correction: try a simpler type and report the issue if it still reproduces.
- Exemple:
```vit
let x: int = 0
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0023"></a>
## E0023 - unexpected HIR expr kind

- Code: `E0023`
- Couche: `IR`
- Symptôme: the compiler encountered an unexpected HIR expression kind.
- Cause probable: lowering reached an expression shape that is not handled correctly.
- Correction: try a simpler expression and report the issue if it still reproduces.
- Exemple:
```vit
let x = 1
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0024"></a>
## E0024 - select requires at least one when branch

- Code: `E0024`
- Couche: `resolve`
- Symptôme: a `select` statement needs at least one `when` branch.
- Cause probable: the `select` block was opened but not populated with valid branches.
- Correction: add a `when` clause and optionally `otherwise`.
- Exemple:
```vit
select x
  when int(v) { return v }
otherwise { return 0 }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0025"></a>
## E0025 - select branch must be a when statement

- Code: `E0025`
- Couche: `resolve`
- Symptôme: each `select` branch must be a `when` statement.
- Cause probable: a branch uses a statement form that is not allowed in this position.
- Correction: replace the branch with a `when` pattern or use `otherwise`.
- Exemple:
```vit
select x
  when int(v) { return v }
otherwise { return 0 }
```
- Voir aussi: `docs/book/poche/07-lire-les-erreurs.md`, `docs/book/cli.md`

<a id="e0026"></a>
## E0026 - unexpected HIR stmt kind

- Code: `E0026`
- Couche: `IR`
- Symptôme: the compiler encountered an unexpected HIR statement kind.
- Cause probable: lowering hit a statement shape that should not reach this path.
- Correction: try a simpler statement and report it as a compiler bug if it reproduces.
- Exemple:
```vit
return 0
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0027"></a>
## E0027 - unexpected HIR pattern kind

- Code: `E0027`
- Couche: `IR`
- Symptôme: the compiler encountered an unexpected HIR pattern kind.
- Cause probable: lowering hit a pattern shape that should not reach this path.
- Correction: try a simpler pattern and report it as a compiler bug if it reproduces.
- Exemple:
```vit
when x is Option.None { return 0 }
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`

<a id="e0028"></a>
## E0028 - unexpected HIR decl kind

- Code: `E0028`
- Couche: `IR`
- Symptôme: the compiler encountered an unexpected HIR declaration kind.
- Cause probable: lowering hit a declaration shape that should not reach this path.
- Correction: try a simpler declaration and report it as a compiler bug if it reproduces.
- Exemple:
```vit
proc main() -> int { return 0 }
```
- Voir aussi: `docs/book/cli.md`, `docs/book/INDEX-technique.md`
