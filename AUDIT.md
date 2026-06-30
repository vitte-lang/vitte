# Vitte Compiler Engineering Audit

Date: 2026-06-26
Repo state: `main` @ `12ef79453`
Source of truth grammar: `src/vitte/grammar/vitte.ebnf`

## Scope and method

This audit is based only on code, tests, scripts, and generated artifacts present in the repository. If a feature is not demonstrated by code paths, tests, or wired pipeline usage, it is treated as absent or incomplete.

The official grammar source contains 271 rules. The main compiler pipeline audited here is the source-native path around:

- `src/vitte/compiler/main.vit`
- `src/vitte/compiler/driver/*`
- `src/vitte/compiler/frontend/*`
- `src/vitte/compiler/analysis/*`
- `src/vitte/compiler/middle/*`
- `src/vitte/compiler/backend/*`

## 1. Inventory

| Component | Status | Score | Why |
| --- | --- | ---: | --- |
| Lexer | Partial | 40% | Real scanner exists in `frontend/lexer`, but it does not cover the full grammar token set. |
| Parser | Partial | 45% | Broad parsing surface exists, but a meaningful part of the grammar is only accepted superficially or skipped. |
| AST | Partial | 50% | Rich AST enums exist, but many variants are not constructed reliably by the parser. |
| HIR | Present | 55% | Real HIR exists and is used by the middle pipeline. |
| Name resolution | Partial | 35% | Local/import resolution is real; full module/trait/generic resolution is not demonstrated. |
| Semantic analysis | Partial | 35% | Real passes exist, but only a narrow semantic subset is proven. |
| Type checker | Partial | 35% | Production typeck does basic checks; advanced typing is not demonstrated end-to-end. |
| Borrow checker | Present | 70% | Ownership, moves, borrows, lifetime-like checks, canonical call-signature consumption, and targeted tests are present. |
| Constant evaluation | Partial | 45% | Real evaluator exists for a subset, especially const/static-assert oriented flows. |
| MIR | Present | 50% | MIR is real, validated, and used in lowering, but not yet a mature optimization IR. |
| Optimizer | Partial | 20% | Pass pipeline exists mostly as infrastructure; optimization depth is limited. |
| Canonical IR | Present | 50% | Backend IR exists and is part of the pipeline. |
| Backend C | Present | 60% | Most credible backend today; real lowering/emission pipeline is present. |
| Backend LLVM | Partial | 30% | LLVM-related surfaces exist and now consume canonical IR signature and nominal-call metadata, but the path is still not production-grade. |
| Backend ASM/native | Partial | 30% | Native codegen machinery exists, but maturity and feature depth remain limited. |
| Backend WASM | Absent in practice | 5% | Only stub-level support is demonstrated. |
| Linker | Partial | 30% | Internal link artifact modeling exists, not a full demonstrated system linker replacement. |
| Runtime | Partial | 25% | Runtime responsibilities are split across C backend runtime, stdlib, and tools. |
| Stdlib | Partial | 35% | Broad module surface exists, but completeness is not demonstrated. |
| LSP | Partial | 15% | LSP-shaped diagnostics exist; a full language server is not demonstrated. |
| Formatter | Partial | 10% | Formatter exists, but it is whitespace-oriented rather than syntax-aware. |
| Linter | Partial | 20% | A few lint rules are implemented and tested. |
| Documentation | Partial | 50% | Large doc surface exists, but implementation alignment is uneven. |
| Tests | Present | 68% | Large suite exists, and more analysis/backend suites now run through stable strict gates, but coverage is still stronger for selected surfaces than for full semantic completeness. |
| Bootstrap | Present | 80% | Seed, stages, gates, and real-native checks are well developed. |
| Self-hosting | Partial | 50% | Self-host trajectory is real, with stronger strict-gate coverage, but full parity/closure is not yet demonstrated. |

## 2. Code audit

### Strengths

- Real multi-stage compiler structure exists: frontend, analysis, HIR, MIR, IR, backends, bootstrap.
- The bootstrap and real-native gating story is stronger than the average component maturity.
- Borrow checking is one of the most substantial implemented subsystems.
- The repository has serious testing and tooling volume.
- Canonical ownership/signature metadata now flows through HIR, MIR, IR, borrowck, C lowering, and LLVM lowering on the proven path.

### Technical debt and fragility

- The declared language surface is significantly larger than the reliably implemented one.
- The frontend is architecturally split between a broad token-driven parser and a second AST reconstruction path; those paths do not fully agree.
- Canonical backend code under `src/vitte/compiler/backend/*` still coexists with legacy/adapter surfaces under `src/vitte/compiler/backends/*`, which increases ambiguity even though the canonical path is better defined than before.
- Some “live-looking” frontend modules appear stale or internally inconsistent.

### Concrete inconsistencies

- `src/vitte/compiler/frontend/parse/lookahead.vit` imports `frontend/lex/token`, while the actual directory is `frontend/lexer`.
- `lookahead.vit` and `frontend/grammar_alignment_checker.vit` reference token kinds such as `Newline`, `Comment`, `Identifier`, `LeftBrace`, `RightBrace` that do not exist in `frontend/lexer/token.vit`.
- `./bin/vitte check` currently succeeds on such files, which indicates the compiler’s own internal consistency checks are still too weak.
- `tools/parser_sync_coverage_report.py` reports parser sync success while checking only a very small subset of the grammar. It cannot support claims of full EBNF coverage.

### Dead code / likely stale code

- `src/vitte/compiler/frontend/parse/lookahead.vit` is a strong candidate for stale or unused code.
- `src/vitte/compiler/frontend/grammar_alignment_checker.vit` appears semantically out of sync with the active token API.
- Several legacy backend entry points look more like placeholder compatibility surfaces than production paths.

## 3. Parser audit

### What exists

The parser has real entry points for declarations, statements, expressions, types, and patterns:

- `parse_decl`
- `parse_stmt`
- `parse_expr`
- `parse_type_expr`
- `parse_pattern`
- `parse_proc_decl`
- `parse_match_expr`

It also has AST-building helpers such as:

- `build_ast_module`
- `parse_ast_expr_from_tokens`
- `parse_stmt_from_tokens`
- `parse_block_stmts`

### What is implemented well enough to count as present

- Core items: `space`, `use`, `export`, `const`, `static`, `global`, `type`, `proc`, `test`, `bench`
- Core statements: `let`, `const`, `set`, `give`, `if`, `while`, `for`, `loop`, `match`, `unsafe`, `defer`
- Core expressions: literals, identifiers/paths, unary/binary operators, calls, indexing, members, blocks

### What is only superficially implemented

Many advanced declaration families are parsed at the surface but largely skipped rather than lowered into robust typed AST structures:

- `trait_decl`
- `impl_decl`
- `extern_block`
- `query_decl`
- `compiler_decl`
- `pass_decl`
- `backend_decl`
- `diagnostic_decl`
- `macro_decl`
- parts of `opaque_type_decl`

The same pattern appears for several expression and statement forms where parsing support exists but AST construction or semantics are incomplete:

- `try_stmt`
- `asm_stmt`
- `emit_stmt`
- `panic_stmt`
- `assert_stmt`
- `break_stmt`
- `continue_stmt`
- `unreachable_stmt`
- `select_stmt`
- `when_stmt`
- `with_stmt`
- `resource_lit`
- `map_lit`
- `set_lit`
- `proc_expr`
- `lambda_expr`
- `await_suffix`
- `slice_expr`
- `builtin_expr`

### Missing or incomplete EBNF coverage

The repository contains the full 271-rule grammar, but the implementation does not demonstrate full rule-by-rule coverage with AST construction, diagnostics, and tests. The largest missing or incomplete areas are:

- advanced top-level compiler/meta declarations
- trait/impl/associated type semantics
- advanced type grammar
- advanced pattern grammar
- several statement forms beyond the core imperative subset
- several container/member forms
- parts of async/resource/diagnostic/backend declaration syntax

### Parser design issue

There is a concrete divergence between surface parsing and AST reconstruction for control flow. For example, AST rebuilding for `if` and `while` expects a parenthesized condition in places where the surface parser does not require that shape. This is a real correctness risk.

## 4. Lexer audit

### Implemented

The lexer recognizes:

- identifiers and keywords
- integers and floats
- double-quoted strings
- comments
- single-char and selected multi-char symbols

### Missing tokens relative to the grammar

The following grammar-relevant tokens are not correctly demonstrated in the active scanner:

- char literals using single quotes
- `~`
- `&&`
- `||`
- `<<=`
- `>>=`

### Design limitation

`TokenKind` is currently too coarse for some internal consumers. Several frontend files expect finer token categories that the active token model does not provide.

## 5. AST audit

### Present and used

The AST layer is broad and includes items, statements, expressions, types, and patterns. Core nodes are actively used:

- local declarations and assignments
- control flow
- match
- basic calls and operators
- core item declarations

### Incomplete or inconsistently built

Many enum variants exist without strong evidence that they are reliably constructed by the parser in the production path. These include significant parts of:

- `AstExprKind`
- `AstStmtKind`
- `AstTypeKind`
- `AstPatternKind`

### Variants with weak construction evidence

Based on parser-path inspection, the following families are especially suspect:

- `Await`, `Yield`, `Closure`, `Slice`, `Macro`, `Comptime`, `Resource`
- statement kinds for `Try`, `Asm`, `Emit`, `Panic`, `Break`, `Continue`, `Unreachable`
- advanced type forms such as trait objects, infer, self type, never type, const/variadic/future/result forms
- advanced patterns such as slice/rest/variant/typed binding patterns

## 6. HIR audit

- HIR exists in `src/vitte/compiler/middle/hir/hir.vit`.
- It is used by the active middle pipeline.
- It carries more surface area than the proven frontend subset feeding it.
- The main issue is not absence, but mismatch between HIR breadth and frontend/type-system completeness.

Verdict: HIR is real and structurally important, but only moderately mature.

## 7. Semantic analysis audit

### Demonstrated

- local symbol introduction
- duplicate binding diagnostics
- unresolved name diagnostics
- basic import resolution
- some projected/member/index target handling

### Not demonstrated as complete

- full module visibility rules
- robust namespace layering
- trait resolution
- impl lookup
- associated items
- generic parameter and constraint solving
- full extern/import/module semantics

Verdict: sema exists, but it is still a narrow semantic core rather than a full language semantics engine.

## 8. Type checker audit

### Checks that are clearly present

- unknown inferred type rejection for some `let` forms
- assignment compatibility checks
- boolean/truthy condition checks for `if`, `while`, and `static_assert`
- some match fallback/exhaustiveness-related checking

### Missing or not demonstrated

- generic inference and unification
- trait solving
- impl selection
- richer coercions
- full advanced callable/signature validation semantics beyond the now-canonical param transport
- robust algebraic type checking
- full pattern typing
- deep exhaustiveness
- effect/async typing

There is also an architectural split: a `complete` typeck path exists, but the production pipeline uses `run_typeck_hir`. That indicates an unfinished migration rather than a single hardened type-checker.

## 9. Borrow checker audit

### Demonstrated

- ownership tracking
- moves
- mutable/immutable borrow conflict checks
- return-of-local-reference checks
- partial move scenarios
- scope-sensitive borrow ending in tests
- call handling that consumes canonical MIR function params, including impl-method-style signatures

### Missing or uncertain

- full language-wide integration across all advanced types and syntax
- proof of complete lifetime semantics on the scale of Rustc
- stronger rejection of malformed MIR/IR ownership and nominal-call forms before all backend consumers

Verdict: borrow checking is one of the strongest subsystems in the repository.

## 10. MIR audit

### Present

- MIR types and structures
- lowering from HIR
- MIR validation
- MIR participation in the backend pipeline
- canonical function/signature metadata carried into lower backend stages

### Missing or not demonstrated

- full SSA form
- sophisticated CFG optimization pipeline
- a mature pass ecosystem
- complete hardening of validator rules for canonical params, borrow forms, and dispatch targets

Verdict: MIR is real and useful, but not yet a high-end optimization IR.

## 11. Backend audit

### C backend

- Status: present
- Strength: strongest backend path today
- Missing: proof of full language lowering coverage, complete runtime integration, broader platform confidence

### LLVM backend

- Status: partial
- Strength: canonical IR emission exists and now consumes canonical borrow ABI and nominal-call metadata
- Missing: evidence of a canonical, production-grade LLVM path with deep validation and target completeness

### ASM/native backend

- Status: partial
- Strength: instruction selection and emitter/codegen machinery exist
- Missing: demonstrated completeness across language surface and targets

### WASM backend

- Status: effectively absent
- Missing: real codegen pipeline

## 12. Runtime audit

### Demonstrated

- some backend C runtime hooks
- panic/runtime support surfaces
- stdlib-facing host/runtime assumptions

### Not demonstrated as mature runtime subsystems

- allocator/runtime memory model completeness
- collections runtime guarantees
- scheduler/threads maturity
- async runtime maturity
- IO runtime completeness

Verdict: runtime exists as a fragmented support layer, not yet as a clearly complete standalone subsystem.

## 13. Stdlib audit

### Broad families present in source tree

- `core`
- `collections`
- `io`
- `path`
- `network`
- `threading`
- `async`
- `json`
- `crypto`
- `encoding`
- `compression`
- `data`
- `ffi`
- `kernel`
- `math`

### Completed modules

No module is marked “complete” in this audit, because the repository does not provide enough implementation proof to justify that claim conservatively.

### Incomplete modules

All families above should currently be considered incomplete until a stronger implementation contract and coverage story exists.

## 14. Tests audit

### Stronger coverage areas

- lexer
- parser core paths
- borrow checker
- bootstrap/native gates
- backend surface audits
- strict bridge execution of promoted analysis, middle, and backend suites

### Weaker coverage areas

- full rule-by-rule EBNF conformance
- semantic completeness for traits/impls/generics/modules
- end-to-end type-system completeness
- advanced backend correctness beyond the promoted canonical borrow/nominal-call subset
- runtime and stdlib behavioral contracts

### Overall assessment

The test volume is high, but the suite is better at protecting selected surfaces and gates than at proving complete language/compiler correctness.

## 15. Documentation audit

### Present

- language/spec docs
- compiler architecture docs
- backend docs
- bootstrap/self-host docs
- generated grammar/book/docs site

### Gaps

- implementation status is not always clearly separated from aspirational or declarative surface
- grammar documentation is much broader than proven parser coverage
- stdlib docs do not prove stdlib maturity

Verdict: documentation is broad and valuable, but it currently overstates practical implementation breadth if read as a capability contract.

## 16. Main grammar violations and mismatches

These are the clearest mismatches between the official grammar and demonstrated implementation:

- official grammar contains char literals, active lexer does not prove them
- official grammar uses operators/tokens not recognized by the active scanner
- official grammar contains broad trait/impl/backend/compiler declaration families with no demonstrated full AST+sema pipeline
- official grammar contains broader pattern/type/resource/async syntax than the production parser/typeck path proves

## 17. Roadmap to self-hosting

### Phase 1: Frontend correctness

- Goal: align lexer, parser, AST construction, diagnostics, and tests with the official EBNF
- Dependencies: none
- Difficulty: high
- Estimate: 4 to 8 weeks
- Main files: `frontend/lexer/*`, `frontend/parse/*`, `frontend/ast/*`, `grammar/vitte.ebnf`, parser/lexer tests
- Validation: full grammar coverage matrix, zero stale frontend token consumers, no parser/AST divergence

### Phase 2: Single semantic core

- Goal: harden sema and converge on one production type-checking path
- Dependencies: phase 1
- Difficulty: high
- Estimate: 6 to 10 weeks
- Main files: `analysis/sema/*`, `analysis/typeck/*`, HIR typing/lowering tests
- Validation: modules/imports/visibility/core generics/type rules tested end-to-end

### Phase 3: Intermediate representation contract

- Goal: define and enforce the supported AST -> HIR -> MIR subset
- Dependencies: phases 1 and 2
- Difficulty: medium/high
- Estimate: 3 to 6 weeks
- Main files: `middle/*`, HIR/MIR validators, pipeline tests
- Validation: every supported frontend construct lowers cleanly and is validated, and invalid canonical params/borrow/dispatch forms are rejected deterministically

### Phase 4: Canonical backend closure

- Goal: choose one reference backend path and isolate legacy backend surfaces
- Dependencies: phases 1 to 3
- Difficulty: high
- Estimate: 4 to 8 weeks
- Main files: `backend/*`, `backends/*`, driver pipeline, emission tests
- Validation: `bin/vitte build src/vitte/compiler/main.vit -o ...` succeeds through the canonical path only

### Phase 5: Self-host parity

- Goal: reach reproducible self-hosted closure without bootstrap bridge dependencies
- Dependencies: phases 1 to 4
- Difficulty: very high
- Estimate: 6 to 12 weeks
- Main files: bootstrap scripts, compiler driver, backend runtime/link path, stage gates
- Validation: stage2/stage3 parity, reproducible outputs, mandatory real-native success gates

## 18. Absolute priorities

1. Fix lexer coverage to match the official grammar.
2. Replace shallow parser sync checks with a real 271-rule coverage matrix.
3. Unify parser surface parsing and AST construction.
4. Harden sema/typeck so the compiler catches its own internal inconsistencies.
5. Tighten MIR/IR validation around the canonical ownership and nominal-call model while continuing to quarantine legacy backend surfaces.

## 19. Main risks

- false confidence from wide surface area with shallow implementation
- frontend drift hidden by weak internal checking
- semantic migration split between multiple partially overlapping systems
- backend ambiguity between canonical and legacy paths, despite recent canonicalization progress
- self-hosting blocked by correctness debt rather than by missing infrastructure

## 20. Top five strengths

1. Real compiler architecture exists across all major stages.
2. Bootstrap and real-native engineering discipline are strong.
3. Borrow checking is significantly more mature than most other advanced subsystems.
4. The repository already has serious testing and audit tooling.
5. The path to self-hosting is explicit and technically plausible.

## 21. Final conclusion

The Vitte compiler is not a toy frontend anymore. It has a real compiler-shaped architecture, a credible bootstrap story, a meaningful HIR/MIR pipeline, and one notably solid advanced subsystem in borrow checking.

The main problem is not lack of ambition or lack of code. It is the gap between declared surface and proven implementation. The official grammar, AST surface, backend surface, and documentation all describe a language/compiler that is broader than the subset the repository currently demonstrates reliably. Recent work substantially improved canonical ownership/signature flow and stable gate coverage, but the next bottleneck is now validator hardening and frontend/spec alignment rather than raw pipeline wiring.

Conservative overall maturity: `46%`.
