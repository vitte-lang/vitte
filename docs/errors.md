# Vitte Errors

This page documents diagnostic codes and common fixes.

## E0001: expected identifier

**Summary**  
The parser expected a name for something (variable, type, module, etc.).

**Fix**  
Add a valid identifier where the error points (letters, digits, and `_` after the first character).

**Example**
```
proc main() -> i32 { return 0 }
```

## E0005: expected 'end'

**Summary**  
A block was opened but not closed with `end` or `.end`.

**Fix**  
Add the missing terminator for the construct you opened (for example: `end` for procs, `.end` for form/pick blocks).

**Example**
```
form Point
  field x as int
.end
```

## E0002: expected expression

**Summary**  
The parser expected an expression at this location.

**Fix**  
Provide a value, call, or block expression (e.g., `1`, `name`, `call()`, `{ ... }`).

**Example**
```
proc main() -> i32 { return 0 }
```

## E0004: expected type

**Summary**  
The parser expected a type name.

**Fix**  
Use a built-in type (int, bool, string) or a named type (e.g., `Option[T]`).

**Example**
```
proc id(x: int) -> int { return x }
```

## E0013: unknown identifier

**Summary**  
A referenced name was not found in the current scope.

**Fix**  
Check spelling, or import it from a module with `use` or `pull`.

**Example**
```
use std/io/print.print
proc main() -> int { print("hi"); return 0 }
```

## E0018: extern proc cannot have a body

**Summary**  
An extern procedure cannot define a body.

**Fix**  
Remove the body or drop `#[extern]` if you want to implement it here.

**Example**
```
#[extern]
proc puts(s: string) -> int
```

## E0019: proc requires a body unless marked #[extern]

**Summary**  
A procedure must have a body unless marked `#[extern]`.

**Fix**  
Add a body with `{ ... }` or mark it `#[extern]` if it is provided by the runtime.

**Example**
```
proc add(a: int, b: int) -> int { return a + b }
```

## E0020: type alias requires a target type

**Summary**  
A type alias must specify a target type.

**Fix**  
Provide the right-hand side of the alias.

**Example**
```
type Size = int
```

## E0024: select requires at least one when branch

**Summary**  
A select statement needs at least one when branch.

**Fix**  
Add a `when` clause (and optionally `otherwise`).

**Example**
```
select x
  when int(v) { return v }
otherwise { return 0 }
```

## E0026: unexpected HIR stmt kind

**Summary**  
The compiler encountered an unexpected HIR statement kind.

**Fix**  
This is likely a compiler bug; try a simpler statement and report it.

**Example**
```
return 0
```

## E0027: unexpected HIR pattern kind

**Summary**  
The compiler encountered an unexpected HIR pattern kind.

**Fix**  
This is likely a compiler bug; try a simpler pattern and report it.

**Example**
```
when x is Option.None { return 0 }
```

## E0028: unexpected HIR decl kind

**Summary**  
The compiler encountered an unexpected HIR declaration kind.

**Fix**  
This is likely a compiler bug; try a simpler declaration and report it.

**Example**
```
proc main() -> int { return 0 }
```

## E0025: select branch must be a when statement

**Summary**  
Each select branch must be a `when` statement.

**Fix**  
Replace the branch with a `when` pattern (or use `otherwise`).

**Example**
```
select x
  when int(v) { return v }
otherwise { return 0 }
```

## E0003: expected pattern

**Summary**  
The parser expected a pattern.

**Fix**  
Use a pattern like an identifier or constructor (e.g., `Some(x)`).

**Example**
```
when x is Option.Some { return 0 }
```

## E0006: expected proc after attribute

**Summary**  
An attribute must be followed by a proc declaration.

**Fix**  
Place the attribute directly above a proc.

**Example**
```
#[inline]
proc add(a: int, b: int) -> int { return a + b }
```

## E0007: expected top-level declaration

**Summary**  
The parser expected a top-level declaration.

**Fix**  
Top-level items include `space`, `use`, `form`, `pick`, `type`, `const`, `proc`, and `entry`.

**Example**
```
space my/app
proc main() -> int { return 0 }
```

## E0008: duplicate pattern binding

**Summary**  
A pattern bound the same name more than once.

**Fix**  
Use distinct names for each binding in the pattern.

**Example**
```
when Pair(x, y) { return 0 }
```

## E0009: unknown type

**Summary**  
A referenced type name was not found.

**Fix**  
Check spelling or import the type with `use` or `pull`.

**Example**
```
use std/core/option.Option
proc f(x: Option[int]) -> int { return 0 }
```

## E0010: unknown generic base type

**Summary**  
The base type of a generic was not found.

**Fix**  
Check spelling or import the base type with `use` or `pull`.

**Example**
```
use std/core/option.Option
let x: Option[int] = Option.None
```

## E0011: generic type requires at least one argument

**Summary**  
A generic type must include at least one argument.

**Fix**  
Provide one or more type arguments inside `[ ]`.

**Example**
```
let x: Option[int] = Option.None
```

## E0012: unsupported type

**Summary**  
This type form is not supported yet.

**Fix**  
Use a supported type (built-ins, named types, pointers, slices, proc types).

**Example**
```
let p: *int = &value
```

## E0014: invoke has no callee

**Summary**  
An invocation is missing its callee.

**Fix**  
Provide a function or proc name before the arguments.

**Example**
```
print("hi")
```

## E0015: unsupported expression in HIR

**Summary**  
This expression is not supported by the HIR lowering yet.

**Fix**  
Rewrite the expression using supported constructs.

**Example**
```
let x = value
```

## E0016: unsupported pattern in HIR

**Summary**  
This pattern is not supported by the HIR lowering yet.

**Fix**  
Rewrite the pattern using supported constructs.

**Example**
```
when x is Option.Some { return 0 }
```

## E0017: unsupported statement in HIR

**Summary**  
This statement is not supported by the HIR lowering yet.

**Fix**  
Rewrite the statement using supported constructs.

**Example**
```
return 0
```

## E0021: generic type requires at least one type argument

**Summary**  
A generic type needs at least one type argument.

**Fix**  
Provide type arguments inside `[ ]`.

**Example**
```
let xs: List[int] = List.empty()
```

## E0022: unexpected HIR type kind

**Summary**  
The compiler encountered an unexpected HIR type kind.

**Fix**  
This is likely a compiler bug; try a simpler type and report it.

**Example**
```
let x: int = 0
```

## E0023: unexpected HIR expr kind

**Summary**  
The compiler encountered an unexpected HIR expression kind.

**Fix**  
This is likely a compiler bug; try a simpler expression and report it.

**Example**
```
let x = 1
```
