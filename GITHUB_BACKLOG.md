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

### Completed recently. Canonical entrypoints split from compatibility wrappers across analysis/middle/prelude/backend

- Status: done
- Notes:
- canonical named entrypoints now exist for analysis, middle, prelude helpers, backend pipeline, and C backend pipeline
- driver and backend production consumers were switched to the canonical surfaces
- legacy generic helpers remain only as compatibility wrappers with equivalence tests
- backend C production lowering now consumes nominal metadata from canonical IR, with `HIR+IR` kept only as tested compatibility fallback
- canonical ownership and call-signature metadata now flow from frontend and HIR through MIR, IR, borrowck, C lowering, and LLVM lowering
- backend and analysis stable gates now execute the promoted `borrowck`, `typeck`, `const_eval`, `diagnostic_snapshot`, `codegen`, and `c_backend` suites through the strict bridge path

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
