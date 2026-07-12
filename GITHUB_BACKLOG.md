# Vitte Compiler GitHub Backlog

Date: 2026-06-28
Derived from: `AUDIT.md`

## Current focus

### Active next step. Tighten MIR and IR validation around the canonical ownership and nominal-call model

- Priority: P2
- Dependencies: 7, 10, 11
- Description: Finish the hardening pass now that the canonical ownership/call-signature path exists end to end, by making MIR and IR validators reject structurally invalid canonical borrow, param, and nominal-call forms before backend lowering.
- Acceptance criteria:
- MIR validation rejects invalid canonical function params and unresolved method-dispatch targets deterministically
- IR validation rejects malformed `variant-ctor` and `method-dispatch` calls against canonical nominal metadata
- backend consumers use canonical MIR/IR borrow and nominal-call forms without unchecked fallback paths
- regression tests cover valid and invalid regular proc calls, impl method calls, borrow passthrough, and nominal dispatch/variant construction
- Status note:
- MIR-side param and call-shape validation is now materially tighter and CI-visible; the remaining closure on this active step is primarily IR-side nominal metadata verification and backend-consumer cleanup

### Completed recently. Canonical entrypoints split from compatibility wrappers across analysis/middle/prelude/backend

- Status: done
- Notes:
- canonical named entrypoints now exist for analysis, middle, prelude helpers, backend pipeline, and C backend pipeline
- driver and backend production consumers were switched to the canonical surfaces
- legacy generic helpers remain only as compatibility wrappers with equivalence tests
- backend C production lowering now consumes nominal metadata from canonical IR, with `HIR+IR` kept only as tested compatibility fallback
- canonical ownership and call-signature metadata now flow from frontend and HIR through MIR, IR, borrowck, C lowering, and LLVM lowering
- backend and analysis stable gates now execute the promoted `borrowck`, `typeck`, `const_eval`, `diagnostic_snapshot`, `codegen`, and `c_backend` suites through the strict bridge path

### Completed recently. Lexer EBNF surface and token contract are now on the canonical frontend path

- Status: done
- Notes:
- the active scanner now covers the current EBNF lexer surface, including char literals, `~`, `&&`, `||`, `<<=`, and `>>=`
- canonical token consumers are pinned to `frontend/lexer/token.vit`
- fast CI now checks `src/vitte/compiler/tests/lexer_tests.vit` explicitly and runs the frontend token consistency guard
- `tools/lexer_ebnf_surface_check.py` now classifies every quoted terminal in `src/vitte/grammar/vitte.ebnf` against lexer support, so new lexical terminals cannot be added silently
- README status for `Lexer` is `100%` for the active EBNF lexical surface only; parser, AST, and semantic completeness remain tracked separately

### Completed recently. Parser coverage reporting is now exhaustive and is the CI source of truth

- Status: done
- Notes:
- `tools/parser_sync_coverage_report.py` now emits an exhaustive rule matrix from `src/vitte/grammar/vitte.ebnf`
- CI publishes the `grammar_coverage` report artifact and uses it as the canonical parser coverage signal
- parser coverage can no longer report `green` from a tiny subset, and the active frontend now reaches `missing=0`
- README status for `Parser` is `100%` for the active grammar coverage/reporting contract only; AST construction, semantic validation, and type checking remain tracked separately

### Completed recently. Active parsed frontend surface now has AST construction coverage

- Status: done
- Notes:
- the last parsed-without-AST false negative (`postfix_expr`) is now recognized through the canonical AST construction path in `parser.vit`
- the active parsed frontend surface no longer has rules that are `parsed` without also being `ast-built`
- `frontend-ast-test` now runs `src/vitte/compiler/tests/ast_tests.vit` and `tools/ast_coverage_gate.py` in the fast core-language gate
- README status for `AST` is `100%` for the active non-lexical parsed grammar surface; lexical grammar rules remain owned by the lexer gate

### Completed recently. HIR trust boundary now rejects unsupported or partial lowering more explicitly

- Status: done
- Notes:
- `lower_ast_to_hir` now emits explicit diagnostics when valid AST expr/stmt/item kinds do not map to supported HIR kinds
- HIR validation now checks lowered proc params, nominal members, and proc signature arity consistency instead of trusting partial surfaces
- `src/vitte/compiler/tests/hir_tests.vit` is wired into the fast core-language gate so HIR regressions are visible alongside lexer/parser guards

### Completed recently. MIR call and param validation now enforces more of the canonical contract

- Status: done
- Notes:
- MIR validation now rejects duplicate param names, malformed variadic param surface, and inconsistent borrow/mut param flags deterministically
- regular calls and `method-dispatch` calls now validate argument type and borrow compatibility against the resolved MIR function signature, not only arity
- `src/vitte/compiler/tests/mir_tests.vit` is wired into the fast core-language gate so canonical MIR contract regressions fail early

### Completed recently. MIR package coverage was expanded across lowering, validation, builder, dataflow, transform, and pretty helpers

- Status: done
- Notes:
- `src/vitte/compiler/tests/mir_tests.vit` now exercises the exported `middle/mir` helper surface, including builder context/value accounting, dataflow summaries, transform reports, pretty rendering, canonical call validation, borrow validation, and CFG validation
- `mir-lowering-test` remains the fast CI signal for MIR package regressions through `bin/vitte check src/vitte/compiler/tests/mir_tests.vit`
- this improves the proven MIR surface, but MIR remains an incomplete optimization IR and should not be presented as feature-complete

### Completed recently. HIR package coverage was expanded across lowering, validation, builder, CFG, and pretty helpers

- Status: done
- Notes:
- `src/vitte/compiler/tests/hir_tests.vit` now exercises not only AST->HIR lowering and validation but also the `builder`, `control_flow`, and `pretty` helper surfaces through active selftests
- the fast HIR gate now covers the full `middle/hir` package surface that is still exported through `mod.vit`
- this is package-level coverage, not a claim that the complete language surface lowers through HIR

### Completed recently. Semantic resolution now covers alias imports, visibility rejection, and cross-module export boundaries more explicitly

- Status: done
- Notes:
- sema multi-module resolution now supports `use demo/math as math; math.sum()` through an explicit imported module alias symbol and member lookup against exported items
- sema tests now cover invalid visibility rejection, alias-member import resolution, and the fact that non-exported module items stay unresolved across module boundaries
- `src/vitte/compiler/tests/sema_tests.vit` is wired into the fast core-language gate so semantic-regression coverage is no longer indirect only through later phases

### Completed recently. Semantic package coverage was expanded across resolver, names, visibility, scopes, modules, and diagnostic helpers

- Status: done
- Notes:
- `src/vitte/compiler/tests/sema_tests.vit` now exercises the exported helper surface of `analysis/sema`, including place-root parsing, visibility normalization, symbol-table behavior, module summary/import parsing, and semantic diagnostic context helpers
- the semantic fast gate now proves the package exported by `src/vitte/compiler/analysis/sema/mod.vit`, not only the main `run_sema_hir` entrypoint
- semantic analysis remains partial beyond the proven subset

### Completed recently. Compile-time simulation now rejects constant impossible paths

- Status: done
- Notes:
- `analysis/const_eval` now reports `CONST_EVAL_E_IMPOSSIBLE_BRANCH` when compile-time simulation proves an `if` or `while` condition is constant false
- `src/vitte/compiler/tests/const_eval_tests.vit` covers deterministic arithmetic, division by zero, unknown names, non-const calls, static assertions, cycles, and compile-time branch simulation
- `const-eval-analysis-test` is wired into the fast core-language gate so const-eval and compile-time simulation regressions fail directly

### Completed recently. Production type-checking package coverage was expanded across checker, infer, traits, coercion, unify, errors, and API helpers

- Status: done
- Notes:
- `src/vitte/compiler/tests/typeck_tests.vit` now exercises the exported production helper surface of `analysis/typeck`, including pipeline-status API helpers, type trait/class queries, coercion/projection helpers, unify rules, infer environment lifecycle helpers, and type-check diagnostic/result summaries
- `src/vitte/compiler/tests/typeck_tests.vit` is wired into the fast core-language gate so type-check regressions are visible directly rather than only through downstream borrowck/backend failures
- the production `hir` pipeline is still a partial type checker; the `complete` pipeline remains explicitly experimental in the API

### Completed recently. Borrow checker package coverage was expanded across ownership, moves, loans, lifetimes, regions, diagnostics, and canonical HIR/MIR entrypoints

- Status: done
- Notes:
- `src/vitte/compiler/tests/borrowck_tests.vit` now exercises the exported borrow checker helper surface, including ownership place tracking, move conflict state, loan alias/conflict tables, lifetime/region summaries, and borrow diagnostic report helpers
- temporal ownership windows now make lifetime duration explicit: aliases must start after the owner exists and end before the owner dies, with a regression test for escaped aliases
- memory regions are now first-class at the active frontend surface: `region` is tokenized, parsed as a top-level AST item, lowered through HIR, and backed by a borrowck region model that binds places to declared regions and rejects escapes after region closure
- the borrow checker production path is covered through `borrow_check_source`, `borrow_check_hir`, and the canonical `borrow_check_hir_mir` path used after frontend/HIR/MIR lowering
- `src/vitte/compiler/tests/borrowck_tests.vit` is wired into the fast core-language gate through `borrowck-analysis-test`
- borrow checking is one of the stronger subsystems, but it is not full language-complete ownership semantics

### Completed recently. LLVM backend coverage was expanded across canonical IR emission, object bundles, native-toolchain diagnostics, profile metadata, and backend gate artifacts

- Status: done
- Notes:
- `src/vitte/compiler/tests/llvm_tests.vit` now exercises the LLVM binding surface, including adapter helpers, toolchain status, canonical IR emission, LLVM object sections/symbols/relocations, LTO/PGO/debug diagnostics, native toolchain failures, and invalid option rejection
- `llvm-backend-gate` now runs the Vitte LLVM tests and the bindings smoke test before the Python validation and artifact generation checks
- generated LLVM coverage remains CI-visible through `target/reports/llvm_backend_coverage.md` and `target/reports/llvm_backend_validation.md`
- `tools/llvm/check_backend_reports.py` now fails the gate if validation is not `PASS`, required LLVM adapter features are absent, or the LLVM IR artifact/hash is missing
- `tools/llvm/native_final_gate.py` adds the conditional final native smoke path: Vitte bootstrap source -> native IR -> LLVM IR -> clang object -> linked executable -> run
- README status for `LLVM` is `100%` for the checked adapter plus native smoke contract
- the remaining LLVM work is deeper language/ABI coverage, not absence of an end-to-end native smoke gate

## P0

### 1. Align lexer with official EBNF token surface

- Priority: P0
- Dependencies: none
- Description: Update the active scanner and token model so the lexer recognizes the operator and literal surface required by `src/vitte/grammar/vitte.ebnf`.
- Acceptance criteria:
- char literals are tokenized and tested
- `~`, `&&`, `||`, `<<=`, `>>=` are tokenized and tested
- token API consumers compile against one canonical token model

### 2. Replace shallow parser sync audit with full grammar coverage reporting

- Priority: P0
- Dependencies: none
- Description: Replace the current partial parser sync tooling with a complete rule coverage matrix over the 271 EBNF rules.
- Acceptance criteria:
- every grammar rule is classified as `parsed`, `ast-built`, `diagnosed`, `tested`, or `missing`
- the report is generated in CI
- parser status can no longer be marked green from a tiny subset

### 3. Remove or repair stale frontend token consumers

- Priority: P0
- Dependencies: none
- Description: Eliminate stale frontend files or update them to the active token API and import layout.
- Acceptance criteria:
- no file under `src/vitte/compiler/frontend/*` references nonexistent token kinds
- no file imports `frontend/lex/*` if the canonical path is `frontend/lexer/*`
- `bin/vitte check` fails on token API mismatches introduced by regression fixtures

### 4. Unify parser surface parsing and AST construction

- Priority: P0
- Dependencies: 1, 2, 3
- Description: Remove divergence between token-stream parsing and AST reconstruction so supported syntax has one coherent implementation contract.
- Acceptance criteria:
- `if` and `while` condition syntax is consistent across parsing and AST building
- supported stmt and expr forms are built directly or through one validated reconstruction path
- parser tests cover the reconciled behavior

## P1

### 5. Promote one production type-checking pipeline

- Priority: P1
- Dependencies: 4
- Description: Converge `analysis/typeck` onto one canonical production path and reduce split-brain behavior between partial and “complete” implementations.
- Acceptance criteria:
- one typeck pipeline is used by the compiler driver
- duplicate or shadow typeck entry points are either removed or explicitly marked experimental
- typeck tests run against the production path

### 6. Expand semantic analysis beyond locals and simple imports

- Priority: P1
- Dependencies: 4, 5
- Description: Implement and test core module visibility, namespace handling, and foundational trait/impl lookup behavior.
- Acceptance criteria:
- module visibility rules have dedicated tests
- import resolution covers non-trivial module paths
- basic trait/impl resolution is demonstrated in sema tests

### 7. Define the supported AST -> HIR -> MIR contract

- Priority: P1
- Dependencies: 4, 5
- Description: Publish and enforce the exact frontend subset that lowers cleanly through HIR and MIR.
- Status note:
- the HIR side is now hardened and CI-visible; the remaining closure work for this item is on MIR-side contract publication and validation parity
- Acceptance criteria:
- every supported AST family maps to HIR and MIR with tests
- unsupported constructs fail with explicit diagnostics
- HIR/MIR validators reject partial or placeholder lowering

### 8. Harden compiler self-checking

- Priority: P1
- Dependencies: 5, 6, 7
- Description: Make the compiler catch its own internal API and semantic inconsistencies earlier.
- Acceptance criteria:
- regression fixtures for bad token kinds and invalid internal enum references fail
- compiler checks detect stale frontend API usage
- CI includes internal consistency checks, not only surface success paths

## P2

### 9. Consolidate canonical backend and quarantine legacy backends

- Priority: P2
- Dependencies: 7, 8
- Description: Choose one reference backend path and isolate legacy adapter surfaces from the main driver.
- Acceptance criteria:
- driver pipeline names one canonical backend route
- legacy `src/vitte/compiler/backends/*` paths are either removed from the driver or clearly marked non-canonical
- backend surface audits fail if canonical driver code depends on quarantined legacy paths

### 10. Raise C backend from credible to reference-grade

- Priority: P2
- Dependencies: 7, 9
- Description: Expand the C backend so it becomes the trusted closure path for the supported language subset.
- Acceptance criteria:
- supported AST/HIR/MIR subset lowers through C backend end-to-end
- runtime hooks and artifact production are covered by tests
- `bin/vitte build src/vitte/compiler/main.vit -o ...` works on the canonical path without sidecar bridge assumptions

### 11. Tighten MIR validation and pass pipeline

- Priority: P2
- Dependencies: 7
- Description: Strengthen MIR invariants and formalize pass preconditions/postconditions.
- Acceptance criteria:
- MIR validator checks are expanded
- canonical method-dispatch targets must resolve to real MIR functions
- canonical function params must be structurally valid before passes run
- each MIR pass declares and tests its invariants
- invalid MIR fixtures are rejected deterministically

### 12. Convert self-hosting from transition to closure

- Priority: P2
- Dependencies: 8, 9, 10, 11
- Description: Finish the self-hosting closure so bootstrap no longer relies on transitional bridge assumptions.
- Acceptance criteria:
- stage2 and stage3 parity is enforced
- reproducible outputs are demonstrated by gates
- bootstrap bridge artifacts are disallowed in strict paths

## P3

### 13. Extend grammar support only after frontend correctness closure

- Priority: P3
- Dependencies: 1 through 8
- Description: Add missing language surface from the official grammar only after the existing subset is fully aligned.
- Acceptance criteria:
- each new rule lands with lexer, parser, AST, diagnostics, and tests together
- grammar coverage report trends upward without placeholder acceptance

### 14. Expand ownership and optimization surface

- Priority: P3
- Dependencies: 7, 10, 11, 12
- Description: Broaden borrow checking and optimizer sophistication once the supported subset is stable.
- Acceptance criteria:
- new ownership cases are covered by borrowck tests
- optimization passes preserve MIR/IR invariants
- performance work is gated by correctness fixtures

### 15. Rebaseline documentation against proven implementation

- Priority: P3
- Dependencies: 2, 7, 12
- Description: Make docs clearly distinguish implemented, experimental, and specified-but-not-yet-supported language/compiler surface.
- Acceptance criteria:
- grammar, compiler, and stdlib docs expose implementation status
- docs do not imply support that the driver pipeline cannot prove
- status pages are generated from code-backed inventories where possible

## Suggested milestones

### Milestone A. Frontend correctness closure

- Scope: 1, 2, 3, 4
- Goal: stop frontend drift and make grammar claims auditable
- Exit condition:
- lexer token surface matches the currently claimed core grammar subset
- stale frontend token consumers are removed or repaired
- grammar coverage report is generated and consumed by CI
- parser and AST construction no longer disagree on supported control-flow forms

### Milestone B. Semantic convergence

- Scope: 5, 6, 7, 8
- Goal: ensure that accepted source forms either lower cleanly or fail explicitly
- Exit condition:
- one production type-checking path is wired through the compiler driver
- semantic and type support is tested end to end for the declared supported subset
- unsupported forms are rejected before backend-facing lowering
- internal consistency checks catch stale API drift

### Milestone C. Canonical backend closure

- Scope: 9, 10, 11
- Goal: narrow the trusted compiler path and harden its invariants
- Exit condition:
- one canonical backend path is clearly defined
- MIR and IR validators reject malformed canonical ownership and nominal-call forms
- the C backend closes the supported subset without bridge-era assumptions

### Milestone D. Credible self-hosting

- Scope: 12, 13, 14, 15
- Goal: move from active transition to reproducible, defensible self-host evidence
- Exit condition:
- strict stage parity and reproducibility gates pass
- docs reflect proven implementation rather than aspirational breadth
- new surface expansion follows the hardened lexer/parser/sema/backend contract

## Next issue batch to open

These are the most useful follow-up issues if the repository wants execution
momentum rather than a high-level backlog only.

### A1. Generate grammar implementation coverage report

- Parent: 2
- Priority: P0
- Labels: `compiler`, `frontend`, `grammar`, `audit`
- Description: Produce a machine-generated report that maps every rule in
  `src/vitte/grammar/vitte.ebnf` to parser coverage, AST construction,
  diagnostics, and tests.
- Acceptance criteria:
- report is written under `target/reports/grammar_coverage/`
- every rule gets one explicit status
- CI fails if the report is missing or stale

### A2. Add strict gate for stale frontend token API usage

- Parent: 3
- Priority: P0
- Labels: `compiler`, `frontend`, `ci`
- Description: Add a strict check that rejects references to nonexistent token
  kinds or stale frontend import paths.
- Acceptance criteria:
- stale `frontend/lex/*` imports are rejected
- references to removed token kinds are rejected
- the gate runs in CI and is easy to reproduce locally

### A3. Reconcile `if` parsing and AST reconstruction contract

- Parent: 4
- Priority: P0
- Labels: `compiler`, `parser`, `ast`
- Description: Remove parser/AST disagreement for `if` forms and codify one
  accepted syntax contract.
- Acceptance criteria:
- one canonical `if` condition shape is implemented
- parser tests and AST tests cover the same accepted/rejected forms
- no reconstruction-only `if` special cases remain in the supported path

### A4. Reconcile `while` parsing and AST reconstruction contract

- Parent: 4
- Priority: P0
- Labels: `compiler`, `parser`, `ast`
- Description: Apply the same reconciliation to `while` forms.
- Acceptance criteria:
- one canonical `while` condition shape is implemented
- parser tests and AST tests cover the same accepted/rejected forms
- no reconstruction-only `while` special cases remain in the supported path

### B1. Select and lock one production typeck entrypoint

- Parent: 5
- Priority: P1
- Labels: `compiler`, `typeck`, `architecture`
- Description: Decide which type-checking path is production, wire the driver
  to it explicitly, and mark all others as compatibility or experimental.
- Acceptance criteria:
- compiler driver uses one canonical typeck entrypoint
- non-canonical typeck paths are renamed, documented, or removed
- production typeck tests execute only through the canonical path

### B2. Reject unsupported advanced forms before HIR trust boundary

- Parent: 7
- Priority: P1
- Labels: `compiler`, `sema`, `hir`
- Description: Replace shallow acceptance of unsupported advanced syntax with
  explicit diagnostics before trusted HIR lowering.
- Acceptance criteria:
- unsupported forms produce deterministic diagnostics
- unsupported forms do not enter HIR as placeholder constructs
- negative fixtures cover the rejected cases

### C1. Validate canonical MIR function params

- Parent: 11
- Priority: P2
- Labels: `compiler`, `mir`, `validation`
- Description: Expand MIR validation so canonical function params are checked as
  a structural contract before pass execution.
- Acceptance criteria:
- malformed canonical param lists are rejected deterministically
- pass pipeline does not run on invalid param structures
- regression fixtures cover valid and invalid param shapes

### C2. Validate IR nominal-call and variant construction metadata

- Parent: Active next step, 11
- Priority: P2
- Labels: `compiler`, `ir`, `validation`, `backend`
- Description: Reject malformed canonical nominal-call and variant-construction
  forms before backend lowering.
- Acceptance criteria:
- malformed `method-dispatch` and `variant-ctor` calls are rejected
- backend consumers do not rely on unchecked nominal metadata
- regression fixtures cover valid and invalid nominal-call forms

## Ordering policy

When choosing between backlog items, prefer the item that increases proof
quality over the item that increases language surface.

Recommended order:

1. frontend drift elimination
2. grammar coverage reporting
3. parser/AST contract unification
4. semantic and type-checking convergence
5. validator hardening at HIR/MIR/IR boundaries
6. canonical backend closure
7. self-host reproducibility tightening
8. only then, new surface expansion

## Issue writing policy

Each GitHub issue derived from this backlog should include:

- one explicit trust boundary being improved
- one list of unsupported cases that must fail clearly
- one local reproduction command
- one CI or strict-gate integration point
- one removal condition for any temporary compatibility shim or allowlist

That policy should keep the backlog aligned with the audit principle already
used in `AUDIT.md`: claims must be backed by code paths, tests, and active
pipeline usage rather than by declared surface alone.
