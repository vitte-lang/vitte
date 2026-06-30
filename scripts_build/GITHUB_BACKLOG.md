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
