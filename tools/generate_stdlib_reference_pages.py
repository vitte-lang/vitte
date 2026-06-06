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
<meta http-equiv="Content-Security-Policy" content="default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; font-src 'self'; connect-src 'self';">
</head>
<body class="classic-doc">
<a class="skip-link" href="#main-content">Skip to content</a>
<div class="site-shell">
<header class="site-header">
<a class="site-brand" href="{rel_root}index.html"><img class="site-brand-mark" src="{rel_root}svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a>
<nav class="site-nav" aria-label="Primary">
<ul class="nav-band">
<li><a class="nav-chip" href="{rel_root}index.html"><img src="{rel_root}svg/home.svg" alt="" width="14" height="14" aria-hidden="true"><span>Welcome</span></a></li>
<li><a class="nav-chip" href="{rel_root}doc.html"><img src="{rel_root}svg/docs.svg" alt="" width="14" height="14" aria-hidden="true"><span>Documentation</span></a></li>
<li><a class="nav-chip" href="{rel_root}book/stdlib.html"><img src="{rel_root}svg/docs.svg" alt="" width="14" height="14" aria-hidden="true"><span>Stdlib</span></a></li>
<li><a class="nav-chip" href="{rel_root}book/compiler-stdlib-contract.html"><img src="{rel_root}svg/docs.svg" alt="" width="14" height="14" aria-hidden="true"><span>Contract</span></a></li>
<li><a class="nav-chip" href="{rel_root}source.html"><img src="{rel_root}svg/source.svg" alt="" width="14" height="14" aria-hidden="true"><span>Source</span></a></li>
</ul>
</nav>
</header>
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
    for raw in text.splitlines():
        line = raw.rstrip()
        if not line.strip():
            continue
        if line.strip().startswith("<<<"):
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
    }


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
        "siblings": [s["rel_path"] for s in siblings[:8]],
    }


def render_inventory_table(module: dict[str, object]) -> str:
    symbols = module["symbols"]  # type: ignore[assignment]
    rows = [
        ("Procedures", symbols["proc"]),  # type: ignore[index]
        ("Forms", symbols["form"]),  # type: ignore[index]
        ("Picks", symbols["pick"]),  # type: ignore[index]
        ("Constants", symbols["const"]),  # type: ignore[index]
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
<li><a href="#inventory">Top-level API inventory</a></li>
<li><a href="#usage">How to use this module</a></li>
<li><a href="#source-shape">Source shape</a></li>
<li><a href="#boundaries">Integration boundaries</a></li>
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
<section id="inventory">
<h2>Top-level API inventory</h2>
{render_inventory_table(module)}
<h3>Imported surfaces</h3>
{render_imports(module["symbols"])}
</section>
<section id="usage">
<h2>How to use this module</h2>
<p>Start by reading the file as an ownership boundary. Ask three questions: what enters this module, what stable types or procedures it exports, and what adjacent module should stay outside of it.</p>
<ol>
<li>Open the family page first to understand why this area of the stdlib exists.</li>
<li>Read the source excerpt below to see the namespace, imports, and first declared surfaces.</li>
<li>Check the neighbor list to avoid coupling this module with an adjacent responsibility by habit.</li>
</ol>
</section>
<section id="source-shape">
<h2>Source shape</h2>
<pre><code class="language-vitl">{html.escape(str(module["excerpt"]))}</code></pre>
<p>The excerpt is not meant to replace the file. It exists to make the module recognizable at first glance, the same way a Wikipedia infobox helps the reader orient before reading the whole article.</p>
</section>
<section id="boundaries">
<h2>Integration boundaries</h2>
<p>Within <code>{html.escape(str(module["family_title"]))}</code>, this file should remain focused. If a future helper changes the host boundary, scheduling boundary, or data-shape boundary, it probably belongs in a neighbor module instead of being added here by convenience.</p>
<ul>
<li>Family responsibility: {html.escape(str(FAMILY_INFO[module["family_key"]]["purpose"]))}</li>
<li>Family architecture role: {html.escape(str(FAMILY_INFO[module["family_key"]]["story"]))}</li>
</ul>
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
{''.join(family_cards)}
<nav class="doc-pagination keyword-doc-pagination">
<a href="../stdlib.html">← Stdlib chapter</a>
<a href="../compiler-stdlib-contract.html">Contract</a>
<span></span>
</nav>
"""
    return page_shell("Stdlib Reference Atlas", "../../", body, "index.html")


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
    print(f"generated_stdlib_reference_pages={len(modules)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
