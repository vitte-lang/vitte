#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import html
import json
import re


ROOT = Path(__file__).resolve().parents[1]
KEYWORDS_DIR = ROOT / "docs" / "book" / "chapters" / "keywords"
KEYWORD_PORTRAITS_DIR = KEYWORDS_DIR / "portraits"
KEYWORD_JSON_DIR = KEYWORDS_DIR / "json"
KEYWORD_DATA_PATH = KEYWORDS_DIR / "keywords-data.json"

SKIP = {
    "index.html",
    "couverture.html",
    "parcours.html",
    "packs-apprentissage.html",
    "non-utilises.html",
    "erreurs-compilateur.html",
    "commented-examples.html",
    "keyword.html",
}

ARTICLE_RE = re.compile(r"<article class=\"doc-content\">\s*(.*?)\s*</article>", re.S)

def load_keyword_data() -> dict[str, object]:
    return json.loads(KEYWORD_DATA_PATH.read_text(encoding="utf-8"))


KEYWORD_DATA = load_keyword_data()
DEEP_KEYWORD_PROFILES: dict[str, dict[str, object]] = KEYWORD_DATA["deep_profiles"]  # type: ignore[assignment]


def code_block(language: str, text: str) -> str:
    return f'<pre><code class="language-{language}">{html.escape(text)}</code></pre>'


def keyword_pages() -> list[str]:
    return sorted(page.stem for page in KEYWORDS_DIR.glob("*.html") if page.name not in SKIP)


def portrait_snippet_for(keyword: str, kind: str) -> str:
    snippets: dict[str, str] = KEYWORD_DATA["portrait_snippets"]  # type: ignore[assignment]
    if keyword in snippets:
        return snippets[keyword]
    sample, _, _, _ = examples_for(keyword, kind)
    return "\n".join(sample.splitlines()[:3]).strip()


def svg_portrait_for(keyword: str, kind: str) -> str:
    snippet = portrait_snippet_for(keyword, kind)
    lines = snippet.splitlines()[:6]
    width = 640
    height = 640
    pad = 36
    line_y = 228
    line_gap = 42
    palette = {
        "type": ("#0b2239", "#67d4ff"),
        "literal": ("#18263a", "#ffd166"),
        "control": ("#1d2433", "#ff7a59"),
        "declaration": ("#162b25", "#74d3ae"),
        "binding": ("#2a2137", "#f4a8ff"),
        "module": ("#1d2631", "#8ad1ff"),
        "operator": ("#31231f", "#ffb86b"),
        "advanced": ("#2b1b22", "#ff8fab"),
        "generic": ("#202531", "#b4c7ff"),
    }
    bg, accent = palette[kind]
    text = "#eef4ff"
    muted = "#b9c4d8"
    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img" aria-labelledby="title desc">',
        f'<title id="title">Keyword portrait for {html.escape(keyword)}</title>',
        f'<desc id="desc">Small code excerpt highlighting the keyword {html.escape(keyword)}.</desc>',
        f'<rect width="{width}" height="{height}" rx="38" fill="{bg}"/>',
        f'<rect x="18" y="18" width="{width-36}" height="{height-36}" rx="30" fill="none" stroke="{accent}" stroke-opacity="0.35" stroke-width="2"/>',
        f'<circle cx="{width-88}" cy="82" r="42" fill="{accent}" fill-opacity="0.18"/>',
        f'<circle cx="{width-88}" cy="82" r="24" fill="{accent}" fill-opacity="0.9"/>',
        f'<text x="{pad}" y="84" font-family="ui-monospace, SFMono-Regular, Menlo, Consolas, monospace" font-size="18" fill="{muted}">vitte keyword</text>',
        f'<text x="{pad}" y="132" font-family="Georgia, Times New Roman, serif" font-size="44" font-weight="700" fill="{text}">{html.escape(keyword)}</text>',
        f'<rect x="{pad}" y="154" width="168" height="36" rx="18" fill="{accent}" fill-opacity="0.18"/>',
        f'<text x="{pad + 18}" y="178" font-family="ui-sans-serif, system-ui, sans-serif" font-size="16" fill="{accent}">{html.escape(family_label(kind))}</text>',
        f'<rect x="{pad}" y="210" width="{width - (pad*2)}" height="310" rx="24" fill="#0f1726" fill-opacity="0.72" stroke="{accent}" stroke-opacity="0.24"/>',
    ]
    for idx, line in enumerate(lines):
        y = line_y + idx * line_gap
        svg.append(f'<text x="{pad + 24}" y="{y}" font-family="ui-monospace, SFMono-Regular, Menlo, Consolas, monospace" font-size="24" fill="{text}">{html.escape(line)}</text>')
    svg.extend([
        f'<text x="{pad}" y="{height - 64}" font-family="ui-sans-serif, system-ui, sans-serif" font-size="16" fill="{muted}">Reference micro-snippet for {html.escape(keyword)}</text>',
        "</svg>",
    ])
    return "\n".join(svg)


def keyword_kind(keyword: str) -> str:
    if keyword in {"int", "bool", "string"}:
        return "type"
    if keyword in {"true", "false"}:
        return "literal"
    if keyword in {"if", "else", "loop", "for", "break", "continue", "match", "case", "otherwise", "select", "when", "return", "give"}:
        return "control"
    if keyword in {"proc", "form", "pick", "type", "trait", "macro", "const", "field", "entry", "space"}:
        return "declaration"
    if keyword in {"let", "make", "set"}:
        return "binding"
    if keyword in {"use", "pull", "share", "as", "all"}:
        return "module"
    if keyword in {"and", "or", "not", "is", "in"}:
        return "operator"
    if keyword in {"unsafe", "asm"}:
        return "advanced"
    return "generic"


def level_for(kind: str) -> str:
    return {
        "type": "Beginner",
        "literal": "Beginner",
        "control": "Beginner",
        "binding": "Beginner",
        "declaration": "Intermediate",
        "module": "Intermediate",
        "operator": "Intermediate",
        "advanced": "Advanced",
        "generic": "Intermediate",
    }[kind]


def nearby_for(keyword: str, kind: str) -> str:
    mapping = {
        "if": "match",
        "else": "otherwise",
        "loop": "for",
        "for": "loop",
        "break": "continue",
        "continue": "break",
        "match": "if",
        "case": "pick",
        "otherwise": "else",
        "select": "match",
        "when": "if",
        "return": "give",
        "give": "return",
        "proc": "entry",
        "form": "type",
        "pick": "form",
        "type": "form",
        "trait": "type",
        "macro": "proc",
        "const": "let",
        "field": "form",
        "entry": "proc",
        "space": "pull",
        "let": "set",
        "make": "let",
        "set": "let",
        "use": "pull",
        "pull": "use",
        "share": "pull",
        "as": "use",
        "and": "or",
        "or": "and",
        "not": "and",
        "is": "match",
        "in": "for",
        "unsafe": "asm",
        "asm": "unsafe",
        "int": "string",
        "bool": "int",
        "string": "int",
        "true": "false",
        "false": "true",
        "at": "entry",
        "all": "share",
    }
    return mapping.get(keyword, "if" if kind == "control" else "proc")


def examples_for(keyword: str, kind: str) -> tuple[str, str, str, str]:
    if keyword == "int":
        return (
            "form Counter {\n  value: int\n}\n\nproc bump(c: Counter) -> int {\n  give c.value + 1\n}",
            "struct Counter {\n  int value;\n};\n\nint bump(struct Counter c) {\n  return c.value + 1;\n}",
            "proc bad_int() -> int {\n  let label: int = \"forty-two\"\n  give label\n}",
            "The integer contract is broken because the declared numeric value receives text."
        )
    if keyword == "bool":
        return (
            "proc can_run(enabled: bool) -> int {\n  if enabled { give 0 }\n  give 1\n}",
            "int can_run(_Bool enabled) {\n  if (enabled) return 0;\n  return 1;\n}",
            "proc bad_bool() -> int {\n  let enabled: bool = 1\n  if enabled { give 0 }\n  give 1\n}",
            "The boolean contract is broken because the declaration does not use a boolean value."
        )
    if keyword == "string":
        return (
            "proc greet(name: string) -> string {\n  give \"hello, \" + name\n}",
            "const char* greet(const char* name) {\n  return name;\n}",
            "proc bad_string() -> string {\n  let name: string = 12\n  give name\n}",
            "The textual contract is broken because the declaration receives a numeric value."
        )
    if keyword in {"true", "false"}:
        lit = keyword
        other = "false" if lit == "true" else "true"
        return (
            f"proc flag() -> bool {{\n  give {lit}\n}}",
            f"int flag(void) {{\n  return {1 if lit == 'true' else 0};\n}}",
            f"proc bad_flag() -> bool {{\n  give {other if lit == 'true' else '1'}\n}}",
            "The literal is used to express a boolean result; the invalid variant mixes the wrong surface."
        )
    if keyword in {"if", "else", "when"}:
        branch = "if x < 0 { give 0 }\n  else { give x }" if keyword == "else" else "if x < 0 { give 0 }\n  give x"
        return (
            f"proc clamp_low(x: int) -> int {{\n  {branch}\n}}",
            "int clamp_low(int x) {\n  if (x < 0) return 0;\n  return x;\n}",
            "proc bad_branch(x: int) -> int {\n  if x { give 0 }\n  give x\n}",
            "The branch contract is broken because the condition surface is not valid."
        )
    if keyword in {"loop", "for", "break", "continue"}:
        vit = {
            "loop": "proc sum_to(n: int) -> int {\n  let i: int = 0\n  let acc: int = 0\n  loop {\n    if i >= n { break }\n    set acc = acc + i\n    set i = i + 1\n  }\n  give acc\n}",
            "for": "proc sum_all(values: list[int]) -> int {\n  let acc: int = 0\n  for value in values {\n    set acc = acc + value\n  }\n  give acc\n}",
            "break": "proc first_positive(values: list[int]) -> int {\n  let out: int = 0\n  for value in values {\n    if value > 0 {\n      set out = value\n      break\n    }\n  }\n  give out\n}",
            "continue": "proc count_non_zero(values: list[int]) -> int {\n  let acc: int = 0\n  for value in values {\n    if value == 0 { continue }\n    set acc = acc + 1\n  }\n  give acc\n}",
        }[keyword]
        return (
            vit,
            "/* C comparison: use an explicit loop and keep the branch purpose visible. */",
            f"proc bad_{keyword}() -> int {{\n  {keyword}\n  give 0\n}}",
            "The control-flow surface is broken because the keyword is not used in a valid loop context."
        )
    if keyword in {"match", "case", "otherwise", "select"}:
        return (
            "pick Resp {\n  case Ok(value: int)\n  case Err(code: int)\n}\n\nproc to_code(r: Resp) -> int {\n  match r {\n    case Ok(value) { give value }\n    case Err(code) { give code }\n    otherwise { give 70 }\n  }\n}",
            "/* C comparison: this is usually expressed with enum tags plus switch/case. */",
            "proc bad_match(r: int) -> int {\n  match r\n    case 0 { give 0 }\n  }\n}",
            "The pattern surface is broken because the branch form is not structurally complete."
        )
    if keyword in {"return", "give"}:
        verb = keyword
        return (
            f"proc done(x: int) -> int {{\n  {verb} x + 1\n}}",
            "int done(int x) {\n  return x + 1;\n}",
            f"proc bad_{keyword}(x: int) -> int {{\n  if x < 0 {{ {verb} }}\n  {verb} x\n}}",
            "The exit surface is broken because the branch does not produce the expected value form."
        )
    if kind == "declaration":
        examples = {
            "proc": (
                "proc add(a: int, b: int) -> int {\n  give a + b\n}",
                "int add(int a, int b) {\n  return a + b;\n}",
                "proc bad(a: int, b: int) {\n  give a + b\n}",
                "The declaration omits part of the expected contract."
            ),
            "form": (
                "form User {\n  id: int\n  name: string\n}",
                "struct User {\n  int id;\n  const char* name;\n};",
                "form User {\n  id int\n}",
                "The field surface is malformed."
            ),
            "pick": (
                "pick Result {\n  case Ok(value: int)\n  case Err(code: int)\n}",
                "enum ResultTag { OK, ERR };",
                "pick Result {\n  Ok(value: int)\n}",
                "The variant declaration is missing the expected case surface."
            ),
            "entry": (
                "entry main at core/app {\n  return 0\n}",
                "int main(void) {\n  return 0;\n}",
                "entry main {\n  return 0\n}",
                "The entry declaration does not expose the full entry shape."
            ),
            "space": (
                "space demo/app\n\nproc run() -> int {\n  give 0\n}",
                "/* C comparison: namespaces are usually simulated with prefixes or files. */",
                "proc run() -> int {\n  space demo/app\n  give 0\n}",
                "The namespace declaration is top-level and cannot be moved into a block."
            ),
        }
        return examples.get(keyword, (
            f"{keyword} demo_contract {{\n  # adapt this declaration to the concrete construct\n}}",
            "/* C comparison: declaration shape is usually expressed with a different surface. */",
            f"{keyword}\n",
            "The declaration surface is incomplete or misplaced."
        ))
    if kind == "binding":
        return (
            {
                "let": "proc read_counter() -> int {\n  let value: int = 1\n  give value\n}",
                "make": "proc build_flag() -> int {\n  make value: int = 1\n  give value\n}",
                "set": "proc bump() -> int {\n  let value: int = 1\n  set value = value + 1\n  give value\n}",
            }[keyword],
            "/* C comparison: declarations and assignments are usually split across local variables and direct assignment. */",
            f"proc bad_{keyword}() -> int {{\n  {keyword}\n  give 0\n}}",
            "The binding surface is incomplete or appears in the wrong form."
        )
    if kind == "module":
        return (
            {
                "use": "space app/core\nuse std.io.{read, write} as io\n\nproc ping() -> int {\n  give 0\n}",
                "pull": "space app/core\npull std/path as path\n\nproc ping() -> int {\n  give 0\n}",
                "share": "space app/core\nshare proc ping() -> int {\n  give 0\n}",
                "all": "space demo/api\n\nproc open() -> int {\n  give 1\n}\n\nproc close() -> int {\n  give 0\n}\n\nshare all",
                "as": "space app/core\nuse std.io.{read} as io\n\nproc ping() -> int {\n  give 0\n}",
            }[keyword],
            "/* C comparison: imports are usually done with includes and naming conventions instead of module aliases. */",
            f"proc bad_{keyword}() -> int {{\n  {keyword}\n  give 0\n}}",
            "The module-level surface is misplaced or incomplete."
        )
    if kind == "operator":
        ops = {
            "and": "proc both(a: bool, b: bool) -> bool {\n  give a and b\n}",
            "or": "proc either(a: bool, b: bool) -> bool {\n  give a or b\n}",
            "not": "proc invert(a: bool) -> bool {\n  give not a\n}",
            "is": "proc code_ok(r: Result) -> bool {\n  give r is Result.Ok\n}",
            "in": "proc sum(values: list[int]) -> int {\n  let acc: int = 0\n  for value in values {\n    set acc = acc + value\n  }\n  give acc\n}",
        }
        return (
            ops[keyword],
            "/* C comparison: this role is usually expressed with operators or explicit loops and conditionals. */",
            f"proc bad_{keyword}() -> int {{\n  give {keyword}\n}}",
            "The operator surface is incomplete because it is missing the operands or the valid surrounding construct."
        )
    if kind == "advanced":
        return (
            {
                "unsafe": "unsafe proc raw_copy(dst: ptr[int], src: ptr[int]) -> int {\n  give 0\n}",
                "asm": "proc tick() -> int {\n  asm {\n    \"nop\"\n  }\n  give 0\n}",
            }[keyword],
            "/* C comparison: this role usually appears through inline assembly or unsafe pointer manipulation. */",
            f"proc bad_{keyword}() -> int {{\n  {keyword}\n  give 0\n}}",
            "The advanced surface is malformed because it lacks the enclosing contract required by the language."
        )
    if keyword == "at":
        return (
            "entry main at core/app {\n  return 0\n}",
            "/* C comparison: entry attachment is usually implied by main and linker layout. */",
            "entry main core/app {\n  return 0\n}",
            "The attachment surface is incomplete because the entry form is missing its connector."
        )
    if keyword == "all":
        return (
            "space demo/api\n\nproc open() -> int {\n  give 1\n}\n\nproc close() -> int {\n  give 0\n}\n\nshare all",
            "/* C comparison: a whole public surface is usually exposed by header design and naming conventions rather than by a dedicated export selector. */",
            "space demo/api\n\nproc open() -> int {\n  give 1\n}\n\nshare",
            "The export selector is incomplete: `share` expects an explicit target such as `all` or a symbol list."
        )
    return (
        f"proc sample_{keyword}(x: int) -> int {{\n  give x\n}}",
        "/* C comparison: the same intent is usually expressed with a simpler token surface. */",
        f"proc bad_{keyword}() -> int {{\n  {keyword}\n  give 0\n}}",
        "The keyword is used outside its valid grammatical or contractual context."
    )


def syntax_for(keyword: str, kind: str) -> str:
    mapping = {
        "if": "Canonical form: `if condition { ... }`.",
        "else": "Canonical form: `if condition { ... } else { ... }`.",
        "loop": "Canonical form: `loop { ... }`.",
        "for": "Canonical form: `for value in values { ... }`.",
        "match": "Canonical form: `match value { case ... }`.",
        "case": "Canonical form: `case Pattern { ... }` inside `pick`, `match`, or a related branching surface.",
        "otherwise": "Canonical form: `otherwise { ... }` as an explicit fallback branch.",
        "select": "Canonical form: `select { when ... }`.",
        "when": "Canonical form: `when condition { ... }` inside a selection surface.",
        "return": "Canonical form: `return expression`.",
        "give": "Canonical form: `give expression`.",
        "proc": "Canonical form: `proc name(args) -> type { ... }`.",
        "form": "Canonical form: `form Name { field: type }`.",
        "pick": "Canonical form: `pick Name { case ... }`.",
        "type": "Canonical form: `type Name = ...`.",
        "trait": "Canonical form: `trait Name { ... }`.",
        "macro": "Canonical form: `macro name(...) { ... }`.",
        "const": "Canonical form: `const NAME: type = value`.",
        "entry": "Canonical form: `entry main at module/path { ... }`.",
        "space": "Canonical form: `space module/path`.",
        "let": "Canonical form: `let name: type = value`.",
        "make": "Canonical form: `make name: type = value`.",
        "set": "Canonical form: `set name = value`.",
        "use": "Canonical form: `use path.{symbols} as alias`.",
        "pull": "Canonical form: `pull module/path as alias`.",
        "share": "Canonical form: `share declaration`.",
        "all": "Canonical form: `share all`.",
        "as": "Canonical form: `... as alias`.",
        "and": "Canonical form: `left and right`.",
        "or": "Canonical form: `left or right`.",
        "not": "Canonical form: `not value`.",
        "is": "Canonical form: `value is Pattern`.",
        "in": "Canonical form: `for value in values` or a membership form depending on context.",
        "unsafe": "Canonical form: `unsafe ...` around an explicit unsafe boundary.",
        "asm": "Canonical form: `asm { ... }`.",
        "at": "Canonical form: `entry name at module/path { ... }`.",
        "int": "Canonical form: `let x: int = 0` or `proc f(x: int) -> int`.",
        "bool": "Canonical form: `let ok: bool = true`.",
        "string": "Canonical form: `let name: string = \"demo\"`.",
        "true": "Canonical form: `give true`.",
        "false": "Canonical form: `give false`.",
    }
    return mapping.get(keyword, f"Canonical form: use `{keyword}` only in its valid grammatical role.")


def chapters_for(kind: str, keyword: str) -> list[str]:
    base = {
        "control": [
            "docs/book/chapters/07-control.html",
            "docs/book/chapters/27-grammar.html",
            "docs/book/chapters/31-build-errors.html",
        ],
        "type": [
            "docs/book/chapters/05-types.html",
            "docs/book/chapters/17-stdlib.html",
            "docs/book/chapters/27-grammar.html",
        ],
        "literal": [
            "docs/book/chapters/05-types.html",
            "docs/book/chapters/07-control.html",
            "docs/book/chapters/27-grammar.html",
        ],
        "declaration": [
            "docs/book/chapters/06-procedures.html",
            "docs/book/chapters/09-modules.html",
            "docs/book/chapters/27-grammar.html",
        ],
        "binding": [
            "docs/book/chapters/05-types.html",
            "docs/book/chapters/06-procedures.html",
            "docs/book/chapters/27-grammar.html",
        ],
        "module": [
            "docs/book/chapters/09-modules.html",
            "docs/book/chapters/17-stdlib.html",
            "docs/book/chapters/27-grammar.html",
        ],
        "operator": [
            "docs/book/chapters/05-types.html",
            "docs/book/chapters/07-control.html",
            "docs/book/chapters/27-grammar.html",
        ],
        "advanced": [
            "docs/book/chapters/16-interop.html",
            "docs/book/chapters/27-grammar.html",
            "docs/book/chapters/65-abi-and-vitte-interop-contracts.html",
        ],
        "generic": [
            "docs/book/chapters/27-grammar.html",
            "docs/book/chapters/31-build-errors.html",
            "docs/book/chapters/34-compiler-mental-model.html",
        ],
    }
    if keyword == "at":
        return [
            "docs/book/chapters/21-cli-project.html",
            "docs/book/chapters/27-grammar.html",
            "docs/book/chapters/31-build-errors.html",
        ]
    return base[kind]


def family_label(kind: str) -> str:
    return {
        "type": "Built-in type",
        "literal": "Literal",
        "control": "Control flow",
        "declaration": "Declaration",
        "binding": "Binding and mutation",
        "module": "Module organization",
        "operator": "Lexical operator",
        "advanced": "Advanced surface",
        "generic": "General keyword",
    }[kind]


def role_overview(keyword: str, kind: str) -> str:
    mapping = {
        "type": f"The keyword <code>{keyword}</code> makes a value contract explicit in a signature, local declaration, or data structure. Its main role is not decorative: it bounds what the compiler and the reader may expect from an expression.",
        "literal": f"The keyword <code>{keyword}</code> expresses a boolean truth directly. It matters because it avoids implicit conventions and keeps the decision visible in the source code.",
        "control": f"The keyword <code>{keyword}</code> structures execution. It determines when a block opens, when a branch is chosen, when a loop continues or stops, or when an exit becomes observable.",
        "declaration": f"The keyword <code>{keyword}</code> defines a program shape: procedure, type, variant, entry point, namespace, or another structural boundary. It should therefore be read architecturally before it is read locally.",
        "binding": f"The keyword <code>{keyword}</code> acts on the visible state of a block. It introduces, initializes, or modifies a value, which makes it a critical point for invariant readability.",
        "module": f"The keyword <code>{keyword}</code> organizes symbol circulation across modules. It clarifies what is imported, shared, renamed, or attached to a visibility boundary.",
        "operator": f"The keyword <code>{keyword}</code> participates in a relation between values or in the logical reading of an expression. It should not be read in isolation, but as an operator that changes the meaning of the whole expression.",
        "advanced": f"The keyword <code>{keyword}</code> marks a zone where the language deliberately exposes a riskier or more machine-facing surface. Its value is precision, not convenience.",
        "generic": f"The keyword <code>{keyword}</code> belongs to Vitte's structural vocabulary. Its correct reading depends on the overall contract of the block in which it appears.",
    }
    return mapping[kind]


def semantic_details(keyword: str, kind: str) -> str:
    mapping = {
        "type": f"Semantically, <code>{keyword}</code> is not just a type name: it constrains operation compatibility, assignment validity, and return shapes. As soon as it appears, the reader should check which operations become legal and which conversions become forbidden.",
        "literal": f"Semantically, <code>{keyword}</code> carries an immediate and unambiguous meaning. It introduces no extra computation; it directly fixes the logical state expected by the branch, guard, or return value.",
        "control": f"Semantically, <code>{keyword}</code> reorganizes the execution path. The right question is not only 'where is it placed?' but 'which path becomes possible, impossible, or preferred from this point onward?'",
        "declaration": f"Semantically, <code>{keyword}</code> changes the shape of the program before execution even begins. It introduces an entity that other blocks will name, call, instantiate, or reference.",
        "binding": f"Semantically, <code>{keyword}</code> evolves the state of a local name. It should therefore be read together with scope, lifetime, and the distinction between introducing a value and updating an existing one.",
        "module": f"Semantically, <code>{keyword}</code> modifies symbol visibility. It influences how a reader reconstructs the origin of names and how the compiler resolves references.",
        "operator": f"Semantically, <code>{keyword}</code> only makes sense with its operands and the surrounding form. The full expression must be read to understand the logical or structural contract it imposes.",
        "advanced": f"Semantically, <code>{keyword}</code> opens a strongly explicit zone. It signals that a safety, low-level, or interoperability assumption is no longer implicit and must be owned openly.",
        "generic": f"Semantically, <code>{keyword}</code> changes how the whole construction is read. Its real meaning appears only in relation to the surrounding grammar and the effect produced on the program.",
    }
    return mapping[kind]


def usage_points(keyword: str, kind: str) -> list[str]:
    common = [
        f"When <code>{keyword}</code> makes the block contract more explicit at first reading.",
        "When it reduces the number of implicit assumptions the reader must reconstruct mentally.",
    ]
    specific = {
        "type": ["When a signature must state the nature of the manipulated data clearly."],
        "literal": ["When a branch, guard, or return must carry a boolean truth without hidden convention."],
        "control": ["When a branch choice, repetition, or flow exit must be made visible."],
        "declaration": ["When the program must introduce a stable entity that will be reused elsewhere."],
        "binding": ["When local state must be introduced or modified without ambiguity about the name's scope."],
        "module": ["When the origin or outward exposure of a symbol must stay visible at module level."],
        "operator": ["When a logical or structural relation must be read as an integral part of the expression."],
        "advanced": ["When a low-level boundary must be marked explicitly instead of being hidden."],
        "generic": ["When its presence genuinely clarifies the grammar and intent of the block."],
    }[kind]
    return common + specific


def avoid_points(keyword: str, kind: str) -> list[str]:
    return [
        f"Avoid <code>{keyword}</code> when another, more precise keyword already carries the block's intent.",
        f"Avoid <code>{keyword}</code> when it adds only surface noise without clarifying the contract.",
        "Avoid reading or teaching it as an isolated token with no relation to the full structure.",
    ]


def grammar_role_for(keyword: str, kind: str) -> str:
    specific = {
        "if": "Introduces a conditional branch inside a procedure body, loop, or another control construction.",
        "else": "Completes an already opened conditional branch and provides the explicit fallback path.",
        "loop": "Opens an unbounded repetition loop whose exit depends on an internal condition or an explicit break.",
        "for": "Introduces a structured iteration loop in which a local variable successively receives the elements of a source.",
        "break": "Immediately interrupts the current loop and transfers execution after that loop.",
        "continue": "Interrupts the current iteration and jumps directly to the next iteration of the active loop.",
        "match": "Opens a pattern or variant analysis, distributing execution across multiple branch forms.",
        "case": "Declares either a variant in a definition or a specialized branch in a pattern analysis.",
        "otherwise": "Declares the explicit fallback path when specialized branches do not apply.",
        "select": "Opens a multi-branch selection oriented around conditions or explicitly listed cases.",
        "when": "Introduces a specialized condition inside a selection construction.",
        "return": "Produces the output of the procedure or entry point within a full return convention.",
        "give": "Produces the value of the block or procedure according to Vitte's preferred exit convention.",
        "proc": "Introduces a named procedure with parameters, return type, and executable body.",
        "form": "Introduces a structured form that serves as a data contract.",
        "pick": "Introduces a sum type or a closed family of identified variants.",
        "type": "Introduces or rewrites a type name in declaration grammar.",
        "trait": "Introduces a behavior or interface contract intended to be satisfied by other forms.",
        "macro": "Introduces a metaprogrammed construction that generates or rewrites syntactic structure.",
        "const": "Introduces a named constant whose value is fixed by declaration.",
        "field": "Introduces a field in a declarative structure.",
        "entry": "Introduces the executable entry point of a program or artifact.",
        "space": "Introduces the namespace or logical path of the current module.",
        "let": "Introduces an initialized local binding.",
        "make": "Introduces an explicit creation of a value or binding according to the local language convention.",
        "set": "Updates an already existing binding.",
        "use": "Imports symbols into the current module.",
        "pull": "Attaches a module or full surface for local use.",
        "share": "Deliberately exposes part of the current module to the outside.",
        "all": "Extends sharing to the entire eligible surface of the current module.",
        "as": "Renames an import, symbol, or attachment locally.",
        "and": "Combines two logical expressions by requiring joint validity.",
        "or": "Combines two logical expressions by accepting that at least one is valid.",
        "not": "Inverts the logical polarity of a boolean expression.",
        "is": "Tests a shape, pattern, or variant membership.",
        "in": "Connects an iteration variable to its source or expresses membership depending on grammatical context.",
        "unsafe": "Marks a zone where the language's usual safety must be assumed explicitly by the author.",
        "asm": "Introduces an assembly block or machine escape hatch.",
        "at": "Attaches an entry or declaration to an explicit logical or modular location.",
        "int": "Specifies the language's native integer type.",
        "bool": "Specifies the language's boolean type.",
        "string": "Specifies the language's text type.",
        "true": "Provides the boolean true value.",
        "false": "Provides the boolean false value.",
    }
    if keyword in specific:
        return specific[keyword]
    return {
        "type": "Participates in the grammar of types and value contracts.",
        "literal": "Participates in the grammar of literal values.",
        "control": "Participates in the grammar of control flow.",
        "declaration": "Participates in the grammar of structuring declarations.",
        "binding": "Participates in the grammar of local bindings and assignments.",
        "module": "Participates in the grammar of modules, imports, and exports.",
        "operator": "Participates in the grammar of operator expressions.",
        "advanced": "Participates in the grammar of advanced or unsafe surfaces.",
        "generic": "Participates in the general grammar of the language.",
    }[kind]


def execution_effect_for(keyword: str, kind: str) -> str:
    return {
        "type": f"<code>{keyword}</code> does not directly modify control flow; it restricts what the program may legally produce, consume, or transform.",
        "literal": f"<code>{keyword}</code> injects a truth value directly into execution and simplifies the reading of guards and returns.",
        "control": f"<code>{keyword}</code> immediately changes the execution path, whether by choosing a branch, repeating, interrupting, or terminating a flow.",
        "declaration": f"<code>{keyword}</code> acts first on the shape of the program. Its main effect appears in the entities it makes callable, instantiable, or visible during execution.",
        "binding": f"<code>{keyword}</code> acts on the program's local state by introducing or modifying a named value that subsequent instructions will read.",
        "module": f"<code>{keyword}</code> acts mainly on name resolution and cross-module visibility. Its execution-time effect is indirect but decisive for the structure of the compiled code.",
        "operator": f"<code>{keyword}</code> acts at expression level by changing the logical or structural result computed at that exact point.",
        "advanced": f"<code>{keyword}</code> opens a zone where the real effect may touch memory, the ABI, or machine instructions. It should be read as a boundary of strong responsibility.",
        "generic": f"<code>{keyword}</code> changes the interpretation of the construction where it appears and should therefore be read with the block's global effect in mind.",
    }[kind]


def canonical_variants_for(keyword: str, kind: str) -> list[str]:
    variants = {
        "if": ["`if condition { ... }`", "`if condition { ... } else { ... }`"],
        "match": ["`match value { case ... }`", "`match value { case ... otherwise { ... } }`"],
        "for": ["`for value in values { ... }`"],
        "loop": ["`loop { ... }` with `break` for exit"],
        "proc": ["`proc name(args) -> type { ... }`", "`share proc name(args) -> type { ... }`"],
        "use": ["`use path.{symbols}`", "`use path.{symbols} as alias`"],
        "pull": ["`pull module/path`", "`pull module/path as alias`"],
        "share": ["`share declaration`", "`share all`"],
        "let": ["`let name: type = value`", "`let name = value` when local inference is allowed"],
        "set": ["`set name = value`"],
        "type": ["`type Name = ...`"],
        "pick": ["`pick Name { case ... }`"],
        "entry": ["`entry main at module/path { ... }`"],
    }
    if keyword in variants:
        return variants[keyword]
    return [syntax_for(keyword, kind).replace("Canonical form: ", "")]


def interaction_rows_for(keyword: str, kind: str, nearby: str) -> list[tuple[str, str]]:
    rows = [
        (nearby, f"Direct neighboring keyword: it helps explain what <code>{keyword}</code> does, either by contrast or by complement."),
    ]
    extras = {
        "if": [("else", "Completes the branch opened by <code>if</code> and avoids an implicit fallback path."), ("when", "Provides a specialized condition form in another selection setting.")],
        "match": [("case", "Provides the specialized branches analyzed by <code>match</code>."), ("otherwise", "Provides the fallback path when the previous cases do not match.")],
        "for": [("in", "Part of the canonical reading of the iteration loop."), ("break", "Lets the iteration stop early when the exit condition is reached.")],
        "loop": [("break", "Almost always necessary to make the exit condition readable."), ("continue", "Lets execution jump directly to the next iteration.")],
        "proc": [("give", "Often used inside <code>proc</code> to produce the output value."), ("entry", "Structural neighbor: one defines a procedure, the other defines the entry point.")],
        "share": [("all", "Extends sharing to the full eligible module surface."), ("use", "Natural counterpart: one exposes, the other imports.")],
        "use": [("as", "Makes an import more readable or less ambiguous."), ("pull", "Module-level neighbor: the import intent is not carried in the same way.")],
        "let": [("set", "The first introduces a binding, the second modifies it."), ("make", "Creation/initialization neighbor whose contract should be distinguished.")],
        "return": [("give", "Both express an exit, but not necessarily at the same language-convention level.")],
        "give": [("return", "Exit neighbor whose scope and convention should be distinguished.")],
    }
    rows.extend(extras.get(keyword, []))
    return rows


def detailed_reading_steps(keyword: str, kind: str) -> list[str]:
    base = [
        f"First locate the full construction that contains <code>{keyword}</code>, not the isolated word.",
        f"Then identify which contract becomes visible because of <code>{keyword}</code>: type, branch, binding, module, exit, or advanced boundary.",
        f"Finish by checking the observable effect produced by the construction that contains <code>{keyword}</code>.",
    ]
    kind_specific = {
        "control": ["For a control keyword, mentally reconstruct the possible paths and the ones that become impossible."],
        "declaration": ["For a declaration keyword, verify which stable entity is created and how it will be referenced later."],
        "binding": ["For a binding keyword, distinguish name introduction, mutation, and value scope."],
        "module": ["For a module keyword, verify the origin and destination of visible symbols."],
        "operator": ["For an operator keyword, reread the full expression to avoid an overly local interpretation."],
        "advanced": ["For an advanced surface, explicitly verify the safety or interoperability boundary it opens."],
    }.get(kind, [])
    return base + kind_specific


def implementation_notes_for(keyword: str, kind: str) -> list[str]:
    notes = {
        "control": [
            "Useful diagnostics for this family usually signal an incomplete branch shape, a mistyped condition, or an illegal placement in control flow.",
            "In a compiler, these keywords directly influence control-flow graph construction or intermediate representation building.",
        ],
        "declaration": [
            "Useful diagnostics for this family often concern incomplete signatures, constituent ordering, or declarative scope.",
            "In a compiler, these keywords primarily feed symbol tables and the structural representation of the program.",
        ],
        "binding": [
            "Common errors concern scope, illegal reassignment, or an incompatible type contract between introduction and use.",
            "In a compiler, these keywords modify the local environment and mutability or assignment information.",
        ],
        "module": [
            "Common errors concern name resolution, alias collisions, or a shared surface that is too broad or incomplete.",
            "In a compiler, these keywords participate in cross-file resolution and visible API construction.",
        ],
        "operator": [
            "The most useful diagnostics signal missing operands, an incompatible value category, or a reading ambiguity.",
            "In the front-end, these keywords influence expression hierarchy and sometimes precedence rules.",
        ],
        "advanced": [
            "Diagnostics must stay precise, because a poorly documented advanced surface quickly creates expensive-to-debug errors.",
            "In a compiler, these keywords often act as guard rails around unsafe transformations or calls.",
        ],
    }
    return notes.get(kind, [
        "A useful diagnostic for this keyword should always connect the observed syntax to the expected contract.",
        "From the compiler's point of view, this keyword should remain identifiable early enough to produce coherent, localized messages.",
    ])


def common_misreadings_for(keyword: str, kind: str) -> list[str]:
    specific = {
        "if": ["Thinking that <code>if</code> documents only a local condition, when it actually restructures the entire exit path of the block."],
        "proc": ["Reading <code>proc</code> as simple function decoration, when it opens a full contract of parameters, return, and visibility."],
        "share": ["Thinking that <code>share</code> is just syntactic sugar, when it modifies the module's public contract."],
        "let": ["Confusing binding introduction with mutation of an existing binding."],
        "match": ["Reading it as a repeated `if`, when the branching logic may be governed by variants or patterns instead."],
    }
    if keyword in specific:
        return specific[keyword]
    return [
        f"Reducing <code>{keyword}</code> to a local token instead of reading it as part of a full construction.",
        "Explaining only the syntax and forgetting the reading or diagnostic contract it imposes.",
    ]


def invalid_reason_for(keyword: str, kind: str, invalid_note: str) -> str:
    return (
        f"The counter-example is not merely wrong: it is wrong in an instructive way. "
        f"It shows which grammar or execution-contract assumption is no longer accepted when <code>{keyword}</code> is moved, truncated, or combined with the wrong context. "
        f"Concretely, {localized_invalid_note(keyword, kind, invalid_note)}"
    )


def localized_invalid_note(keyword: str, kind: str, invalid_note: str) -> str:
    explicit = {
        "int": "the integer contract breaks because a textual value is assigned to something declared as numeric.",
        "bool": "the boolean contract breaks because the declaration does not use an explicit boolean value.",
        "string": "the textual contract breaks because a numeric value is injected into a textual declaration.",
        "if": "the condition does not have the valid form expected by a conditional branch.",
        "else": "the fallback branch does not attach correctly to an already opened condition.",
        "when": "the specialized condition does not appear inside the selection surface that gives it meaning.",
        "loop": "the loop surface is used without the block that gives it execution scope.",
        "for": "the iteration loop loses either its variable, its source, or its block.",
        "break": "the loop break appears outside a valid loop.",
        "continue": "the jump to the next iteration appears outside a valid loop.",
        "match": "the pattern-analysis structure is incomplete or badly closed.",
        "case": "the specialized branch does not appear in a construction that allows it.",
        "otherwise": "the fallback branch appears outside a valid selection or analysis structure.",
        "select": "the multi-branch selection is truncated or structured in an invalid way.",
        "return": "the produced exit does not have the form expected by the return contract.",
        "give": "the produced value does not respect the expected exit contract.",
        "proc": "the procedure declaration omits an essential part of its signature or body contract.",
        "form": "the form declaration is malformed or incomplete.",
        "pick": "the variant declaration is incomplete or malformed.",
        "entry": "the entry-point declaration does not expose the full expected structure.",
        "space": "the namespace declaration appears at the wrong level or in an incomplete form.",
        "let": "the local binding is introduced in an incomplete form.",
        "make": "the explicit value creation does not have the full expected form.",
        "set": "the mutation targets a binding or assignment form that is incomplete.",
        "use": "the import does not provide a full target or resolution structure.",
        "pull": "the module attachment does not provide the expected path or alias form.",
        "share": "the exported surface is not expressed in a complete form.",
        "all": "the export selector is used outside the `share all` construction that gives it meaning.",
        "as": "the renaming is not attached to the construction that supplies the alias.",
        "and": "the logical operator does not have all the operands it needs.",
        "or": "the logical operator does not have all the operands it needs.",
        "not": "the logical inversion does not receive the expression it is supposed to invert.",
        "is": "the shape or pattern test does not receive a complete structure.",
        "in": "the iteration or membership relation appears outside its valid construction.",
        "unsafe": "the unsafe boundary is badly opened or does not enclose the expected surface.",
        "asm": "the machine block does not receive the expected structure.",
        "at": "the logical attachment is missing or badly connected to the declaration it completes.",
        "true": "the literal value used is not coherent with the expected boolean contract.",
        "false": "the literal value used is not coherent with the expected boolean contract.",
    }
    if keyword in explicit:
        return explicit[keyword]
    family = {
        "type": "the announced type contract is no longer respected by the provided value or operation.",
        "literal": "the literal value no longer matches the expected logical surface.",
        "control": "the control construction can no longer be interpreted correctly.",
        "declaration": "the declaration is incomplete or moved into an invalid grammatical layer.",
        "binding": "the binding or mutation does not respect the expected form.",
        "module": "the module-level surface is incomplete or badly positioned.",
        "operator": "the operator is deprived of an essential context or operand.",
        "advanced": "the advanced surface is not enclosed by the required technical contract.",
        "generic": "the contract expected by the construction is no longer respected.",
    }[kind]
    return family


def navigation_for(keyword: str, ordered_keywords: list[str]) -> tuple[str | None, str | None]:
    index = ordered_keywords.index(keyword)
    prev_keyword = ordered_keywords[index - 1] if index > 0 else None
    next_keyword = ordered_keywords[index + 1] if index < len(ordered_keywords) - 1 else None
    return prev_keyword, next_keyword


def deep_profile_for(keyword: str) -> dict[str, object] | None:
    return DEEP_KEYWORD_PROFILES.get(keyword)


def render_page(keyword: str, ordered_keywords: list[str]) -> str:
    kind = keyword_kind(keyword)
    level = level_for(kind)
    nearby = nearby_for(keyword, kind)
    prev_keyword, next_keyword = navigation_for(keyword, ordered_keywords)
    deep_profile = deep_profile_for(keyword)
    vitte_example, c_example, invalid_example, invalid_note = examples_for(keyword, kind)
    syntax = syntax_for(keyword, kind)
    used = chapters_for(kind, keyword)
    definition = {
        "type": f"<code>{keyword}</code> is a built-in type keyword used to make a value contract explicit.",
        "literal": f"<code>{keyword}</code> is a boolean literal keyword used to express a truth value directly in code.",
        "control": f"<code>{keyword}</code> is a control-flow keyword that changes the program's execution path.",
        "declaration": f"<code>{keyword}</code> is a declaration keyword that changes the shape of a module, type, or executable contract.",
        "binding": f"<code>{keyword}</code> is a binding or assignment keyword that changes how local state is introduced or updated.",
        "module": f"<code>{keyword}</code> is a module-surface keyword used to organize visibility, imports, or sharing.",
        "operator": f"<code>{keyword}</code> is an operator keyword that changes how values are combined, tested, or traversed.",
        "advanced": f"<code>{keyword}</code> marks an advanced surface where low-level or unsafe behavior must remain explicit.",
        "generic": f"<code>{keyword}</code> is a Vitte keyword whose role must remain explicit in both grammar and program contract.",
    }[kind]

    pitfalls = [
        f"Using <code>{keyword}</code> in a grammatical layer where it does not belong.",
        "Confusing the role of the keyword with the role of the full surrounding block.",
        "Showing only the nominal form and never how the contract fails.",
    ]

    errors = [
        ("unexpected token near " + keyword, "The keyword appears in an invalid form or grammatical layer.", "Return to the canonical form and verify placement and delimiters."),
        ("type mismatch", "The keyword participates in a block whose value contract is incoherent.", "Realign the surrounding types, branches, or produced values."),
        ("invalid construct", "The keyword is present but the surrounding construction is incomplete.", "Restore the missing branch, declarative part, or operands."),
    ]

    see_also = [
        "docs/book/chapters/keywords/erreurs-compilateur.html",
        f"docs/book/chapters/keywords/{nearby}.html",
        "docs/book/glossary.html",
    ]
    c_note = (
        "This C comparison is structural: it aligns the role of the keyword with a familiar surface without claiming that the two languages carry exactly the same contracts."
        if "/*" not in c_example
        else "For this keyword, the parallel with C remains approximate. The comparison mainly indicates that in C the same idea is often spread across file conventions, operators, or less explicit control structures."
    )

    article = [
        f"<h1>Keyword <code>{keyword}</code></h1>",
        f"<p><code>{keyword}</code> is documented here as a full reference entry: grammatical role, semantics, canonical form, valid example, counter-example, diagnostics, interactions, and design notes.</p>",
        '<section class="keyword-hero lead-panel">',
        '<figure class="keyword-portrait">',
        f'<img src="portraits/{keyword}.svg" alt="Visual portrait of keyword {keyword}" width="320" height="320">',
        f"<figcaption>Syntax portrait: a code vignette centered on <code>{keyword}</code>.</figcaption>",
        "</figure>",
        '<div class="keyword-hero-copy">',
        f"<p><strong>Visual anchor:</strong> each page now has its own wiki-style profile image. It shows a small code excerpt where <code>{keyword}</code> appears in its most recognizable form.</p>",
        f"<p><strong>Quick navigation:</strong> use the <em>previous</em>, <em>summary</em>, and <em>next</em> links to move through the full keyword series without manually returning to the index.</p>",
        "</div>",
        "</section>",
        '<nav class="doc-pagination keyword-doc-pagination">',
        (f'<a href="{prev_keyword}.html">← {prev_keyword}</a>' if prev_keyword else "<span></span>"),
        '<a href="index.html">Summary</a>',
        (f'<a href="{next_keyword}.html">{next_keyword} →</a>' if next_keyword else "<span></span>"),
        "</nav>",
        '<section class="lead-panel">',
        "<h2>Summary</h2>",
        '<ul>',
        '<li><a href="#overview">Overview</a></li>',
        '<li><a href="#definition">Definition</a></li>',
        '<li><a href="#grammatical-role">Grammatical role</a></li>',
        '<li><a href="#syntax">Canonical syntax</a></li>',
        '<li><a href="#semantics">Detailed semantics</a></li>',
        ('<li><a href="#specific-profile">Specific profile</a></li>' if deep_profile else ''),
        '<li><a href="#execution-effect">Effect on execution</a></li>',
        '<li><a href="#variants">Valid variants</a></li>',
        '<li><a href="#vitte-example">Vitte example</a></li>',
        '<li><a href="#reading">Guided reading of the example</a></li>',
        '<li><a href="#c-comparison">Comparison with C</a></li>',
        '<li><a href="#usage">Recommended uses</a></li>',
        '<li><a href="#invalid">Invalid example and diagnostic</a></li>',
        '<li><a href="#errors">Common errors</a></li>',
        '<li><a href="#neighbors">Neighbor keywords</a></li>',
        '<li><a href="#misreadings">Common misreadings</a></li>',
        '<li><a href="#implementation">Implementation notes</a></li>',
        '<li><a href="#chapters">Presence in the book</a></li>',
        '</ul>',
        '</section>',
        '<section class="lead-panel" id="overview">',
        "<h2>Overview</h2>",
        '<table border="1" cellpadding="6" cellspacing="0">',
        "<tr><th>Field</th><th>Value</th></tr>",
        f"<tr><td>Keyword</td><td><code>{keyword}</code></td></tr>",
        f"<tr><td>Family</td><td>{family_label(kind)}</td></tr>",
        f"<tr><td>Suggested level</td><td>{level}</td></tr>",
        f"<tr><td>Main neighbor</td><td><a href=\"{nearby}.html\"><code>{nearby}</code></a></td></tr>",
        f"<tr><td>Short role</td><td>{definition}</td></tr>",
        f"<tr><td>Main effect</td><td>{execution_effect_for(keyword, kind)}</td></tr>",
        "</table>",
        f"<p>{role_overview(keyword, kind)}</p>",
        f"<p>A useful encyclopedic reading should answer three questions: where can <code>{keyword}</code> appear, what does it change in the block contract, and how does the compiler signal misuse?</p>",
        "</section>",
        '<section id="definition">',
        "<h2>Definition</h2>",
        f"<p>{definition}</p>",
        f"<p>{role_overview(keyword, kind)}</p>",
        "</section>",
        '<section id="grammatical-role">',
        "<h2>Grammatical role</h2>",
        f"<p>{grammar_role_for(keyword, kind)}</p>",
        f"<p>This grammatical role is essential: if a reader understands the structural place of <code>{keyword}</code>, they already understand much of the diagnostics that will appear when it is moved or truncated.</p>",
        "</section>",
        '<section id="syntax">',
        "<h2>Canonical syntax</h2>",
        f"<p>{syntax}</p>",
        f"<p>The canonical form matters because it gives the compiler and the reader the same reference structure. A large share of diagnostics related to <code>{keyword}</code> come from an abbreviated, displaced, or incomplete form.</p>",
        "</section>",
        '<section id="semantics">',
        "<h2>Detailed semantics</h2>",
        f"<p>{semantic_details(keyword, kind)}</p>",
        f"<p>In an encyclopedic reading, <code>{keyword}</code> should not be reduced to a dictionary definition. Its effect on scope, block shape, value visibility, control progression, and the diagnostic family it activates when misused must also be considered.</p>",
        "</section>",
        '<section id="execution-effect">',
        "<h2>Effect on execution</h2>",
        f"<p>{execution_effect_for(keyword, kind)}</p>",
        f"<p>In other words, the presence of <code>{keyword}</code> is not merely syntactic: it helps the reader predict what will be executed, produced, exposed, or forbidden from this point in the program.</p>",
        "</section>",
        '<section id="variants">',
        "<h2>Valid variants</h2>",
        "<ul>",
    ]
    if deep_profile:
        article[article.index('<section id="execution-effect">'):article.index('<section id="execution-effect">')] = [
            '<section id="specific-profile">',
            "<h2>Specific profile</h2>",
            f"<p>{deep_profile['summary']}</p>",
            "<h3>Design notes</h3>",
            "<ul>",
            *[f"<li>{note}</li>" for note in deep_profile["design_notes"]],
            "</ul>",
            "<h3>Reading questions</h3>",
            "<ul>",
            *[f"<li>{question}</li>" for question in deep_profile["reading_questions"]],
            "</ul>",
            "<h3>Targeted anti-patterns</h3>",
            "<ul>",
            *[f"<li>{item}</li>" for item in deep_profile["anti_patterns"]],
            "</ul>",
            "</section>",
        ]
    for variant in canonical_variants_for(keyword, kind):
        article.append(f"<li>{variant}</li>")
    article.extend([
        "</ul>",
        f"<p>These variants are not free synonyms. They indicate the legitimate forms from which one can reason about diagnostics, scope differences, or contract readability.</p>",
        "</section>",
        '<section id="vitte-example">',
        "<h2>Vitte example</h2>",
        code_block("vit", vitte_example),
        f"<p>This example shows <code>{keyword}</code> in a nominal context. It should be read globally: where the contract begins, which values are constrained, which output becomes observable, and why the presence of the keyword is justified.</p>",
        "</section>",
        '<section id="reading">',
        "<h2>Guided reading of the example</h2>",
        "<ol>",
    ])
    for step in detailed_reading_steps(keyword, kind):
        article.append(f"<li>{step}</li>")
    article.extend([
        "</ol>",
        f"<p>This guided reading is intentionally closer to a reference page than to a tutorial: it helps reconstruct the exact role of <code>{keyword}</code> in a complete block.</p>",
        "</section>",
        '<section id="c-comparison">',
        "<h2>Comparison with C</h2>",
        code_block("c", c_example),
        f"<p>{c_note}</p>",
        f"<p>The source of truth remains Vitte grammar and semantics. The comparison with C should be read as a cultural marker, not as a parallel specification.</p>",
        "</section>",
        '<section id="usage">',
        "<h2>Recommended uses</h2>",
        f"<p><code>{keyword}</code> deserves to appear when it simplifies the reading of the block's global contract, not when it merely adds one more surface form.</p>",
        "<h3>When to use it</h3>",
        "<ul>",
    ])
    for item in usage_points(keyword, kind):
        article.append(f"<li>{item}</li>")
    article.extend([
        "</ul>",
        "<h3>When to avoid it</h3>",
        "<ul>",
    ])
    for item in avoid_points(keyword, kind):
        article.append(f"<li>{item}</li>")
    article.extend([
        "</ul>",
        "<h3>Common pitfalls</h3>",
        "<ul>",
    ])
    for pitfall in pitfalls:
        article.append(f"<li>{pitfall}</li>")
    article.extend([
        "</ul>",
        "</section>",
        '<section id="invalid">',
        "<h2>Invalid example and diagnostic</h2>",
        code_block("vit", invalid_example),
        f"<p>{invalid_note}</p>",
        f"<p>{invalid_reason_for(keyword, kind, invalid_note)}</p>",
        f"<p>A good encyclopedic counter-example does not show arbitrarily broken code: it isolates the precise reason why <code>{keyword}</code> can no longer support the expected contract. Its teaching value is diagnostic before it is syntactic.</p>",
        "</section>",
        '<section id="errors">',
        "<h2>Common compilation errors</h2>",
        '<table border="1" cellpadding="6" cellspacing="0">',
        "<tr><th>Typical message</th><th>Usual cause</th><th>Fix</th></tr>",
    ])
    for msg, cause, corr in errors:
        article.append(
            f"<tr><td><code>{html.escape(msg)}</code></td><td>{html.escape(cause)}</td><td>{html.escape(corr)}</td></tr>"
        )
    article.extend([
        "</table>",
        f"<p>This table does not replace the compiler's exact diagnostics. It serves as a mental map: when <code>{keyword}</code> fails, the problem usually comes from an invalid grammatical form, an incoherent type contract, or an incomplete construction.</p>",
        "</section>",
        '<section id="neighbors">',
        "<h2>Neighbor keywords</h2>",
        '<table border="1" cellpadding="6" cellspacing="0">',
        "<tr><th>Keyword</th><th>Operational difference</th></tr>",
    ])
    for related, explanation in interaction_rows_for(keyword, kind, nearby):
        article.append(f"<tr><td><a href=\"{related}.html\"><code>{related}</code></a></td><td>{explanation}</td></tr>")
    article.extend([
        "</table>",
        f"<p>Comparison with neighboring keywords is essential on a wiki-style page: <code>{keyword}</code> is better understood when one knows precisely what it does not do.</p>",
        "</section>",
        '<section id="misreadings">',
        "<h2>Common misreadings</h2>",
        "<ul>",
    ])
    for item in common_misreadings_for(keyword, kind):
        article.append(f"<li>{item}</li>")
    article.extend([
        "</ul>",
        "</section>",
        '<section id="implementation">',
        "<h2>Implementation and diagnostic notes</h2>",
        "<ul>",
    ])
    for note in implementation_notes_for(keyword, kind):
        article.append(f"<li>{note}</li>")
    article.extend([
        "</ul>",
        "</section>",
        '<section id="chapters">',
        "<h2>Presence in the book</h2>",
        "<ul>",
    ])
    for chapter in used:
        rel = chapter.replace("docs/", "../../../")
        article.append(f"<li><a href=\"{rel}\"><code>{chapter}</code></a></li>")
    article.extend([
        "</ul>",
        "</section>",
        "<section>",
        "<h2>See also</h2>",
        "<ul>",
    ])
    for ref in see_also:
        rel = ref.replace("docs/", "../../../")
        article.append(f"<li><a href=\"{rel}\"><code>{ref}</code></a></li>")
    article.extend([
        "</ul>",
        "</section>",
        '<nav class="doc-pagination keyword-doc-pagination">',
        (f'<a href="{prev_keyword}.html">← {prev_keyword}</a>' if prev_keyword else "<span></span>"),
        '<a href="index.html">Summary</a>',
        (f'<a href="{next_keyword}.html">{next_keyword} →</a>' if next_keyword else "<span></span>"),
        "</nav>",
    ])

    body = "\n".join(article)
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Keyword `{keyword}`</title>
<link rel="icon" href="../../../svg/logo.svg" type="image/svg+xml">
<link rel="stylesheet" href="../../../css/site.css">
<link rel="stylesheet" href="../../../css/print.css" media="print">
<link rel="alternate" hreflang="en" href="https://vitte-lang.org/{keyword}.html">
<link rel="alternate" hreflang="x-default" href="https://vitte-lang.org/{keyword}.html">
<meta http-equiv="Content-Security-Policy" content="default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none';">
</head>
<body class="classic-doc">
<a class="skip-link" href="#main-content">Skip to content</a>
<div class="site-shell">
<header class="site-header">
<a class="site-brand" href="../../../index.html"><img class="site-brand-mark" src="../../../svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a>
<nav class="site-nav" aria-label="Primary">
<ul class="nav-band"><li><a class="nav-chip" href="../../../index.html"><img src="../../../svg/home.svg" alt="" width="14" height="14" aria-hidden="true"><span>Welcome</span></a></li><li><a class="nav-chip" href="../../../doc.html"><img src="../../../svg/docs.svg" alt="" width="14" height="14" aria-hidden="true"><span>Documentation</span></a></li><li><a class="nav-chip" href="../../../download.html"><img src="../../../svg/download.svg" alt="" width="14" height="14" aria-hidden="true"><span>Download</span></a></li><li><a class="nav-chip" href="../../../source.html"><img src="../../../svg/source.svg" alt="" width="14" height="14" aria-hidden="true"><span>Source</span></a></li><li><a class="nav-chip" href="../../../community.html"><img src="../../../svg/community.svg" alt="" width="14" height="14" aria-hidden="true"><span>Community</span></a></li><li><a class="nav-chip" href="../../../news.html"><img src="../../../svg/news.svg" alt="" width="14" height="14" aria-hidden="true"><span>News</span></a></li><li><a class="nav-chip" href="../../../diagnostics.html"><img src="../../../svg/diagnostics.svg" alt="" width="14" height="14" aria-hidden="true"><span>Diagnostics</span></a></li><li><a class="nav-chip" href="../../../suggestions.html"><img src="../../../svg/suggestions.svg" alt="" width="14" height="14" aria-hidden="true"><span>Suggestions</span></a></li></ul>
</nav>
</header>
<main id="main-content" class="site-main">
<article class="doc-content">
{body}
</article>
</main>
<footer class="site-footer">
<p class="site-footer-path">book/chapters/keywords/{keyword}.html</p>
<p><a href="../../../index.html">Back to home</a></p>
</footer>
</div>
<script type="module" src="../../../js/book-learning.js"></script>
<script type="module" src="../../../js/main.js"></script>
</body>
</html>
"""


def render_keyword_json(keyword: str, ordered_keywords: list[str]) -> str:
    html_page = render_page(keyword, ordered_keywords)
    match = ARTICLE_RE.search(html_page)
    body_html = match.group(1).strip() if match else ""
    prev_keyword, next_keyword = navigation_for(keyword, ordered_keywords)
    payload = {
        "keyword": keyword,
        "title": f"Keyword `{keyword}`",
        "path": f"{keyword}.html",
        "json_path": f"json/{keyword}.json",
        "prev_keyword": prev_keyword,
        "next_keyword": next_keyword,
        "body_html": body_html,
        "portrait_path": f"portraits/{keyword}.svg",
    }
    return json.dumps(payload, ensure_ascii=False, indent=2) + "\n"


def render_keyword_shell() -> str:
    return """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Keyword Viewer</title>
<link rel="icon" href="../../../svg/logo.svg" type="image/svg+xml">
<link rel="stylesheet" href="../../../css/site.css">
<link rel="stylesheet" href="../../../css/print.css" media="print">
<meta http-equiv="Content-Security-Policy" content="default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none';">
</head>
<body class="classic-doc">
<a class="skip-link" href="#main-content">Skip to content</a>
<div class="site-shell">
<header class="site-header">
<a class="site-brand" href="../../../index.html"><img class="site-brand-mark" src="../../../svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a>
<nav class="site-nav" aria-label="Primary">
<ul class="nav-band"><li><a class="nav-chip" href="../../../index.html"><img src="../../../svg/home.svg" alt="" width="14" height="14" aria-hidden="true"><span>Welcome</span></a></li><li><a class="nav-chip" href="../../../doc.html"><img src="../../../svg/docs.svg" alt="" width="14" height="14" aria-hidden="true"><span>Documentation</span></a></li><li><a class="nav-chip" href="../../../download.html"><img src="../../../svg/download.svg" alt="" width="14" height="14" aria-hidden="true"><span>Download</span></a></li><li><a class="nav-chip" href="../../../source.html"><img src="../../../svg/source.svg" alt="" width="14" height="14" aria-hidden="true"><span>Source</span></a></li><li><a class="nav-chip" href="../../../community.html"><img src="../../../svg/community.svg" alt="" width="14" height="14" aria-hidden="true"><span>Community</span></a></li><li><a class="nav-chip" href="../../../news.html"><img src="../../../svg/news.svg" alt="" width="14" height="14" aria-hidden="true"><span>News</span></a></li><li><a class="nav-chip" href="../../../diagnostics.html"><img src="../../../svg/diagnostics.svg" alt="" width="14" height="14" aria-hidden="true"><span>Diagnostics</span></a></li><li><a class="nav-chip" href="../../../suggestions.html"><img src="../../../svg/suggestions.svg" alt="" width="14" height="14" aria-hidden="true"><span>Suggestions</span></a></li></ul>
</nav>
</header>
<main id="main-content" class="site-main">
<article class="doc-content" id="keyword-article">
<h1>Keyword Viewer</h1>
<p>Open this page with <code>?id=if</code> or another keyword name to load its JSON source.</p>
</article>
</main>
<footer class="site-footer">
<p class="site-footer-path">book/chapters/keywords/keyword.html</p>
<p><a href="../../../index.html">Back to home</a></p>
</footer>
</div>
<script type="module" src="../../../js/keyword-viewer.js"></script>
<script type="module" src="../../../js/book-learning.js"></script>
<script type="module" src="../../../js/main.js"></script>
</body>
</html>
"""


def main() -> int:
    KEYWORD_PORTRAITS_DIR.mkdir(parents=True, exist_ok=True)
    KEYWORD_JSON_DIR.mkdir(parents=True, exist_ok=True)
    ordered_keywords = keyword_pages()
    (KEYWORDS_DIR / "keyword.html").write_text(render_keyword_shell(), encoding="utf-8")
    updated = 0
    for page in sorted(KEYWORDS_DIR.glob("*.html")):
        if page.name in SKIP:
            continue
        keyword = page.stem
        page.write_text(render_page(keyword, ordered_keywords), encoding="utf-8")
        (KEYWORD_JSON_DIR / f"{keyword}.json").write_text(
            render_keyword_json(keyword, ordered_keywords),
            encoding="utf-8",
        )
        (KEYWORD_PORTRAITS_DIR / f"{keyword}.svg").write_text(
            svg_portrait_for(keyword, keyword_kind(keyword)),
            encoding="utf-8",
        )
        updated += 1
    print(f"generated_keywords={updated}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
