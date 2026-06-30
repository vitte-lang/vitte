#!/usr/bin/env python3
from __future__ import annotations

import json
import html
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CHAPTERS_DIR = ROOT / "docs" / "book" / "chapters"
POCKET_DIR = ROOT / "docs" / "book" / "poche" / "chapters"
META_PATH = ROOT / "docs" / "book" / "chapters-meta.json"


def code_block(language: str, text: str) -> str:
    return f'<pre><code class="language-{language}">{html.escape(text)}</code></pre>'


def vitte_code_block(text: str) -> str:
    return code_block("vit", align_vitte_docs_syntax(text))


def align_vitte_docs_syntax(text: str) -> str:
    source = text.strip()
    source = re.sub(
        r"entry main at [^{]+ \{\n(?P<body>.*?)\n\}",
        lambda m: (
            "proc main(args: list[string]) -> int {\n"
            + re.sub(r"\breturn\b", "give", m.group("body"))
            + "\n}"
        ),
        source,
        flags=re.S,
    )
    source = re.sub(r"\breturn\b", "give", source)
    source = source.replace("otherwise", "else")
    source = re.sub(r"case ([^\n,{}]+)\n", r"case \1,\n", source)
    source = source.replace("BuildInput(\"demo\", 1)", 'BuildInput { name: "demo", attempts: 1 }')
    source = source.replace("RunConfig(\"demo\", true)", 'RunConfig { target: "demo", verbose: true }')
    source = source.replace("ReportInput(\"daily\", 3, true)", 'ReportInput { title: "daily", count: 3, enabled: true }')
    source = source.replace("Report(\"daily\", 3, true)", 'Report { name: "daily", retries: 3, ok: true }')
    source = source.replace("ReportConfig(\"daily\", 4)", 'ReportConfig { name: "daily", rows: 4 }')
    source = source.replace("Totals(count, sum)", "Totals { count: count, sum: sum }")
    source = source.replace("IrUnit(0, 1)", "IrUnit { nodes: 0, warnings: 1 }")
    source = source.replace("IrUnit(nodes, 0)", "IrUnit { nodes: nodes, warnings: 0 }")
    source = source.replace("Manifest(\"\", \"\", 0)", 'Manifest { name: "", root_path: "", targets: 0 }')
    source = source.replace("Manifest(\"demo\", \"\")", 'Manifest { name: "demo", root: "" }')
    source = source.replace("Command(\"build\", false)", 'Command { name: "build", dry_run: false }')
    source = source.replace("BuildConfig(\"demo\", 2)", 'BuildConfig { project_name: "demo", max_retries: 2 }')
    source = source.replace("Request(\"demo\", 1)", 'Request { name: "demo", value: 1 }')
    source = source.replace("Pair[int](1, 2)", "Pair[int] { left: 1, right: 2 }")
    if source.startswith("space ") and "export *" not in source and "invalid" not in source.lower():
        source = f"{source}\n\nexport *"
    return source


def doc_ref_link(ref: str, rel: str, *, code: bool = True) -> str:
    label = html.escape(ref)
    href = f"{rel}{html.escape(ref)}"
    if code:
        return f'<a href="{href}"><code>{label}</code></a>'
    return f'<a href="{href}">{label}</a>'


def anchor_slug(text: str) -> str:
    raw = html.unescape(re.sub(r"<[^>]+>", "", text)).lower()
    raw = re.sub(r"[^a-z0-9]+", "-", raw).strip("-")
    return raw or "section"


def add_heading_ids_and_toc(article: str) -> str:
    entries: list[tuple[int, str, str]] = []
    used: dict[str, int] = {}

    def replace_heading(match: re.Match[str]) -> str:
        level = int(match.group(1))
        attrs = match.group(2) or ""
        content = match.group(3)
        if " id=" in attrs:
            id_match = re.search(r'id="([^"]+)"', attrs)
            anchor = id_match.group(1) if id_match else anchor_slug(content)
        else:
            base = anchor_slug(content)
            count = used.get(base, 0)
            used[base] = count + 1
            anchor = base if count == 0 else f"{base}-{count + 1}"
            attrs = f'{attrs} id="{anchor}"'
        entries.append((level, anchor, re.sub(r"<[^>]+>", "", html.unescape(content)).strip()))
        return f"<h{level}{attrs}>{content}</h{level}>"

    article = re.sub(r"<h([2-4])([^>]*)>(.*?)</h\1>", replace_heading, article, flags=re.S)
    toc_items = [
        f'<li class="toc-level-{level}"><a href="#{html.escape(anchor)}">{html.escape(label)}</a></li>'
        for level, anchor, label in entries
        if label
    ]
    toc = (
        '<nav class="chapter-toc" aria-label="Chapter table of contents">'
        '<h2>Contents</h2>'
        '<ol>'
        + "\n".join(toc_items)
        + '</ol>'
        '</nav>'
    )
    if '<p class="chapter-meta"' in article:
        article = re.sub(
            r'(<p class="chapter-meta"[^>]*>.*?</p>)',
            r'\1\n' + toc,
            article,
            count=1,
            flags=re.S,
        )
    else:
        article = re.sub(r"(</h1>)", r"\1\n" + toc, article, count=1)
    return article


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


PROFILE_CONTEXT = {
    "philosophy": {
        "role": "sets the design lens before syntax becomes the main topic",
        "boundary": "intent, contract, and observable failure",
        "reader": "a reader deciding what kind of code Vitte is trying to reward",
    },
    "starting": {
        "role": "turns the first file into a complete executable shape",
        "boundary": "entry point, helper procedure, and returned status",
        "reader": "a beginner who needs one stable program before learning variants",
    },
    "syntax": {
        "role": "shows how valid code is shaped before semantics get deeper",
        "boundary": "declaration, block, branch, and explicit result",
        "reader": "a reader learning to recognize well-formed Vitte at a glance",
    },
    "types": {
        "role": "connects type names to domain meaning",
        "boundary": "field meaning, value shape, and compiler-enforced mismatch",
        "reader": "a reader deciding which type makes a contract clearer",
    },
    "procedures": {
        "role": "keeps behavior visible through signatures and return values",
        "boundary": "input, branch, and output",
        "reader": "a reader extracting behavior without hiding state",
    },
    "control": {
        "role": "makes program paths readable and testable",
        "boundary": "nominal path, guard path, and fallback path",
        "reader": "a reader tracing why execution moved from one branch to another",
    },
    "collections": {
        "role": "teaches data shape as an engineering choice",
        "boundary": "single value, grouped values, and aggregate result",
        "reader": "a reader choosing a container because the domain requires one",
    },
    "modules": {
        "role": "turns file organization into ownership",
        "boundary": "private helper, public surface, and imported dependency",
        "reader": "a reader deciding what belongs together",
    },
    "diagnostics": {
        "role": "links compiler messages back to broken contracts",
        "boundary": "expected shape, actual shape, and correction path",
        "reader": "a reader learning to reduce an error instead of guessing",
    },
    "pointers": {
        "role": "keeps low-level power narrow and reviewable",
        "boundary": "safe caller, explicit unsafe edge, and checked result",
        "reader": "a reader deciding when a low-level surface is justified",
    },
    "generics": {
        "role": "asks what reuse is worth paying for",
        "boundary": "specific operation, reusable shape, and concrete call site",
        "reader": "a reader avoiding abstraction without a visible payoff",
    },
    "macros": {
        "role": "treats generation as a contract, not a shortcut",
        "boundary": "input pattern, generated surface, and reviewable output",
        "reader": "a reader deciding whether generated code preserves clarity",
    },
    "compiler": {
        "role": "keeps compiler stages owned and inspectable",
        "boundary": "source input, intermediate representation, and emitted result",
        "reader": "a reader mapping language constructs to implementation stages",
    },
    "interop": {
        "role": "makes foreign boundaries explicit",
        "boundary": "external edge, safe wrapper, and local contract",
        "reader": "a reader integrating another runtime without hiding risk",
    },
    "stdlib": {
        "role": "classifies library families by responsibility",
        "boundary": "domain logic, library helper, and runtime-facing effect",
        "reader": "a reader deciding whether code belongs in stdlib or application space",
    },
    "testing": {
        "role": "shows what evidence each test is supposed to provide",
        "boundary": "invariant, scenario, and regression fixture",
        "reader": "a reader writing tests that prove a claim",
    },
    "performance": {
        "role": "makes cost visible before optimization starts",
        "boundary": "data movement, repeated work, and measured result",
        "reader": "a reader separating measured cost from intuition",
    },
    "debugging": {
        "role": "turns failure into a reproducible investigation",
        "boundary": "symptom, reduced case, and verified correction",
        "reader": "a reader preserving evidence while narrowing scope",
    },
    "projects": {
        "role": "shows architecture through one complete project shape",
        "boundary": "entry, domain, integration, and output",
        "reader": "a reader scaling a small example into a production skeleton",
    },
    "grammar": {
        "role": "connects grammar rules to parser expectations",
        "boundary": "token, declaration, expression, and ambiguous edge",
        "reader": "a reader explaining why one parse path is valid",
    },
    "design": {
        "role": "turns conventions into concrete maintainability rules",
        "boundary": "name, compatibility promise, and review expectation",
        "reader": "a reader making code easier to evolve",
    },
    "delivery": {
        "role": "connects local checks to release confidence",
        "boundary": "local evidence, CI evidence, and review evidence",
        "reader": "a reader deciding whether a change is ready to ship",
    },
    "generic": {
        "role": "anchors the chapter in one concrete contract",
        "boundary": "input, transformation, and result",
        "reader": "a reader extracting one reusable engineering rule",
    },
}


def explain_vit_line(line: str, profile_name: str) -> str:
    stripped = line.strip()
    if not stripped:
        return ""
    if stripped.startswith("space "):
        return "names the ownership boundary before any behavior appears"
    if stripped.startswith("use "):
        return "declares an external dependency instead of hiding it inside the flow"
    if stripped.startswith("form "):
        return "introduces a data contract that later branches can rely on"
    if stripped.startswith("pick "):
        return "makes possible outcomes explicit instead of encoding them as magic values"
    if stripped.startswith("case "):
        return "names one outcome that callers must be ready to handle"
    if stripped.startswith("trait "):
        return "separates required behavior from a concrete implementation"
    if stripped.startswith("impl "):
        return "attaches behavior to a type without changing the public idea"
    if stripped.startswith("proc "):
        return "states the callable contract: inputs first, result shape last"
    if stripped.startswith("entry "):
        return "marks the executable boundary where the example becomes observable"
    if stripped.startswith("let "):
        return "keeps an intermediate decision visible for review"
    if stripped.startswith("if "):
        return "guards a failure or edge case before the nominal result"
    if stripped.startswith(("give ", "return ")):
        return "ends the local path with an explicit result"
    if profile_name == "grammar":
        return "keeps the syntactic shape visible for parser reasoning"
    if profile_name == "performance":
        return "is a point where data shape or repeated work can affect cost"
    if profile_name == "testing":
        return "is a behavior that should map to a test claim"
    return "supports the chapter contract without adding hidden behavior"


def code_lines(text: str) -> list[str]:
    return [line for line in text.splitlines() if line.strip()]


def complete_example_variants(profile_name: str) -> list[tuple[str, str, str]]:
    variants = {
        "philosophy": [
            (
                "Explicit decision boundary",
                "This full block keeps request data, validation, and result shape separate.",
                """space examples/philosophy/decision

form BuildInput {
  name: string
  attempts: int
}

pick BuildStatus {
  case Ready(name: string)
  case Refused(code: int)
}

proc prepare(input: BuildInput) -> BuildStatus {
  if input.name == "" { give BuildStatus.Refused(11) }
  if input.attempts < 0 { give BuildStatus.Refused(12) }
  give BuildStatus.Ready(input.name)
}

entry main at core/app {
  let input: BuildInput = BuildInput("demo", 1)
  let status: BuildStatus = prepare(input)
  return 0
}""",
            ),
            (
                "Stable failure surface",
                "This example gives every failure a named result instead of a hidden convention.",
                """space examples/philosophy/failure

pick LoadResult {
  case Loaded(size: int)
  case Empty
  case Invalid(code: int)
}

proc load_size(size: int) -> LoadResult {
  if size < 0 { give LoadResult.Invalid(40) }
  if size == 0 { give LoadResult.Empty }
  give LoadResult.Loaded(size)
}

entry main at core/app {
  let result: LoadResult = load_size(8)
  return 0
}""",
            ),
        ],
        "starting": [
            (
                "Small command program",
                "This program has one helper, one guard, and one observable entry result.",
                """space examples/starting/command

proc command_exit(name: string) -> int {
  if name == "" { give 64 }
  give 0
}

entry main at core/app {
  return command_exit("build")
}""",
            ),
            (
                "First structured input",
                "This version adds a form while keeping the executable path direct.",
                """space examples/starting/input

form RunConfig {
  target: string
  verbose: bool
}

proc validate(cfg: RunConfig) -> int {
  if cfg.target == "" { give 11 }
  give 0
}

entry main at core/app {
  let cfg: RunConfig = RunConfig("demo", true)
  return validate(cfg)
}""",
            ),
        ],
        "syntax": [
            (
                "Declaration and branch shape",
                "This block keeps declarations, locals, branches, and final result easy to scan.",
                """space examples/syntax/shape

proc clamp(value: int, max: int) -> int {
  let floor: int = 0
  if value < floor { give floor }
  if value > max { give max }
  give value
}

entry main at core/app {
  return clamp(12, 10)
}""",
            ),
            (
                "Nested readable block",
                "This example keeps a larger block readable without hiding the exit path.",
                """space examples/syntax/nested

proc route(code: int) -> int {
  if code < 0 {
    give 10
  }
  if code == 0 {
    give 0
  }
  give 1
}

entry main at core/app {
  return route(1)
}""",
            ),
        ],
        "types": [
            (
                "Typed domain form",
                "This example shows text, number, and boolean contracts in one complete flow.",
                """space examples/types/report

form ReportInput {
  title: string
  count: int
  enabled: bool
}

proc readiness(input: ReportInput) -> int {
  if input.title == "" { give 11 }
  if input.count < 1 { give 12 }
  if not input.enabled { give 13 }
  give 0
}

entry main at core/app {
  let input: ReportInput = ReportInput("daily", 3, true)
  return readiness(input)
}""",
            ),
            (
                "Result type contract",
                "This version makes success and rejection visible through a result shape.",
                """space examples/types/result

pick ParseResult {
  case Parsed(value: int)
  case Rejected(code: int)
}

proc parse_count(raw: int) -> ParseResult {
  if raw < 0 { give ParseResult.Rejected(20) }
  give ParseResult.Parsed(raw)
}

entry main at core/app {
  let result: ParseResult = parse_count(7)
  return 0
}""",
            ),
        ],
        "procedures": [
            (
                "Procedure as contract",
                "This example keeps input checks and returned meaning inside a named procedure.",
                """space examples/procedures/contract

proc normalize_port(port: int) -> int {
  if port < 1 { give 8080 }
  if port > 65535 { give 8080 }
  give port
}

entry main at core/app {
  return normalize_port(3000)
}""",
            ),
            (
                "Procedure composition",
                "This program composes two small procedures without hiding the returned status.",
                """space examples/procedures/composition

proc is_valid(size: int) -> int {
  if size <= 0 { give 11 }
  give 0
}

proc run(size: int) -> int {
  let status: int = is_valid(size)
  if status != 0 { give status }
  give size
}

entry main at core/app {
  return run(4)
}""",
            ),
        ],
        "control": [
            (
                "Guarded control path",
                "This example makes each branch protect a visible outcome.",
                """space examples/control/guards

proc classify(value: int) -> int {
  if value < 0 { give 10 }
  if value == 0 { give 0 }
  give 1
}

entry main at core/app {
  return classify(5)
}""",
            ),
            (
                "Loop with explicit accumulator",
                "This flow keeps repeated work and final state visible.",
                """space examples/control/loop

proc count_enabled(values: list[int]) -> int {
  let count: int = 0
  for value in values {
    if value <= 0 { continue }
    set count = count + 1
  }
  give count
}

entry main at core/app {
  return count_enabled([1, 0, 2])
}""",
            ),
        ],
        "collections": [
            (
                "Aggregate values into a form",
                "This program shows why grouped output deserves its own shape.",
                """space examples/collections/aggregate

form Totals {
  count: int
  sum: int
}

proc totals(values: list[int]) -> Totals {
  let count: int = 0
  let sum: int = 0
  for value in values {
    set count = count + 1
    set sum = sum + value
  }
  give Totals(count, sum)
}

entry main at core/app {
  let result: Totals = totals([1, 2, 3])
  return result.count
}""",
            ),
            (
                "Filter before counting",
                "This variant uses the collection because the access pattern requires iteration.",
                """space examples/collections/filter

proc accepted(values: list[int]) -> int {
  let count: int = 0
  for value in values {
    if value < 10 { continue }
    set count = count + 1
  }
  give count
}

entry main at core/app {
  return accepted([3, 10, 12])
}""",
            ),
        ],
        "modules": [
            (
                "Facade module boundary",
                "This complete block models a small facade that keeps the public surface narrow.",
                """space examples/modules/facade

use examples/modules/report.{build_summary} as report

proc run_report() -> int {
  let status: int = report.build_summary()
  if status != 0 { give status }
  give 0
}

entry main at core/app {
  return run_report()
}""",
            ),
            (
                "Local module responsibility",
                "This module keeps validation local and exposes one stable procedure.",
                """space examples/modules/report

form ReportConfig {
  name: string
  rows: int
}

proc build_summary() -> int {
  let cfg: ReportConfig = ReportConfig("daily", 4)
  if cfg.name == "" { give 11 }
  if cfg.rows <= 0 { give 12 }
  give 0
}""",
            ),
        ],
        "diagnostics": [
            (
                "Reduced failing boundary",
                "This example keeps the operation small so a diagnostic maps to one contract.",
                """space examples/diagnostics/reduced

proc require_positive(value: int) -> int {
  if value <= 0 { give 11 }
  give value
}

entry main at core/app {
  return require_positive(3)
}""",
            ),
            (
                "Corrected diagnostic path",
                "This variant names the correction instead of leaving the failure implicit.",
                """space examples/diagnostics/correction

proc safe_index(index: int, size: int) -> int {
  if size <= 0 { give 40 }
  if index < 0 { give 41 }
  if index >= size { give 42 }
  give index
}

entry main at core/app {
  return safe_index(1, 3)
}""",
            ),
        ],
        "pointers": [
            (
                "Narrow unsafe helper",
                "This keeps the unsafe operation behind one named boundary.",
                """space examples/pointers/narrow

unsafe proc store_value(dst: ptr[int], value: int) -> int {
  give 0
}

proc write_checked(dst: ptr[int], value: int) -> int {
  if value < 0 { give 11 }
  give store_value(dst, value)
}""",
            ),
            (
                "Safe caller surface",
                "This wrapper keeps low-level detail outside normal application flow.",
                """space examples/pointers/wrapper

unsafe proc host_write(code: int) -> int {
  give code
}

proc write_status(code: int) -> int {
  if code < 0 { give 12 }
  give host_write(code)
}

entry main at core/app {
  return write_status(0)
}""",
            ),
        ],
        "generics": [
            (
                "Reusable value wrapper",
                "This generic code keeps the input and output type identical.",
                """space examples/generics/value

proc keep[T](value: T) -> T {
  give value
}

entry main at core/app {
  let code: int = keep[int](7)
  return code
}""",
            ),
            (
                "Generic pair shape",
                "This variant gives abstraction a concrete data shape.",
                """space examples/generics/pair

form Pair[T] {
  left: T
  right: T
}

proc first[T](pair: Pair[T]) -> T {
  give pair.left
}

entry main at core/app {
  let pair: Pair[int] = Pair[int](1, 2)
  return first[int](pair)
}""",
            ),
        ],
        "macros": [
            (
                "Bounded generated surface",
                "This macro-like block emits only the repeated surface it owns.",
                """space examples/macros/trace

macro emit_trace(name, code) {
  emit name
  emit code
}

proc run() -> int {
  give 0
}

entry main at core/app {
  return run()
}""",
            ),
            (
                "Prefer procedure when logic is ordinary",
                "This example keeps normal behavior in procedures and reserves generation for shape.",
                """space examples/macros/restraint

proc trace_status(code: int) -> int {
  if code != 0 { give code }
  give 0
}

entry main at core/app {
  return trace_status(0)
}""",
            ),
        ],
        "compiler": [
            (
                "Staged compiler flow",
                "This program keeps parse and lower stages visibly separate.",
                """space examples/compiler/stages

pick Stage {
  case Parsed(nodes: int)
  case Failed(code: int)
}

proc parse(size: int) -> Stage {
  if size <= 0 { give Stage.Failed(11) }
  give Stage.Parsed(size)
}

proc emit(stage: Stage) -> int {
  match stage {
    case Parsed(nodes) { give nodes }
    case Failed(code) { give code }
    otherwise { give 70 }
  }
}

entry main at core/app {
  return emit(parse(3))
}""",
            ),
            (
                "Intermediate representation boundary",
                "This variant names the intermediate state before producing an exit code.",
                """space examples/compiler/ir

form IrUnit {
  nodes: int
  warnings: int
}

proc lower(nodes: int) -> IrUnit {
  if nodes < 0 { give IrUnit(0, 1) }
  give IrUnit(nodes, 0)
}

proc finish(unit: IrUnit) -> int {
  if unit.warnings > 0 { give 20 }
  give unit.nodes
}""",
            ),
        ],
        "interop": [
            (
                "Safe wrapper around host call",
                "This block isolates the host edge and exposes a local status.",
                """space examples/interop/wrapper

unsafe proc host_call(code: int) -> int {
  give code
}

proc call_checked(code: int) -> int {
  if code < 0 { give 11 }
  give host_call(code)
}

entry main at core/app {
  return call_checked(0)
}""",
            ),
            (
                "Translate foreign result",
                "This version turns a foreign code into a local result shape.",
                """space examples/interop/translate

pick HostResult {
  case Ok
  case Failed(code: int)
}

unsafe proc raw_host() -> int {
  give 0
}

proc host_result() -> HostResult {
  let code: int = raw_host()
  if code != 0 { give HostResult.Failed(code) }
  give HostResult.Ok
}""",
            ),
        ],
        "stdlib": [
            (
                "Separate domain and library helper",
                "This flow keeps pure validation apart from utility-style normalization.",
                """space examples/stdlib/manifest

form Manifest {
  name: string
  root: string
}

proc normalize_root(root: string) -> string {
  if root == "" { give "." }
  give root
}

proc validate_manifest(m: Manifest) -> int {
  if m.name == "" { give 11 }
  let root: string = normalize_root(m.root)
  give 0
}""",
            ),
            (
                "Classify library responsibility",
                "This block keeps transformation local before any runtime-facing effect.",
                """space examples/stdlib/classify

proc trim_code(code: int) -> int {
  if code < 0 { give 0 }
  give code
}

proc render_status(code: int) -> string {
  if code == 0 { give "ok" }
  give "error"
}

entry main at core/app {
  let code: int = trim_code(0)
  return code
}""",
            ),
        ],
        "testing": [
            (
                "Invariant target",
                "This example exposes one behavior that a unit test can protect.",
                """space examples/testing/invariant

proc bounded(value: int) -> int {
  if value < 0 { give 0 }
  if value > 10 { give 10 }
  give value
}

entry main at core/app {
  return bounded(8)
}""",
            ),
            (
                "Scenario target",
                "This complete flow gives scenario tests a stable entry path.",
                """space examples/testing/scenario

proc prepare(name: string) -> int {
  if name == "" { give 11 }
  give 0
}

proc run(name: string) -> int {
  let status: int = prepare(name)
  if status != 0 { give status }
  give 0
}

entry main at core/app {
  return run("demo")
}""",
            ),
        ],
        "performance": [
            (
                "Single traversal",
                "This version keeps traversal count and accumulator state visible.",
                """space examples/performance/traversal

proc sum_once(values: list[int]) -> int {
  let total: int = 0
  for value in values {
    set total = total + value
  }
  give total
}

entry main at core/app {
  return sum_once([1, 2, 3])
}""",
            ),
            (
                "Avoid repeated work",
                "This variant stores the intermediate result before branching on it.",
                """space examples/performance/cache

proc classify_total(values: list[int]) -> int {
  let total: int = 0
  for value in values {
    set total = total + value
  }
  if total < 0 { give 10 }
  give total
}

entry main at core/app {
  return classify_total([2, 3, 4])
}""",
            ),
        ],
        "debugging": [
            (
                "Reduced reproducible case",
                "This keeps the failing boundary small enough to inspect.",
                """space examples/debugging/repro

proc parse_port(raw: int) -> int {
  if raw < 1 { give 11 }
  if raw > 65535 { give 12 }
  give raw
}

entry main at core/app {
  return parse_port(8080)
}""",
            ),
            (
                "Traceable correction",
                "This block names each intermediate value that helps debugging.",
                """space examples/debugging/trace

proc normalize(raw: int) -> int {
  let fallback: int = 80
  if raw <= 0 { give fallback }
  give raw
}

entry main at core/app {
  let port: int = normalize(0)
  return port
}""",
            ),
        ],
        "projects": [
            (
                "Project-shaped CLI flow",
                "This block keeps entry, domain validation, and exit code separated.",
                """space examples/projects/cli

form Command {
  name: string
  dry_run: bool
}

proc validate(cmd: Command) -> int {
  if cmd.name == "" { give 64 }
  give 0
}

entry main at core/app {
  let cmd: Command = Command("build", false)
  return validate(cmd)
}""",
            ),
            (
                "Project integration boundary",
                "This variant shows a service shape that can grow without moving the entry contract.",
                """space examples/projects/service

proc load_config(path: string) -> int {
  if path == "" { give 11 }
  give 0
}

proc start(path: string) -> int {
  let status: int = load_config(path)
  if status != 0 { give status }
  give 0
}

entry main at core/app {
  return start("vitte.toml")
}""",
            ),
        ],
        "grammar": [
            (
                "Top-level and block rules",
                "This program shows a space declaration, a procedure block, and an entry block.",
                """space examples/grammar/top

proc ok(value: int) -> int {
  if value < 0 { give 1 }
  give 0
}

entry main at core/app {
  return ok(2)
}""",
            ),
            (
                "Pattern-oriented shape",
                "This variant keeps match arms and returned results syntactically clear.",
                """space examples/grammar/pattern

pick State {
  case Ready(code: int)
  case Failed(code: int)
}

proc finish(state: State) -> int {
  match state {
    case Ready(code) { give code }
    case Failed(code) { give code }
    otherwise { give 70 }
  }
}""",
            ),
        ],
        "design": [
            (
                "Named configuration contract",
                "This example shows maintainability through explicit domain names.",
                """space examples/design/config

form BuildConfig {
  project_name: string
  max_retries: int
}

proc validate_config(cfg: BuildConfig) -> int {
  if cfg.project_name == "" { give 11 }
  if cfg.max_retries < 0 { give 12 }
  give 0
}

entry main at core/app {
  let cfg: BuildConfig = BuildConfig("demo", 2)
  return validate_config(cfg)
}""",
            ),
            (
                "Compatibility-preserving wrapper",
                "This variant keeps a stable public name while improving the internal path.",
                """space examples/design/compat

proc validate_name(name: string) -> int {
  if name == "" { give 11 }
  give 0
}

proc validate_project(name: string) -> int {
  give validate_name(name)
}

entry main at core/app {
  return validate_project("demo")
}""",
            ),
        ],
        "delivery": [
            (
                "Local release evidence",
                "This program exposes a release contract that local checks can cover.",
                """space examples/delivery/local

proc validate_release(name: string, version: int) -> int {
  if name == "" { give 11 }
  if version <= 0 { give 12 }
  give 0
}

entry main at core/app {
  return validate_release("demo", 1)
}""",
            ),
            (
                "CI-facing status",
                "This variant keeps the result code stable for automation.",
                """space examples/delivery/ci

proc check_artifact(size: int) -> int {
  if size <= 0 { give 21 }
  give 0
}

proc release(size: int) -> int {
  let status: int = check_artifact(size)
  if status != 0 { give status }
  give 0
}

entry main at core/app {
  return release(42)
}""",
            ),
        ],
    }
    return variants.get(profile_name, [
        (
            "Complete service flow",
            "This example anchors the chapter in a full input, validation, and result path.",
            """space examples/generic/service

form Request {
  name: string
  value: int
}

proc validate(req: Request) -> int {
  if req.name == "" { give 11 }
  if req.value < 0 { give 12 }
  give 0
}

entry main at core/app {
  let req: Request = Request("demo", 1)
  return validate(req)
}""",
        ),
        (
            "Explicit result flow",
            "This variant names the result so later refactors keep the same contract.",
            """space examples/generic/result

pick Result {
  case Ok(code: int)
  case Failed(code: int)
}

proc run(value: int) -> Result {
  if value < 0 { give Result.Failed(11) }
  give Result.Ok(value)
}

entry main at core/app {
  let result: Result = run(3)
  return 0
}""",
        ),
    ])


def render_complete_examples(profile_name: str, profile: dict) -> str:
    examples = [
        (
            "Primary coherent example",
            "This is the compact chapter anchor used by the surrounding explanation.",
            profile["example"],
        ),
        *complete_example_variants(profile_name),
    ]
    lines = [
        '<section class="chapter-examples">',
        "<h2>Complete examples</h2>",
        "<p>Each block below is a complete reading unit with its own boundary, data shape, and observable result.</p>",
    ]
    for title, description, source in examples:
        lines.extend([
            f"<h3>{html.escape(title)}</h3>",
            f"<p>{html.escape(description)}</p>",
            vitte_code_block(source),
        ])
    lines.append("</section>")
    return "\n".join(lines)


def immediate_chapter_work(profile_name: str) -> list[str]:
    shared = [
        "Keep the first code block complete and aligned with current Vitte docs syntax.",
        "Keep the invalid block focused on one broken contract only.",
        "Replace generic prose with one concrete rule visible in the code.",
        "Keep every example small enough to review from top to bottom.",
    ]
    specific = {
        "philosophy": [
            "Show the design rule through a real data shape, not through slogans.",
            "Tie every claim about clarity to a named contract in the example.",
            "Keep failure surfaces explicit before adding broader language values.",
        ],
        "starting": [
            "Use one executable `proc main(args: list[string]) -> int` path.",
            "Keep the first program minimal while still complete.",
            "Make the exit code flow visible from helper to main.",
        ],
        "syntax": [
            "Separate declarations, statements, expressions, and block boundaries.",
            "Use one valid block that demonstrates the chapter shape without noise.",
            "Make the malformed block fail at syntax shape, not business logic.",
        ],
        "types": [
            "Use forms and picks to show domain meaning, not isolated type names.",
            "Prefer constructor syntax already used by Vitte docs.",
            "Show one mismatch where the type contract catches the error early.",
        ],
        "procedures": [
            "Make the signature and returned value carry the full procedure contract.",
            "Keep branch logic inside the procedure instead of hiding global state.",
            "Add one composition example only when the call boundary stays obvious.",
        ],
        "control": [
            "Show the normal path, guard path, and fallback path in one flow.",
            "Use branches and loops only when they change a visible result.",
            "Keep each control construct tied to a named execution path.",
        ],
        "collections": [
            "Show the access pattern that justifies the collection.",
            "Return a grouped result when aggregation creates a new domain shape.",
            "Avoid listing containers without a flow that uses them.",
        ],
        "modules": [
            "Show one facade module and one local responsibility module.",
            "Keep imports at the module surface, not inside local procedure bodies.",
            "Make public surface smaller than implementation detail.",
        ],
        "diagnostics": [
            "Connect each diagnostic to the exact contract it protects.",
            "Keep repro code minimal and complete.",
            "Show the corrected shape immediately after the broken shape.",
        ],
        "pointers": [
            "Show a safe caller, an explicit unsafe edge, and a checked result.",
            "Keep raw pointers out of broad public APIs.",
            "Name the review risks: aliasing, lifetime, null, escape, and mutation.",
            "Use interop or runtime boundaries as the justification for unsafe code.",
        ],
        "generics": [
            "Show the concrete reuse payoff before introducing type parameters.",
            "Keep input and output type contracts consistent.",
            "Avoid generic examples that only rename `int` without adding reuse.",
        ],
        "macros": [
            "Show the generated surface and the readability cost.",
            "Prefer a procedure when ordinary control flow is enough.",
            "Keep macro examples bounded to one repeated shape.",
        ],
        "compiler": [
            "Separate source input, intermediate representation, and emitted result.",
            "Make stage ownership visible in the data passed between procedures.",
            "Keep pipeline examples small but multi-stage.",
        ],
        "interop": [
            "Show the foreign edge and the local wrapper as different contracts.",
            "Translate raw host codes into local result shapes.",
            "Prevent foreign assumptions from leaking into ordinary code.",
        ],
        "stdlib": [
            "Classify each helper by responsibility: core, collection, transform, I/O, runtime.",
            "Keep pure domain logic separate from runtime-facing effects.",
            "Use examples that scale to the library family, not only one call.",
        ],
        "testing": [
            "Name the evidence each test layer provides.",
            "Keep one unit-level invariant and one scenario-level path visible.",
            "Use invalid fixtures to protect known failure contracts.",
        ],
        "performance": [
            "Name the measured boundary before suggesting optimization.",
            "Show traversal, allocation, or copy cost in the code shape.",
            "Keep the optimized variant behaviorally equivalent to the baseline.",
        ],
        "debugging": [
            "Reduce the failure to one stable repro.",
            "Keep intermediate values named when they aid inspection.",
            "Preserve the evidence that proves the correction.",
        ],
        "projects": [
            "Keep entry, domain, integration, and output as separate surfaces.",
            "Use project examples that can grow without rewriting the entry contract.",
            "Name which boundary changes for CLI, HTTP, service, embedded, or editor variants.",
        ],
        "grammar": [
            "Map examples to top-level, statement, expression, and pattern layers.",
            "Keep parse failures separate from type or domain failures.",
            "Use grammar examples that preserve the current Vitte docs surface.",
        ],
        "design": [
            "Turn naming advice into a before and after code contract.",
            "Tie conventions to review cost and compatibility risk.",
            "Keep the public API promise visible in the example.",
        ],
        "delivery": [
            "Connect local checks, CI checks, and review gates to named evidence.",
            "Keep release status codes stable for automation.",
            "Show which contract each delivery gate protects.",
        ],
        "generic": [
            "Anchor the page in a complete service-style flow.",
            "Use the chapter title to specialize the generic rule.",
            "Keep the result contract stable across refactors.",
        ],
    }
    return specific.get(profile_name, specific["generic"]) + shared


def render_immediate_chapter_work(profile_name: str) -> str:
    lines = [
        '<section class="chapter-now">',
        "<h2>Immediate work for this chapter</h2>",
        "<ul>",
    ]
    for item in immediate_chapter_work(profile_name):
        lines.append(f"<li>{html.escape(item)}</li>")
    lines.extend([
        "</ul>",
        "</section>",
    ])
    return "\n".join(lines)


def profile_deep_dive(profile_name: str) -> list[tuple[str, list[str]]]:
    generic = [
        ("Contract to expose", [
            "Name the input shape before the first branch.",
            "Keep transformation local and observable.",
            "Return a result that preserves the chapter's main rule.",
        ]),
        ("Review focus", [
            "The public name should explain the contract.",
            "The example should remain executable as a small standalone module.",
            "The invalid case should break the same contract the text explains.",
        ]),
        ("Diagnostics to expect", [
            "Mismatched type or result shape.",
            "Missing boundary around a value that later code depends on.",
            "A broad claim that no concrete code line supports.",
        ]),
    ]
    dives = {
        "pointers": [
            ("Safe wrapper", [
                "The normal caller should see a checked procedure returning an ordinary status code.",
                "The raw pointer should appear only in the smallest procedure that needs it.",
                "The wrapper must validate obvious preconditions before crossing the unsafe edge.",
            ]),
            ("Unsafe edge", [
                "`unsafe proc` marks the review boundary, not a permission to spread raw memory through the module.",
                "Interop, runtime storage, and controlled buffer operations are valid reasons to expose the edge.",
                "The low-level operation should return a status instead of leaking raw pointer state.",
            ]),
            ("Memory risks", [
                "Aliasing risk appears when two names can mutate the same storage.",
                "Lifetime risk appears when a pointer can outlive the value it points to.",
                "Null, dangling, escape, and shared mutation risks must be named near the example.",
            ]),
            ("Review checklist", [
                "Unsafe code is local and easy to grep.",
                "Raw pointers do not cross the broad public API.",
                "The safe caller checks inputs and validates the returned status.",
            ]),
        ],
        "modules": [
            ("Public surface", [
                "Imports belong at the module surface so ownership is visible before behavior.",
                "The facade should expose one stable procedure rather than every helper.",
                "Public names should describe responsibility, not file layout.",
            ]),
            ("Private implementation", [
                "Validation and construction can remain local to the module that owns the data.",
                "Implementation helpers should stay behind the facade until another module needs them.",
                "A module split is justified only when it reduces coupling or clarifies ownership.",
            ]),
            ("Layout discipline", [
                "Use one module for entry orchestration and another for domain responsibility.",
                "Avoid imports inside procedure bodies unless the docs intentionally teach that exception.",
                "Keep circular dependency risks visible in the invalid case or diagnostics.",
            ]),
        ],
        "types": [
            ("Domain shapes", [
                "`form` should group fields that move together through the program.",
                "`pick` should represent named outcomes instead of magic integers.",
                "Aliases should clarify intent rather than hide a primitive type without benefit.",
            ]),
            ("Constructor discipline", [
                "Use `Form { field: value }` so field meaning is visible at the call site.",
                "Keep field names stable when refactoring internal logic.",
                "Prefer explicit result variants when the caller must handle several outcomes.",
            ]),
            ("Type diagnostics", [
                "A mismatch between `string`, `int`, and `bool` should point back to domain meaning.",
                "Return type errors are contract errors, not formatting problems.",
                "Invalid examples should break one type promise at a time.",
            ]),
        ],
        "syntax": [
            ("Block shape", [
                "Top-level declarations establish the module before executable code starts.",
                "Procedure bodies should make local bindings, branches, and final `give` easy to scan.",
                "Closing braces are part of the readable contract because they define scope.",
            ]),
            ("Statement layer", [
                "`let`, `set`, `if`, `for`, `match`, and `give` belong to different reading roles.",
                "A syntax chapter should teach placement before advanced meaning.",
                "The malformed example should fail because the shape is incomplete.",
            ]),
        ],
        "control": [
            ("Guard path", [
                "Guards should appear before the nominal path they protect.",
                "Each guard should return or continue with an observable reason.",
                "A guard that changes no result is probably noise.",
            ]),
            ("Loop path", [
                "Loops need a named accumulator or clear side effect.",
                "Continue and break should make the path simpler, not harder to follow.",
                "The final result should be traceable without simulating hidden state.",
            ]),
            ("Fallback path", [
                "Match or branch fallback should be explicit when the input domain is not exhausted.",
                "The fallback value should encode policy, not an accidental default.",
                "Tests should cover at least one nominal path and one fallback path.",
            ]),
        ],
        "collections": [
            ("Access pattern", [
                "Choose a collection because the code needs grouping, iteration, lookup, or aggregation.",
                "The example should show where values enter and leave the container.",
                "A collection without an access pattern is vocabulary, not design.",
            ]),
            ("Aggregation result", [
                "Use a result form when a loop produces more than one fact.",
                "Keep counters and totals visible while the aggregation runs.",
                "Return the grouped result rather than scattering related values across locals.",
            ]),
        ],
        "procedures": [
            ("Signature contract", [
                "The parameter list names what the caller must provide.",
                "The return type names what the caller may rely on.",
                "A missing return shape makes the procedure impossible to review precisely.",
            ]),
            ("Composition", [
                "Small procedures should compose through explicit status values or result shapes.",
                "Do not hide control flow in globals when a return value can express it.",
                "A procedure chain should preserve the same contract at each boundary.",
            ]),
        ],
        "diagnostics": [
            ("Failure shape", [
                "Start from the expected contract, then show the actual broken shape.",
                "Keep the repro small enough that the first diagnostic is credible.",
                "Explain the correction as a structural change, not a guess.",
            ]),
            ("Diagnostic reading", [
                "Parser diagnostics point to shape.",
                "Type diagnostics point to contract mismatch.",
                "Module diagnostics point to visibility, ownership, or path resolution.",
            ]),
        ],
        "generics": [
            ("Reuse boundary", [
                "Type parameters should preserve a relationship between input and output.",
                "The call site should show at least one concrete instantiation.",
                "Generic code should remove duplication without hiding meaning.",
            ]),
            ("Abstraction cost", [
                "Add a generic only when two concrete uses share the same real contract.",
                "Keep constraints near the operation that needs them.",
                "Invalid generic examples should break the reusable promise directly.",
            ]),
        ],
        "macros": [
            ("Generation boundary", [
                "Macros should generate a narrow, reviewable surface.",
                "The generated shape should be easier to inspect than duplicated hand-written code.",
                "Ordinary logic should stay in procedures unless generation changes the structure.",
            ]),
            ("Debuggability", [
                "The macro call should make the generated intent visible.",
                "Diagnostics should point back to the input pattern or generated surface.",
                "Avoid examples where macro power hides the actual control path.",
            ]),
        ],
        "compiler": [
            ("Stage ownership", [
                "Parse, lower, analyze, and emit stages should exchange named data shapes.",
                "Each stage should own one responsibility and one output contract.",
                "Collapsed stages make diagnostics and testing harder.",
            ]),
            ("Pipeline evidence", [
                "Show the source input, intermediate state, and emitted result.",
                "Keep failure variants available across stage boundaries.",
                "Tests should pin at least one successful path and one failed stage.",
            ]),
        ],
        "interop": [
            ("Foreign edge", [
                "The host call should stay behind an `unsafe proc` or equivalent boundary.",
                "Foreign status codes should be translated into local result shapes.",
                "The rest of the module should not depend on foreign calling conventions.",
            ]),
            ("Wrapper discipline", [
                "Validate local inputs before invoking the foreign edge.",
                "Keep the wrapper name domain-oriented.",
                "Return ordinary Vitte values from the wrapper.",
            ]),
        ],
        "stdlib": [
            ("Family responsibility", [
                "Core helpers, collections, transforms, I/O, and runtime edges solve different problems.",
                "The example should show which family owns which step.",
                "Do not mix pure validation with host effects in the same explanation.",
            ]),
            ("Catalog usefulness", [
                "A stdlib chapter should help classify future code.",
                "Examples should scale from one helper to a family rule.",
                "References should point to modules that reinforce ownership.",
            ]),
        ],
        "testing": [
            ("Evidence layers", [
                "Unit tests protect small invariants.",
                "Scenario tests protect user-visible flows.",
                "Regression tests protect previously broken contracts.",
            ]),
            ("Fixture design", [
                "Keep valid and invalid fixtures close to the code shape they prove.",
                "Name the claim before naming the test file.",
                "Avoid giant fixtures when a smaller one proves the same claim.",
            ]),
        ],
        "performance": [
            ("Measured boundary", [
                "Name traversal, allocation, copying, or dispatch before optimizing.",
                "The baseline and improved shape must return the same result.",
                "Performance prose should point to visible data movement.",
            ]),
            ("Cost review", [
                "Loops should expose accumulator and iteration count.",
                "Avoid optimizing syntax when the data shape dominates cost.",
                "Keep benchmark claims separate from design claims.",
            ]),
        ],
        "debugging": [
            ("Repro boundary", [
                "Reduce the bug to one stable input and one observable output.",
                "Name intermediate values that explain the failure path.",
                "Keep the corrected case beside the failing shape.",
            ]),
            ("Investigation record", [
                "Preserve the symptom, reduced case, and verified correction.",
                "Do not replace evidence with speculation.",
                "Regression coverage should protect the reduced case.",
            ]),
        ],
        "projects": [
            ("Project shape", [
                "Entry, domain, integration, and output should remain separate.",
                "The entry module should orchestrate rather than own every rule.",
                "A project example should be small but ready to grow.",
            ]),
            ("Variant pressure", [
                "CLI, HTTP, service, embedded, and editor projects stress different boundaries.",
                "Only the integration edge should change when the domain contract is stable.",
                "Keep configuration validation near the entry boundary.",
            ]),
        ],
        "grammar": [
            ("Grammar layer", [
                "Top-level items, statements, expressions, patterns, and types should be identifiable.",
                "A parser failure should not be explained as a business rule failure.",
                "Ambiguity notes should name the two possible readings.",
            ]),
            ("Valid and invalid surface", [
                "Use one full valid program as the reference shape.",
                "Use one invalid fragment that breaks a single grammar layer.",
                "Keep examples aligned with the current Vitte docs syntax.",
            ]),
        ],
        "design": [
            ("Maintainability rule", [
                "A naming or convention rule must improve a concrete code contract.",
                "Before and after examples should show reduced ambiguity.",
                "Compatibility promises belong in public names and result shapes.",
            ]),
            ("Review value", [
                "A reviewer should be able to point to the exact improved line.",
                "Avoid slogans that do not change code.",
                "Keep migration cost visible when changing public surfaces.",
            ]),
        ],
        "delivery": [
            ("Evidence flow", [
                "Local checks prove fast invariants.",
                "CI checks prove repeatability on clean infrastructure.",
                "Review checks preserve architecture and intent.",
            ]),
            ("Release contract", [
                "Automation needs stable status codes and predictable output.",
                "Docs and tests should protect the same readiness claim.",
                "A release gate should say which regression it prevents.",
            ]),
        ],
        "philosophy": [
            ("Design lens", [
                "The chapter should show explicitness, contracts, and failure surfaces in code.",
                "Readable structure is demonstrated by named data and results.",
                "Language values become useful only when they change review behavior.",
            ]),
            ("Practical rule", [
                "Avoid prose-only principles.",
                "Tie every principle to a small executable shape.",
                "Keep failure handling as part of the design story.",
            ]),
        ],
        "starting": [
            ("First executable shape", [
                "The first program should show a complete path from helper to main.",
                "The entry result should be obvious without extra tooling.",
                "Avoid introducing advanced constructs before the first stable program.",
            ]),
            ("Beginner contract", [
                "Keep names plain and domain-neutral.",
                "Show one failure code before adding richer result types.",
                "Preserve the exact syntax used by docs and tests.",
            ]),
        ],
    }
    return dives.get(profile_name, generic)


def render_profile_deep_dive(profile_name: str) -> str:
    lines = [
        "<h3>Profile-specific deep dive</h3>",
    ]
    for title, items in profile_deep_dive(profile_name):
        lines.append(f"<h4>{html.escape(title)}</h4>")
        lines.append("<ul>")
        for item in items:
            lines.append(f"<li>{html.escape(item)}</li>")
        lines.append("</ul>")
    return "\n".join(lines)


CHAPTER_OVERRIDES = {
    "12-pointers.html": {
        "problem": "This chapter must teach raw memory as an explicitly reviewed boundary. The reader should leave with a concrete distinction between ordinary safe callers, the smallest unsafe edge, and the checks that keep raw pointer usage from spreading through a module.",
        "examples": [
            (
                "Checked write wrapper",
                "A normal caller validates domain input before invoking the unsafe pointer-facing operation.",
                """space examples/pointers/checked_write

unsafe proc raw_write(dst: ptr[int], value: int) -> int {
  give 0
}

proc write_checked(dst: ptr[int], value: int) -> int {
  if value < 0 { give 11 }
  give raw_write(dst, value)
}

proc main(args: list[string]) -> int {
  let status: int = write_checked(0, 7)
  give status
}""",
            ),
            (
                "Read-only borrow boundary",
                "A read-facing helper returns an ordinary value and keeps pointer risk out of callers.",
                """space examples/pointers/read_only

unsafe proc raw_read(src: ptr[int]) -> int {
  give 0
}

proc read_status(src: ptr[int]) -> int {
  let value: int = raw_read(src)
  if value < 0 { give 12 }
  give value
}

proc main(args: list[string]) -> int {
  give read_status(0)
}""",
            ),
            (
                "Mutation status boundary",
                "Mutation stays behind a narrow status-returning surface instead of escaping into public API.",
                """space examples/pointers/mutation_status

unsafe proc raw_increment(slot: ptr[int]) -> int {
  give 0
}

proc increment_checked(slot: ptr[int], enabled: bool) -> int {
  if not enabled { give 20 }
  give raw_increment(slot)
}

proc main(args: list[string]) -> int {
  give increment_checked(0, true)
}""",
            ),
        ],
        "risk_rows": [
            ("Aliasing", "Two mutable names can affect the same storage.", "Keep mutation behind one wrapper and one status result."),
            ("Lifetime", "The pointer can outlive the value it targets.", "Do not store raw pointers in broad public data shapes."),
            ("Null or dangling pointer", "The pointer value does not identify valid storage.", "Validate at the wrapper boundary and return an error code."),
            ("Unsafe escape", "Raw memory assumptions leak into ordinary callers.", "Expose safe procedures, not raw pointer operations."),
        ],
        "checklist": [
            "Unsafe code is local, named, and grep-friendly.",
            "Every unsafe procedure has a safe caller or wrapper beside it.",
            "The public API returns ordinary Vitte values or status codes.",
            "Aliasing, lifetime, null, escape, and mutation are mentioned near the example.",
            "The invalid case breaks exactly one pointer contract.",
        ],
        "production": [
            "Use this pattern for interop, runtime buffers, allocator internals, and constrained device memory.",
            "Require review on every change that widens the raw pointer surface.",
            "Pin regression fixtures around unsafe wrappers rather than around every internal instruction.",
        ],
        "avoid": [
            "Do not use pointers to bypass a type contract that can be modeled as a form or pick.",
            "Do not let raw pointers appear in project-level service APIs.",
            "Do not treat unsafe code as an optimization until measurement identifies the memory boundary.",
        ],
    },
    "04-syntax.html": {
        "problem": "This chapter must teach syntax as readable program shape. The reader should see where declarations, statements, expressions, and final results belong before learning advanced constructs.",
        "examples": [
            (
                "Minimal block shape",
                "One procedure keeps binding, guard, and final result in the canonical order.",
                """space examples/syntax/minimal_block

proc clamp(value: int) -> int {
  let min: int = 0
  if value < min { give min }
  give value
}

proc main(args: list[string]) -> int {
  give clamp(3)
}""",
            ),
            (
                "Nested branch shape",
                "A larger block remains readable because every branch returns a visible status.",
                """space examples/syntax/nested_branch

proc route(code: int) -> int {
  if code < 0 {
    give 10
  }
  if code == 0 {
    give 0
  }
  give 1
}

proc main(args: list[string]) -> int {
  give route(5)
}""",
            ),
            (
                "Declaration then execution",
                "Top-level declarations come before the procedure that makes behavior observable.",
                """space examples/syntax/declaration_flow

const EXIT_OK: int = 0

proc status(enabled: bool) -> int {
  if not enabled { give 11 }
  give EXIT_OK
}

proc main(args: list[string]) -> int {
  give status(true)
}""",
            ),
        ],
        "risk_rows": [
            ("Incomplete block", "A declaration or procedure body has no closed scope.", "Keep braces and final `give` visible."),
            ("Misplaced statement", "A local statement appears at module level.", "Separate top-level declarations from procedure bodies."),
            ("Ambiguous result", "The reader cannot locate the returned value.", "End the path with a clear `give`."),
        ],
        "checklist": [
            "Top-level items are distinct from executable statements.",
            "Each procedure has a visible result path.",
            "The invalid example breaks syntax shape only.",
            "Examples use current docs syntax and no obsolete entry form.",
        ],
        "production": [
            "Use this chapter as the reference shape for all later tutorial examples.",
            "Keep code blocks short enough for parser diagnostics to map to one line family.",
            "Prefer one canonical form over several equivalent-looking variants.",
        ],
        "avoid": [
            "Do not teach syntax through disconnected fragments.",
            "Do not mix parser errors with type or domain errors in the first example.",
            "Do not add advanced compiler constructs before block shape is stable.",
        ],
    },
    "04-syntaxe.html": {
        "alias": "04-syntax.html",
    },
    "05-types.html": {
        "problem": "This chapter must teach types as domain contracts. The reader should see primitive values, forms, picks, and result shapes working together instead of memorizing isolated type names.",
        "examples": [
            (
                "Domain form contract",
                "A form groups values that move together through validation.",
                """space examples/types/domain_form

form Package {
  name: string
  size: int
  signed: bool
}

proc validate(pkg: Package) -> int {
  if pkg.name == "" { give 11 }
  if pkg.size <= 0 { give 12 }
  if not pkg.signed { give 13 }
  give 0
}

proc main(args: list[string]) -> int {
  let pkg: Package = Package { name: "core", size: 4, signed: true }
  give validate(pkg)
}""",
            ),
            (
                "Named result variants",
                "A pick makes success and failure visible to the caller.",
                """space examples/types/result_variants

pick LoadResult {
  case Loaded(size: int)
  case Failed(code: int)
}

proc load(size: int) -> LoadResult {
  if size <= 0 { give LoadResult.Failed(20) }
  give LoadResult.Loaded(size)
}

proc main(args: list[string]) -> int {
  let result: LoadResult = load(8)
  give 0
}""",
            ),
            (
                "Alias with intent",
                "A type alias is useful only when the alias carries a stronger name than the primitive.",
                """space examples/types/alias_intent

type ExitCode = int

proc normalize(code: ExitCode) -> ExitCode {
  if code < 0 { give 1 }
  give code
}

proc main(args: list[string]) -> int {
  give normalize(0)
}""",
            ),
        ],
        "risk_rows": [
            ("Weak primitive", "A raw `int` hides domain meaning.", "Use a named alias, form field, or result variant when the meaning matters."),
            ("Field mismatch", "A value is assigned to a field with the wrong type.", "Use constructor syntax with explicit field names."),
            ("Result mismatch", "The procedure promises one result shape and returns another.", "Make return type and `give` expression match."),
        ],
        "checklist": [
            "Every field name explains domain meaning.",
            "Constructors use `Form { field: value }` syntax.",
            "Picks represent named outcomes rather than magic codes.",
            "The invalid case breaks one type contract.",
        ],
        "production": [
            "Use forms at module boundaries where several values must stay coherent.",
            "Use picks for status surfaces that callers must branch on.",
            "Use aliases only when the name improves review precision.",
        ],
        "avoid": [
            "Do not introduce aliases that only rename a primitive without adding meaning.",
            "Do not collapse several domain fields into one loosely named integer.",
            "Do not use invalid examples that are merely strange instead of diagnostic.",
        ],
    },
    "07-control.html": {
        "problem": "This chapter must teach execution paths. The reader should follow guard paths, loop paths, and fallback paths without guessing hidden state.",
        "examples": [
            (
                "Guarded status flow",
                "Guards appear before the nominal result and return explicit status codes.",
                """space examples/control/guarded_status

proc validate(size: int) -> int {
  if size < 0 { give 11 }
  if size == 0 { give 12 }
  give 0
}

proc main(args: list[string]) -> int {
  give validate(4)
}""",
            ),
            (
                "Loop accumulator",
                "The accumulator is visible from initialization to final result.",
                """space examples/control/loop_accumulator

proc sum_positive(values: list[int]) -> int {
  let total: int = 0
  for value in values {
    if value < 0 { continue }
    set total = total + value
  }
  give total
}

proc main(args: list[string]) -> int {
  give sum_positive([1, -1, 3])
}""",
            ),
            (
                "Fallback classification",
                "The fallback path is an intentional policy, not an accidental default.",
                """space examples/control/fallback

proc classify(code: int) -> int {
  if code == 0 { give 0 }
  if code == 1 { give 10 }
  give 99
}

proc main(args: list[string]) -> int {
  give classify(2)
}""",
            ),
        ],
        "risk_rows": [
            ("Hidden path", "A branch changes control flow without visible effect.", "Return or update a named value in every meaningful branch."),
            ("Loop drift", "The accumulator is hard to audit.", "Initialize, update, and return the accumulator clearly."),
            ("Accidental fallback", "The final path is a leftover default.", "Name the fallback as policy in code and prose."),
        ],
        "checklist": [
            "Guard path appears before nominal path.",
            "Loop state is named and local.",
            "Fallback path has a clear status value.",
            "Invalid control examples break control shape, not unrelated typing.",
        ],
        "production": [
            "Use guard-first control in validation-heavy boundaries.",
            "Use loops only when repeated work is the clearest shape.",
            "Add regression cases for nominal, guard, and fallback paths.",
        ],
        "avoid": [
            "Do not add branches that produce the same observable result.",
            "Do not hide fallback policy in an unexplained final `give`.",
            "Do not combine loop mutation and broad side effects in the teaching example.",
        ],
    },
    "07-controle.html": {
        "alias": "07-control.html",
    },
    "08-structures.html": {
        "problem": "This chapter must teach structures as data-shape decisions. The reader should understand why grouped values, collections, and aggregate results exist in a flow.",
        "examples": [
            (
                "Aggregate summary",
                "A form gives the loop result a stable shape.",
                """space examples/structures/aggregate_summary

form Summary {
  count: int
  total: int
}

proc summarize(values: list[int]) -> Summary {
  let count: int = 0
  let total: int = 0
  for value in values {
    set count = count + 1
    set total = total + value
  }
  give Summary { count: count, total: total }
}

proc main(args: list[string]) -> int {
  let summary: Summary = summarize([1, 2, 3])
  give summary.count
}""",
            ),
            (
                "Filter shape",
                "The collection is justified by an access pattern, not by vocabulary.",
                """space examples/structures/filter_shape

proc count_ready(values: list[int]) -> int {
  let ready: int = 0
  for value in values {
    if value <= 0 { continue }
    set ready = ready + 1
  }
  give ready
}

proc main(args: list[string]) -> int {
  give count_ready([0, 2, 4])
}""",
            ),
            (
                "Grouped status",
                "Related status values stay together instead of drifting across locals.",
                """space examples/structures/grouped_status

form Status {
  code: int
  retries: int
}

proc build_status(code: int) -> Status {
  if code != 0 { give Status { code: code, retries: 0 } }
  give Status { code: 0, retries: 1 }
}

proc main(args: list[string]) -> int {
  let status: Status = build_status(0)
  give status.code
}""",
            ),
        ],
        "risk_rows": [
            ("Scattered values", "Related values move independently and lose meaning.", "Group them in a form when they are reviewed together."),
            ("Container without pattern", "A list appears without iteration, lookup, or aggregation.", "Name the access pattern in code and prose."),
            ("Aggregate ambiguity", "The result of a loop is split across unrelated locals.", "Return a grouped result shape."),
        ],
        "checklist": [
            "Every structure has a visible access pattern.",
            "Aggregates return named fields.",
            "The example shows where grouped data enters and leaves.",
            "Invalid examples break data shape rather than spelling.",
        ],
        "production": [
            "Use forms for stable API payloads and module boundaries.",
            "Use collections when repeated access is part of the contract.",
            "Keep aggregate outputs named so tests can assert each part.",
        ],
        "avoid": [
            "Do not list containers without using them in a flow.",
            "Do not keep related facts as separate unnamed integers.",
            "Do not choose data structures for style when access pattern is the real reason.",
        ],
    },
    "09-modules.html": {
        "problem": "This chapter must teach modules as ownership boundaries. The reader should see facade modules, local responsibility modules, imports, and exported surfaces as architecture rather than file decoration.",
        "examples": [
            (
                "Facade entry module",
                "The entry-facing module imports one public service and keeps orchestration small.",
                """space examples/modules/facade_entry

use examples/modules/service.{run_service} as service

proc run() -> int {
  give service.run_service()
}

proc main(args: list[string]) -> int {
  give run()
}

export *""",
            ),
            (
                "Service responsibility module",
                "The service owns validation and exposes one stable public procedure.",
                """space examples/modules/service

form ServiceConfig {
  name: string
  workers: int
}

proc run_service() -> int {
  let cfg: ServiceConfig = ServiceConfig { name: "api", workers: 2 }
  if cfg.name == "" { give 11 }
  if cfg.workers <= 0 { give 12 }
  give 0
}

export *""",
            ),
            (
                "Narrow import surface",
                "The consumer imports only the symbol it needs.",
                """space examples/modules/consumer

use examples/modules/service.{run_service} as service

proc check() -> int {
  let status: int = service.run_service()
  if status != 0 { give status }
  give 0
}

export *""",
            ),
        ],
        "risk_rows": [
            ("Oversized public surface", "A module exports internals and makes coupling permanent.", "Expose one facade procedure and keep helpers local."),
            ("Hidden dependency", "An import appears inside local logic and hides architecture.", "Keep imports at the module surface."),
            ("Cycle pressure", "Two modules own each other's responsibilities.", "Move shared contracts to a third boundary or reduce the split."),
        ],
        "checklist": [
            "Imports appear before behavior.",
            "Public surface is smaller than implementation detail.",
            "The example shows at least two module roles.",
            "The invalid case shows a structural import or ownership failure.",
        ],
        "production": [
            "Use facades for stable project-level boundaries.",
            "Keep domain validation in the module that owns the data.",
            "Review exports as API commitments, not convenience shortcuts.",
        ],
        "avoid": [
            "Do not import entire module trees when one symbol is enough.",
            "Do not use module splits to hide unclear ownership.",
            "Do not move imports into procedures to make examples look shorter.",
        ],
    },
}


def resolved_chapter_override(slug: str) -> dict | None:
    override = CHAPTER_OVERRIDES.get(slug)
    if override and "alias" in override:
        return CHAPTER_OVERRIDES.get(override["alias"])
    return override


def fallback_chapter_override(slug: str, title: str, profile_name: str, profile: dict) -> dict:
    ctx = PROFILE_CONTEXT.get(profile_name, PROFILE_CONTEXT["generic"])
    variants = complete_example_variants(profile_name)
    examples = [
        (
            f"{title} chapter anchor",
            "The primary example is promoted here as the first chapter-specific production reading unit.",
            profile["example"],
        ),
        *variants[:2],
    ]
    return {
        "problem": f"{title} needs a concrete production-grade anchor around {ctx['boundary']}. The page should keep the examples, diagnostics, and review rules tied to that exact boundary instead of drifting back to generic tutorial prose.",
        "examples": examples,
        "risk_rows": [
            ("Boundary drift", f"The chapter loses sight of {ctx['boundary']}.", "Restate the boundary beside the first code block and every invalid case."),
            ("Generic prose", "A paragraph would still be true in another chapter.", "Replace it with a code-specific rule from this page."),
            ("Weak diagnostic", "The failure does not point back to the chapter contract.", "Reduce the invalid case until one failure explains the rule."),
        ],
        "checklist": [
            "The first example is complete and aligned with Vitte docs syntax.",
            "The production section names where this construct belongs in real code.",
            "The risk table connects each failure to a diagnostic or review action.",
            "The avoid list rejects broad misuse without adding quiz-like prompts.",
        ],
        "production": [
            f"Use this chapter when a code review needs to preserve {ctx['boundary']}.",
            "Keep examples small enough to copy into fixtures or docs smoke tests.",
            "Treat the invalid case as regression material for future docs checks.",
        ],
        "avoid": [
            "Do not add a second topic that hides the chapter's main contract.",
            "Do not expand examples by adding unrelated subsystems.",
            "Do not rely on prose when a small Vitte block can show the rule.",
        ],
    }


def render_chapter_override(meta: dict, profile_name: str, profile: dict) -> str:
    slug = Path(meta["path"]).name
    override = resolved_chapter_override(slug) or fallback_chapter_override(slug, meta["title"], profile_name, profile)
    lines = [
        '<section class="chapter-override">',
        f"<h2>Chapter override: {html.escape(slug)}</h2>",
        "<h3>Dedicated problem</h3>",
        f"<p>{html.escape(override['problem'])}</p>",
        "<h3>Specific complete examples</h3>",
    ]
    for title, description, source in override["examples"][:5]:
        lines.extend([
            f"<h4>{html.escape(title)}</h4>",
            f"<p>{html.escape(description)}</p>",
            vitte_code_block(source),
        ])
    lines.extend([
        "<h3>Risks and diagnostics</h3>",
        "<table>",
        "<thead><tr><th>Risk</th><th>Diagnostic signal</th><th>Action</th></tr></thead>",
        "<tbody>",
    ])
    for risk, signal, action in override["risk_rows"]:
        lines.append(
            f"<tr><td>{html.escape(risk)}</td><td>{html.escape(signal)}</td><td>{html.escape(action)}</td></tr>"
        )
    lines.extend([
        "</tbody>",
        "</table>",
        "<h3>Review checklist</h3>",
        "<ul>",
    ])
    for item in override["checklist"]:
        lines.append(f"<li>{html.escape(item)}</li>")
    lines.extend([
        "</ul>",
        "<h3>Production use</h3>",
        "<ul>",
    ])
    for item in override["production"]:
        lines.append(f"<li>{html.escape(item)}</li>")
    lines.extend([
        "</ul>",
        "<h3>What to avoid</h3>",
        "<ul>",
    ])
    for item in override["avoid"]:
        lines.append(f"<li>{html.escape(item)}</li>")
    lines.extend([
        "</ul>",
        "</section>",
    ])
    return "\n".join(lines)


def render_chapter_expansion(title: str, profile_name: str, profile: dict) -> str:
    focus = html.escape(title)
    ctx = PROFILE_CONTEXT.get(profile_name, PROFILE_CONTEXT["generic"])
    example_lines = code_lines(align_vitte_docs_syntax(profile["example"]))
    invalid_lines = code_lines(align_vitte_docs_syntax(profile["invalid"]))
    summary = [html.escape(item) for item in profile["summary"]]
    pitfalls = [html.escape(item) for item in profile["pitfalls"]]

    lines = [
        '<section class="chapter-expansion">',
        "<h2>Chapter deep dive</h2>",
        f"<p><strong>{focus}</strong> {html.escape(ctx['role'])}. The chapter is written for {html.escape(ctx['reader'])}.</p>",
        f"<p>The practical boundary is: <strong>{html.escape(ctx['boundary'])}</strong>. Keep that boundary in view while reading the example, the invalid case, and the exercise.</p>",
        "<h3>Role in the learning path</h3>",
        f"<p>{html.escape(profile['problem'])}</p>",
        f"<p>{html.escape(profile['thread'])}</p>",
        f"<p>{html.escape(profile['for_what'])}</p>",
        render_profile_deep_dive(profile_name),
        "<h3>Reading the valid example</h3>",
        "<ol>",
    ]
    for idx, line in enumerate(example_lines, start=1):
        explanation = explain_vit_line(line, profile_name)
        lines.append(
            f"<li><code>{html.escape(line.strip())}</code>: {html.escape(explanation)}.</li>"
        )
    lines.extend([
        "</ol>",
        "<h3>Lesson from the invalid example</h3>",
        "<ol>",
    ])
    for idx, line in enumerate(invalid_lines, start=1):
        explanation = explain_vit_line(line, profile_name)
        lines.append(
            f"<li><code>{html.escape(line.strip())}</code>: this line helps isolate the failure because it {html.escape(explanation)}.</li>"
        )
    lines.extend([
        "</ol>",
        "<h3>Engineering decisions to preserve</h3>",
        "<ul>",
    ])
    decision_labels = [
        "Name the boundary before changing code",
        "Keep the smallest example executable",
        "Make the invalid path explain one failure only",
        "Prefer a visible contract over an implied convention",
        "Leave a review anchor that another maintainer can verify",
    ]
    for label, point in zip(decision_labels, summary):
        lines.append(f"<li><strong>{html.escape(label)}:</strong> {point}</li>")
    lines.extend([
        "</ul>",
        "<h3>Context-specific review criteria</h3>",
        "<ul>",
    ])
    criteria = [
        f"The page makes the {ctx['boundary']} boundary visible before the first code block.",
        f"The intended reader, {ctx['reader']}, can follow the valid example through named contracts instead of memorized tokens.",
        "The invalid example fails for the same reason the prose discusses.",
        "The exercise extends the same contract instead of introducing an unrelated concept.",
        "The next chapter can reuse the vocabulary introduced here without redefining it.",
        "The chapter stays specific enough that its title materially changes the meaning of the page.",
        "Every warning connects to a concrete code shape.",
        "The summary leaves one durable engineering rule behind.",
    ]
    for item in criteria:
        lines.append(f"<li>{html.escape(item)}</li>")
    lines.extend([
        "</ul>",
        "<h3>Contract matrix</h3>",
        "<table>",
        "<thead><tr><th>Concern</th><th>Chapter rule</th><th>Evidence to keep</th></tr></thead>",
        "<tbody>",
    ])
    matrix_rows = [
        ("Ownership", "Code belongs behind the boundary named by the chapter.", f"The chapter keeps ownership visible through {ctx['boundary']}."),
        ("Input contract", "The procedure receives a shape that is named before branching.", "The valid example names the accepted shape before branching."),
        ("Nominal path", "The clean path remains readable without hidden state.", "The successful result can be found without reading hidden state."),
        ("Failure path", "The invalid case isolates one failure reason.", "The broken example has one main reason to fail."),
        ("Naming", "Names explain the domain rather than only the mechanism.", "Names remain tied to the chapter goal."),
        ("Types", "Types remove ambiguity from values and results.", "Fields and return values carry domain meaning."),
        ("Control flow", "Branches stay traceable from guard to result.", "Guards appear before the result they protect."),
        ("Module boundary", "The public surface stays smaller than implementation detail.", "The public surface remains smaller than the implementation detail."),
        ("Diagnostic value", "The failure path points back to the exact contract.", "The invalid example points back to the exact contract."),
        ("Test value", "Regression evidence covers one passing path and one failing path.", "One passing case and one failing case cover the lesson."),
        ("Refactor value", "Implementation cleanup preserves the result shape.", "The result shape stays stable during local cleanup."),
        ("Publication value", "The chapter leaves one concrete engineering rule.", "The chapter leaves one concrete engineering rule."),
    ]
    for concern, rule, evidence in matrix_rows:
        lines.append(
            f"<tr><td>{html.escape(concern)}</td><td>{html.escape(rule)}</td><td>{html.escape(evidence)}</td></tr>"
        )
    lines.extend([
        "</tbody>",
        "</table>",
        "<h3>Rewrite path for this chapter</h3>",
        "<ol>",
    ])
    rewrite_steps = [
        f"Rewrite the opening paragraph so it names {ctx['boundary']} before naming syntax.",
        "Keep the valid example small enough that the full contract fits on screen.",
        "Move any broad claim back to a specific line in the example.",
        "Preserve one invalid case that fails for the chapter's main reason.",
        "Add one sentence explaining why the invalid case is not a random error.",
        "Make every pitfall actionable by naming the code shape it damages.",
        "Keep the exercise inside the same domain as the example.",
        "Avoid introducing a second unrelated project just to show variety.",
        "Use the summary to restate the chapter rule, not the table of contents.",
        "Check that the next chapter can build on this vocabulary.",
        "Remove any sentence that would still be true in every other chapter.",
        "Keep the last action small, local, and testable.",
    ]
    for step in rewrite_steps:
        lines.append(f"<li>{html.escape(step)}</li>")
    lines.extend([
        "</ol>",
        "<h3>Diagnostic anchors</h3>",
        "<ul>",
    ])
    diagnostic_anchors = [
        "The first inspected line is the one that declares the chapter's main contract.",
        "The central type, field, procedure, or branch carries the chapter's main idea.",
        "The invalid example includes a sentence-level explanation of its failure.",
        "Refactors preserve the detail that would otherwise mislead a future reader.",
        "The behavior that must stay stable is named before implementation changes begin.",
        "Vague names are replaced before they become review friction.",
        "Regression coverage protects the chapter's main contract.",
        "Implementation details stay out of public API unless the chapter explicitly teaches that surface.",
        "Beginner-facing diagnostics point to the contract, not to a random syntax detail.",
        "The next chapter can assume one clearly named concept from this page.",
    ]
    for anchor in diagnostic_anchors:
        lines.append(f"<li>{html.escape(anchor)}</li>")
    lines.extend([
        "</ul>",
        "<h3>When extending this chapter</h3>",
        "<ul>",
    ])
    extension_rules = [
        f"Extend toward {ctx['reader']}, not toward a broader catalog of features.",
        "Add a second example only if it sharpens the same contract.",
        "Prefer a small variant over a new subsystem.",
        "Keep prose close to code; every abstract claim should point to a visible shape.",
        "Do not hide a new concept in the exercise.",
        "If a paragraph explains policy, add the concrete code boundary it protects.",
        "If a paragraph explains syntax, add the semantic reason the syntax matters.",
        "If a paragraph explains architecture, identify the owner of each boundary.",
        "If a paragraph explains failure, keep the failing line close to the explanation.",
        "Stop expanding when the chapter has one complete, testable lesson.",
    ]
    for rule in extension_rules:
        lines.append(f"<li>{html.escape(rule)}</li>")
    lines.extend([
        "</ul>",
        "<h3>Failure modes to avoid</h3>",
        "<ul>",
    ])
    for item in pitfalls:
        lines.append(f"<li>{item}</li>")
    lines.extend([
        "</ul>",
        "<h3>Practice scenario</h3>",
        f"<p>Start from the coherent example in <strong>{focus}</strong>. Change one identifier, one guard, and one returned value. After each change, write down whether the public contract is still the same contract or a new one.</p>",
        "<p>If the contract changed, update the type or result shape first. If only the implementation changed, keep the external name stable and add one regression note explaining what should not change again.</p>",
        "<h3>Before moving on</h3>",
        "<ul>",
        f"<li>You can state the chapter role: {html.escape(ctx['role'])}.</li>",
        f"<li>You can point to the main boundary: {html.escape(ctx['boundary'])}.</li>",
        f"<li>You can connect the invalid case to the problem statement: {html.escape(profile['problem'])}</li>",
        f"<li>You can perform the exercise: {html.escape(profile['exercise'])}</li>",
        "</ul>",
        "</section>",
    ])
    return "\n".join(lines)


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
            "The first useful diagnostic move is naming the broken contract.",
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
        "global": "A generics chapter should make the reuse payoff explicit. Weak payoff means the generic surface is probably too abstract for the reader and the code.",
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
        "global": "Macros should be taught as controlled power tools. The important criterion is whether generation preserves or destroys the clarity of the resulting program surface.",
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
        "exercise": "Take the example and write one local check, one CI check, and one review criterion that would protect it.",
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
        "global": "The chapter should always return to the same core contract carried by the construct in the full block. That is the reason every page still needs one complete, testable example.",
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
        "<h2>Work in this chapter</h2>",
        f"<p>{html.escape(profile['what'])}</p>",
        "<h2>Coherent example</h2>",
        vitte_code_block(profile["example"]),
        render_complete_examples(profile_name, profile),
        render_immediate_chapter_work(profile_name),
        render_chapter_override(meta, profile_name, profile),
        "<h2>Global explanation</h2>",
        f"<p>{html.escape(profile['global'])}</p>",
        "<h2>Invalid case</h2>",
        vitte_code_block(profile["invalid"]),
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
        "<section class=\"chapter-see-also\"><h2>See also</h2><ul>",
    ])
    for ref in see_also[:4]:
        article.append(f"<li>{doc_ref_link(ref, rel, code=True)}</li>")
    article.extend([
        "</ul></section>",
        "<section class=\"chapter-cta\"><h2>Next best action</h2><p>Extend the coherent example by one small, justified step and keep the same contract visible from input to output.</p></section>",
        render_chapter_expansion(title, profile_name, profile),
    ])
    return add_heading_ids_and_toc("\n".join(article))


def render_page(meta: dict) -> str:
    profile_name = slug_profile(meta["path"], meta["title"])
    article = render_article(meta, profile_name)
    title = meta["title"]
    rel = "../../"
    header = f"""<header class="site-header"><a class="site-brand" href="{rel}index.html"><img class="site-brand-mark" src="{rel}svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a><nav class="site-nav" aria-label="Primary"><ul class="nav-band"><li><a class="nav-chip" href="{rel}index.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-home"></use></svg><span>Welcome</span></a></li><li><a class="nav-chip" href="{rel}doc.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Documentation</span></a></li><li><a class="nav-chip" href="{rel}social-preview.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-home"></use></svg><span>Visual Preview</span></a></li><li><a class="nav-chip" href="{rel}download.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Download</span></a></li><li><a class="nav-chip" href="{rel}source.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Source</span></a></li><li><a class="nav-chip" href="{rel}community.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Community</span></a></li><li><a class="nav-chip" href="{rel}news.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-news"></use></svg><span>News</span></a></li><li><a class="nav-chip" href="{rel}diagnostics.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Diagnostics</span></a></li><li><a class="nav-chip" href="{rel}suggestions.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Suggestions</span></a></li></ul></nav><form class="doc-search" role="search" action="{rel}search.html" method="get"><input class="doc-search-input" type="search" name="q" placeholder="Search docs, book, grammar" aria-label="Search documentation" autocomplete="off"><div class="doc-search-controls" aria-label="Search filters"><select class="doc-search-filter doc-search-section" name="section" aria-label="Filter by section"><option value="all">All sections</option><option value="docs">Docs</option><option value="book">Book</option><option value="grammar">Grammar</option></select><select class="doc-search-filter doc-search-lang" name="lang" aria-label="Filter by language"><option value="all">All languages</option><option value="en">EN</option><option value="fr">FR</option></select></div><div class="doc-search-results" hidden></div><div class="doc-search-footer" hidden></div></form></header>"""
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)}</title>
<meta name="description" content="{html.escape(title)} in the Vitte documentation book.">
<link rel="icon" href="{rel}svg/logo.svg" type="image/svg+xml">
<link rel="stylesheet" href="{rel}css/site.css">
<link rel="stylesheet" href="{rel}css/print.css" media="print">
<link rel="canonical" href="https://vitte-lang.org/{html.escape(meta['path'])}">
<link rel="alternate" hreflang="en" href="https://vitte-lang.org/{html.escape(Path(meta['path']).name)}">
<link rel="alternate" hreflang="x-default" href="https://vitte-lang.org/{html.escape(Path(meta['path']).name)}">
<meta property="og:title" content="{html.escape(title)}">
<meta property="og:description" content="{html.escape(title)} in the Vitte documentation book.">
<meta property="og:type" content="website">
<meta property="og:url" content="https://vitte-lang.org/{html.escape(meta['path'])}">
<meta property="og:image" content="https://vitte-lang.org/svg/readme-social-card.svg">
<meta property="og:image:type" content="image/svg+xml">
<meta property="og:image:width" content="1200">
<meta property="og:image:height" content="630">
<meta name="twitter:card" content="summary">
<meta name="twitter:title" content="{html.escape(title)}">
<meta name="twitter:description" content="{html.escape(title)} in the Vitte documentation book.">
<meta name="twitter:image" content="https://vitte-lang.org/svg/readme-social-card.svg">
<meta http-equiv="Content-Security-Policy" content="default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none';">
</head>
<body class="classic-doc book-chapter-doc">
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
    header = f"""<header class="site-header"><a class="site-brand" href="{rel}index.html"><img class="site-brand-mark" src="{rel}svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a><nav class="site-nav" aria-label="Primary"><ul class="nav-band"><li><a class="nav-chip" href="{rel}index.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-home"></use></svg><span>Welcome</span></a></li><li><a class="nav-chip" href="{rel}doc.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Documentation</span></a></li><li><a class="nav-chip" href="{rel}social-preview.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-home"></use></svg><span>Visual Preview</span></a></li><li><a class="nav-chip" href="{rel}download.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Download</span></a></li><li><a class="nav-chip" href="{rel}source.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Source</span></a></li><li><a class="nav-chip" href="{rel}community.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Community</span></a></li><li><a class="nav-chip" href="{rel}news.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-news"></use></svg><span>News</span></a></li><li><a class="nav-chip" href="{rel}diagnostics.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Diagnostics</span></a></li><li><a class="nav-chip" href="{rel}suggestions.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel}svg/sprite.svg#i-docs"></use></svg><span>Suggestions</span></a></li></ul></nav><form class="doc-search" role="search" action="{rel}search.html" method="get"><input class="doc-search-input" type="search" name="q" placeholder="Search docs, book, grammar" aria-label="Search documentation" autocomplete="off"><div class="doc-search-controls" aria-label="Search filters"><select class="doc-search-filter doc-search-section" name="section" aria-label="Filter by section"><option value="all">All sections</option><option value="docs">Docs</option><option value="book">Book</option><option value="grammar">Grammar</option></select><select class="doc-search-filter doc-search-lang" name="lang" aria-label="Filter by language"><option value="all">All languages</option><option value="en">EN</option><option value="fr">FR</option></select></div><div class="doc-search-results" hidden></div><div class="doc-search-footer" hidden></div></form></header>"""
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
        vitte_code_block(profile["example"]),
        render_complete_examples(profile_name, profile),
        render_immediate_chapter_work(profile_name),
        render_chapter_override(meta, profile_name, profile),
        "<h2>Global explanation</h2>",
        f"<p>{html.escape(profile['global'])}</p>",
        "<h2>Invalid case</h2>",
        vitte_code_block(profile["invalid"]),
        "<p>This invalid case stays small so the broken contract remains visible.</p>",
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
        "<section class=\"chapter-cta\"><h2>Next best action</h2><p>Keep the example small, reproduce it locally, then continue to the full chapter if you need the broader context.</p></section>",
        render_chapter_expansion(title, profile_name, profile),
    ])
    body = add_heading_ids_and_toc("\n".join(article))
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
