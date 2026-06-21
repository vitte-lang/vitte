#!/usr/bin/env python3
from __future__ import annotations

import json
import html
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CHAPTERS_DIR = ROOT / "docs" / "book" / "chapters"
POCKET_DIR = ROOT / "docs" / "book" / "poche" / "chapters"
META_PATH = ROOT / "docs" / "book" / "chapters-meta.json"


def code_block(language: str, text: str) -> str:
    return f'<pre><code class="language-{language}">{html.escape(text)}</code></pre>'


def doc_ref_link(ref: str, rel: str, *, code: bool = True) -> str:
    label = html.escape(ref)
    href = f"{rel}{html.escape(ref)}"
    if code:
        return f'<a href="{href}"><code>{label}</code></a>'
    return f'<a href="{href}">{label}</a>'


def slug_profile(path: str, title: str) -> str:
    key = f"{path} {title}".lower()
    if any(t in key for t in ["foreword", "preface", "philosophy", "avant-propos"]):
        return "philosophy"
    if any(t in key for t in ["start", "demarrer", "project", "projet"]):
        return "starting"
    if "syntax" in key or "syntaxe" in key:
        return "syntax"
    if "types" in key or "values" in key:
        return "types"
    if "procedures" in key or "rules" in key:
        return "procedures"
    if "control" in key or "controle" in key:
        return "control"
    if any(t in key for t in ["structures", "collections", "data structures"]):
        return "collections"
    if "modules" in key or "organization" in key:
        return "modules"
    if any(t in key for t in ["diagnostics", "errors", "error-message", "build-errors"]):
        return "diagnostics"
    if "pointers" in key:
        return "pointers"
    if "generics" in key or "generiques" in key:
        return "generics"
    if "macro" in key:
        return "macros"
    if any(t in key for t in ["pipeline", "architecture", "compiler mental model", "ast-to-ir", "ast mapping"]):
        return "compiler"
    if any(t in key for t in ["interop", "binding", "abi"]):
        return "interop"
    if any(t in key for t in ["stdlib", "modules catalog"]):
        return "stdlib"
    if any(t in key for t in ["tests", "test", "regression", "integration", "property-based", "fuzzing"]):
        return "testing"
    if any(t in key for t in ["performance", "allocations", "data structures", "measure before optimizing"]):
        return "performance"
    if any(t in key for t in ["repro", "debug", "observability", "tracing"]):
        return "debugging"
    if any(t in key for t in ["cli", "http", "system", "kv", "arduino", "editor", "production-ready", "service", "embedded"]):
        return "projects"
    if any(t in key for t in ["grammar", "ebnf", "ambiguity", "diagram generation"]):
        return "grammar"
    if any(t in key for t in ["conventions", "style", "naming", "api design", "compatibility", "versioning", "hexagonal", "skill growth", "documenting documentation", "suggested chapters", "checklist"]):
        return "design"
    if any(t in key for t in ["ci", "code review", "technical documentation"]):
        return "delivery"
    return "generic"


PROFILES = {
    "philosophy": {
        "problem": "Readers often learn syntax before learning the design contract of the language. That creates code that compiles but is hard to maintain, review, or debug.",
        "thread": "One tiny service module evolves from a vague script into a deliberate Vitte design with explicit contracts, stable names, and visible failure boundaries.",
        "for_what": "This chapter helps the reader understand what Vitte optimizes for before touching advanced syntax.",
        "what": "You will read one small module, identify the design choices that make it readable, then compare that with a weaker variant.",
        "example": """space demo/philosophy

form BuildRequest {
  name: string
  retries: int
}

pick Decision {
  case Accepted(name: string)
  case Rejected(code: int)
}

proc decide(req: BuildRequest) -> Decision {
  if req.name == "" { give Decision.Rejected(11) }
  if req.retries < 0 { give Decision.Rejected(12) }
  give Decision.Accepted(req.name)
}

entry main at core/app {
  return 0
}""",
        "invalid": """proc vague(x: int) -> int {
  if x { give 1 }
  give 0
}""",
        "global": "The chapter is not about one token. It is about explicit contracts, clear boundaries, and stable failure surfaces. The example is small, but it already separates domain data, decision logic, and process entry.",
        "pitfalls": [
            "Talking about beauty or simplicity without showing a concrete contract boundary.",
            "Using philosophy chapters as prose-only pages with no executable anchor.",
            "Describing intent without showing what the compiler can actually enforce.",
        ],
        "exercise": "Take one vague helper in your own code and rewrite it so that the domain input, the branch conditions, and the result boundary are explicit.",
        "summary": [
            "Vitte favors explicit contracts over implicit convention.",
            "A philosophy chapter still needs code, not only prose.",
            "The smallest useful example already separates domain and transport.",
            "Invalid paths are part of the design contract.",
            "Readable structure is a technical property, not decoration.",
        ],
    },
    "starting": {
        "problem": "Beginners often see the first file as a bag of syntax instead of the smallest end-to-end executable program.",
        "thread": "The same first project grows from one entry point to a file with data, a procedure, a check, and one visible exit code.",
        "for_what": "This chapter helps a new reader build and understand the first complete Vitte program.",
        "what": "You will read one minimal project, identify the essential pieces, then inspect one broken variant and the reason it fails.",
        "example": """space demo/starting

proc greet(name: string) -> int {
  if name == "" { give 11 }
  give 0
}

entry main at core/app {
  return greet("world")
}""",
        "invalid": """entry main {
  return 0
}""",
        "global": "The first chapter should prove one thing clearly: a Vitte project is an entry point, a small contract, and an observable result. That is enough to understand before learning richer constructs.",
        "pitfalls": [
            "Teaching a first program as disconnected syntax fragments.",
            "Adding too many concepts before the reader has one stable executable example.",
            "Skipping the invalid case that explains why the entry contract matters.",
        ],
        "exercise": "Extend the example with one small data form and keep the same clean flow from input to exit code.",
        "summary": [
            "A first program must be complete, not decorative.",
            "The entry contract matters from the beginning.",
            "A single procedure is enough to teach flow and failure.",
            "The invalid example should isolate one structural mistake.",
            "The goal is understanding the whole program shape, not memorizing tokens.",
        ],
    },
    "syntax": {
        "problem": "Syntax pages become noisy when they enumerate tokens without showing how a complete block is supposed to look.",
        "thread": "One small block shows declarations, expressions, and returns in a single readable flow.",
        "for_what": "This chapter helps the reader recognize valid Vitte shapes quickly.",
        "what": "You will read one valid block, map its parts to grammar roles, then inspect an invalid variant that breaks the shape.",
        "example": """space demo/syntax

proc compute(x: int, y: int) -> int {
  let sum: int = x + y
  if sum < 0 { give 0 }
  give sum
}

entry main at core/app {
  return compute(1, 2)
}""",
        "invalid": """proc broken(x: int) -> int
  give x
}""",
        "global": "Essential syntax is best taught as structure. The reader should see where declarations live, where statements live, and how a block closes with an explicit result.",
        "pitfalls": [
            "Teaching surface tokens without explaining block shape.",
            "Using syntax pages to comment every line instead of naming the pattern.",
            "Confusing grammar shape with later semantic checks.",
        ],
        "exercise": "Add one extra branch and one extra local binding while preserving the same readable block structure.",
        "summary": [
            "Syntax is about shape before detail.",
            "A valid block is the smallest useful teaching unit.",
            "Statements and declarations belong to different layers.",
            "Invalid examples should isolate shape errors.",
            "The reader should leave able to recognize a healthy block instantly.",
        ],
    },
    "types": {
        "problem": "Readers often learn type names but not what type contracts buy them in a real block.",
        "thread": "A tiny reporting example uses integers, strings, and a form to make value contracts visible.",
        "for_what": "This chapter helps the reader choose types based on meaning, not habit.",
        "what": "You will inspect a typed example, identify where each type contract matters, then compare it to an invalid variant.",
        "example": """space demo/types

form Report {
  name: string
  retries: int
  ok: bool
}

proc score(r: Report) -> int {
  if not r.ok { give 0 }
  give r.retries + 1
}""",
        "invalid": """proc bad_report() -> int {
  let name: int = "demo"
  give name
}""",
        "global": "The value of the chapter is not the list of types. It is the way each type removes ambiguity from the block: text is text, counters are numeric, flags are boolean, and the compiler can enforce that split.",
        "pitfalls": [
            "Using a type by habit instead of by domain meaning.",
            "Explaining individual fields without explaining the full contract.",
            "Forgetting that invalid examples should show a broken meaning, not just a weird token.",
        ],
        "exercise": "Take the example form and add one field whose meaning is obviously not numeric. Choose the type that makes the contract clearest.",
        "summary": [
            "Types are contracts, not decorations.",
            "A small form is a better teaching unit than isolated declarations.",
            "The chapter should connect type names to domain meaning.",
            "Invalid examples should show broken contracts clearly.",
            "Type clarity reduces later diagnostic work.",
        ],
    },
    "procedures": {
        "problem": "Procedure chapters often explain signatures and bodies separately, so readers miss how a complete procedure contract works.",
        "thread": "A small service procedure validates input, transforms it, and returns an observable result.",
        "for_what": "This chapter helps the reader design procedures as explicit contracts.",
        "what": "You will read one complete procedure flow, identify where the contract begins and ends, then inspect a broken variant.",
        "example": """space demo/procedures

proc normalize_score(raw: int) -> int {
  if raw < 0 { give 0 }
  if raw > 100 { give 100 }
  give raw
}""",
        "invalid": """proc broken_score(raw: int) {
  if raw < 0 { give 0 }
}""",
        "global": "A procedure chapter should teach one stable idea: the signature, the branch logic, and the returned result are one contract. The reader should not have to infer what comes back from the procedure.",
        "pitfalls": [
            "Showing signatures without a meaningful body.",
            "Using procedures to hide global state instead of exposing a clean contract.",
            "Failing to explain how the result remains observable and testable.",
        ],
        "exercise": "Add one extra input parameter and keep the procedure contract as readable as before.",
        "summary": [
            "A procedure is a full contract, not just a named block.",
            "The signature and the result shape belong together.",
            "Branch logic should remain visible inside the procedure.",
            "Broken procedures usually hide a missing contract part.",
            "A good example is small but complete.",
        ],
    },
    "control": {
        "problem": "Control-flow chapters become repetitive when they explain keywords separately without one scenario showing how branches cooperate.",
        "thread": "One scoring flow uses guards, loops, and branching to keep the path explicit from input to output.",
        "for_what": "This chapter helps the reader decide when to branch, when to loop, and when to stop.",
        "what": "You will inspect one controlled flow, identify the branch points, then compare it to a malformed control surface.",
        "example": """space demo/control

proc sum_positive(values: list[int]) -> int {
  let acc: int = 0
  for value in values {
    if value < 0 { continue }
    set acc = acc + value
  }
  give acc
}""",
        "invalid": """proc bad_control(x: int) -> int {
  if x { give 0 }
  give x
}""",
        "global": "Control flow must be taught as a route through the program, not as isolated keywords. The reader should see where the path changes, why it changes, and how the result remains understandable.",
        "pitfalls": [
            "Teaching `if`, `for`, and `match` as unrelated pages with no shared scenario.",
            "Adding branches that do not change any observable outcome.",
            "Using invalid examples that break types instead of control shape.",
        ],
        "exercise": "Add one fallback branch to the example and explain what new path it creates.",
        "summary": [
            "Control flow is about path ownership.",
            "A single scenario teaches branches better than isolated snippets.",
            "Each branch should change an observable outcome or invariant.",
            "Invalid control examples should isolate structural mistakes.",
            "Read the path first, the tokens second.",
        ],
    },
    "collections": {
        "problem": "Readers often see containers as names only, not as data-shape decisions embedded in a complete flow.",
        "thread": "One report-building flow stores values, aggregates them, and returns a grouped result.",
        "for_what": "This chapter helps the reader choose structures based on access pattern and meaning.",
        "what": "You will read one grouped-data scenario, identify why the container exists, then inspect a broken variant.",
        "example": """space demo/collections

form Metrics {
  count: int
  total: int
}

proc absorb(values: list[int]) -> Metrics {
  let count: int = 0
  let total: int = 0
  for value in values {
    set count = count + 1
    set total = total + value
  }
  give Metrics(count, total)
}""",
        "invalid": """proc bad_metrics() -> int {
  let values: int = [1, 2, 3]
  give values
}""",
        "global": "Collections pages are useful only when they explain why a shape exists. The example shows aggregation as a real need for a container-like surface instead of presenting structures as vocabulary alone.",
        "pitfalls": [
            "Listing containers without an access pattern or use case.",
            "Treating structure choice as style instead of behavior.",
            "Hiding where grouped data enters and leaves the block.",
        ],
        "exercise": "Replace the single summary form with two grouped buckets and explain why the new shape is justified.",
        "summary": [
            "Data structures are architectural choices.",
            "Use cases matter more than names.",
            "Aggregation flows are better teaching units than isolated declarations.",
            "Broken examples should show a mismatched shape.",
            "Container choice should remain visible in the code story.",
        ],
    },
    "modules": {
        "problem": "Module chapters often stop at import syntax and never explain ownership boundaries across files and packages.",
        "thread": "A tiny application is split into a domain module, a service module, and an entry module.",
        "for_what": "This chapter helps the reader structure code so that imports reflect ownership instead of accident.",
        "what": "You will inspect a small module layout, identify public vs local boundaries, then inspect an import misuse.",
        "example": """space demo/modules

use demo/report.{build_summary} as report

proc run() -> int {
  give report.build_summary()
}

entry main at core/app {
  return run()
}""",
        "invalid": """proc bad_module() -> int {
  use demo/report
  give 0
}""",
        "global": "The point of a modules chapter is not the import token itself. The point is ownership: what belongs together, what should stay private, and what the public surface of a module should look like.",
        "pitfalls": [
            "Using module pages as import syntax cheat sheets only.",
            "Importing too much because boundaries were never designed.",
            "Putting module-surface declarations inside local blocks.",
        ],
        "exercise": "Split one procedure from the example into another logical module and keep the public boundary explicit.",
        "summary": [
            "Modules are ownership boundaries.",
            "Imports should reveal architecture, not hide it.",
            "Public surface and internal detail should be easy to distinguish.",
            "Import misuse is a structural error, not style noise.",
            "Good module docs connect layout and responsibility.",
        ],
    },
    "diagnostics": {
        "problem": "Readers often see diagnostics as messages to memorize rather than structured signals tied to a failing contract.",
        "thread": "One broken program produces one actionable error, then a corrected version restores the intended flow.",
        "for_what": "This chapter helps the reader classify and use diagnostics instead of fearing them.",
        "what": "You will read a valid example, a broken example, and the contract that the diagnostic is pointing back to.",
        "example": """space demo/diagnostics

proc safe_div(total: int, count: int) -> int {
  if count == 0 { give 0 }
  give total
}""",
        "invalid": """proc bad_div(total: int, count: int) -> int {
  if count { give total }
  give 0
}""",
        "global": "A diagnostics chapter should explain the shape of a failure: what contract was expected, which surface broke it, and how to return to a valid program. The message is a pointer back to structure, not an isolated artifact.",
        "pitfalls": [
            "Teaching diagnostics as a table of codes with no code story.",
            "Explaining only the message text, not the broken contract.",
            "Using overly large examples that hide the first real mistake.",
        ],
        "exercise": "Take the invalid example and reduce it until one diagnostic still reproduces the same contract failure.",
        "summary": [
            "Diagnostics point back to a broken contract.",
            "A valid and invalid pair is the right teaching unit.",
            "Smaller repros are more educational than giant failing files.",
            "The first useful question is what contract broke.",
            "Actionable fixes start from structure, not from guesswork.",
        ],
    },
    "pointers": {
        "problem": "Low-level chapters become dangerous when they show syntax without ownership, lifetime, or boundary reasoning.",
        "thread": "One explicit pointer-facing helper isolates a low-level boundary while keeping the rest of the flow simple.",
        "for_what": "This chapter helps the reader understand why pointer-like surfaces should remain explicit and narrow.",
        "what": "You will inspect one narrow low-level helper, then compare it to an unsafe misuse.",
        "example": """space demo/pointers

unsafe proc write_value(dst: ptr[int], value: int) -> int {
  give 0
}""",
        "invalid": """proc bad_ptr() -> int {
  let dst: ptr[int] = 0
  give dst
}""",
        "global": "Pointer chapters are about boundaries. The goal is not to normalize low-level code everywhere, but to show how a narrow low-level surface can remain explicit and reviewable.",
        "pitfalls": [
            "Treating low-level code as ordinary glue with no special boundary.",
            "Explaining syntax before explaining risk.",
            "Failing to isolate the low-level edge from the rest of the program.",
        ],
        "exercise": "Wrap the low-level helper behind a higher-level procedure that exposes a safer contract.",
        "summary": [
            "Low-level surfaces need narrow boundaries.",
            "The chapter should explain risk and ownership first.",
            "Unsafe examples should stay explicit.",
            "The reader should learn where the boundary belongs.",
            "Reviewability matters more than token novelty.",
        ],
    },
    "generics": {
        "problem": "Generic pages often show parameter syntax without explaining why a reusable contract is worth the extra abstraction.",
        "thread": "One reusable container-like helper works for multiple element shapes while keeping the contract readable.",
        "for_what": "This chapter helps the reader use abstraction only when reuse really needs it.",
        "what": "You will inspect one generic helper, then compare it to a malformed or unnecessary abstraction.",
        "example": """space demo/generics

proc identity[T](value: T) -> T {
  give value
}""",
        "invalid": """proc bad_identity[T](value: T) -> int {
  give value
}""",
        "global": "A generics chapter should answer one question: what reuse is being bought by the abstraction? If that answer is weak, the generic surface is probably too abstract for the reader and the code.",
        "pitfalls": [
            "Adding type parameters with no real reuse benefit.",
            "Teaching generic call syntax before generic design intent.",
            "Mixing valid generic syntax with invalid return contracts.",
        ],
        "exercise": "Add one second generic procedure that reuses the same abstraction for a clearly different value shape.",
        "summary": [
            "Generics justify themselves through reusable contracts.",
            "The reader should see the gain, not only the syntax.",
            "A tiny but real reusable helper is the best teaching unit.",
            "Malformed abstractions fail at the contract boundary.",
            "Use abstraction when it clarifies, not when it decorates.",
        ],
    },
    "macros": {
        "problem": "Macro chapters become misleading when they present expansion power without naming the readability cost.",
        "thread": "One small macro-like helper exists to remove duplication while preserving a stable public shape.",
        "for_what": "This chapter helps the reader keep macro use intentional and bounded.",
        "what": "You will inspect one use case where a macro-like surface is justified, then compare it to a noisy misuse.",
        "example": """space demo/macros

macro trace_value(name, value) {
  emit name
  emit value
}""",
        "invalid": """macro broken {
  give 0
}""",
        "global": "Macros should be taught as controlled power tools. The question is not merely what they can generate, but whether they preserve or destroy the clarity of the resulting program surface.",
        "pitfalls": [
            "Using macros to hide ordinary control flow.",
            "Expanding syntax when a procedure would stay clearer.",
            "Skipping the readability and debugging cost of generated surfaces.",
        ],
        "exercise": "Take one repetitive pattern from another chapter and argue whether it deserves a macro or a plain procedure.",
        "summary": [
            "Macros should pay their readability cost.",
            "A bounded use case teaches better than a flashy demo.",
            "Generated surface still needs a clear contract.",
            "Debuggability matters as much as expressiveness.",
            "Macro pages should teach restraint, not only power.",
        ],
    },
    "compiler": {
        "problem": "Compiler-facing chapters become vague when they talk about stages but never show how data crosses stage boundaries.",
        "thread": "One small compiler-like flow reads a source input, validates shape, transforms state, and produces an exit-oriented result.",
        "for_what": "This chapter helps the reader build a mental model of pipeline boundaries.",
        "what": "You will inspect a staged flow, name what each stage owns, then compare it to a collapsed design that hides those boundaries.",
        "example": """space demo/compiler

pick ParseState {
  case Parsed(nodes: int)
  case Failed(code: int)
}

proc parse(size: int) -> ParseState {
  if size <= 0 { give ParseState.Failed(11) }
  give ParseState.Parsed(size)
}

proc lower(state: ParseState) -> int {
  match state {
    case Parsed(nodes) { give nodes }
    case Failed(code) { give code }
    otherwise { give 70 }
  }
}""",
        "invalid": """proc broken_pipeline(size: int) -> int {
  match size
    case 0 { give 0 }
  }
}""",
        "global": "Pipeline chapters are most useful when they keep stage ownership explicit. The reader should know what the parse stage owns, what the transform stage owns, and where errors cross boundaries.",
        "pitfalls": [
            "Describing a pipeline as a list of names with no data movement.",
            "Collapsing stages so tightly that diagnostics lose their context.",
            "Teaching advanced compiler topics without a small staged example.",
        ],
        "exercise": "Add one extra stage to the example and explain what data it receives and what it returns.",
        "summary": [
            "Stages are ownership boundaries.",
            "Data movement matters as much as stage names.",
            "Small compiler examples should still show multiple layers.",
            "Broken stage structure should remain visible in invalid examples.",
            "A mental model is a data-flow model.",
        ],
    },
    "interop": {
        "problem": "Interop pages become dangerous when they normalize foreign boundaries instead of isolating them.",
        "thread": "One narrow bridge surface translates between a Vitte-facing contract and an external-facing edge.",
        "for_what": "This chapter helps the reader keep foreign or runtime boundaries explicit and narrow.",
        "what": "You will inspect one bridge-like helper, then compare it to a misuse where the foreign edge leaks too far.",
        "example": """space demo/interop

unsafe proc call_host(code: int) -> int {
  if code < 0 { give 11 }
  give 0
}""",
        "invalid": """proc leaked_host(code: int) -> int {
  unsafe
  give code
}""",
        "global": "Interop is not a convenience feature. It is a coupling boundary. The chapter should therefore explain why the foreign edge exists, how narrow it is, and how the rest of the program stays readable around it.",
        "pitfalls": [
            "Letting foreign assumptions spread through ordinary code.",
            "Skipping the boundary rationale and teaching raw syntax only.",
            "Using interop examples with no visible safe wrapper around them.",
        ],
        "exercise": "Wrap the interop call in a safer procedure that exposes a domain-oriented result instead of a raw foreign detail.",
        "summary": [
            "Interop is a narrow boundary, not a default tool.",
            "The chapter should expose coupling cost clearly.",
            "Unsafe or foreign edges must stay visible.",
            "Safe wrappers are part of the teaching model.",
            "The goal is controlled integration, not hidden magic.",
        ],
    },
    "stdlib": {
        "problem": "Readers often see stdlib pages as catalogs with no architectural guidance, so they cannot place new code correctly.",
        "thread": "One small application flow uses pure helpers, path normalization, and summary rendering while naming which library family owns each concern.",
        "for_what": "This chapter helps the reader classify library work by family and responsibility.",
        "what": "You will inspect one complete flow, then map its responsibilities to stdlib families and compare it to an invalid domain variant.",
        "example": """space demo/stdlib

form Manifest {
  name: string
  root_path: string
  targets: int
}

pick Plan {
  case Ready(summary: string)
  case Invalid(code: int)
}

proc normalize_root(root_path: string) -> string {
  if root_path == "" { give "." }
  give root_path
}

proc validate_manifest(m: Manifest) -> Plan {
  if m.name == "" { give Plan.Invalid(11) }
  if m.targets <= 0 { give Plan.Invalid(12) }
  give Plan.Ready(m.name)
}""",
        "invalid": """entry main at core/app {
  let manifest: Manifest = Manifest("", "", 0)
  return 0
}""",
        "global": "The stdlib should be taught by ownership. Core helpers, collections, data transforms, path and I/O boundaries, JSON, concurrency, and runtime-facing families all solve different problems. The chapter is useful only if the reader can classify future code with it.",
        "pitfalls": [
            "Listing module names without explaining responsibility.",
            "Reducing stdlib docs to hello-world snippets.",
            "Mixing host interaction and pure domain logic in one explanation.",
        ],
        "exercise": "Take the example and say which future step would belong to `json`, which to `io`, and which should remain in pure domain code.",
        "summary": [
            "Stdlib families exist to separate responsibilities.",
            "A complete scenario teaches more than a flat catalog.",
            "Validation and transport should stay apart.",
            "Family boundaries matter in docs as much as in code.",
            "The chapter should scale to the whole library tree.",
        ],
    },
    "testing": {
        "problem": "Testing chapters often become lists of test names instead of explanations of what evidence each layer should provide.",
        "thread": "One feature is checked by a unit-level invariant, a scenario-level flow, and an invalid-path assertion.",
        "for_what": "This chapter helps the reader design tests as evidence, not ceremony.",
        "what": "You will inspect one tested flow, then classify which part belongs to unit, scenario, regression, or generation-oriented validation.",
        "example": """space demo/tests

proc classify_score(x: int) -> int {
  if x < 0 { give 11 }
  if x > 100 { give 12 }
  give 0
}""",
        "invalid": """proc bad_test_surface(x: int) -> int {
  if x { give 11 }
  give 0
}""",
        "global": "Testing pages should explain what each test proves. A useful chapter distinguishes local invariants, end-to-end scenarios, regression proof, and failure-focused fixtures without collapsing them into one vague category.",
        "pitfalls": [
            "Treating every test as the same kind of evidence.",
            "Adding giant scenarios where a small invariant test would be stronger.",
            "Explaining tooling before explaining the proof obligation.",
        ],
        "exercise": "Take the example and write one invariant it needs, one scenario it needs, and one invalid-path check it needs.",
        "summary": [
            "Tests are evidence of a specific claim.",
            "Different layers prove different things.",
            "A compact flow is enough to teach testing structure.",
            "Invalid paths deserve explicit fixtures.",
            "The chapter should teach proof design, not ritual.",
        ],
    },
    "performance": {
        "problem": "Performance chapters become hand-wavy when they talk about speed without naming what is being measured or allocated.",
        "thread": "One data-processing path is measured before and after a structural improvement.",
        "for_what": "This chapter helps the reader connect performance claims to concrete data movement and memory behavior.",
        "what": "You will inspect one measurable flow, identify the costly boundary, and compare it to a safer or cheaper variant.",
        "example": """space demo/performance

proc accumulate(values: list[int]) -> int {
  let total: int = 0
  for value in values {
    set total = total + value
  }
  give total
}""",
        "invalid": """proc noisy_accumulate(values: list[int]) -> int {
  if values { give 0 }
  give 0
}""",
        "global": "A performance chapter should say what is being moved, copied, or traversed. Without that, 'faster' and 'slower' are just adjectives. The chapter needs one concrete flow that can be reasoned about before it is benchmarked.",
        "pitfalls": [
            "Optimizing before measuring the real boundary.",
            "Talking about performance without naming allocation or traversal cost.",
            "Using invalid examples that are only syntactic, not operational.",
        ],
        "exercise": "Take the example and explain one reason why data shape, not just arithmetic, could dominate the cost.",
        "summary": [
            "Performance claims need a concrete flow.",
            "Allocation and traversal cost should stay visible.",
            "Measurement should follow a clear boundary hypothesis.",
            "Structural choices matter more than slogans.",
            "The chapter should teach reasoning before micro-optimizing.",
        ],
    },
    "debugging": {
        "problem": "Debugging chapters often list tools without showing how a failure becomes reproducible and explainable.",
        "thread": "One failure is reduced, reproduced, inspected, and fixed through a small stable scenario.",
        "for_what": "This chapter helps the reader move from vague bug reports to reproducible technical evidence.",
        "what": "You will inspect one reproducible failing case, the narrowed contract it breaks, and the fixed version.",
        "example": """space demo/debug

proc normalize_port(raw: int) -> int {
  if raw < 0 { give 0 }
  if raw > 65535 { give 65535 }
  give raw
}""",
        "invalid": """proc bad_port(raw: int) -> int {
  if raw { give 0 }
  give raw
}""",
        "global": "Debugging should be taught as reduction and evidence. The useful move is to isolate a contract, reproduce the failure with the smallest still-meaningful case, and keep the path stable while you inspect it.",
        "pitfalls": [
            "Keeping bug reports too large to reason about.",
            "Changing too many things before reproducing the failure.",
            "Using tooling pages without a concrete failing story.",
        ],
        "exercise": "Reduce the invalid example to the smallest still-failing shape and write the contract it breaks.",
        "summary": [
            "Debugging begins with reduction.",
            "A stable failing scenario is better than a huge noisy one.",
            "The contract matters more than the symptom wording.",
            "Reproducibility is a technical asset.",
            "Fixes should be explained against the reduced case.",
        ],
    },
    "projects": {
        "problem": "Project chapters often list folders or commands but never show how a whole project shape expresses architecture.",
        "thread": "One project family shows entry point, domain module, integration boundary, and output path in one small but complete example.",
        "for_what": "This chapter helps the reader see project structure as architecture, not as directory decoration.",
        "what": "You will inspect one project-shaped example, identify the boundaries, then compare it to a weak structure that blurs them.",
        "example": """space demo/project

form Config {
  name: string
}

proc run(cfg: Config) -> int {
  if cfg.name == "" { give 11 }
  give 0
}

entry main at core/app {
  let cfg: Config = Config("demo")
  return run(cfg)
}""",
        "invalid": """entry main {
  let cfg = Config("")
  return 0
}""",
        "global": "Project chapters are useful when they show architecture in miniature: entry, domain, service, boundary, output. Whether the project is CLI, HTTP, system, KV, Arduino, or editor-oriented, those ownership lines should remain readable.",
        "pitfalls": [
            "Documenting a directory tree with no architectural explanation.",
            "Teaching commands before teaching boundaries.",
            "Treating all project types as cosmetic variations of the same thing.",
        ],
        "exercise": "Take the example and say which part would change first if the same domain became a CLI project versus an HTTP project.",
        "summary": [
            "Project shape expresses architecture.",
            "Entry and domain should remain easy to distinguish.",
            "Different delivery targets still need the same ownership clarity.",
            "Commands are secondary to structure.",
            "A tiny project can still teach real architecture.",
        ],
    },
    "grammar": {
        "problem": "Grammar pages become unusable when they explain snippets without naming the parser layer or ambiguity they belong to.",
        "thread": "One complete program is read as top-level declarations, block statements, expressions, and branch forms.",
        "for_what": "This chapter helps the reader classify parser problems by grammar layer.",
        "what": "You will inspect one valid program, one invalid program, and the grammar layer each one exercises.",
        "example": """space demo/grammar

pick Resp {
  case Ok(value: int)
  case Err(code: int)
}

proc run(x: int) -> int {
  match x {
    case 0 { give 0 }
    otherwise { give x }
  }
}

entry main at core/app {
  return run(1)
}""",
        "invalid": """proc broken(x: int) -> int {
  match x
    case 0 { give 0 }
  }
}""",
        "global": "Grammar should be read by layer: top-level declarations, block statements, expressions, patterns, and ambiguity points. The reader should leave able to say what sort of parser expectation has failed.",
        "pitfalls": [
            "Commenting each token instead of naming the grammar layer.",
            "Confusing parse failure with type or business failure.",
            "Skipping ambiguity notes where the same surface can be read two ways.",
        ],
        "exercise": "Take the valid example and identify one top-level rule, one statement rule, and one expression rule it exercises.",
        "summary": [
            "Grammar is a layered model.",
            "A full valid program is the right teaching unit.",
            "Invalid examples should isolate parse structure failures.",
            "Ambiguity must be explained, not hidden.",
            "The chapter should naturally connect to diagnostics and tests.",
        ],
    },
    "design": {
        "problem": "Design and convention chapters often become prose-only advice with no proof that the rule changes code quality.",
        "thread": "One module is revised from a weak naming or boundary choice into a stronger, more maintainable form.",
        "for_what": "This chapter helps the reader connect design advice to concrete changes in code review, maintenance, and migration.",
        "what": "You will inspect one before/after style or architecture decision, then identify the contract it improves.",
        "example": """space demo/design

form BuildConfig {
  project_name: string
  retry_limit: int
}

proc validate_config(cfg: BuildConfig) -> int {
  if cfg.project_name == "" { give 11 }
  if cfg.retry_limit < 0 { give 12 }
  give 0
}""",
        "invalid": """form Cfg {
  x: string
  y: int
}""",
        "global": "Design pages need code because design rules only matter when they improve a concrete contract. Naming, conventions, compatibility boundaries, and review habits are technical because they change how easily the code can be read and evolved.",
        "pitfalls": [
            "Giving advice with no concrete code delta.",
            "Using generic slogans instead of stable review criteria.",
            "Forgetting to connect naming or convention to future maintenance cost.",
        ],
        "exercise": "Rename one vague field or procedure in the example and explain what ambiguity the new name removes.",
        "summary": [
            "Design advice must change code, not just prose.",
            "Naming and conventions are technical levers.",
            "Concrete before/after examples teach better than slogans.",
            "Maintainability is visible in small code decisions.",
            "The chapter should leave a reusable review rule behind.",
        ],
    },
    "delivery": {
        "problem": "Delivery chapters often list CI or review steps without connecting them to the evidence they are supposed to protect.",
        "thread": "One feature moves through local validation, CI checks, and review expectations with one visible contract.",
        "for_what": "This chapter helps the reader understand delivery as proof preservation.",
        "what": "You will inspect one feature flow, identify what local checks prove, what CI proves, and what review still needs a human decision.",
        "example": """space demo/delivery

proc validate_release(name: string) -> int {
  if name == "" { give 11 }
  give 0
}""",
        "invalid": """proc broken_release(name: string) -> int {
  if name { give 0 }
  give 11
}""",
        "global": "Delivery pages should explain evidence flow. Local checks, CI pipelines, and review rules are useful only when the reader knows what claim each one is guarding and what kind of regression it can still miss.",
        "pitfalls": [
            "Turning CI pages into command inventories only.",
            "Assuming automated checks remove the need for architectural review.",
            "Treating documentation and code evidence as separate concerns.",
        ],
        "exercise": "Take the example and write one local check, one CI check, and one review question that would protect it.",
        "summary": [
            "Delivery is proof preservation.",
            "Each gate should protect a named claim.",
            "Local, CI, and review evidence are not interchangeable.",
            "The chapter should connect process to code.",
            "Good delivery docs reduce ambiguity about readiness.",
        ],
    },
    "generic": {
        "problem": "Generic chapters without a stable scenario produce repetition and weak retention.",
        "thread": "One small service module is used to explain the topic through input, validation, transformation, and output.",
        "for_what": "This chapter helps the reader connect the topic to one concrete engineering use.",
        "what": "You will inspect one coherent example, then one invalid variant, and extract the rule that the chapter is really teaching.",
        "example": """space demo/generic

proc run(value: int) -> int {
  if value < 0 { give 11 }
  give value
}""",
        "invalid": """proc broken(value: int) -> int {
  if value { give 11 }
  give value
}""",
        "global": "The chapter should always return to the same core question: what contract does this construct give to the full block? That is the reason every page still needs one complete, testable example.",
        "pitfalls": [
            "Explaining abstractions with no anchored scenario.",
            "Repeating template prose without changing the engineering point.",
            "Using invalid examples that do not isolate the chapter's real topic.",
        ],
        "exercise": "Take the coherent example and rewrite one line so that the chapter's central contract becomes even more explicit.",
        "summary": [
            "Every chapter needs one concrete engineering story.",
            "The contract matters more than the token list.",
            "An invalid example should isolate the chapter's real lesson.",
            "Global explanation beats line-by-line commentary.",
            "The page should leave one reusable rule behind.",
        ],
    },
}


def render_article(meta: dict, profile_name: str) -> str:
    profile = PROFILES[profile_name]
    title = meta["title"]
    level = meta.get("level", "intermediate")
    minutes = meta.get("minutes", 10)
    prereq = meta.get("prereq", "")
    last_review = meta.get("last_review", "2026-06-05")
    essential = "essential" if meta.get("essential", False) else "supplemental"
    maturity = meta.get("maturity", "draft")
    rel = "../../"
    prereq_display = doc_ref_link(prereq, rel) if prereq else "<code>none</code>"
    see_also = []
    if prereq:
        see_also.append(prereq)
    path = meta["path"]
    stem = Path(path).name
    number = stem.split("-")[0]
    see_also.extend([
        "book/chapters/27-grammar.html",
        "book/chapters/31-build-errors.html",
    ])
    if "stdlib" in path:
        see_also.append("book/stdlib.html")

    article = [
        '<p class="book-progress"></p>',
        f"<h1>{html.escape(title)}</h1>",
        (
            f'<p class="chapter-meta" data-chapter-meta="1">Level: <strong>{html.escape(level)}</strong> '
            f'· Reading time: <strong>{minutes} min</strong> · Prerequisite: {prereq_display} '
            f'· Track: <strong>{html.escape(essential)}</strong> · Maturity: <strong>{html.escape(maturity)}</strong> '
            f'· Last review: <strong>{html.escape(last_review)}</strong></p>'
        ),
        "<section class=\"chapter-tldr\"><h2>TL;DR (5 lines)</h2><ul>",
    ]
    for item in profile["summary"]:
        article.append(f"<li>{html.escape(item)}</li>")
    article.extend([
        "</ul></section>",
        "<section class=\"chapter-errors\"><h2>Frequent mistakes</h2><ul>",
    ])
    see_also_inline = ", ".join(doc_ref_link(ref, rel) for ref in see_also[:3])
    for item in profile["pitfalls"]:
        article.append(f"<li>{html.escape(item)}</li>")
    article.extend([
        "</ul></section>",
        f"<p>Prerequisites: {prereq_display}. See also: {see_also_inline}.</p>",
        "<h2>Concrete Problem</h2>",
        f"<p>{html.escape(profile['problem'])}</p>",
        "<h2>Red Thread (Single Project)</h2>",
        f"<p>{html.escape(profile['thread'])}</p>",
        "<h2>For what</h2>",
        f"<p>{html.escape(profile['for_what'])}</p>",
        "<h2>What you are going to do</h2>",
        f"<p>{html.escape(profile['what'])}</p>",
        "<h2>Coherent example</h2>",
        code_block("vit", profile["example"]),
        "<h2>Global explanation</h2>",
        f"<p>{html.escape(profile['global'])}</p>",
        "<h2>Invalid case</h2>",
        code_block("vit", profile["invalid"]),
        "<p>This invalid case is intentionally small. It exists to isolate the contract failure that the chapter is trying to teach.</p>",
        "<h2>Common pitfalls</h2><ul>",
    ])
    for item in profile["pitfalls"]:
        article.append(f"<li>{html.escape(item)}</li>")
    article.extend([
        "</ul>",
        "<h2>Short exercise</h2>",
        f"<p>{html.escape(profile['exercise'])}</p>",
        "<h2>Summary in 5 points</h2><ol>",
    ])
    for item in profile["summary"]:
        article.append(f"<li>{html.escape(item)}</li>")
    article.extend([
        "</ol>",
        "<section class=\"chapter-quiz\"><h2>Mini quiz</h2><ol>",
        "<li>What contract is the coherent example trying to make explicit?</li>",
        "<li>Why does the invalid example fail?</li>",
        "<li>What boundary should remain visible if you extend the example?</li>",
        "</ol><button class=\"quiz-save\" type=\"button\">Mark as reviewed</button></section>",
        "<section class=\"chapter-see-also\"><h2>See also</h2><ul>",
    ])
    for ref in see_also[:4]:
        article.append(f"<li>{doc_ref_link(ref, rel, code=True)}</li>")
    article.extend([
        "</ul></section>",
        "<section class=\"chapter-cta\"><h2>Next best action</h2><p>Extend the coherent example by one small, justified step and keep the same contract visible from input to output.</p></section>",
    ])
    return "\n".join(article)


def render_page(meta: dict) -> str:
    profile_name = slug_profile(meta["path"], meta["title"])
    article = render_article(meta, profile_name)
    title = meta["title"]
    rel = "../../"
    header = f"""<header class="site-header"><a class="site-brand" href="{rel}index.html"><img class="site-brand-mark" src="{rel}svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a><nav class="site-nav" aria-label="Primary"><ul class="nav-band"><li><a class="nav-chip" href="{rel}index.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-home"></use></svg><span>Welcome</span></a></li><li><a class="nav-chip" href="{rel}doc.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Documentation</span></a></li><li><a class="nav-chip" href="{rel}download.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Download</span></a></li><li><a class="nav-chip" href="{rel}source.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Source</span></a></li><li><a class="nav-chip" href="{rel}community.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Community</span></a></li><li><a class="nav-chip" href="{rel}news.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-news"></use></svg><span>News</span></a></li><li><a class="nav-chip" href="{rel}diagnostics.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Diagnostics</span></a></li><li><a class="nav-chip" href="{rel}suggestions.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Suggestions</span></a></li></ul></nav><form class="doc-search" role="search" action="{rel}search.html" method="get"><input class="doc-search-input" type="search" name="q" placeholder="Search docs, book, grammar" aria-label="Search documentation" autocomplete="off"><div class="doc-search-controls" aria-label="Search filters"><select class="doc-search-filter doc-search-section" name="section" aria-label="Filter by section"><option value="all">All sections</option><option value="docs">Docs</option><option value="book">Book</option><option value="grammar">Grammar</option></select><select class="doc-search-filter doc-search-lang" name="lang" aria-label="Filter by language"><option value="all">All languages</option><option value="en">EN</option><option value="fr">FR</option></select></div><div class="doc-search-results" hidden></div><div class="doc-search-footer" hidden></div></form></header>"""
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)}</title>
<link rel="icon" href="{rel}svg/logo.svg" type="image/svg+xml">
<link rel="stylesheet" href="{rel}css/site.css">
<link rel="stylesheet" href="{rel}css/print.css" media="print">
<link rel="alternate" hreflang="en" href="https://vitte-lang.org/{html.escape(Path(meta['path']).name)}">
<link rel="alternate" hreflang="x-default" href="https://vitte-lang.org/{html.escape(Path(meta['path']).name)}">
<meta http-equiv="Content-Security-Policy" content="default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none';">
</head>
<body class="classic-doc">
<a class="skip-link" href="#main-content">Skip to content</a>
<div class="site-shell">
{header}
<main id="main-content" class="site-main">
<article class="doc-content">
{article}
</article>
</main>
<footer class="site-footer">
<p class="site-footer-path">{html.escape(meta['path'])}</p>
<p><a href="{rel}index.html">Back to home</a></p>
</footer>
</div>
<script type="module" src="{rel}js/book-learning.js"></script>
<script type="module" src="{rel}js/main.js"></script>
</body>
</html>
"""


def render_pocket_page(meta: dict) -> str:
    profile_name = slug_profile(meta["path"], meta["title"])
    profile = PROFILES[profile_name]
    title = meta["title"]
    rel = "../../../"
    header = f"""<header class="site-header"><a class="site-brand" href="{rel}index.html"><img class="site-brand-mark" src="{rel}svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a><nav class="site-nav" aria-label="Primary"><ul class="nav-band"><li><a class="nav-chip" href="{rel}index.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-home"></use></svg><span>Welcome</span></a></li><li><a class="nav-chip" href="{rel}doc.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Documentation</span></a></li><li><a class="nav-chip" href="{rel}download.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Download</span></a></li><li><a class="nav-chip" href="{rel}source.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Source</span></a></li><li><a class="nav-chip" href="{rel}community.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Community</span></a></li><li><a class="nav-chip" href="{rel}news.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-news"></use></svg><span>News</span></a></li><li><a class="nav-chip" href="{rel}diagnostics.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Diagnostics</span></a></li><li><a class="nav-chip" href="{rel}suggestions.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Suggestions</span></a></li></ul></nav><form class="doc-search" role="search" action="{rel}search.html" method="get"><input class="doc-search-input" type="search" name="q" placeholder="Search docs, book, grammar" aria-label="Search documentation" autocomplete="off"><div class="doc-search-controls" aria-label="Search filters"><select class="doc-search-filter doc-search-section" name="section" aria-label="Filter by section"><option value="all">All sections</option><option value="docs">Docs</option><option value="book">Book</option><option value="grammar">Grammar</option></select><select class="doc-search-filter doc-search-lang" name="lang" aria-label="Filter by language"><option value="all">All languages</option><option value="en">EN</option><option value="fr">FR</option></select></div><div class="doc-search-results" hidden></div><div class="doc-search-footer" hidden></div></form></header>"""
    article = [
        '<p class="book-progress"></p>',
        f"<h1>{html.escape(title)} (pocket version)</h1>",
        f"<section class=\"chapter-tldr\"><h2>TL;DR (5 lines)</h2><ul>"
    ]
    for item in profile["summary"]:
        article.append(f"<li>{html.escape(item)}</li>")
    article.extend([
        "</ul></section>",
        "<h2>Concrete Problem</h2>",
        f"<p>{html.escape(profile['problem'])}</p>",
        "<h2>Coherent example</h2>",
        code_block("vit", profile["example"]),
        "<h2>Global explanation</h2>",
        f"<p>{html.escape(profile['global'])}</p>",
        "<h2>Short exercise</h2>",
        f"<p>{html.escape(profile['exercise'])}</p>",
        "<section class=\"chapter-cta\"><h2>Next best action</h2><p>Keep the example small, reproduce it locally, then continue to the full chapter if you need the broader context.</p></section>",
    ])
    body = "\n".join(article)
    pocket_path = Path(meta["path"]).name
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)} (pocket version)</title>
<link rel="icon" href="{rel}svg/logo.svg" type="image/svg+xml">
<link rel="stylesheet" href="{rel}css/site.css">
<link rel="stylesheet" href="{rel}css/print.css" media="print">
<link rel="alternate" hreflang="en" href="https://vitte-lang.org/{html.escape(pocket_path)}">
<link rel="alternate" hreflang="x-default" href="https://vitte-lang.org/{html.escape(pocket_path)}">
<meta http-equiv="Content-Security-Policy" content="default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none';">
</head>
<body class="classic-doc">
<a class="skip-link" href="#main-content">Skip to content</a>
<div class="site-shell">
{header}
<main id="main-content" class="site-main">
<article class="doc-content">
{body}
</article>
</main>
<footer class="site-footer">
<p class="site-footer-path">book/poche/chapters/{html.escape(pocket_path)}</p>
<p><a href="{rel}index.html">Back to home</a></p>
</footer>
</div>
<script type="module" src="{rel}js/book-learning.js"></script>
<script type="module" src="{rel}js/main.js"></script>
</body>
</html>
"""


def main() -> int:
    meta = json.loads(META_PATH.read_text(encoding="utf-8"))
    by_path = {entry["path"]: entry for entry in meta["chapters"]}

    updated = 0
    for page in sorted(CHAPTERS_DIR.glob("*.html")):
        rel = f"book/chapters/{page.name}"
        if rel not in by_path:
            continue
        page.write_text(render_page(by_path[rel]), encoding="utf-8")
        updated += 1

    pocket_updated = 0
    for page in sorted(POCKET_DIR.glob("*.html")):
        rel = f"book/chapters/{page.name}"
        if rel not in by_path:
            continue
        page.write_text(render_pocket_page(by_path[rel]), encoding="utf-8")
        pocket_updated += 1

    print(f"generated_chapters={updated}")
    print(f"generated_pocket_chapters={pocket_updated}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
