# compiler/src/sema

The semantic analyzer validates the parsed AST before backend emission.

## Contract

- No dependency on `runtime/*`.
- Input is an initialized AST rooted at a module node.
- Type resolution uses `compiler/src/type`.
- Bindings use `compiler/src/symbol` and `compiler/src/scope`.
- Builtin operators and functions come from `compiler/src/builtin`.
- Optional constant evaluation uses `compiler/src/constant_fold`.
- Diagnostics go through `compiler/src/diagnostic`.
- Errors use `compiler/src/api/error.h`.

## Current Checks

- duplicate import declarations and conflicting visible import names
- explicit export targets must resolve to local top-level declarations
- explicit export names must not collide with other exported names
- imported symbol visibility for direct, glob, and module-alias usage
- duplicate top-level `proc` and `const`
- unknown identifiers
- unknown type names
- return value compatibility
- `let` initializer compatibility
- exact and safe conversions are accepted across assignments, arguments,
  fields, returns, pointers and references; lossy conversions require casts
- integer and floating literals are context-typed only when their value is
  representable in the requested destination type
- generic parameter scopes for type, trait, impl, form and procedure bodies
- generic type arguments are resolved in the importing scope, checked for
  arity, interned as nominal instances, and substituted into form fields and
  member access; imported nominal identities remain distinct
- tuple/list declaration and `for` binding patterns
- `if` condition type validity
- callability, call arity, and procedure argument type compatibility
- builtin function argument compatibility
- operator operand compatibility
- canonical logical operator semantics for `and` and `or`

Import names follow three distinct forms: `use path;` binds the module under
its final path segment, so a member is referenced as `module::member`;
`use path::{member};` binds that exported member directly; and `as name` gives
either form a local alias. Repeating an import is reported as E0304; conflicting
explicit aliases are E0402; and one unaliased name supplied by multiple grouped
or glob imports is E0403 with both declaration locations and module names. An
unresolved bare member available through a module import explains the qualified
and grouped forms, including when an alias is needed to select one candidate.

Private member use reports the defining declaration as a related location
(E0404), with `pub` and module export-clause forms shown as concrete access
changes. Same-scope declaration conflicts use E0402; repeated pattern bindings
use E0406, and both point back to the earlier declaration or binding. Export
clauses are accepted only at root or module scope (E0405 elsewhere). Missing
export targets and attempts to re-export imported symbols use E0407; export
paths use `::`, and a dependency should expose its own API before consumers
import it with `use path::{name};`.

## Current Limits

- generic procedure instantiation and trait-bound enforcement remain a later
  pass; nominal type arguments and their field/member substitutions are
  already validated here
- constructor pattern typing and exhaustiveness for non-boolean subjects remain a later pass
- export clauses publish local declarations only; re-exporting imported symbols is diagnosed as E0407 and remains unsupported
- imported modules are analyzed for compiler visibility and declaration validity only

## Driver Integration

The compiler driver now runs semantic analysis after AST validation and before
backend/codegen. Syntax-only success is no longer enough to reach C17 emission.

## Bootstrap 0.1.1 diagnostic checks

Contracts are checked in the procedure parameter scope before its body:

- `requires` and `ensures` must have type `bool` (`E0704`, `E0705`).
- `result` is implicit only in `ensures` for a procedure returning a value;
  other contract uses produce `E0707`, without a follow-up operand error.
- Statically false `requires` is contradictory (`E0709`); statically false
  `ensures` is an impossible postcondition (`E0702`). Bounded evaluation
  includes immutable bindings, integer arithmetic and short-circuit logic.
- Calls with known arguments violating a precondition produce `E0701`,
  including forward calls. Known explicit and implicit match returns that
  violate postconditions produce `E0702`.
- Assignment, suspension and error propagation in contracts produce `E0710`.
- Unknown predicates remain runtime checks. Preconditions run before the body;
  postconditions run on normal returns after deferred actions. Return values
  are evaluated once. Parameter bindings are captured at entry (references
  retain reference semantics), so local shadowing cannot change their meaning.
  Checks remain enabled independently of host assertion settings.
- Contract diagnostics preserve the enclosing clause, expression, procedure
  and reason through terminal, JSON, SARIF and LSP output. Runtime failures
  include the public code, procedure, clause location and corrective help.

Structural flow analysis reports `E1101` at the closing brace when a
value-returning procedure has a known fallthrough path. It handles nested
blocks, conditions, loops, loop-local breaks, unsafe blocks and exhaustive
boolean matches. Infinite loops do not require a return. Procedures without a
body are excluded. Statements after a terminating path receive `W0020`.

Static checks also detect integer overflow (`E1502`), division or remainder by
zero (`E1503`), constant dependency cycles (`E1504`), invalid shifts (`E0512`)
and known array index bounds (`E1201`). Statically dead branches and
short-circuited operands do not generate arithmetic or contract failures.

Boolean and pick matches require complete coverage (`E1601`); help lists the
missing boolean form or pick variants, and the pick declaration is attached
as a related location. Other subjects require an unguarded wildcard because
the compiler does not prove their full value space. Inaccessible arms receive
`E1602` for duplicate boolean, integer, identical string/character and pick
patterns, patterns after an unguarded wildcard, covered integer ranges, and
statically false guards. Partly overlapping integer/character ranges receive
`W0052`. Integer coverage uses values, including alternate radix spellings;
guarded arms do not establish coverage. Pattern mismatches and invalid
array/struct/variant shapes receive `E1603`; inconsistent alternative
bindings receive `E1604`. Tuple syntax is parsed, but tuple value types are not
part of the language's type model, so tuple patterns receive `E1603`.
Distinct escape spellings are not yet normalized for duplicate
string/character detection. The grammar defines `match_statement` only; there
is no match-expression production.

Deferred work rejects `return`, `?`, `break`/`continue` that leave the
deferred action, and nested `defer` with `E1105`. Loops created inside a
deferred block may use their own `break` and `continue`. Captures keep a local
available until scope exit; moving a captured value early reports `E1208` and
relates the move to its `defer` site.

The bootstrap reference pass tracks direct local and parameter roots, simple
reference aliases, lexical scopes and the last syntactic use. It reports
`E1205` for a local escape or an unknown reference origin, `E1206` for
overlapping incompatible borrows, and `E1207` for an access that conflicts
with a live borrow. Calls compare reference arguments when their roots are
known. Direct procedure calls that return one of their reference parameters
propagate that argument's origin, so returning a borrowed local through an
identity helper is rejected as well. Provenance through aliases inside the
callee, block-valued expressions, aggregates, raw pointers, closures, indirect
calls and other returned references is not modeled completely, so this remains
an experimental safety check rather than a complete alias or lifetime proof.

`move` is accepted for a whole local binding or parameter. The bootstrap marks
that binding unavailable until it is assigned again, rejects use after move
(`E1209`), and rejects moving a value while a tracked reference is live
(`E1208`). Moving a reference binding transfers its tracked local borrow.
Partial moves and moves through fields or indirection are rejected; C17 keeps
the value representation unchanged because the bootstrap has no destructor or
runtime ownership machinery.

`make test-bootstrap-semantics` verifies invalid and valid semantic cases plus
compiled executable contract behavior using the bootstrap alone.

`make test-diagnostic-contract` covers exact source ranges, codes, phases,
severity, help, contract metadata, independent errors and valid procedures.
