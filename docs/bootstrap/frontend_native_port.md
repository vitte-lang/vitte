# Bootstrap: native Vitte compiler front-end

This document tracks the transition from C bootstrap bindings to compiler
modules implemented in Vitte. `src/bootstrap/` still supplies the executable
and host-toolchain boundary. Selected `src/compiler/` modules are compiled
from Vitte into the linked native pipeline archive by the stage0 bootstrap;
the compiler is not yet self-hosted.

## Binding audit

The audit counts `export extern proc` declarations in
`src/compiler/**/*.vitte`. The current snapshot contains 271 bindings across
17 files. This is a binding count, not a count of fully declaration-only
modules. Audit date: 2026-10-23.

| Area | Modules | Extern declarations |
| --- | --- | ---: |
| Parsing and semantic analysis | `parser`, `scope`, `symbol`, `type`, `sema` | 0 |
| Pipeline and code generation | `backend/c17/backend` 1, `backend/c17/writer` 3 | 4 |
| Front-end support | `constant_fold/constant` 5, `hir` 29, `import` 25, `ir` 49, `module` 31, `scanner` 46 | 185 |
| CLI and configuration | `config` 1, `filesystem` 21, `printer` 32 | 54 |
| Diagnostics | `diagnostic/cli_bridge` 8, `label` 2, `render_json` 6, `render_lsp` 5, `render_sarif` 3, `render_terminal` 4 | 28 |
| **Total** | **17 files** | **271** |

`source`, `lexer`, `builtin`, `parser`, `scope`, `symbol`, `type`, `sema`,
`driver`, `pipeline`, `codegen`, `unicode`, `diagnostic::registry`,
`diagnostic::source_map`, and `diagnostic::suggestion` have no
`export extern proc` declarations in this snapshot. Diagnostic suggestions
and fix plans own their spans and replacement strings. The LSP position
converter now reads the native `SourceContext` and handles UTF-8, UTF-16 and
UTF-32 character units. The diagnostic externs include host stream output
entrypoints and the scalar CLI transport adapter. Their C renderers consume
bootstrap C-layout records; the native `Diagnostic` and `DiagnosticBag` must
not be passed to them without an explicit ABI conversion. The CLI uses a scalar
host adapter that allocates and fills the C renderer layout; replacing
format-specific rendering with native serializers or a byte-writing-only host
boundary remains open. A zero count does not establish complete semantic
coverage or integration into the default route.

### API and driver — native frontend with persistent C17 output

`api_compile_source` invokes `native_pipeline_run` in Vitte. The active path
runs the Vitte lexer, parser, semantic analyzer, validated HIR/IR lowering,
and native C17 backend for inputs selected for the native route. The IR path
handles scalar programs, value-producing `if` expressions, integer range loops,
and the currently supported match patterns. Some forms, aggregate cases, test
declarations, and non-range `for` iterables use the native AST C17 compatibility
emitter; selected unsupported source features are routed to bootstrap before
native frontend selection. It stores generated C17 bytes
and formatted diagnostics in the returned `ApiResult`; it does not send the
generated source through a temporary C stream. The CLI uses the shared pipeline
for eligible `compile`, `run`, `check`, and `--emit-ir` inputs, then passes
successful C17 output to the host C17 compile driver. Once selected, native
`compile` and `run` failures are authoritative and do not retry through
bootstrap; when the frontend returns no diagnostic, the CLI emits a structured
phase failure. Lexer, parser and
semantic diagnostics are sent as scalar fields through the explicit
host adapter, which allocates the C renderer records itself. The CLI treats
those diagnostics as authoritative and does not rerun bootstrap analysis for
them. Inspection still uses bootstrap stages. The host
CLI adapter still formats terminal, JSON, SARIF and LSP output; no Vitte
structure is passed as a bootstrap C layout.

## Port status

### Source manager — implemented in Vitte

`src/compiler/source/source.vitte` now implements all 27 functions declared
by `source.h`, with no extern bindings. It owns source entries and line offsets in
Vitte arrays and strings. It implements registration, UTF-8 validation,
CR/LF/CRLF indexing, line/column conversion, slices, spans, validation,
statistics, and the deterministic FNV fingerprint. It calls only the native
Vitte Unicode module.

### Lexer — implemented in Vitte

`src/compiler/lexer/lexer.vitte` now implements all 56 public/header helper
functions declared by `lexer.h`, with no extern bindings. It owns token storage in a Vitte array and
implements Unicode identifiers, keywords and reserved words, numbers,
strings, character literals, escapes, nested comments, maximal-munch
operators, spans, validation, and fingerprints. The token enum was aligned
with the bootstrap header, which had `KwPrint` and `KwReserved` entries missing
from the Vitte declaration. Header inline queries are also implemented as
native Vitte procedures. Reference fields are emitted through explicit
dereferences, and token lookup reads the Vitte array through a typed local
view so the bootstrap C emitter produces valid C17.

### AST and parser — native parser on the ordinary compile route

`src/compiler/ast/ast.vitte` implements a separate arena-backed AST store.
`src/compiler/parser/parser.vitte` implements parser initialization, reset,
destruction, validity checks, resource-limit updates, the token cursor, public
token lookups, diagnostic storage and reporting, and flat AST-node storage in
Vitte. It also contains delimiter-aware recovery primitives for declarations,
statements, and lists, bounded by the configured recovery-token limit, plus
grammar helpers for identifier and path AST nodes and `use`, `export`, and
value-declaration productions. `use` supports aliases, glob imports, and
grouped items; `export` supports paths, aliases, export-all, and grouped items.
The `const` and `static` productions accept optional type annotations and
expressions. The native expression subset includes literals and paths,
parenthesized and array expressions, unary and binary operators with bootstrap
precedence and associativity, form expressions, calls, indexing, member access,
postfix `?`, casts, `if` expressions, block expressions, `print`, `sizeof` and
`alignof`. Some expression and type productions remain incomplete. Grouped
imports and exports recover at list separators.
Paths accept contextual keywords, the reserved module segments `compiler` and
`diagnostic`, and `::` or `/` separators. The parser module currently has no
`export extern proc` declarations.

The distinct `vitte_parser_run_native` entry now drives a translation-unit
loop for `space`, `use`, export clauses, `const`, `static`, `type`, `opaque`,
`form`, `pick`, `trait`, `impl`, `proc`, `extern proc`, `macro`, and `test`
declarations. Type aliases support optional bounds;
`opaque [type]` declarations support an optional underlying type. Forms accept
typed fields separated by commas or semicolons. Picks accept named variants
with optional positional type payloads. Procedures accept named typed
parameters, an optional return type, and either a body or prototype terminator;
external procedures accept an optional ABI string and require a semicolon.
Traits accept a name and declaration body. Implementations accept a target type,
an optional `for` trait type, and declaration body. Macros accept a name,
optional typed parameters, and a body. Tests accept an optional string or
identifier name and a body. Type generic parameters accept optional
`Bound + Bound` lists; `where` clauses accept trait bounds and comparison
predicates on type subjects. These are supported on type, opaque, form, pick,
procedure, trait, and implementation declarations. Procedure `requires` and
`ensures` clauses accept expressions terminated by semicolons and retain their
contract kind on each condition node. Form field initializers and members other
than typed fields, pick underlying types, explicit variant values, and complex
parameter patterns remain unsupported. Trait and implementation bodies are
limited to the declaration forms currently recognized by the native parser.
`#[name]` and `#![name]` attribute prefixes accept qualified names and the
bootstrap's simple argument list syntax. The parser stores each attribute's
name and argument token spans as well as the declaration flag.
Unsupported declaration forms produce parser diagnostics and use declaration
recovery. `vitte_parser_lower_to_ast` recursively maps completed flat nodes to
the arena-backed AST, carries spans, names, operators, modifier flags, attribute
presence and contract-kind flags, and preserves type-alias and type-expression
flags. It rewinds the arena if conversion fails. The bootstrap ABI entry
`vitte_parser_run` stays bound to the existing C parser.

`pipeline.vitte` converts native lexer tokens to the parser ABI, runs the
native parser and semantic analyzer, validates HIR and IR, and dispatches to
the native IR C17 backend by default for supported programs. Constructs without
IR lowering use the native AST emitter when eligible; known compatibility
cases can be routed to bootstrap before native frontend selection. The root
Makefile and CMake executable link this pipeline for eligible `compile`,
`run`, `check`, `--emit-c`, and `--emit-ir` inputs. A failure from a selected
native frontend is authoritative. The bridge C file
supplies ABI wrappers and host integration, not parser, semantic, HIR/IR, or
emitter implementations.

Diagnostic spans come from the current token, configured diagnostic limits are
enforced, and duplicate errors are suppressed while recovery is active. Node
IDs are one-based, node and child-ID buffers grow with overflow checks, and
allocation statistics saturate at `UINT64_MAX`. The cursor handles EOF without
advancing past it, saturates its consumption counter, and rejects overflowing
lookahead. Diagnostic and node lookups validate the parser context and bounds.
Its token, span, enum, and context layouts match `parser.h`, including
`OptionalType` and the recovery flag.

The node-kind enum and parser flags now match `parser.h`; a missing `Pattern`
node kind and the earlier flag-width/bit-position mismatch were corrected
during this audit.

### Name resolution and semantic analysis — native pipeline, parity incomplete

The active pipeline calls the Vitte parser and semantic analyzer. Parser,
scope, symbol, type, and sema have no `export extern proc` bindings in the
current source audit, but the supported language and cross-phase behavior are
still incomplete. Successful compilation lowers through validated native
HIR/IR; the IR C17 backend handles the supported subset and the AST emitter
covers constructs still missing IR lowering. `ApiResult` records an error
category and diagnostic origin with stage-specific counts, but does not expose
parser/sema diagnostic locations. Ordinary `compile`/`run` stop on native
errors.

### C17 emission and host toolchain

The default CLI emits C17 through the native IR backend when HIR/IR lowering
supports the input, and through the native AST emitter for remaining aggregate
constructs eligible for that route. Some unsupported source shapes select the
bootstrap compatibility path before native frontend selection. The C host
driver writes emitted output or invokes the configured host C compiler for
object generation, linking, and execution. Inspection and host toolchain
operations still use C boundaries; full self-hosting and backend parity remain
open.

### Language grammar versus native parser coverage

`grammar/vitte.ebnf` defines the normative language surface. The generated
copies under `docs/grammar/` and `docs/book/grammar/` are references, not
acceptance promises for the native pipeline. The active Vitte parser is still
a subset; parser acceptance also does not imply semantic analysis or codegen
support.

| Surface | Native parser status | Native compilation status |
| --- | --- | --- |
| `space`, imports, exports, constants, statics, aliases, procedures | Parsed | Import resolution and downstream parity remain incomplete |
| `form`, `pick`, generics, `where`, `trait`, `impl` | Parsing routines exist | Some form and aggregate cases use AST C17 compatibility emission; type checking, layouts, member access, variants, and trait use remain partial |
| `if`, `while`, `loop`, integer-range `for`, `let`, `set`, `return`, `break`, `continue` | Parsed | Supported control flow lowers through HIR/IR; non-range `for` iterables and broader parity remain open |
| `match` and pattern arms | Parsed | Supported literal, wildcard, and scalar/pick patterns lower through native IR; wider pattern coverage remains incomplete |
| Macro declarations | Declaration syntax is parsed | Macro expansion is not implemented in the default pipeline |
| `async` and `await` | Syntax/AST markers exist | Default backend lowering and emission are unsupported |
| `compiler`, `pass`, `backend`, `diagnostic`, and other meta declarations | In the normative grammar; not all are dispatched by the native parser | Metadata does not imply an executable compiler pass/backend |

This table tracks the native parser and its current compilation route. The
bootstrap parser has a different support surface; compare it separately in
the parity corpus rather than treating either implementation as the language
definition.

## Dependency order

```text
source -> lexer -> AST/parser -> scope + symbol + type -> sema
                                                     -> HIR -> IR -> codegen -> backend -> driver
```

The linked Vitte pipeline reaches C17 source generation through the Vitte
lexer, parser, semantic analyzer, HIR/IR validators, codegen orchestrator, and
native C17 backend. The AST emitter remains in the call chain for constructs
without IR lowering. Inspection and host C compilation/linking remain C
boundaries; the compiler has not completed a clean self-hosting cycle.

The default build first creates `vitte-bootstrap` from C files under
`src/bootstrap/`, uses it to compile the Vitte pipeline, then links that
pipeline into `vitte`. The CLI calls the native pipeline for eligible
`compile`, `run`, `check`, and `--emit-ir` inputs and routes successful C17
output through the host compiler.
Bootstrap C remains linked for compatibility routing, inspection, diagnostic
rendering and host toolchain operations. Removing those modules is a later
cutover step, after native diagnostics, backend output, host compilation, and
execution work end to end.

## Verification status

`make` and the CMake default target include the native archive in the final
link and install a CLI caller for the pipeline. Native Vitte procedure symbols
include their `space` path, which keeps them distinct from bootstrap C symbols.
The default C17 backend is the Vitte IR emitter for the supported subset. The
integration test now checks that scalar `compile --emit-c` uses it and that a
form declaration selects the AST compatibility emitter.

On 2026-10-26, `make build/bin/vitte` completed and linked the native pipeline
into the CLI. `make test-native-frontend-subset` passed with 5 valid and 17
invalid frontend cases. `make compiler-power-gate` passed, including 30 native
versus bootstrap check-parity cases (18 valid, 4 parser errors, 8 semantic
errors), 12 shared runtime cases, and 4 additional native IR runtime cases.
The scalar compile fixture uses IR C17 emission; three aggregate fixtures still
exercise the AST compatibility emitter. These results cover the focused gate,
not full language parity or a clean self-hosting cycle. The current source
audit counts 271 `export extern proc` declarations in 17 compiler modules;
this supersedes older release-note counts of 268.

The wider `make compiler-max-gate-fast` and `make compiler-gate` targets, as
well as a compiler self-rebuild, have not been verified in this status update.

## Remaining work checklist

Parser milestones 1–10 are implemented in the native parser: translation-unit
loop and AST lowering, ordinary compile-route wiring, declaration families,
generics and `where`, attributes, and procedure contracts. The checklist tracks
what remains before the native front-end can replace the bootstrap route.

### P0 — prove parser and AST parity

- [x] Link the native pipeline into the standard executable and call it for
  eligible `compile`, `run`, `check`, and `--emit-ir` inputs.
- [ ] Add parser fixtures for every supported declaration, expression and
  statement form, including valid and invalid examples.
- [ ] Compare native and bootstrap parse results over the same corpus: accepted
  source, declaration order, node kinds, child order, names, operators, flags,
  spans and error locations.
- [ ] Cover the new generic and `where` forms, repeated `requires`/`ensures`,
  external procedures, and all attribute prefix/argument forms in the parity
  corpus.
- [ ] Exercise recovery after malformed nested lists, missing delimiters,
  malformed contracts and invalid declarations; confirm parsing resumes at the
  same following declaration as bootstrap.
- [ ] Exercise EOF, overflowing lookahead, recursion limits, node and diagnostic
  limits, recovery-token limits, and allocation-failure cleanup.
- [ ] Lower each accepted native tree into the arena AST and validate ownership,
  parent links, child counts, flag masks, spans and rollback on conversion error.

### P1 — close known grammar and AST gaps

- [ ] Add pick underlying types and explicit variant values where they are part
  of the bootstrap grammar.
- [ ] Complete form declarations beyond typed fields, including field
  initializers and any other bootstrap-supported member form.
- [ ] Support the bootstrap's complex procedure parameter patterns and verify
  mutable/reference markers and parameter spans.
- [ ] Cross-check every remaining bootstrap declaration, type, expression,
  pattern and statement production against the native grammar; record each
  intentional omission here instead of relying on the broad “partial grammar”
  label.
- [ ] Bring trait and implementation member parsing to bootstrap parity,
  including prototypes and associated declarations supported by the language.
- [ ] Map each retained `ParserAttribute` into `AstNode.attributes`, preserving
  its qualified name, argument/value representation, inner/outer form and
  source span. Parsing stores these details; AST lowering currently copies
  only the `HAS_ATTRIBUTES` flag.
- [ ] Verify attribute placement and modifier ordering on every declaration
  family, including repeated attributes and recovery from malformed values.
- [ ] Verify generic parameter and `where` syntax against bootstrap for empty
  lists, trailing commas, multiple bounds, comparisons and nested types.
- [ ] Document the exact expression and type subset. Calls, indexing, member
  access, postfix `?` and casts are already parsed; keep them out of the open
  list unless a parity gap is found.
- [ ] Verify all parser flags and declaration metadata survive arena-AST
  lowering, including the `requires`/`ensures` condition flags and individual
  attributes.
- [x] Remove parser `export extern proc` bindings; the current source audit
  finds none in `parser.vitte`.

### P2 — make the native parser authoritative

- [x] Define the stable boundary from native parser tokens and AST nodes to
  semantic analysis. `ParserTokenSpan` carries file identity and byte offsets;
  `vitte_sema_init` consumes the native `Parser` directly.
- [x] Feed native parser output to name/type analysis and diagnostics through
  `native_pipeline_run_with_config`, retaining source identity and byte spans.
- [x] Keep the C bridge limited to ABI wrappers; it does not call bootstrap
  lexer, parser, semantic, HIR/IR, or emitter implementations.
- [x] Stop retrying ordinary `compile`/`run` through the bootstrap parser after
  native failures; preserve explicit native diagnostics instead.
- [x] Route eligible `check` and `--emit-ir` inputs through the shared native
  pipeline.
- [ ] Migrate inspection and expand native routing as frontend parity and
  failure-path coverage improve.

### P2 — port semantic analysis and front-end support

- [ ] Complete scope and symbol parity checks, including declaration and
  lexical-scope construction from the parser AST.
- [ ] Implement native type storage, interning, unification and conversions;
  validate generics, bounds, traits, opaque types and procedure contracts.
- [ ] Port semantic analysis for declarations, expressions, statements,
  patterns, imports, visibility and diagnostics.
- [ ] Connect native builtin registration, constant folding, module loading and
  import-graph resolution to the front end.
- [x] Remove silent bootstrap fallback from ordinary `compile` and `run`:
  native failures now stop with their diagnostics or a phase diagnostic when
  no structured diagnostic is available.
- [ ] Expand native diagnostic parity and failure-path coverage before
  declaring compilation at parity; some unsupported inputs still select
  compatibility paths.

### P3 — port lowering and code generation

The active Vitte pipeline validates HIR and IR and routes supported programs
through the native IR C17 backend. Value-producing `if`, integer-range loops,
and supported match patterns use this path. Forms and aggregate cases still
use AST C17 compatibility emission; non-range `for` and selected unsupported
source features use compatibility routes.

- [x] Connect native HIR lowering and validation to the default compile path.
- [x] Connect validated native IR to the native C17 backend for its supported
  subset; verify default dispatch and AST compatibility dispatch in integration
  coverage.
- [ ] Complete native IR lowering, verification and target-independent passes;
  retain deterministic fingerprints for differential checks.
- [ ] Connect the native codegen orchestrator to native IR and remove its
  bootstrap callback dependencies.
- [ ] Integrate the separate Vitte C17 backend with the driver and verify parity
  with the current bootstrap C17 IR emitter.
- [ ] Keep backend tracks distinct until the native backend handles declarations,
  calls, aggregates, strings, imports, runtime helpers and diagnostics end to end.

### P4 — complete driver cutover

- [x] Return generated source and formatted diagnostics through the native
  `ApiResult`; the API copies the pipeline's generated bytes into owned result
  storage instead of routing them through a temporary C stream.
- [x] Pass native frontend output into the host C17 driver for `--emit-c`,
  object generation, linking, and `run`. The CLI maps C17 emission, external
  compiler, linker, and execution failures to distinct diagnostic codes.
- [x] Make native lexer/parser/sema source diagnostics authoritative in the CLI
  and preserve codes, phase, message, details and source offsets through the
  scalar host adapter. Ordinary `compile`/`run` no longer retry through the
  bootstrap analyzer after native infrastructure or backend failures.
- [ ] Move terminal, JSON, SARIF and LSP formatting into Vitte, or reduce the
  C side to a byte-writing boundary. The current host adapter still performs
  format-specific rendering over records it allocates from scalar CLI fields.
- [ ] Move target compilation/link orchestration behind the native driver
  contract, retaining per-stage diagnostics from the selected host toolchain.
- [ ] Verify a clean self-hosting cycle and remove C bootstrap modules only when
  a Vitte compiler can rebuild and run the compiler without hidden C bridges.
- [x] Re-audit `export extern proc` bindings and update this table from the
  source tree after each module port.

### P3 — cut the bootstrap from the executable path

- [x] Route eligible `compile`, `check`, `--emit-ir` and `run` inputs through
  one native pipeline contract. Unsupported source features can still select
  compatibility routes.
- [x] Remove the ordinary `compile`/`run` CLI fallback to bootstrap after
  native infrastructure failures; the CLI now reports the native failure.
  The native selection boundary and its compatibility cases remain open work.
- [ ] Move object compilation, linking and execution orchestration behind the
  native driver contract while preserving per-stage host-tool diagnostics.
  These operations still run in the C CLI/C17 host driver.
- [ ] Replace the remaining compiler C bindings. The current source audit is
  271 external declarations across 17 files, including 28 in diagnostic
  renderers and the CLI bridge.
- [ ] Build a clean compiler with stage0, then use that compiler to rebuild
  itself and run the resulting executable without bootstrap C modules linked
  into its runtime.
- [x] Record the open cutover gaps and the available verification evidence in
  this tracker and the unreleased changelog. The linked CLI build and focused
  `compiler-power-gate` pass; wider compiler gates and self-hosting remain
  unverified.

### P5 — keep documentation and release evidence aligned

- [x] Synchronize the native parser subset with the grammar reference and
  generated grammar artifacts; distinguish language support from native
  parser support.
- [x] Update this status and checklist when a gate passes, and move completed
  items out of the open list rather than leaving stale “unsupported” notes.
- [x] Synchronize the affected HTML reader pages and search/index artifacts
  with their canonical Markdown or grammar sources.
- [x] Record the compiler route, gate evidence, remaining C dependencies and
  known limitations in release notes before declaring the native front end
  complete. The focused power gate passes; wider gates and self-hosting are
  still outstanding.
