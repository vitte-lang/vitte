# Vitte diagnostics explanations (English)
#
# Keys:
#   E0001.summary
#   E0001.fix
#   E0001.example

E0001.summary = The parser expected a name for something (variable, type, module, etc.).
E0001.fix = Add a valid identifier where the error points (letters, digits, and '_' after the first character).
E0001.example = proc main() -> i32 { return 0 }
E0002.summary = The parser expected an expression at this location.
E0002.fix = Provide a value, call, or block expression (e.g., 1, name, call(), { ... }).
E0002.example = proc main() -> i32 { return 0 }
E0004.summary = The parser expected a type name.
E0004.fix = Use a built-in type (int, bool, string) or a named type (e.g., Option[T]).
E0004.example = proc id(x: int) -> int { return x }
E0013.summary = A referenced name was not found in the current scope.
E0013.fix = Check spelling, or import it from a module with 'use' or 'pull'.
E0013.example = use std/io/print.print\nproc main() -> int { print(\"hi\"); return 0 }
E0018.summary = An extern procedure cannot define a body.
E0018.fix = Remove the body or drop #[extern] if you want to implement it here.
E0018.example = #[extern]\nproc puts(s: string) -> int
E0019.summary = A procedure must have a body unless marked #[extern].
E0019.fix = Add a body with { ... } or mark it #[extern] if it is provided by the runtime.
E0019.example = proc add(a: int, b: int) -> int { return a + b }
E0020.summary = A type alias must specify a target type.
E0020.fix = Provide the right-hand side of the alias.
E0020.example = type Size = int
E0021.summary = A generic type needs at least one type argument.
E0021.fix = Provide type arguments inside [ ].
E0021.example = let xs: List[int] = List.empty()
E0022.summary = The compiler encountered an unexpected HIR type kind.
E0022.fix = This is likely a compiler bug; try a simpler type and report it.
E0022.example = let x: int = 0
E0023.summary = The compiler encountered an unexpected HIR expression kind.
E0023.fix = This is likely a compiler bug; try a simpler expression and report it.
E0023.example = let x = 1
E0024.summary = A select statement needs at least one when branch.
E0024.fix = Add a when clause (and optionally otherwise).
E0024.example = select x\n  when int(v) { return v }\notherwise { return 0 }
E0025.summary = Each select branch must be a when statement.
E0025.fix = Replace the branch with a when pattern (or use otherwise).
E0025.example = select x\n  when int(v) { return v }\notherwise { return 0 }
E0026.summary = The compiler encountered an unexpected HIR statement kind.
E0026.fix = This is likely a compiler bug; try a simpler statement and report it.
E0026.example = return 0
E0027.summary = The compiler encountered an unexpected HIR pattern kind.
E0027.fix = This is likely a compiler bug; try a simpler pattern and report it.
E0027.example = when x is Option.None { return 0 }
E0028.summary = The compiler encountered an unexpected HIR declaration kind.
E0028.fix = This is likely a compiler bug; try a simpler declaration and report it.
E0028.example = proc main() -> int { return 0 }
E0003.summary = The parser expected a pattern.
E0003.fix = Use a pattern like an identifier or constructor (e.g., Some(x)).
E0003.example = when x is Option.Some { return 0 }
E0006.summary = An attribute must be followed by a proc declaration.
E0006.fix = Place the attribute directly above a proc.
E0006.example = #[inline]\nproc add(a: int, b: int) -> int { return a + b }
E0007.summary = The parser expected a top-level declaration.
E0007.fix = Top-level items include space, use, form, pick, type, const, proc, and entry.
E0007.example = space my/app\nproc main() -> int { return 0 }
E0008.summary = A pattern bound the same name more than once.
E0008.fix = Use distinct names for each binding in the pattern.
E0008.example = when Pair(x, y) { return 0 }
E0009.summary = A referenced type name was not found.
E0009.fix = Check spelling or import the type with 'use' or 'pull'.
E0009.example = use std/core/option.Option\nproc f(x: Option[int]) -> int { return 0 }
E0010.summary = The base type of a generic was not found.
E0010.fix = Check spelling or import the base type with 'use' or 'pull'.
E0010.example = use std/core/option.Option\nlet x: Option[int] = Option.None
E0011.summary = A generic type must include at least one argument.
E0011.fix = Provide one or more type arguments inside [ ].
E0011.example = let x: Option[int] = Option.None
E0012.summary = This type form is not supported yet.
E0012.fix = Use a supported type (built-ins, named types, pointers, slices, proc types).
E0012.example = let p: *int = &value
E0014.summary = An invocation is missing its callee.
E0014.fix = Provide a function or proc name before the arguments.
E0014.example = print(\"hi\")
E0015.summary = This expression is not supported by the HIR lowering yet.
E0015.fix = Rewrite the expression using supported constructs.
E0015.example = let x = value
E0016.summary = This pattern is not supported by the HIR lowering yet.
E0016.fix = Rewrite the pattern using supported constructs.
E0016.example = when x is Option.Some { return 0 }
E0017.summary = This statement is not supported by the HIR lowering yet.
E0017.fix = Rewrite the statement using supported constructs.
E0017.example = return 0
E0005.summary = A block was opened but not closed with 'end' or '.end'.
E0005.fix = Add the missing terminator for the construct you opened (for example: 'end' for procs, '.end' for form/pick blocks).
E0005.example = form Point\n  field x as int\n.end
