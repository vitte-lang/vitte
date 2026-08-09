# Bootstrap Language Coverage

This document fixes the current Vitte language perimeter for the C17 bootstrap
against the grammar under `src/vitte/grammar`.

## Real Vitte Perimeter

Primary grammar entry points and reference families:

- `src/vitte/grammar/ebnf/grammar.ebnf`
- `src/vitte/grammar/ebnf/modules.ebnf`
- `src/vitte/grammar/ebnf/imports.ebnf`
- `src/vitte/grammar/ebnf/parameters.ebnf`
- `src/vitte/grammar/ebnf/expressions.ebnf`
- `src/vitte/grammar/reference/*`
- `src/vitte/grammar/specification/*`

The real language surface described there includes:

- module headers: `space path`
- imports: path, alias, group, glob
- exports
- procedures with modifiers, generics, parameters, return types, where clauses
- forms, picks, traits, impls, aliases, constants, statics, macros, externs
- blocks and structured statements
- patterns
- literals
- qualified paths
- unary, multiplicative, additive, shift, bitwise, comparison, equality, logical, assignment expressions
- attributes, visibility, generics, where clauses

## Bootstrap Frontend Contract

Current bootstrap frontend invariants:

- parser owns no syntax tree storage outside the AST arena
- module header and imports stay in the AST module root
- procedure parameters are explicit AST nodes
- type names and qualified identifiers are preserved as source text
- errors flow through `bootstrap/src/api/error.h` and `bootstrap/src/diagnostic`

## Coverage Matrix

Legend:

- `yes`: implemented
- `partial`: subset only
- `no`: absent

| Feature | Parser | Sema | HIR | IR | C17 |
| --- | --- | --- | --- | --- | --- |
| Module header `space path` | yes | partial | no | no | no |
| Simple import `use path` | yes | partial | no | no | no |
| Import alias `use path as name` | yes | partial | no | no | no |
| Import group / glob | yes | partial | no | no | no |
| Top-level `proc` | yes | yes | yes | yes | yes |
| Procedure parameters | yes | yes | yes | yes | yes |
| Procedure return type | yes | yes | yes | yes | yes |
| Procedure modifiers / generics / where | no | no | no | no | no |
| Top-level `const` | yes | yes | yes | yes | yes |
| `let` statements | yes | yes | yes | yes | yes |
| `if` / `else` | yes | yes | yes | yes | yes |
| Block statements | yes | yes | yes | yes | yes |
| `give` return statements | yes | yes | yes | yes | yes |
| Expression statements | no | no | no | no | no |
| Integer literals | yes | yes | yes | yes | yes |
| String literals | yes | yes | yes | yes | yes |
| Qualified identifiers `a::b` | yes | partial | no | partial | partial |
| Qualified type names `a::b` | yes | partial | partial | partial | partial |
| Calls | yes | yes | yes | yes | yes |
| Unary `+ - ! not` | yes | partial | partial | partial | partial |
| `* / %` | yes | yes | yes | yes | yes |
| `+ -` | yes | yes | yes | yes | yes |
| `<< >>` | yes | partial | no | no | no |
| `& ^ \|` | yes | partial | no | no | no |
| `< <= > >=` | yes | yes | yes | yes | yes |
| `== !=` | yes | yes | yes | yes | yes |
| `and or && \|\|` | yes | partial | no | no | no |
| Assignment expressions | no | no | no | no | no |
| Patterns | no | no | no | no | no |
| Attributes / visibility | no | no | no | no | no |
| Forms / picks / traits / impls | no | no | no | no | no |

## Immediate Gaps

The highest-value remaining gaps for real-language coverage are:

1. procedure modifiers, generics, and where clauses
2. richer type semantics for qualified names
3. semantic resolution for imports and qualified names
4. non-procedure declarations from the real grammar
5. loops and richer control-flow statement forms

## Notes

- `partial` in semantic stages means the syntax is accepted, but bootstrap type
  resolution still understands mainly builtin names.
- import groups and globs are desugared by the parser into flat dotted import
  declarations such as `math.ops.mul`.
- the driver now resolves declared imports through `vitte_module_t` and the
  import resolver before semantic analysis.
- qualified builtin and same-module names currently fall back to their last
  segment during bootstrap semantic/IR resolution, for example `core::int` and
  `io::len`.
