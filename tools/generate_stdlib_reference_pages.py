#!/usr/bin/env python3
from __future__ import annotations

import html
import json
import re
from pathlib import Path

from generate_stdlib_family_docs import FAMILY_INFO


ROOT = Path(__file__).resolve().parents[1]
STDLIB = ROOT / "src" / "vitte" / "stdlib"
OUT = ROOT / "docs" / "book" / "stdlib-reference"
MODULES_DIR = OUT / "modules"
FAMILIES_DIR = OUT / "families"
PORTRAITS_DIR = OUT / "portraits"
JSON_DIR = OUT / "json"

PROC_RE = re.compile(r"^\s*proc\s+([A-Za-z_][A-Za-z0-9_]*)", re.M)
FORM_RE = re.compile(r"^\s*form\s+([A-Za-z_][A-Za-z0-9_]*)", re.M)
PICK_RE = re.compile(r"^\s*pick\s+([A-Za-z_][A-Za-z0-9_]*)", re.M)
CONST_RE = re.compile(r"^\s*const\s+([A-Za-z_][A-Za-z0-9_]*)", re.M)
USE_RE = re.compile(r"^\s*use\s+([A-Za-z0-9_/\.\{\}\,\s]+)", re.M)
SPACE_RE = re.compile(r"^\s*space\s+([A-Za-z0-9_/\-]+)", re.M)
EXPORT_RE = re.compile(r"^\s*export\s+(.+?)\s*$", re.M)
DECL_RE = re.compile(r"^\s*(proc|form|pick|const|use|space|export)\s+(.+?)\s*$")
IF_RE = re.compile(r"\bif\b")
WHILE_RE = re.compile(r"\bwhile\b")
FOR_RE = re.compile(r"\bfor\b")
MATCH_RE = re.compile(r"\bmatch\b")
LET_RE = re.compile(r"\blet\b")
GIVE_RE = re.compile(r"\bgive\b")


def shared_header(rel_root: str) -> str:
    return f"""<header class="site-header"><a class="site-brand" href="{rel_root}index.html"><img class="site-brand-mark" src="{rel_root}svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a><nav class="site-nav" aria-label="Primary"><ul class="nav-band"><li><a class="nav-chip" href="{rel_root}index.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel_root}svg/sprite.svg#i-home"></use></svg><span>Welcome</span></a></li><li><a class="nav-chip" href="{rel_root}doc.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel_root}svg/sprite.svg#i-docs"></use></svg><span>Documentation</span></a></li><li><a class="nav-chip" href="{rel_root}download.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel_root}svg/sprite.svg#i-docs"></use></svg><span>Download</span></a></li><li><a class="nav-chip" href="{rel_root}source.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel_root}svg/sprite.svg#i-docs"></use></svg><span>Source</span></a></li><li><a class="nav-chip" href="{rel_root}community.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel_root}svg/sprite.svg#i-docs"></use></svg><span>Community</span></a></li><li><a class="nav-chip" href="{rel_root}news.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel_root}svg/sprite.svg#i-news"></use></svg><span>News</span></a></li><li><a class="nav-chip" href="{rel_root}diagnostics.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel_root}svg/sprite.svg#i-docs"></use></svg><span>Diagnostics</span></a></li><li><a class="nav-chip" href="{rel_root}suggestions.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{rel_root}svg/sprite.svg#i-docs"></use></svg><span>Suggestions</span></a></li></ul></nav><form class="doc-search" role="search" action="{rel_root}search.html" method="get"><input class="doc-search-input" type="search" name="q" placeholder="Search docs, book, grammar" aria-label="Search documentation" autocomplete="off"><div class="doc-search-controls" aria-label="Search filters"><select class="doc-search-filter doc-search-section" name="section" aria-label="Filter by section"><option value="all">All sections</option><option value="docs">Docs</option><option value="book">Book</option><option value="grammar">Grammar</option></select><select class="doc-search-filter doc-search-lang" name="lang" aria-label="Filter by language"><option value="all">All languages</option><option value="en">EN</option><option value="fr">FR</option></select></div><div class="doc-search-results" hidden></div><div class="doc-search-footer" hidden></div></form></header>"""

VITTE_KEYWORDS = [
    "space",
    "use",
    "const",
    "form",
    "pick",
    "case",
    "proc",
    "let",
    "set",
    "if",
    "else",
    "while",
    "for",
    "match",
    "give",
    "return",
    "entry",
    "at",
    "export",
    "true",
    "false",
    "and",
    "or",
    "not",
    "as",
]
def rel(path: Path, base: Path) -> str:
    return path.relative_to(base).as_posix()


def family_key_for(path: Path) -> str:
    rel_path = path.relative_to(STDLIB)
    if len(rel_path.parts) == 1:
        stem = path.stem
        if stem in FAMILY_INFO:
            return stem
        return "root"
    head = rel_path.parts[0]
    if head in FAMILY_INFO:
        return head
    return "root"


def module_slug(path: Path) -> str:
    return rel(path, STDLIB).replace("/", "--")


def module_title(path: Path) -> str:
    rel_path = path.relative_to(STDLIB)
    if path.name == "mod.vit":
        if len(rel_path.parts) == 1:
            return "stdlib/mod"
        return f"{rel_path.parts[0]}/mod"
    stem = path.stem.replace("_", " ")
    return stem.title()


def page_shell(title: str, rel_root: str, body: str, page_path: str) -> str:
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)}</title>
<link rel="icon" href="{rel_root}svg/logo.svg" type="image/svg+xml">
<link rel="stylesheet" href="{rel_root}css/site.css">
<link rel="stylesheet" href="{rel_root}css/print.css" media="print">
<link rel="alternate" hreflang="en" href="https://vitte-lang.org/book/stdlib-reference/{html.escape(page_path)}">
<link rel="alternate" hreflang="x-default" href="https://vitte-lang.org/book/stdlib-reference/{html.escape(page_path)}">
<meta http-equiv="Content-Security-Policy" content="default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none';">
</head>
<body class="classic-doc">
<a class="skip-link" href="#main-content">Skip to content</a>
<div class="site-shell">
{shared_header(rel_root)}
<main id="main-content" class="site-main">
<article class="doc-content">
{body}
</article>
</main>
<footer class="site-footer">
<p class="site-footer-path">book/stdlib-reference/{html.escape(page_path)}</p>
<p><a href="{rel_root}index.html">Back to home</a></p>
</footer>
</div>
<script type="module" src="{rel_root}js/book-learning.js"></script>
<script type="module" src="{rel_root}js/main.js"></script>
</body>
</html>
"""


def top_code_excerpt(text: str, limit: int = 10) -> str:
    lines: list[str] = []
    in_banner = False
    for raw in text.splitlines():
        line = raw.rstrip()
        if "<<<" in line:
            in_banner = True
        if in_banner:
            if ">>>" in line:
                in_banner = False
            continue
        if not line.strip():
            continue
        lines.append(line)
        if len(lines) >= limit:
            break
    return "\n".join(lines)


def family_palette(family: str) -> tuple[str, str]:
    palette = {
        "root": ("#102033", "#8dd3ff"),
        "core": ("#18263a", "#ffd166"),
        "collections": ("#162b25", "#74d3ae"),
        "compiler": ("#2a2137", "#f4a8ff"),
        "compression": ("#30231f", "#ffb86b"),
        "crypto": ("#211f33", "#b4c7ff"),
        "data": ("#0f2d2c", "#7ee7d9"),
        "encoding": ("#1d2433", "#ff7a59"),
        "ffi": ("#2b1b22", "#ff8fab"),
        "io": ("#172836", "#8ad1ff"),
        "json": ("#142630", "#7cd4ff"),
        "kernel": ("#261e1a", "#f6bd60"),
        "math": ("#1f2535", "#c7ceff"),
        "path": ("#1b2920", "#9fe870"),
        "tests": ("#252525", "#e2e8f0"),
        "threading": ("#1a2430", "#7dd3fc"),
        "async": ("#202531", "#b4c7ff"),
    }
    return palette.get(family, ("#102033", "#8dd3ff"))


def module_portrait_svg(module: dict[str, object]) -> str:
    bg, accent = family_palette(module["family_key"])  # type: ignore[index]
    text = "#eef4ff"
    muted = "#b9c4d8"
    width = 640
    height = 640
    pad = 34
    excerpt = str(module["excerpt"]).splitlines()[:6]
    family = str(module["family_title"])
    rel_path = str(module["rel_path"])
    title = str(module["title"])
    svg = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img" aria-labelledby="title desc">',
        f'<title id="title">Stdlib portrait for {html.escape(rel_path)}</title>',
        f'<desc id="desc">Wiki-style portrait for stdlib module {html.escape(rel_path)}.</desc>',
        f'<rect width="{width}" height="{height}" rx="38" fill="{bg}"/>',
        f'<rect x="18" y="18" width="{width-36}" height="{height-36}" rx="30" fill="none" stroke="{accent}" stroke-opacity="0.35" stroke-width="2"/>',
        f'<circle cx="{width-82}" cy="84" r="42" fill="{accent}" fill-opacity="0.16"/>',
        f'<circle cx="{width-82}" cy="84" r="24" fill="{accent}" fill-opacity="0.92"/>',
        f'<text x="{pad}" y="74" font-family="ui-sans-serif, system-ui, sans-serif" font-size="18" fill="{muted}">stdlib module</text>',
        f'<text x="{pad}" y="118" font-family="Georgia, Times New Roman, serif" font-size="38" font-weight="700" fill="{text}">{html.escape(title)}</text>',
        f'<text x="{pad}" y="148" font-family="ui-monospace, SFMono-Regular, Menlo, Consolas, monospace" font-size="16" fill="{muted}">{html.escape(rel_path)}</text>',
        f'<rect x="{pad}" y="168" width="188" height="34" rx="17" fill="{accent}" fill-opacity="0.18"/>',
        f'<text x="{pad + 16}" y="190" font-family="ui-sans-serif, system-ui, sans-serif" font-size="16" fill="{accent}">{html.escape(family)}</text>',
        f'<rect x="{pad}" y="222" width="{width - pad*2}" height="316" rx="24" fill="#0f1726" fill-opacity="0.74" stroke="{accent}" stroke-opacity="0.24"/>',
    ]
    y = 262
    for line in excerpt:
        svg.append(f'<text x="{pad + 22}" y="{y}" font-family="ui-monospace, SFMono-Regular, Menlo, Consolas, monospace" font-size="23" fill="{text}">{html.escape(line)}</text>')
        y += 42
    svg.extend([
        f'<text x="{pad}" y="{height - 58}" font-family="ui-sans-serif, system-ui, sans-serif" font-size="16" fill="{muted}">Reference portrait for {html.escape(rel_path)}</text>',
        "</svg>",
    ])
    return "\n".join(svg)


def file_kind(path: Path) -> str:
    rel_path = path.relative_to(STDLIB)
    if "tests" in rel_path.parts:
        return "validation module"
    if path.name == "mod.vit":
        return "aggregation module"
    if path.suffix == ".vit":
        return "compiler-facing or orchestrator module"
    return "public stdlib surface"


def purpose_for_module(module: dict[str, object]) -> str:
    family_info = FAMILY_INFO[module["family_key"]]  # type: ignore[index]
    title = str(module["title"]).lower()
    rel_path = str(module["rel_path"])
    kind = str(module["kind"])
    if "readme" in title:
        return f"This file explains the ownership, inventory, and integration contract of `{rel_path}`. It is the narrative entry point for the `{module['family_title']}` family."
    return (
        f"`{rel_path}` is a {kind} inside the `{module['family_title']}` family. "
        f"It should be read as one focused slice of the broader family responsibility: {family_info['purpose']}"
    )


def use_cases_for_module(module: dict[str, object]) -> list[str]:
    family_info = FAMILY_INFO[module["family_key"]]  # type: ignore[index]
    base = list(family_info["example"])  # type: ignore[index]
    title = str(module["title"]).lower()
    if "hash" in title:
        base.insert(0, "Use this module when keyed lookup, de-duplication, or stable addressability is part of the program contract.")
    elif "vector" in title or "list" in title or "queue" in title or "stack" in title or "deque" in title:
        base.insert(0, "Use this module when ordered storage and traversal cost are more important than host-facing effects.")
    elif "json" in title or "parse" in title or "serialize" in title or "stringify" in title:
        base.insert(0, "Use this module when structured exchange format is part of the feature, not just an incidental output.")
    elif "thread" in title or "async" in title or "future" in title or "executor" in title or "channel" in title:
        base.insert(0, "Use this module when coordination and scheduling are explicit parts of the design.")
    elif "file" in title or "stream" in title or "stdio" in title or "path" in title:
        base.insert(0, "Use this module when bytes or paths cross a host boundary and architecture must keep that boundary visible.")
    return base[:5]


def top_symbols(text: str) -> dict[str, list[str]]:
    return {
        "proc": PROC_RE.findall(text),
        "form": FORM_RE.findall(text),
        "pick": PICK_RE.findall(text),
        "const": CONST_RE.findall(text),
        "use": [item.strip() for item in USE_RE.findall(text)[:12]],
        "space": SPACE_RE.findall(text),
        "export": [item.strip() for item in EXPORT_RE.findall(text)[:12]],
    }


def collect_declarations(text: str) -> list[dict[str, object]]:
    declarations: list[dict[str, object]] = []
    for line_no, raw in enumerate(text.splitlines(), start=1):
        match = DECL_RE.match(raw)
        if not match:
            continue
        kind = match.group(1)
        tail = match.group(2).strip()
        if kind in {"use", "export"}:
            name = tail
        else:
            name = re.split(r"[\s\(\:\{<]", tail, maxsplit=1)[0]
        declarations.append(
            {
                "kind": kind,
                "name": name.strip(),
                "line": line_no,
                "signature": raw.strip(),
            }
        )
    return declarations


def source_sections(text: str) -> list[dict[str, object]]:
    sections: list[dict[str, object]] = []
    in_banner = False
    banner_lines: list[str] = []
    banner_start = 0
    for line_no, raw in enumerate(text.splitlines(), start=1):
        line = raw.rstrip()
        if "<<<" in line:
            in_banner = True
            banner_lines = []
            banner_start = line_no
            continue
        if in_banner:
            if ">>>" in line:
                cleaned = [
                    re.sub(r"\s+", " ", part).strip(" .:=-><")
                    for part in banner_lines
                    if any(ch.isalpha() for ch in part)
                ]
                title = " / ".join(part for part in cleaned if part)
                if title:
                    sections.append(
                        {
                            "title": title,
                            "line": banner_start,
                        }
                    )
                in_banner = False
                banner_lines = []
                continue
            banner_lines.append(line)
    return sections


def source_landmarks(text: str) -> list[str]:
    markers: list[str] = []
    in_banner = False
    banner_lines: list[str] = []
    for raw in text.splitlines():
        line = raw.rstrip()
        if "<<<" in line:
            in_banner = True
            banner_lines = []
            continue
        if in_banner:
            if ">>>" in line:
                cleaned = [
                    re.sub(r"\s+", " ", part).strip(" .:=-><")
                    for part in banner_lines
                    if any(ch.isalpha() for ch in part)
                ]
                label = " / ".join(part for part in cleaned if part)
                if label and label not in markers:
                    markers.append(label)
                in_banner = False
                banner_lines = []
                continue
            banner_lines.append(line)
    return markers[:10]


def sectioned_declarations(module: dict[str, object]) -> list[dict[str, object]]:
    sections = module["sections"]  # type: ignore[assignment]
    declarations = module["declarations"]  # type: ignore[assignment]
    if not sections:
        return [{"title": "File surfaces", "items": declarations}]

    grouped: list[dict[str, object]] = []
    current_index = 0
    opening: list[dict[str, object]] = []
    for decl in declarations:
        while current_index + 1 < len(sections) and decl["line"] >= sections[current_index + 1]["line"]:
            current_index += 1
        if decl["line"] < sections[0]["line"]:
            opening.append(decl)
            continue
        title = str(sections[current_index]["title"])
        bucket = next((item for item in grouped if item["title"] == title), None)
        if bucket is None:
            bucket = {"title": title, "items": []}
            grouped.append(bucket)
        bucket["items"].append(decl)
    if opening:
        grouped.insert(0, {"title": "Opening declarations", "items": opening})
    return grouped


def implementation_metrics(text: str) -> dict[str, int]:
    return {
        "if": len(IF_RE.findall(text)),
        "while": len(WHILE_RE.findall(text)),
        "for": len(FOR_RE.findall(text)),
        "match": len(MATCH_RE.findall(text)),
        "let": len(LET_RE.findall(text)),
        "give": len(GIVE_RE.findall(text)),
    }


def keywords_in_text(text: str) -> list[str]:
    found: list[str] = []
    for keyword in VITTE_KEYWORDS:
        if re.search(rf"\b{re.escape(keyword)}\b", text):
            found.append(keyword)
    return found


def decl_return_type(signature: str) -> str:
    match = re.search(r"->\s*([^\{]+)", signature)
    if not match:
        return ""
    return match.group(1).strip()


def decl_params(signature: str) -> list[tuple[str, str]]:
    match = re.search(r"\((.*)\)", signature)
    if not match:
        return []
    params_text = match.group(1).strip()
    if not params_text:
        return []
    params: list[tuple[str, str]] = []
    for raw_part in params_text.split(","):
        part = raw_part.strip()
        if ":" not in part:
            continue
        name, type_name = part.split(":", 1)
        params.append((name.strip(), type_name.strip()))
    return params


def sample_value_for_type(type_name: str) -> str:
    normalized = type_name.strip()
    if normalized in {"string"}:
        return '"sample"'
    if normalized in {"bool"}:
        return "true"
    if normalized in {"int", "i32", "i64"}:
        return "1"
    if normalized in {"f32", "f64"}:
        return "1.0"
    if normalized == "[string]":
        return '["alpha", "beta"]'
    if normalized == "[int]":
        return "[1, 2, 3]"
    if normalized == "[f64]":
        return "[1.0, 2.0, 3.0]"
    if normalized == "[bool]":
        return "[true, false]"
    if normalized == "[[int]]":
        return "[[1, 0], [0, 1]]"
    if normalized == "[[f64]]":
        return "[[1.0, 0.0], [0.0, 1.0]]"
    if normalized == "[[string]]":
        return '[["alpha"], ["beta"]]'
    if normalized.startswith("[") and normalized.endswith("]"):
        return "[]"
    return f"{normalized}()"


def first_procedure(module: dict[str, object]) -> dict[str, object] | None:
    for decl in module["declarations"]:  # type: ignore[index]
        if decl["kind"] == "proc":
            return decl
    return None


def first_bool_procedure(module: dict[str, object]) -> dict[str, object] | None:
    for decl in module["declarations"]:  # type: ignore[index]
        if decl["kind"] != "proc":
            continue
        if decl_return_type(str(decl["signature"])) == "bool":
            return decl
    return None


def first_form_name(module: dict[str, object]) -> str | None:
    for decl in module["declarations"]:  # type: ignore[index]
        if decl["kind"] == "form":
            return str(decl["name"])
    return None


def declaration_by_name(module: dict[str, object], name: str) -> dict[str, object] | None:
    for decl in module["declarations"]:  # type: ignore[index]
        if str(decl["name"]) == name:
            return decl
    return None


def example_call_for_decl(decl: dict[str, object]) -> str:
    params = decl_params(str(decl["signature"]))
    args = ", ".join(sample_value_for_type(type_name) for _, type_name in params)
    return f"{decl['name']}({args})"


def module_namespace(module: dict[str, object]) -> str:
    spaces = module["symbols"]["space"]  # type: ignore[index]
    if spaces:
        return str(spaces[0])
    return f"vitte/stdlib/{module['rel_path']}"


def consumer_module_name(module: dict[str, object]) -> str:
    return str(module["rel_path"]).replace(".vitl", "").replace(".vit", "").replace("/", "_").replace("-", "_")


def maybe_use_line(module: dict[str, object]) -> tuple[list[str], set[str]]:
    keywords = module["keywords"]  # type: ignore[assignment]
    if "use" not in keywords:
        return [], set()
    return [f"use {module_namespace(module)}"], {"use"}


def family_specific_user_example(module: dict[str, object]) -> tuple[str, list[str]] | None:
    family = str(module["family_key"])
    rel_path = str(module["rel_path"])
    keywords = module["keywords"]  # type: ignore[assignment]
    consumer = consumer_module_name(module)

    if family == "path":
        lines = [f"space demo/{consumer}"]
        used = {"space"}
        extra, kws = maybe_use_line(module)
        lines.extend(extra)
        used |= kws
        if "const" in keywords:
            lines.append('const ROOT: string = "src"')
            used.add("const")
        lines.extend([
            "form PathRun {",
            "  pattern: string,",
            "  ready: bool",
            "}",
        ])
        used.add("form")
        lines.append("proc run_example() -> PathRun {")
        used.add("proc")
        if declaration_by_name(module, "path_glob_report") is not None:
            lines.append('  let entries = path_glob_report("src/**/*.vitl")')
        elif declaration_by_name(module, "path_glob") is not None:
            lines.append('  let entries = path_glob("src/*.vitl")')
        else:
            proc = first_procedure(module)
            lines.append(f"  let entries = {example_call_for_decl(proc) if proc is not None else '[]'}")
        used.add("let")
        if declaration_by_name(module, "globbing_ready") is not None:
            lines.append("  let ready: bool = globbing_ready()")
            used.add("let")
        elif declaration_by_name(module, "path_selftest") is not None:
            lines.append("  let ready: bool = path_selftest() == 0")
            used.add("let")
        else:
            lines.append("  let ready: bool = true")
            used.update({"let", "true"})
        if "and" in keywords:
            lines.append("  let stable: bool = ready and entries.len >= 0")
            used.add("and")
        if "if" in keywords:
            lines.append("  if not ready {")
            lines.append('    give PathRun { pattern: "not-ready", ready: false }')
            lines.append("  } else {")
            lines.append('    give PathRun { pattern: "src/**/*.vitl", ready: true }')
            lines.append("  }")
            used.update({"if", "not", "else", "give", "false", "true"})
        else:
            lines.append('  give PathRun { pattern: "src/**/*.vitl", ready: true }')
            used.update({"give", "true"})
        lines.append("}")
        if "export" in keywords:
            lines.append("export run_example")
            used.add("export")
        missing = [keyword for keyword in keywords if keyword not in used]
        return "\n".join(lines), missing

    if family == "threading":
        lines = [f"space demo/{consumer}"]
        used = {"space"}
        extra, kws = maybe_use_line(module)
        lines.extend(extra)
        used |= kws
        if "const" in keywords:
            lines.extend([
                'const SAMPLE_RETRIES: int = 1',
                'const INITIAL_VALUE: int = 0',
            ])
            used.add("const")
        lines.extend([
            "form CounterReport {",
            "  ok: bool,",
            "  value: int",
            "}",
        ])
        used.add("form")
        lines.append("proc run_example() -> CounterReport {")
        used.add("proc")
        if declaration_by_name(module, "mutex_new") is not None:
            lines.append("  let guard = mutex_new<int>(INITIAL_VALUE)")
            lines.append("  let ready: bool = mutex_lock<int>(guard)")
            lines.append("  let wrote: bool = mutex_set<int>(guard, 1)")
            lines.append("  let value: int = mutex_get<int>(guard)")
            lines.append("  let released: bool = mutex_unlock<int>(guard)")
            used.add("let")
            if "set" in keywords:
                lines.append("  let retries: int = 0")
                lines.append("  while retries < SAMPLE_RETRIES {")
                lines.append("    set retries = retries + 1")
                lines.append("  }")
                used.update({"while", "set"})
            if "for" in keywords:
                lines.append("  for worker in [1] {")
                lines.append("    let _seen: int = worker")
                lines.append("  }")
                used.add("for")
        else:
            proc = first_procedure(module)
            lines.append(f"  let ready: bool = {example_call_for_decl(proc) if proc is not None else 'true'}")
            lines.append("  let value: int = 0")
            lines.append("  let released: bool = ready")
            lines.append("  let wrote: bool = ready")
            used.add("let")
        if "and" in keywords:
            lines.append("  let ok: bool = ready and wrote and released")
            used.add("and")
        else:
            lines.append("  let ok: bool = ready")
        if "if" in keywords:
            lines.append("  if not ok {")
            lines.append("    give CounterReport { ok: false, value: value }")
            lines.append("  } else {")
            lines.append("    give CounterReport { ok: true, value: value }")
            lines.append("  }")
            used.update({"if", "not", "else", "give", "false", "true"})
        else:
            lines.append("  give CounterReport { ok: ok, value: value }")
            used.add("give")
        lines.append("}")
        if "export" in keywords:
            lines.append("export run_example")
            used.add("export")
        missing = [keyword for keyword in keywords if keyword not in used]
        return "\n".join(lines), missing

    if family == "tests":
        lines = [f"space demo/{consumer}"]
        used = {"space"}
        extra, kws = maybe_use_line(module)
        lines.extend(extra)
        used |= kws
        lines.append("proc run_example() -> bool {")
        used.add("proc")
        smoke_calls = []
        for name in (
            "stdlib_smoke_core",
            "stdlib_smoke_io",
            "stdlib_smoke_compression",
            "stdlib_smoke_async",
            "stdlib_smoke_crypto",
            "stdlib_smoke_json",
            "stdlib_smoke_path",
            "stdlib_smoke_system",
            "stdlib_smoke_statistics",
        ):
            if declaration_by_name(module, name) is not None:
                lines.append(f"  let {name}: bool = {name}()")
                smoke_calls.append(name)
                used.add("let")
        if smoke_calls:
            combined = " and ".join(smoke_calls)
            if "as" in keywords:
                lines.append(f"  let family_count: f64 = {len(smoke_calls)} as f64")
                used.update({"let", "as"})
            lines.append(f"  give {combined}")
            used.update({"give", "and"})
        else:
            proc = first_bool_procedure(module) or first_procedure(module)
            lines.append(f"  give {example_call_for_decl(proc) if proc is not None else 'true'}")
            used.add("give")
        lines.append("}")
        if "export" in keywords:
            lines.append("export run_example")
            used.add("export")
        missing = [keyword for keyword in keywords if keyword not in used]
        return "\n".join(lines), missing

    if family == "root":
        lines = [f"space demo/{consumer}"]
        used = {"space"}
        extra, kws = maybe_use_line(module)
        lines.extend(extra)
        used |= kws
        if "form" in keywords:
            lines.extend([
                "form DemoState {",
                "  ready: bool,",
                "  note: string",
                "}",
            ])
            used.add("form")
        lines.append(f"proc run_example() -> {'DemoState' if 'form' in keywords else 'bool'} {{")
        used.add("proc")
        if declaration_by_name(module, "vector_new") is not None:
            lines.append("  let values = vector_push<int>(vector_new<int>(), 1)")
            lines.append('  let ready: bool = io_file_exists("README.md") or values.data.len >= 0')
            used.update({"let", "or"})
            if "as" in keywords:
                lines.append("  let count_f64: f64 = values.data.len as f64")
                used.add("as")
        elif declaration_by_name(module, "path_selftest") is not None:
            lines.append("  let ready: bool = path_selftest() == 0")
            used.add("let")
        else:
            proc = first_bool_procedure(module) or first_procedure(module)
            lines.append(f"  let ready: bool = {example_call_for_decl(proc) if proc is not None else 'true'}")
            used.add("let")
        if "if" in keywords:
            lines.append("  if not ready {")
            if "form" in keywords:
                lines.append('    give DemoState { ready: false, note: "not-ready" }')
            else:
                lines.append("    give false")
            lines.append("  } else {")
            if "form" in keywords:
                lines.append('    give DemoState { ready: true, note: "ok" }')
            else:
                lines.append("    give true")
            lines.append("  }")
            used.update({"if", "not", "else", "give", "false", "true"})
        else:
            if "form" in keywords:
                lines.append('  give DemoState { ready: ready, note: "ok" }')
            else:
                lines.append("  give ready")
            used.add("give")
        lines.append("}")
        if "export" in keywords:
            lines.append("export run_example")
            used.add("export")
        missing = [keyword for keyword in keywords if keyword not in used]
        return "\n".join(lines), missing

    return None


def generated_user_example(module: dict[str, object]) -> tuple[str, list[str]]:
    specialized = family_specific_user_example(module)
    if specialized is not None:
        return specialized
    keywords = module["keywords"]  # type: ignore[assignment]
    lines: list[str] = []
    example_keywords: set[str] = set()
    consumer_name = consumer_module_name(module)
    main_proc = first_procedure(module)
    bool_proc = first_bool_procedure(module) or main_proc
    form_name = first_form_name(module)
    array_proc = None
    for decl in module["declarations"]:  # type: ignore[index]
        if decl["kind"] == "proc" and decl_return_type(str(decl["signature"])).startswith("["):
            array_proc = decl
            break

    lines.append(f"space demo/{consumer_name}")
    example_keywords.add("space")

    if "use" in keywords:
        lines.append(f"use {module_namespace(module)}")
        example_keywords.add("use")

    if "const" in keywords:
        lines.append('const SAMPLE_LABEL: string = "demo"')
        example_keywords.add("const")

    if "form" in keywords:
        lines.append("form UserReport {")
        lines.append("  label: string,")
        lines.append("  ready: bool")
        lines.append("}")
        example_keywords.add("form")

    if "pick" in keywords:
        lines.append("pick UserOutcome {")
        lines.append("  case Ready(message: string)")
        lines.append("  case Empty(reason: string)")
        lines.append("}")
        example_keywords.update({"pick", "case"})

    if "proc" in keywords:
        return_type = "string"
        if "pick" in keywords:
            return_type = "UserOutcome"
        elif "form" in keywords:
            return_type = "UserReport"
        lines.append(f"proc run_example() -> {return_type} {{")
        example_keywords.add("proc")

        if "let" in keywords and main_proc is not None:
            if array_proc is not None:
                lines.append(f"  let entries = {example_call_for_decl(array_proc)}")
                lines.append(f"  let ready: bool = {example_call_for_decl(bool_proc) if bool_proc is not None else 'true'}")
            else:
                lines.append(f"  let result = {example_call_for_decl(main_proc)}")
                if bool_proc is not None and bool_proc is not main_proc:
                    lines.append(f"  let ready: bool = {example_call_for_decl(bool_proc)}")
            if bool_proc is None:
                lines.append("  let ready: bool = true")
            if "false" in keywords:
                lines.append("  let failed: bool = false")
                example_keywords.add("false")
            example_keywords.add("let")

        if "and" in keywords:
            lines.append("  let stable: bool = ready and true")
            example_keywords.update({"and", "true"})
        if "or" in keywords:
            lines.append("  let fallback: bool = ready or false")
            example_keywords.update({"or", "false"})

        if "while" in keywords and array_proc is not None:
            lines.append("  let idx: int = 0")
            lines.append("  let count: int = 0")
            lines.append("  while idx < entries.len {")
            if "set" in keywords:
                lines.append("    set count = count + 1")
                lines.append("    set idx = idx + 1")
                example_keywords.add("set")
            else:
                lines.append("    give count")
            lines.append("  }")
            example_keywords.add("while")
        elif "while" in keywords and "set" in keywords:
            lines.append("  let idx: int = 0")
            lines.append("  while idx < 1 {")
            lines.append("    set idx = idx + 1")
            lines.append("  }")
            example_keywords.update({"while", "set"})
        elif "set" in keywords:
            lines.append("  let retries: int = 0")
            lines.append("  set retries = retries + 1")
            example_keywords.update({"set", "let"})

        if "for" in keywords:
            lines.append("  for sample in [1] {")
            lines.append("    let seen: int = sample")
            lines.append("  }")
            example_keywords.update({"for", "let"})

        if "if" in keywords:
            condition = "ready"
            if "not" in keywords:
                condition = f"not {condition}"
                example_keywords.add("not")
            lines.append(f"  if {condition} {{")
            if "pick" in keywords:
                lines.append('    give UserOutcome.Empty("module not ready")')
            elif "form" in keywords:
                lines.append('    give UserReport { label: "not-ready", ready: false }')
                example_keywords.add("false")
            else:
                lines.append('    give "not-ready"')
            example_keywords.add("if")
            if "else" in keywords:
                lines.append("  } else {")
                example_keywords.add("else")
            else:
                lines.append("  }")

        if "match" in keywords and "pick" in keywords:
            lines.append('    let state: UserOutcome = UserOutcome.Ready("ok")')
            lines.append("    match state {")
            lines.append("      case UserOutcome.Ready(message) {")
            lines.append("        give UserOutcome.Ready(message)")
            lines.append("      }")
            lines.append("      case UserOutcome.Empty(reason) {")
            lines.append("        give UserOutcome.Empty(reason)")
            lines.append("      }")
            lines.append("    }")
            example_keywords.update({"match", "case"})
        else:
            if "pick" in keywords:
                lines.append('    give UserOutcome.Ready("ok")')
            elif "form" in keywords:
                tail = "true" if "true" in keywords else "ready"
                if "false" in keywords:
                    example_keywords.add("false")
                if "true" in keywords:
                    example_keywords.add("true")
                lines.append(f'    give UserReport {{ label: "ok", ready: {tail} }}')
            else:
                lines.append('    give "ok"')

        if "if" in keywords and "else" in keywords:
            lines.append("  }")

        if "give" in keywords:
            example_keywords.add("give")
        if "true" in keywords and any("true" in line for line in lines):
            example_keywords.add("true")
        if "false" in keywords and any("false" in line for line in lines):
            example_keywords.add("false")
        if "as" in keywords:
            lines.insert(-1 if lines[-1] == "}" else len(lines), "  let copies: f64 = 1 as f64")
            example_keywords.add("as")
        lines.append("}")

    if "export" in keywords:
        lines.append("export run_example")
        example_keywords.add("export")

    if "entry" in keywords:
        at_clause = " at core/app" if "at" in keywords else ""
        lines.append(f"entry main{at_clause} {{")
        lines.append("  return 0")
        lines.append("}")
        example_keywords.add("entry")
        if "at" in keywords:
            example_keywords.add("at")
        example_keywords.add("return")

    missing = [keyword for keyword in keywords if keyword not in example_keywords]
    return "\n".join(lines), missing


def render_keyword_coverage(module: dict[str, object], missing_keywords: list[str]) -> str:
    keywords = module["keywords"]  # type: ignore[assignment]
    rows = [
        '<div class="table-scroll">',
        "<table border=\"1\" cellpadding=\"6\" cellspacing=\"0\">",
        "<tr><th>Keyword</th><th>Present in module source</th><th>Used in generated user example</th></tr>",
    ]
    missing = set(missing_keywords)
    for keyword in keywords:
        used = "no" if keyword in missing else "yes"
        rows.append(
            "<tr>"
            f"<td><code>{html.escape(str(keyword))}</code></td>"
            "<td>yes</td>"
            f"<td>{used}</td>"
            "</tr>"
        )
    rows.append("</table>")
    rows.append("</div>")
    if missing_keywords:
        joined = ", ".join(f"<code>{html.escape(item)}</code>" for item in missing_keywords)
        rows.append(f"<p>Keywords still not exercised directly in the generated snippet: {joined}. The page still lists them here so the gap is visible.</p>")
    else:
        rows.append("<p>The generated snippet exercises every detected Vitte keyword used by this module.</p>")
    return "\n".join(rows)


def infer_module_traits(module: dict[str, object]) -> list[str]:
    symbols = module["symbols"]  # type: ignore[assignment]
    traits: list[str] = []
    proc_count = len(symbols["proc"])  # type: ignore[index]
    form_count = len(symbols["form"])  # type: ignore[index]
    pick_count = len(symbols["pick"])  # type: ignore[index]
    const_count = len(symbols["const"])  # type: ignore[index]
    import_count = len(symbols["use"])  # type: ignore[index]
    export_count = len(symbols["export"])  # type: ignore[index]

    if proc_count >= 20:
        traits.append("Large algorithm surface: this file exposes many procedures and likely acts as a domain toolkit rather than a single thin wrapper.")
    elif proc_count >= 8:
        traits.append("Medium procedure surface: this file groups several related operations behind one namespace.")
    elif proc_count > 0:
        traits.append("Compact procedure surface: this file is small enough to read end-to-end before depending on it.")

    if form_count or pick_count:
        traits.append("Owns domain vocabulary: the module declares data shapes in addition to executable helpers, so its types are part of the contract.")
    if const_count:
        traits.append("Has tuning constants: part of the module behavior is controlled by named constants that document default precision, limits, or policy.")
    if import_count == 0:
        traits.append("Minimal top-level dependencies: the module reads as mostly self-contained from its opening declarations.")
    elif import_count <= 3:
        traits.append("Narrow dependency fan-in: a small set of imports suggests a focused collaboration surface.")
    else:
        traits.append("Visible dependency fan-in: this file coordinates several imported surfaces and is worth reading as an integration point.")
    if export_count:
        traits.append("Explicit export surface: the file ends with visible export declarations instead of relying only on implicit namespace discovery.")

    if "tests" in str(module["rel_path"]):
        traits.append("Validation-oriented module: expect example flows, smoke checks, or proof that other contracts remain stable.")
    elif str(module["kind"]) == "aggregation module":
        traits.append("Aggregation-oriented module: this file likely collects exports and family-level entry points more than it implements novel algorithms.")
    return traits[:5]


def name_tokens(name: str) -> set[str]:
    return {token for token in re.split(r"[^A-Za-z0-9]+", name.lower()) if token}


def inferred_role(name: str, kind: str, module: dict[str, object]) -> str:
    family = str(module["family_key"])
    lower = name.lower()
    tokens = name_tokens(name)
    if kind == "space":
        return "Declares the namespace that anchors this file in the stdlib tree."
    if kind == "use":
        return "Imports a sibling or supporting surface used by the module."
    if kind == "export":
        return "Re-exports surfaces that the module wants to expose as part of its public boundary."
    if kind == "const":
        if any(token in tokens for token in ("eps", "epsilon", "tolerance", "limit", "max", "min")):
            return "Defines a bound or precision constant that shapes runtime behavior."
        return "Defines a named constant reused across the module."
    if kind == "form":
        return "Introduces a structured data shape that other procedures can exchange."
    if kind == "pick":
        return "Introduces a tagged variant type used to model distinct outcomes."
    if tokens & {"parse", "parser", "lexer", "token", "decode"}:
        return "Transforms an input representation into a structured internal value."
    if tokens & {"stringify", "serialize", "encode", "emit"}:
        return "Turns internal values into a transport or textual representation."
    if tokens & {"hash", "hashing", "hmac", "random", "encrypt", "decrypt", "sign"}:
        return "Implements a security-sensitive transformation in the crypto boundary."
    if tokens & {"integrate", "deriv", "gradient", "newton", "bisection", "simpson", "euler", "rk4", "rk"}:
        return "Implements a numerical-analysis routine within the math family."
    if tokens & {"factorial", "combinations", "comb", "permutations", "permutation", "probability", "prob", "binomial", "poisson"}:
        return "Implements a counting or probability helper inside the math boundary."
    if tokens & {"path", "join", "split", "walk", "walker", "glob", "globbing"}:
        return "Owns path semantics, traversal, or normalization."
    if tokens & {"file", "stream", "buffer", "stdio", "read", "write"}:
        return "Owns byte movement or host I/O interaction."
    if tokens & {"thread", "threads", "mutex", "channel", "future", "executor", "scheduler", "sched"}:
        return "Owns coordination, scheduling, or concurrency behavior."
    if tokens & {"matrix", "vector", "graph", "queue", "stack", "deque", "list", "map", "set"}:
        return "Owns a concrete data shape or the operations that maintain it."
    if family == "compiler":
        return "Supports compiler-facing orchestration, lowering, diagnostics, or backend work."
    return "Represents one top-level surface in the file contract and should be read as part of the module boundary."


def render_declaration_table(module: dict[str, object], kinds: tuple[str, ...], title: str) -> str:
    declarations = [decl for decl in module["declarations"] if decl["kind"] in kinds]  # type: ignore[index]
    if not declarations:
        return f"<p>No top-level {html.escape(title.lower())} are declared in this file.</p>"
    rows = [
        '<div class="table-scroll">',
        "<table border=\"1\" cellpadding=\"6\" cellspacing=\"0\">",
        "<tr><th>Line</th><th>Name</th><th>Kind</th><th>Role</th></tr>",
    ]
    for decl in declarations:
        rows.append(
            "<tr>"
            f"<td>{decl['line']}</td>"
            f"<td><code>{html.escape(str(decl['name']))}</code></td>"
            f"<td>{html.escape(str(decl['kind']))}</td>"
            f"<td>{html.escape(inferred_role(str(decl['name']), str(decl['kind']), module))}</td>"
            "</tr>"
        )
    rows.append("</table>")
    rows.append("</div>")
    rows.append(f"<p>The table is exhaustive for top-level declarations of the selected kinds. This file declares {len(declarations)} matching surfaces.</p>")
    return "\n".join(rows)


def render_signature_list(module: dict[str, object], kinds: tuple[str, ...], limit: int = 18) -> str:
    declarations = [decl for decl in module["declarations"] if decl["kind"] in kinds]  # type: ignore[index]
    if not declarations:
        return "<p>No matching top-level signatures were detected.</p>"
    items = []
    for decl in declarations[:limit]:
        items.append(
            "<li>"
            f"<code>{html.escape(str(decl['signature']))}</code>"
            f" (line {decl['line']})"
            "</li>"
        )
    out = [f"<ul>{''.join(items)}</ul>"]
    if len(declarations) > limit:
        out.append(f"<p>The list is intentionally capped here; the source file declares {len(declarations)} matching signatures in total.</p>")
    return "\n".join(out)


def family_position_text(module: dict[str, object], modules: list[dict[str, object]]) -> str:
    family_modules = [m for m in modules if m["family_key"] == module["family_key"]]
    family_modules_sorted = sorted(family_modules, key=lambda item: str(item["rel_path"]))
    index = family_modules_sorted.index(module) + 1
    proc_rank = sorted(family_modules, key=lambda item: len(item["symbols"]["proc"]), reverse=True).index(module) + 1  # type: ignore[index]
    line_rank = sorted(family_modules, key=lambda item: int(item["line_count"]), reverse=True).index(module) + 1
    return (
        f"This file is module {index} of {len(family_modules_sorted)} in the <code>{html.escape(str(module['family_title']))}</code> family when ordered by path. "
        f"By procedure count it ranks {proc_rank}, and by line count it ranks {line_rank}. "
        "Those ranks are useful as rough signals of breadth, not as quality judgments."
    )


def render_relationship_table(module: dict[str, object], modules: list[dict[str, object]]) -> str:
    family_modules = [m for m in modules if m["family_key"] == module["family_key"] and m["slug"] != module["slug"]]
    rows = [
        '<div class="table-scroll">',
        "<table border=\"1\" cellpadding=\"6\" cellspacing=\"0\">",
        "<tr><th>Neighbor</th><th>Procedures</th><th>Data surfaces</th><th>Why compare it</th></tr>",
    ]
    for neighbor in family_modules[:8]:
        procs = len(neighbor["symbols"]["proc"])  # type: ignore[index]
        data_surfaces = len(neighbor["symbols"]["form"]) + len(neighbor["symbols"]["pick"])  # type: ignore[index]
        why = "Shares the same family boundary but carries a distinct slice of responsibility."
        rows.append(
            "<tr>"
            f"<td><a href=\"{html.escape(str(neighbor['slug']))}.html\"><code>{html.escape(str(neighbor['rel_path']))}</code></a></td>"
            f"<td>{procs}</td>"
            f"<td>{data_surfaces}</td>"
            f"<td>{why}</td>"
            "</tr>"
        )
    rows.append("</table>")
    rows.append("</div>")
    return "\n".join(rows)


def render_complete_kind_catalog(module: dict[str, object]) -> str:
    sections: list[str] = []
    for kinds, title in (
        (("const",), "Constants"),
        (("form", "pick"), "Data surfaces"),
        (("proc",), "Procedures"),
        (("use",), "Imports"),
        (("export",), "Exports"),
    ):
        declarations = [decl for decl in module["declarations"] if decl["kind"] in kinds]  # type: ignore[index]
        if not declarations:
            continue
        rows = [
            '<div class="table-scroll">',
            "<table border=\"1\" cellpadding=\"6\" cellspacing=\"0\">",
            "<tr><th>Line</th><th>Name</th><th>Signature</th><th>Role</th></tr>",
        ]
        for decl in declarations:
            rows.append(
                "<tr>"
                f"<td>{decl['line']}</td>"
                f"<td><code>{html.escape(str(decl['name']))}</code></td>"
                f"<td><code>{html.escape(str(decl['signature']))}</code></td>"
                f"<td>{html.escape(inferred_role(str(decl['name']), str(decl['kind']), module))}</td>"
                "</tr>"
            )
        rows.append("</table>")
        rows.append("</div>")
        sections.append(f"<h3>{html.escape(title)}</h3>{''.join(rows)}")
    if not sections:
        return "<p>No complete kind catalog could be generated for this file.</p>"
    return "".join(sections)


def render_source_section_map(module: dict[str, object]) -> str:
    groups = sectioned_declarations(module)
    parts: list[str] = []
    for group in groups:
        items = group["items"]
        if not items:
            continue
        procs = sum(1 for item in items if item["kind"] == "proc")
        data_shapes = sum(1 for item in items if item["kind"] in {"form", "pick"})
        consts = sum(1 for item in items if item["kind"] == "const")
        names = ", ".join(f"<code>{html.escape(str(item['name']))}</code>" for item in items[:10])
        parts.append(
            f"""
<section class="lead-panel">
<h3>{html.escape(str(group["title"]))}</h3>
<p><strong>Top-level items:</strong> {len(items)}. <strong>Procedures:</strong> {procs}. <strong>Data surfaces:</strong> {data_shapes}. <strong>Constants:</strong> {consts}.</p>
<p><strong>First visible names:</strong> {names}</p>
</section>
"""
        )
    if not parts:
        return "<p>No section map could be reconstructed automatically for this file.</p>"
    return "".join(parts)


def render_metrics_table(module: dict[str, object]) -> str:
    metrics = module["metrics"]  # type: ignore[assignment]
    rows = [
        '<div class="table-scroll">',
        "<table border=\"1\" cellpadding=\"6\" cellspacing=\"0\">",
        "<tr><th>Signal</th><th>Count</th><th>What it suggests</th></tr>",
        f"<tr><td><code>if</code></td><td>{metrics['if']}</td><td>Branching density and local decision-making.</td></tr>",
        f"<tr><td><code>while</code></td><td>{metrics['while']}</td><td>Loop-heavy or iterative implementation style.</td></tr>",
        f"<tr><td><code>for</code></td><td>{metrics['for']}</td><td>Collection-style traversal at source level.</td></tr>",
        f"<tr><td><code>match</code></td><td>{metrics['match']}</td><td>Variant-driven branching or grammar-style decoding.</td></tr>",
        f"<tr><td><code>let</code></td><td>{metrics['let']}</td><td>Local state and intermediate value density.</td></tr>",
        f"<tr><td><code>give</code></td><td>{metrics['give']}</td><td>Number of explicit exit points and result shaping.</td></tr>",
        "</table>",
        "</div>",
    ]
    return "\n".join(rows)


def reading_order_for(module: dict[str, object]) -> list[str]:
    rel_path = str(module["rel_path"])
    symbols = module["symbols"]  # type: ignore[assignment]
    guidance = [
        f"Read <code>space</code> and top-level imports first so the ownership boundary of <code>{html.escape(rel_path)}</code> is explicit.",
    ]
    if symbols["const"]:  # type: ignore[index]
        guidance.append("Scan constants before procedures; they often encode precision, limits, or policy assumptions that explain later behavior.")
    if symbols["form"] or symbols["pick"]:  # type: ignore[index]
        guidance.append("Read declared forms and picks before algorithms so the data vocabulary is stable in your head.")
    if symbols["proc"]:  # type: ignore[index]
        guidance.append("Traverse procedures in source order; the early helpers usually explain the naming and numeric conventions used later.")
    if module["landmarks"]:  # type: ignore[index]
        guidance.append("Use the source landmarks section below as a table of contents when the file is large.")
    guidance.append("Only after that compare neighbor modules, because the right boundary choice matters more than memorizing one helper name.")
    return guidance[:5]


def choose_when_to_use(module: dict[str, object]) -> list[str]:
    items = use_cases_for_module(module)
    rel_path = str(module["rel_path"])
    items.insert(0, f"Choose <code>{html.escape(rel_path)}</code> when the main question is owned by this module rather than by transport, storage, orchestration, or user-interface code.")
    return items[:6]


def avoid_when_for(module: dict[str, object], siblings: list[dict[str, object]]) -> list[str]:
    family = str(module["family_title"])
    items = [
        f"Avoid extending this file when the new helper mostly changes the boundary to host I/O, runtime coordination, or foreign integration instead of staying inside <code>{html.escape(family)}</code>.",
    ]
    if siblings:
        neighbor_names = ", ".join(f"<code>{html.escape(str(s['rel_path']))}</code>" for s in siblings[:3])
        items.append(f"Check nearby modules such as {neighbor_names} before adding convenience wrappers here.")
    if str(module["kind"]) == "aggregation module":
        items.append("Prefer implementing concrete behavior in leaf modules first; keep the aggregation file focused on composition and export shape.")
    if "tests" in str(module["rel_path"]):
        items.append("Do not move production logic into this file just because the test already mentions it; keep proof and implementation separate.")
    return items[:4]


def read_modules() -> list[dict[str, object]]:
    modules: list[dict[str, object]] = []
    for path in sorted(STDLIB.rglob("*")):
        if not path.is_file():
            continue
        if path.name == "README.md" or path.suffix not in {".vit", ".vitl"}:
            continue
        family_key = family_key_for(path)
        family_title = FAMILY_INFO[family_key]["title"]  # type: ignore[index]
        text = path.read_text(encoding="utf-8")
        symbols = top_symbols(text)
        item = {
            "path": path,
            "rel_path": rel(path, STDLIB),
            "family_key": family_key,
            "family_title": family_title,
            "title": module_title(path),
            "slug": module_slug(path),
            "kind": file_kind(path),
            "symbols": symbols,
            "declarations": collect_declarations(text),
            "sections": source_sections(text),
            "landmarks": source_landmarks(text),
            "metrics": implementation_metrics(text),
            "keywords": keywords_in_text(text),
            "excerpt": top_code_excerpt(text),
            "line_count": len(text.splitlines()),
        }
        modules.append(item)
    return modules


def module_json_payload(module: dict[str, object], siblings: list[dict[str, object]]) -> dict[str, object]:
    symbols = module["symbols"]  # type: ignore[assignment]
    return {
        "title": module["title"],
        "path": module["rel_path"],
        "family": module["family_title"],
        "family_key": module["family_key"],
        "kind": module["kind"],
        "line_count": module["line_count"],
        "purpose": purpose_for_module(module),
        "use_cases": use_cases_for_module(module),
        "procedures": symbols["proc"],  # type: ignore[index]
        "forms": symbols["form"],  # type: ignore[index]
        "picks": symbols["pick"],  # type: ignore[index]
        "constants": symbols["const"],  # type: ignore[index]
        "imports": symbols["use"],  # type: ignore[index]
        "exports": symbols["export"],  # type: ignore[index]
        "siblings": [s["rel_path"] for s in siblings[:8]],
    }


def render_inventory_table(module: dict[str, object]) -> str:
    symbols = module["symbols"]  # type: ignore[assignment]
    rows = [
        ("Procedures", symbols["proc"]),  # type: ignore[index]
        ("Forms", symbols["form"]),  # type: ignore[index]
        ("Picks", symbols["pick"]),  # type: ignore[index]
        ("Constants", symbols["const"]),  # type: ignore[index]
        ("Exports", symbols["export"]),  # type: ignore[index]
    ]
    out = ['<table border="1" cellpadding="6" cellspacing="0">', '<tr><th>Surface</th><th>Items</th></tr>']
    for label, items in rows:
        value = ", ".join(f"<code>{html.escape(item)}</code>" for item in items[:12]) if items else "none declared at top level"
        out.append(f"<tr><td>{label}</td><td>{value}</td></tr>")
    out.append("</table>")
    return "\n".join(out)


def render_imports(symbols: dict[str, list[str]]) -> str:
    imports = symbols["use"][:12]
    if not imports:
        return "<p>This file does not advertise a top-level `use` surface in its opening declarations. That often means it is either self-contained or an aggregation layer.</p>"
    items = "".join(f"<li><code>{html.escape(item)}</code></li>" for item in imports)
    return f"<ul>{items}</ul>"


def render_landmarks(module: dict[str, object]) -> str:
    landmarks = module["landmarks"]  # type: ignore[assignment]
    if landmarks:
        items = "".join(f"<li>{html.escape(str(item))}</li>" for item in landmarks)
        return f"<ul>{items}</ul>"
    declarations = module["declarations"][:8]  # type: ignore[index]
    if not declarations:
        return "<p>No obvious landmarks were detected automatically.</p>"
    items = "".join(
        f"<li>Line {decl['line']}: <code>{html.escape(str(decl['signature']))}</code></li>"
        for decl in declarations
    )
    return f"<ul>{items}</ul>"


def module_page(module: dict[str, object], modules: list[dict[str, object]]) -> str:
    family_modules = [m for m in modules if m["family_key"] == module["family_key"]]
    siblings = [m for m in family_modules if m["slug"] != module["slug"]]
    idx = family_modules.index(module)
    prev_mod = family_modules[idx - 1] if idx > 0 else None
    next_mod = family_modules[idx + 1] if idx + 1 < len(family_modules) else None
    payload = module_json_payload(module, siblings)
    (JSON_DIR / f"{module['slug']}.json").write_text(json.dumps(payload, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (PORTRAITS_DIR / f"{module['slug']}.svg").write_text(module_portrait_svg(module), encoding="utf-8")

    use_cases = "".join(f"<li>{html.escape(item)}</li>" for item in use_cases_for_module(module))
    choose_when = "".join(f"<li>{item}</li>" for item in choose_when_to_use(module))
    avoid_when = "".join(f"<li>{item}</li>" for item in avoid_when_for(module, siblings))
    inferred_traits = "".join(f"<li>{html.escape(item)}</li>" for item in infer_module_traits(module))
    reading_order = "".join(f"<li>{item}</li>" for item in reading_order_for(module))
    user_example, missing_keywords = generated_user_example(module)
    sibling_links = "".join(
        f'<li><a href="{html.escape(s["slug"])}.html"><code>{html.escape(str(s["rel_path"]))}</code></a></li>'
        for s in siblings[:8]
    )
    body = f"""
<h1>Stdlib module <code>{html.escape(str(module["rel_path"]))}</code></h1>
<p>This page is a wiki-style reference for one concrete stdlib file. It explains what the file owns, where it fits in the family, and how to decide whether this is the right surface to depend on.</p>
<section class="keyword-hero lead-panel">
<figure class="keyword-portrait">
<img src="../portraits/{html.escape(str(module["slug"]))}.svg" alt="Visual portrait of {html.escape(str(module["rel_path"]))}" width="320" height="320">
<figcaption>Wiki-style portrait for <code>{html.escape(str(module["rel_path"]))}</code>.</figcaption>
</figure>
<div class="keyword-hero-copy">
<p><strong>Family:</strong> <a href="../families/{html.escape(str(module["family_key"]))}.html"><code>{html.escape(str(module["family_title"]))}</code></a></p>
<p><strong>Kind:</strong> {html.escape(str(module["kind"]))}</p>
<p><strong>Page style:</strong> this reference follows the same “encyclopedic card + portrait + usage contract” logic as the keyword pages, but for stdlib modules.</p>
</div>
</section>
<nav class="doc-pagination keyword-doc-pagination">
{f'<a href="{html.escape(str(prev_mod["slug"]))}.html">← {html.escape(str(prev_mod["title"]))}</a>' if prev_mod else '<span></span>'}
<a href="../index.html">Stdlib index</a>
{f'<a href="{html.escape(str(next_mod["slug"]))}.html">{html.escape(str(next_mod["title"]))} →</a>' if next_mod else '<span></span>'}
</nav>
<section class="lead-panel">
<h2>Summary</h2>
<ul>
<li><a href="#overview">Overview</a></li>
<li><a href="#purpose">Purpose</a></li>
<li><a href="#taxonomy">Taxonomy</a></li>
<li><a href="#profile">Implementation profile</a></li>
<li><a href="#inventory">Top-level API inventory</a></li>
<li><a href="#position">Position in family</a></li>
<li><a href="#declarations">Declaration map</a></li>
<li><a href="#signatures">Representative signatures</a></li>
<li><a href="#usage">How to use this module</a></li>
<li><a href="#user-example">User example</a></li>
<li><a href="#keyword-coverage">Keyword coverage</a></li>
<li><a href="#source-shape">Source shape</a></li>
<li><a href="#landmarks">Source landmarks</a></li>
<li><a href="#organization">Source organization</a></li>
<li><a href="#catalog">Complete API catalog</a></li>
<li><a href="#boundaries">Integration boundaries</a></li>
<li><a href="#composition">Composition guidance</a></li>
<li><a href="#relationships">Relationship table</a></li>
<li><a href="#neighbors">Neighbor modules</a></li>
</ul>
</section>
<section id="overview">
<h2>Overview</h2>
<table border="1" cellpadding="6" cellspacing="0">
<tr><th>Field</th><th>Value</th></tr>
<tr><td>Path</td><td><code>{html.escape(str(module["rel_path"]))}</code></td></tr>
<tr><td>Family</td><td><a href="../families/{html.escape(str(module["family_key"]))}.html"><code>{html.escape(str(module["family_title"]))}</code></a></td></tr>
<tr><td>Kind</td><td>{html.escape(str(module["kind"]))}</td></tr>
<tr><td>Line count</td><td>{module["line_count"]}</td></tr>
<tr><td>Declared procedures</td><td>{len(module["symbols"]["proc"])}</td></tr>
<tr><td>Declared forms/picks</td><td>{len(module["symbols"]["form"]) + len(module["symbols"]["pick"])}</td></tr>
</table>
<p>{html.escape(purpose_for_module(module))}</p>
</section>
<section id="purpose">
<h2>Purpose</h2>
<p>This file should be chosen because of responsibility, not because its name “sounds close enough”. Inside the <code>{html.escape(str(module["family_title"]))}</code> family, it carries one focused part of the contract and keeps that responsibility separate from neighboring concerns.</p>
<ul>{use_cases}</ul>
</section>
<section id="taxonomy">
<h2>Taxonomy</h2>
<p>Think of this page as a generated encyclopedia entry rather than a hand-written tutorial. The goal is to show what kind of module this is, how dense it is, and what reading strategy makes sense before depending on it.</p>
<ul>{inferred_traits}</ul>
</section>
<section id="profile">
<h2>Implementation profile</h2>
<p>This profile is inferred directly from the source text. It does not replace reading the file, but it tells you quickly whether the module is mostly declarative, loop-heavy, branch-heavy, or organized around many small exits.</p>
{render_metrics_table(module)}
</section>
<section id="inventory">
<h2>Top-level API inventory</h2>
{render_inventory_table(module)}
<h3>Imported surfaces</h3>
{render_imports(module["symbols"])}
</section>
<section id="position">
<h2>Position in family</h2>
<p>{family_position_text(module, modules)}</p>
</section>
<section id="declarations">
<h2>Declaration map</h2>
<p>The declaration map turns raw source into a scan-friendly catalog. It is useful when the file is large enough that a reader wants to orient by kinds of surfaces first.</p>
{render_declaration_table(module, ("space", "use", "const", "form", "pick", "proc"), "Declarations")}
</section>
<section id="signatures">
<h2>Representative signatures</h2>
<p>These signatures are shown in source order so the page keeps the feel of a reference manual, not just a keyword cloud.</p>
{render_signature_list(module, ("proc", "form", "pick", "const"), 18)}
</section>
<section id="usage">
<h2>How to use this module</h2>
<p>Start by reading the file as an ownership boundary. Ask three questions: what enters this module, what stable types or procedures it exports, and what adjacent module should stay outside of it.</p>
<ol>{reading_order}</ol>
</section>
<section id="user-example">
<h2>User example</h2>
<p>This example is generated from the actual stdlib module surface. Its job is not to be the smallest snippet possible; its job is to show a realistic consumer-shaped file that exercises the module and mirrors the language keywords the module itself relies on.</p>
<pre><code class="language-vitl">{html.escape(user_example)}</code></pre>
</section>
<section id="keyword-coverage">
<h2>Keyword coverage</h2>
<p>This table makes the “all keywords of the module” requirement auditable. It compares the detected Vitte keywords in the source file with the generated consumer example above.</p>
{render_keyword_coverage(module, missing_keywords)}
</section>
<section id="source-shape">
<h2>Source shape</h2>
<pre><code class="language-vitl">{html.escape(str(module["excerpt"]))}</code></pre>
<p>The excerpt is not meant to replace the file. It exists to make the module recognizable at first glance, the same way a Wikipedia infobox helps the reader orient before reading the whole article.</p>
</section>
<section id="landmarks">
<h2>Source landmarks</h2>
<p>Large files are easier to retain when they have visible landmarks. When the source contains explicit section banners, they are surfaced here; otherwise the first major declarations are used as anchors.</p>
{render_landmarks(module)}
</section>
<section id="organization">
<h2>Source organization</h2>
<p>When a file carries its own internal chaptering, those chapters usually reveal the intended reading order better than a flat symbol list. This section reconstructs that organization from the source itself.</p>
{render_source_section_map(module)}
</section>
<section id="catalog">
<h2>Complete API catalog</h2>
<p>This catalog is the exhaustive file-level index for the module. It is intentionally closer to a generated encyclopedia appendix than to a tutorial summary.</p>
{render_complete_kind_catalog(module)}
</section>
<section id="boundaries">
<h2>Integration boundaries</h2>
<p>Within <code>{html.escape(str(module["family_title"]))}</code>, this file should remain focused. If a future helper changes the host boundary, scheduling boundary, or data-shape boundary, it probably belongs in a neighbor module instead of being added here by convenience.</p>
<ul>
<li>Family responsibility: {html.escape(str(FAMILY_INFO[module["family_key"]]["purpose"]))}</li>
<li>Family architecture role: {html.escape(str(FAMILY_INFO[module["family_key"]]["story"]))}</li>
</ul>
</section>
<section id="composition">
<h2>Composition guidance</h2>
<h3>Choose this module when</h3>
<ul>{choose_when}</ul>
<h3>Pause before extending it when</h3>
<ul>{avoid_when}</ul>
</section>
<section id="relationships">
<h2>Relationship table</h2>
<p>This table keeps the page closer to a real encyclopedia entry: a module is easier to understand when compared with its nearest alternatives in the same family.</p>
{render_relationship_table(module, modules)}
</section>
<section id="neighbors">
<h2>Neighbor modules</h2>
<ul>{sibling_links or '<li>No close sibling module found in the same family bucket.</li>'}</ul>
</section>
<nav class="doc-pagination keyword-doc-pagination">
{f'<a href="{html.escape(str(prev_mod["slug"]))}.html">← {html.escape(str(prev_mod["title"]))}</a>' if prev_mod else '<span></span>'}
<a href="../families/{html.escape(str(module["family_key"]))}.html">Family</a>
{f'<a href="{html.escape(str(next_mod["slug"]))}.html">{html.escape(str(next_mod["title"]))} →</a>' if next_mod else '<span></span>'}
</nav>
"""
    return page_shell(
        f"Stdlib module {module['rel_path']}",
        "../../../",
        body,
        f"modules/{module['slug']}.html",
    )


def family_page(key: str, modules: list[dict[str, object]]) -> str:
    info = FAMILY_INFO[key]
    family_modules = [m for m in modules if m["family_key"] == key]
    inventory = "".join(
        f'<li><a href="../modules/{html.escape(str(m["slug"]))}.html"><code>{html.escape(str(m["rel_path"]))}</code></a></li>'
        for m in family_modules
    )
    examples = "".join(f"<li>{html.escape(item)}</li>" for item in info["example"])  # type: ignore[index]
    responsibilities = "".join(f"<li>{html.escape(item)}</li>" for item in info["responsibilities"])  # type: ignore[index]
    body = f"""
<h1>Stdlib family <code>{html.escape(str(info["title"]))}</code></h1>
<p>This family page is the encyclopedia entry for one stdlib area. It explains why the family exists, what responsibility belongs here, and which files make up the surface.</p>
<section class="lead-panel">
<h2>Overview</h2>
<table border="1" cellpadding="6" cellspacing="0">
<tr><th>Field</th><th>Value</th></tr>
<tr><td>Family</td><td><code>{html.escape(str(info["title"]))}</code></td></tr>
<tr><td>Module count</td><td>{len(family_modules)}</td></tr>
<tr><td>Purpose</td><td>{html.escape(str(info["purpose"]))}</td></tr>
</table>
</section>
<section>
<h2>Architecture role</h2>
<p>{html.escape(str(info["story"]))}</p>
</section>
<section>
<h2>Main responsibilities</h2>
<ul>{responsibilities}</ul>
</section>
<section>
<h2>Real usage story</h2>
<ul>{examples}</ul>
</section>
<section>
<h2>Module inventory</h2>
<ul>{inventory}</ul>
</section>
<nav class="doc-pagination keyword-doc-pagination">
<a href="../index.html">← Stdlib index</a>
<a href="../../stdlib.html">Stdlib chapter</a>
<a href="../../compiler-stdlib-contract.html">Contract →</a>
</nav>
"""
    return page_shell(f"Stdlib family {info['title']}", "../../../", body, f"families/{key}.html")


def index_page(modules: list[dict[str, object]]) -> str:
    family_cards: list[str] = []
    for key in sorted(FAMILY_INFO):
        info = FAMILY_INFO[key]
        family_modules = [m for m in modules if m["family_key"] == key]
        if not family_modules:
            continue
        links = "".join(
            f'<li><a href="modules/{html.escape(str(m["slug"]))}.html"><code>{html.escape(str(m["rel_path"]))}</code></a></li>'
            for m in family_modules[:8]
        )
        family_cards.append(
            f"""
<section class="lead-panel">
<h2><a href="families/{html.escape(key)}.html"><code>{html.escape(str(info["title"]))}</code></a></h2>
<p>{html.escape(str(info["purpose"]))}</p>
<p><strong>Architecture role:</strong> {html.escape(str(info["story"]))}</p>
<p><strong>Visible files:</strong> {len(family_modules)}</p>
<ul>{links}</ul>
</section>
"""
        )

    body = f"""
<h1>Stdlib Reference Atlas</h1>
<p>This atlas turns the Vitte stdlib into a wiki-style reference set: one landing page, one page per family, one page per concrete file, and one portrait SVG per module so the tree stays navigable even when it gets large.</p>
<section class="lead-panel">
<h2>What this corpus covers</h2>
<ul>
<li>Every <code>.vit</code> and <code>.vitl</code> file under <code>src/vitte/stdlib</code>.</li>
<li>Family pages aligned with the actual tree and its generated <code>README.md</code> contracts.</li>
<li>Wiki-style portraits and module infoboxes inspired by the keyword pages.</li>
<li>Searchable HTML pages generated directly from the repository source tree.</li>
</ul>
</section>
<section class="lead-panel">
<h2>Entry points</h2>
<ul>
<li><a href="modules/index.html"><code>modules/</code></a> for the full file-by-file catalog.</li>
<li><a href="families/root.html"><code>families/</code></a> for the responsibility map of stdlib areas.</li>
</ul>
</section>
{''.join(family_cards)}
<nav class="doc-pagination keyword-doc-pagination">
<a href="../stdlib.html">← Stdlib chapter</a>
<a href="../compiler-stdlib-contract.html">Contract</a>
<span></span>
</nav>
"""
    return page_shell("Stdlib Reference Atlas", "../../", body, "index.html")


def modules_index_page(modules: list[dict[str, object]]) -> str:
    family_sections: list[str] = []
    for key in sorted(FAMILY_INFO):
        family_modules = [m for m in modules if m["family_key"] == key]
        if not family_modules:
            continue
        rows = [
            "<table border=\"1\" cellpadding=\"6\" cellspacing=\"0\">",
            "<tr><th>Module</th><th>Kind</th><th>Lines</th><th>Procedures</th><th>Data surfaces</th></tr>",
        ]
        for module in family_modules:
            proc_count = len(module["symbols"]["proc"])  # type: ignore[index]
            data_count = len(module["symbols"]["form"]) + len(module["symbols"]["pick"])  # type: ignore[index]
            rows.append(
                "<tr>"
                f"<td><a href=\"{html.escape(str(module['slug']))}.html\"><code>{html.escape(str(module['rel_path']))}</code></a></td>"
                f"<td>{html.escape(str(module['kind']))}</td>"
                f"<td>{module['line_count']}</td>"
                f"<td>{proc_count}</td>"
                f"<td>{data_count}</td>"
                "</tr>"
            )
        rows.append("</table>")
        family_sections.append(
            f"""
<section class="lead-panel">
<h2><a href="../families/{html.escape(key)}.html"><code>{html.escape(str(FAMILY_INFO[key]["title"]))}</code></a></h2>
<p>{html.escape(str(FAMILY_INFO[key]["purpose"]))}</p>
{''.join(rows)}
</section>
"""
        )

    body = f"""
<h1>Stdlib Modules Directory</h1>
<p>This directory is the file-level doorway into the stdlib atlas. It groups every generated module page by family and exposes enough facts to help the reader choose the right file before opening it.</p>
<section class="lead-panel">
<h2>How to use this directory</h2>
<ul>
<li>Start from the family whose responsibility matches your problem.</li>
<li>Use line count and procedure count as rough complexity signals.</li>
<li>Prefer leaf modules when you want concrete behavior; prefer aggregation modules when you want the family entry surface.</li>
</ul>
</section>
{''.join(family_sections)}
<nav class="doc-pagination keyword-doc-pagination">
<a href="../index.html">← Atlas</a>
<a href="../families/root.html">Families</a>
<span></span>
</nav>
"""
    return page_shell("Stdlib Modules Directory", "../../../", body, "modules/index.html")


def main() -> int:
    MODULES_DIR.mkdir(parents=True, exist_ok=True)
    FAMILIES_DIR.mkdir(parents=True, exist_ok=True)
    PORTRAITS_DIR.mkdir(parents=True, exist_ok=True)
    JSON_DIR.mkdir(parents=True, exist_ok=True)

    modules = read_modules()
    manifest = {
        "generated_from": rel(STDLIB, ROOT),
        "module_count": len(modules),
        "family_count": len({m["family_key"] for m in modules}),
        "modules": [],
    }

    for key in sorted(FAMILY_INFO):
        family_modules = [m for m in modules if m["family_key"] == key]
        if not family_modules:
            continue
        (FAMILIES_DIR / f"{key}.html").write_text(family_page(key, modules), encoding="utf-8")

    for module in modules:
        manifest["modules"].append(
            {
                "slug": module["slug"],
                "path": module["rel_path"],
                "family": module["family_key"],
                "kind": module["kind"],
            }
        )
        (MODULES_DIR / f"{module['slug']}.html").write_text(module_page(module, modules), encoding="utf-8")

    (OUT / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    (OUT / "index.html").write_text(index_page(modules), encoding="utf-8")
    (MODULES_DIR / "index.html").write_text(modules_index_page(modules), encoding="utf-8")
    print(f"generated_stdlib_reference_pages={len(modules)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
