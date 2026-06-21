#!/usr/bin/env python3
from __future__ import annotations

import html
import os
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DOCS_ROOT = ROOT / "docs"


def slug_title(path: Path) -> str:
    return path.stem.replace("_", " ").replace("-", " ").strip().title()


def load_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def html_target_exists(base_dir: Path, href: str) -> bool:
    candidate = (base_dir / href).resolve()
    if candidate.suffix.lower() == ".md":
        return candidate.with_suffix(".html").exists()
    return candidate.exists()


def rewrite_href(base_dir: Path, href: str) -> str:
    if href.startswith(("http://", "https://", "mailto:", "#")):
        return href
    if href.endswith(".md") and html_target_exists(base_dir, href):
        return href[:-3] + ".html"
    return href


INLINE_LINK_RE = re.compile(r"\[([^\]]+)\]\(([^)]+)\)")
INLINE_CODE_RE = re.compile(r"`([^`]+)`")
BOLD_RE = re.compile(r"\*\*([^*]+)\*\*")
ITALIC_RE = re.compile(r"(?<!\*)\*([^*\n]+)\*(?!\*)")


def convert_inline(text: str, base_dir: Path) -> str:
    escaped = html.escape(text)

    def repl_link(match: re.Match[str]) -> str:
        label = html.escape(match.group(1))
        href = html.escape(rewrite_href(base_dir, match.group(2)))
        return f'<a href="{href}">{label}</a>'

    escaped = INLINE_LINK_RE.sub(repl_link, escaped)
    escaped = INLINE_CODE_RE.sub(lambda m: f"<code>{html.escape(m.group(1))}</code>", escaped)
    escaped = BOLD_RE.sub(lambda m: f"<strong>{html.escape(m.group(1))}</strong>", escaped)
    escaped = ITALIC_RE.sub(lambda m: f"<em>{html.escape(m.group(1))}</em>", escaped)
    return escaped


def consume_paragraph(lines: list[str], i: int, base_dir: Path) -> tuple[str, int]:
    parts: list[str] = []
    while i < len(lines):
        stripped = lines[i].strip()
        if not stripped:
            break
        if stripped.startswith(("#", "- ", "* ", "|", "```")):
            break
        if re.match(r"\d+\.\s", stripped):
            break
        if stripped == "---":
            break
        parts.append(stripped)
        i += 1
    return f"<p>{convert_inline(' '.join(parts), base_dir)}</p>", i


def consume_list(lines: list[str], i: int, base_dir: Path) -> tuple[str, int]:
    ordered = bool(re.match(r"\d+\.\s", lines[i].strip()))
    tag = "ol" if ordered else "ul"
    items: list[str] = []
    while i < len(lines):
        stripped = lines[i].strip()
        if not stripped:
            break
        if ordered:
            match = re.match(r"(\d+)\.\s+(.*)", stripped)
        else:
            match = re.match(r"[-*]\s+(.*)", stripped)
        if not match:
            break
        item_parts = [match.group(2) if ordered else match.group(1)]
        i += 1
        while i < len(lines):
            continuation = lines[i].rstrip("\n")
            continuation_stripped = continuation.strip()
            if not continuation_stripped:
                break
            if continuation_stripped.startswith(("#", "```", "|")):
                break
            if continuation_stripped == "---":
                break
            if ordered and re.match(r"\d+\.\s", continuation_stripped):
                break
            if not ordered and re.match(r"[-*]\s", continuation_stripped):
                break
            item_parts.append(continuation_stripped)
            i += 1
        item = " ".join(item_parts)
        item = item.replace("[x] ", "Checked: ").replace("[ ] ", "Open: ")
        items.append(f"<li>{convert_inline(item, base_dir)}</li>")
    return f"<{tag}>\n" + "\n".join(items) + f"\n</{tag}>", i


def consume_code(lines: list[str], i: int) -> tuple[str, int]:
    fence = lines[i].strip()
    lang = fence[3:].strip()
    i += 1
    body: list[str] = []
    while i < len(lines) and lines[i].strip() != "```":
        body.append(lines[i].rstrip("\n"))
        i += 1
    if i < len(lines):
        i += 1
    klass = f' class="language-{html.escape(lang)}"' if lang else ""
    return f"<pre><code{klass}>{html.escape(chr(10).join(body))}</code></pre>", i


def consume_table(lines: list[str], i: int, base_dir: Path) -> tuple[str, int]:
    raw: list[str] = []
    while i < len(lines):
        stripped = lines[i].rstrip()
        if not stripped.startswith("|"):
            break
        raw.append(stripped)
        i += 1
    rows = []
    for line in raw:
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if cells and all(set(cell) <= {"-", ":"} for cell in cells):
            continue
        rows.append(cells)
    if not rows:
        return "", i
    head = rows[0]
    body = rows[1:]
    out = ["<table>", "<tr>" + "".join(f"<th>{convert_inline(c, base_dir)}</th>" for c in head) + "</tr>"]
    for row in body:
        out.append("<tr>" + "".join(f"<td>{convert_inline(c, base_dir)}</td>" for c in row) + "</tr>")
    out.append("</table>")
    return "\n".join(out), i


def markdown_to_html(text: str, source_path: Path) -> tuple[str, str]:
    lines = text.splitlines()
    base_dir = source_path.parent
    title = slug_title(source_path)
    body: list[str] = []
    i = 0
    while i < len(lines):
        line = lines[i].rstrip("\n")
        stripped = line.strip()
        if not stripped:
            i += 1
            continue
        if stripped.startswith("```"):
            block, i = consume_code(lines, i)
            body.append(block)
            continue
        if stripped == "---":
            body.append("<hr>")
            i += 1
            continue
        if stripped.startswith("|"):
            block, i = consume_table(lines, i, base_dir)
            if block:
                body.append(block)
            continue
        if stripped.startswith("#"):
            level = len(stripped) - len(stripped.lstrip("#"))
            content = stripped[level:].strip()
            if level == 1:
                title = content
            body.append(f"<h{level}>{convert_inline(content, base_dir)}</h{level}>")
            i += 1
            continue
        if stripped.startswith(("- ", "* ")) or re.match(r"\d+\.\s", stripped):
            block, i = consume_list(lines, i, base_dir)
            body.append(block)
            continue
        paragraph, i = consume_paragraph(lines, i, base_dir)
        body.append(paragraph)
    return title, "\n".join(body)


def relative_prefix(path: Path) -> str:
    depth = len(path.relative_to(DOCS_ROOT).parts) - 1
    return "../" * depth


def render_document(source_path: Path) -> str:
    rel_source = source_path.relative_to(DOCS_ROOT)
    html_path = source_path.with_suffix(".html")
    prefix = relative_prefix(html_path)
    title, body_html = markdown_to_html(load_text(source_path), source_path)
    canonical = f"https://vitte-lang.org/{html_path.relative_to(DOCS_ROOT).as_posix()}"
    source_href = rel_source.name
    back_to_docs = prefix + "doc.html"
    back_to_source = prefix + "source.html"
    back_to_sitemap = prefix + "sitemap.html"
    home = prefix + "index.html"
    css = prefix + "css/site.css?v=34fa1e8389"
    print_css = prefix + "css/print.css"
    js = prefix + "js/main.js"
    icon = prefix + "svg/logo.svg"
    sprite = prefix + "svg/sprite.svg"
    return f"""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(title)}</title>
<link rel="icon" href="{icon}" type="image/svg+xml">
<link rel="stylesheet" href="{css}">
<link rel="stylesheet" href="{print_css}" media="print">
<link rel="canonical" href="{canonical}">
<meta property="og:title" content="{html.escape(title)}">
<meta property="og:type" content="article">
<meta property="og:url" content="{canonical}">
<meta name="twitter:card" content="summary">
<meta http-equiv="Content-Security-Policy" content="default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none';">
</head>
<body class="classic-doc">
<a class="skip-link" href="#main-content">Skip to content</a>
<div class="site-shell">
<header class="site-header">
<a class="site-brand" href="{home}"><img class="site-brand-mark" src="{icon}" alt="" width="32" height="32"><span>Vitte</span></a>
<nav class="site-nav" aria-label="Primary"><ul class="nav-band"><li><a class="nav-chip" href="{home}"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{sprite}#i-home"></use></svg><span>Welcome</span></a></li><li><a class="nav-chip" href="{back_to_docs}"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{sprite}#i-docs"></use></svg><span>Documentation</span></a></li><li><a class="nav-chip" href="{back_to_source}"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{sprite}#i-docs"></use></svg><span>Source</span></a></li><li><a class="nav-chip" href="{prefix}news.html"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{sprite}#i-news"></use></svg><span>News</span></a></li></ul></nav>
</header>
<main id="main-content" class="site-main">
<article class="doc-content">
<center>
<h1>{html.escape(title)}</h1>
<p>HTML reading page generated from the source Markdown document.</p>
</center>
<div class="page-band">
<a href="{back_to_docs}"><img src="{prefix}svg/docs.svg" alt="" width="14" height="14">Documentation</a>
<a href="{back_to_source}"><img src="{prefix}svg/source.svg" alt="" width="14" height="14">Source</a>
<a href="{back_to_sitemap}"><img src="{prefix}svg/summary.svg" alt="" width="14" height="14">Sitemap</a>
<a href="{source_href}"><img src="{prefix}svg/diagnostics.svg" alt="" width="14" height="14">Source Markdown</a>
</div>
<div class="lead-panel">
<h2>Reading mode</h2>
<p>This page is the static HTML reader for <code>{html.escape(rel_source.as_posix())}</code>. Internal Markdown links are rewritten to HTML when a matching reading page exists.</p>
</div>
{body_html}
</article>
</main>
<footer class="site-footer">
<p class="site-footer-path">{html.escape(html_path.relative_to(DOCS_ROOT).as_posix())}</p>
<p><a href="{home}">Back to home</a> · <a href="{prefix}status-public.html">Public status</a></p>
</footer>
</div>
<script type="module" src="{js}"></script>
</body>
</html>
"""


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print("usage: render_docs_markdown.py docs/file.md [...]", file=sys.stderr)
        return 1
    for raw in argv[1:]:
        source_path = (ROOT / raw).resolve()
        if not source_path.exists():
            print(f"missing: {raw}", file=sys.stderr)
            return 2
        if source_path.suffix.lower() != ".md":
            print(f"not markdown: {raw}", file=sys.stderr)
            return 3
        output = source_path.with_suffix(".html")
        output.write_text(render_document(source_path), encoding="utf-8")
        print(output.relative_to(ROOT).as_posix())
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
