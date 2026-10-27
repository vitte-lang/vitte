# Vitte Language and Toolchain Coverage

This is the single implementation-status matrix for the language and the
default compiler. It distinguishes syntax in the normative grammar from what
the bootstrap parser, semantic analyzer and code generator actually support.
The standard Makefile and CMake executable link the staged Vitte pipeline,
while bootstrap C still supplies compatibility parsing/analysis, diagnostics,
and the final CLI C17 output path.

For documentation audiences and language conventions, see
[`documentation_policy.md`](documentation_policy.md).

## Status key

| Status | Meaning |
| --- | --- |
| **Available** | Implemented for the stated feature scope in that column's path |
| **Partial** | Some forms work; the limits below matter for real programs |
| **Experimental** | Staged, alternate, shadow, or otherwise not a default user contract |
| **Absent** | Not implemented in that path |

“Available” does not promise every type combination or platform. The feature
scope in each row is deliberately narrow; use the named source and examples to
check details before relying on a broader form.

## Feature matrix

| Feature family and scope | Normative grammar | Bootstrap parser | Semantic analysis | Default `compile` / `run` (AST → C17) | HIR → IR → C17 path | Native Vitte path |
| --- | --- | --- | --- | --- | --- | --- |
| Source text, tokens, comments and supported literal forms | Available | Available | — | Available | Partial | Experimental |
| `space` declarations and source module identity | Available | Available | Partial | Partial | Partial | Experimental |
| `use` imports, aliases, groups, globs and exports | Available | Available | Partial | Partial | Partial | Experimental |
| Basic synchronous procedures, scalar parameters and returns, calls, and `give` | Available | Available | Available for supported types and calls | Available for the supported subset | Partial | Experimental |
| Local `let`, mutable bindings, assignment and basic expressions | Available | Available | Available for supported expressions and types | Available for the supported subset | Partial | Experimental |
| Built-in scalar types, pointers, references and aggregate types | Available | Available | Partial | Partial | Partial | Experimental |
| `sizeof` and `alignof` type queries | Available | Available | Available for resolvable types | Available | Partial | Experimental |
| `if` / `else`, `while`, `loop`, `break` and `continue` | Available | Available | Available for the supported conditions and control flow | Available | Partial | Experimental |
| `for` statements | Available | Available | Partial | Partial; array/slice iteration with a simple binding | Partial | Experimental |
| `match` statements and patterns | Available | Available | Partial | Partial; scalar patterns and pick variants with positional payload bindings or `_` | Partial | Experimental |
| `form` and `pick` declarations and values | Available | Available | Partial | Partial; named form values/member access and tagged positional pick payloads | Partial | Experimental |
| Generic declarations and `where` clauses | Available | Partial | Partial | Partial; nominal arguments are checked, but procedure monomorphization and bound enforcement are absent; C17 emission rejects generic procedures, aliases, forms, picks and `where` constraints | Partial | Experimental |
| Trait and `impl` declarations | Available | Partial | Partial | Partial; bootstrap checks `self`, trait headers and missing required methods, but not method signatures/dispatch; C17 emission rejects trait/impl declarations | Partial | Experimental |
| Attributes | Available | Available | Partial | Partial | Partial | Experimental |
| Macro declarations and expansion | Available | Partial | Partial | Absent; there is no invocation/expansion pass, and C17 emission rejects macro declarations before writing output | Partial | Experimental |
| Async procedures and `await` | Available | Available | Partial | Absent; the default emitter rejects both before writing C17 because task/future and coroutine lowering are not implemented | Partial | Experimental; the native C17 emitter returns an explicit error |
| Compiler meta declarations (`compiler`, `pass`, `backend`, and related forms) | Available | Partial | Partial | Absent or metadata-only | Partial | Experimental |

### Reading the matrix

- `grammar/vitte.ebnf` is the canonical grammar. Its existence means that a
  syntax form is specified; it does not prove that a compiler stage implements
  the form.
- The focused native subset is exercised by
  `make test-native-frontend-subset`: scalar arithmetic, typed comparisons,
  boolean logical operators and negation, and simple/compound assignment to
  mutable locals or their member projections. Logical negation uses `not`; the
  parser rejects `!` and offers it as a migration suggestion. It also requires
  `set` statements to contain an assignment. Compound assignments follow
  numeric or integer operator rules.
- `src/bootstrap/` supplies CLI and host-tool integration. The standard
  executable routes ordinary `compile`, `run`, and `check` inputs through the
  linked Vitte-native frontend.
- The native pipeline lowers and validates HIR and IR. Ordinary compilation
  uses the IR-to-C17 backend for the supported subset; declarations and
  patterns without IR lowering retain the AST-to-C17 compatibility emitter.
  `--emit-ir` prints the validated IR instead of selecting a separate backend.
- The current `make native-frontend` source build does not complete yet, so a
  fresh native executable and a compiler self-rebuild are not verified.
- `src/compiler/` contains the native frontend used by the standard executable
  for ordinary `compile`, `run`, and `check` operations. Its parser and
  semantic surface are still partial; the scalar core listed above is covered
  by a focused integration test.
- Import resolution handles supported direct imports, aliases and flattened
  forms, but re-exports and some name-collision cases remain limited. Do not
  infer complete module-system support from grammar acceptance.
- The AST-to-C17 compatibility emitter supports `for` iteration over arrays and
  slices with a simple identifier binding, including `mut` bindings.
  Destructuring and non-binding patterns are not yet emitted.
- The AST-to-C17 compatibility emitter lowers `match` statements to ordered C
  conditions and evaluates each subject once. It supports scalar literals/ranges,
  alternatives, simple/`mut` bindings, guards, and pick variant tests. Pick
  payload patterns may bind positional fields or ignore them with `_`; nested
  destructuring, literal tests inside payloads, `ref` bindings, and bindings
  inside alternatives are not emitted. The grammar's `match_expr` form does
  not yet have a parser/AST implementation.
- The AST-to-C17 compatibility emitter outputs forms as C structs and orders
  by-value form and payload dependencies before defining their layouts. It supports named
  form initializers and member access through values, pointers, references,
  and non-generic type aliases.
  Generic forms/picks are not emitted. Picks without payloads remain C enums;
  picks with positional payloads use a tag and union. Equality for payload
  picks is emitted for scalar, string, and pointer-like fields; aggregate
  payload equality remains unsupported. The bootstrap parser still accepts
  only positional type payloads for pick variants and field declarations for
  forms; named variant fields, explicit discriminants, form defaults, and
  associated form items are not implemented end to end.

## CLI behavior

| Command or switch | Current behavior | Status |
| --- | --- | --- |
| `vitte check FILE` | Lex, parse and run semantic analysis; stops before code generation | Available |
| `vitte compile FILE` | Lower and validate HIR/IR, then emit through the IR backend for supported inputs or the AST compatibility emitter for uncovered constructs | Available for the supported subset and host C toolchain |
| `vitte run FILE [-- ARGS…]` | Use the native compile route, then execute the generated program | Available for the supported subset and host |
| `vitte lex FILE`, `vitte parse FILE` | Run and display the corresponding frontend stage | Available |
| `vitte explain CODE`, `vitte completion SHELL`, `vitte help`, `vitte version` | Diagnostics lookup, shell completion, help and version | Available |
| `--emit-ir` | Prints validated HIR/IR for programs accepted by the native lowerer | Available for the supported subset |
| `vitte test` | No such command is declared; repository tests use Make targets such as `make test` | Absent |

`src/bootstrap/cli/command_spec.def` is the declarative CLI inventory. A flag
being accepted or listed there establishes CLI recognition, not that its
promised feature works from command line to generated program. The matrix
above marks end-to-end behavior conservatively.

## Sources of truth and maintenance

- Grammar: `grammar/vitte.ebnf`.
- Bootstrap lexer, parser and semantic analysis: `src/bootstrap/lexer/`,
  `src/bootstrap/parser/` and `src/bootstrap/sema/`.
- Default command routing: `src/bootstrap/cli/command_spec.def`,
  `src/bootstrap/cli/execution.c` and `src/bootstrap/cli/pipeline.c`.
- Native IR backend: `src/compiler/backend/c17/ir_emitter.vitte`; AST
  compatibility emission: `src/compiler/backend/c17/ast_emitter.vitte`.
- Host compilation, linking and execution: `src/bootstrap/backend/c17/`.
- Alternate HIR/IR backend: `src/bootstrap/hir/`, `src/bootstrap/ir/` and
  `src/bootstrap/backend/c17/ir_emitter.c`.
- Staged native port: `src/compiler/` and
  `docs/bootstrap/frontend_native_port.md`.
- Concrete parser, semantic, backend and CLI examples: `src/tests/` and
  `examples/`.

When implementation changes, update this matrix and the relevant examples in
the same change. `make -C src docs-check` checks generated CLI and grammar
artifacts; it does not prove every row in this manually maintained matrix.
