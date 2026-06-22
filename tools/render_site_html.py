#!/usr/bin/env python3
#
# Render Markdown sources to raw HTML documents.
# Shared shell normalization and asset injection are handled later by build_docs_site.py.
from __future__ import annotations

import argparse
import html
import json
import re
import urllib.parse
import urllib.request
from functools import lru_cache
from html.parser import HTMLParser
from pathlib import Path

LINK_RE = re.compile(r"\[([^\]]+)\]\(([^)]+)\)")
CODE_RE = re.compile(r"`([^`]+)`")
EM_RE = re.compile(r"\*\*([^*]+)\*\*")
EM2_RE = re.compile(r"(?<!\*)\*([^*\n]+)\*(?!\*)")
HEADING_RE = re.compile(r"^(#{1,6})\s+(.*)$")
UL_RE = re.compile(r"^(\s*[-*+]\s+)(.*)$")
OL_RE = re.compile(r"^(\s*\d+\.\s+)(.*)$")
HR_RE = re.compile(r"^\s*([-*_])(?:\s*\1){2,}\s*$")
FENCE_RE = re.compile(r"^```(\w+)?\s*$")
TABLE_SEP_RE = re.compile(r"^\s*\|?(?:\s*:?-{3,}:?\s*\|)+\s*$")
RAW_HTML_RE = re.compile(r"^\s*<([A-Za-z!/][^>]*)>")
TAG_RE = re.compile(r"<[^>]+>")
FENCE_BLOCK_RE = re.compile(r"```[\s\S]*?```", re.M)
INLINE_CODE_RE = re.compile(r"`[^`]+`")
VOID_TAGS = {
    "area",
    "base",
    "br",
    "col",
    "embed",
    "hr",
    "img",
    "input",
    "link",
    "meta",
    "param",
    "source",
    "track",
    "wbr",
}


def google_translate(text: str, src: str = "fr", dst: str = "en") -> str:
    text = text.strip()
    if not text:
        return text
    if not re.search(r"[A-Za-zÀ-ÿ]", text):
        return text
    params = urllib.parse.urlencode({
        "client": "gtx",
        "sl": src,
        "tl": dst,
        "dt": "t",
        "q": text,
    })
    url = "https://translate.googleapis.com/translate_a/single?" + params
    with urllib.request.urlopen(url, timeout=30) as resp:
        raw = resp.read().decode("utf-8")
    data = json.loads(raw)
    parts = []
    for chunk in data[0]:
        if chunk and chunk[0]:
            parts.append(chunk[0])
    return "".join(parts)


def split_chunks(text: str, limit: int = 3500) -> list[str]:
    if len(text) <= limit:
        return [text]
    out: list[str] = []
    cur: list[str] = []
    cur_len = 0
    for part in re.split(r"(\n\n+)", text):
        if not part:
            continue
        if cur_len + len(part) > limit and cur:
            out.append("".join(cur))
            cur = [part]
            cur_len = len(part)
        else:
            cur.append(part)
            cur_len += len(part)
    if cur:
        out.append("".join(cur))
    return out


@lru_cache(maxsize=None)
def translate_text(text: str) -> str:
    if not text.strip():
        return text
    prefix = re.match(r"^\s*", text).group(0)
    suffix = re.search(r"\s*$", text).group(0)
    core = text[len(prefix):len(text) - len(suffix)]
    if not re.search(r"[A-Za-zÀ-ÿ]", core):
        return text
    chunks = split_chunks(core)
    translated = []
    for chunk in chunks:
        try:
            translated.append(google_translate(chunk))
        except Exception:
            translated.append(chunk)
    return prefix + "".join(translated) + suffix


def html_escape(text: str) -> str:
    return html.escape(text, quote=True)


def rewrite_href(href: str) -> str:
    if href.startswith(("http://", "https://", "mailto:", "tel:")):
        return href
    if ".md" in href:
        return href.replace(".md", ".html")
    return href


def rel_prefix(md_path: Path, root: Path) -> str:
    rel = md_path.relative_to(root)
    depth = max(0, len(rel.parts) - 1)
    return "../" * depth


NAV_ITEMS = [
    ("Welcome", "index.html", "home.svg"),
    ("Documentation", "doc.html", "docs.svg"),
    ("Download", "download.html", "download.svg"),
    ("Source", "source.html", "source.svg"),
    ("Community", "community.html", "community.svg"),
    ("News", "news.html", "news.svg"),
    ("Diagnostics", "diagnostics.html", "diagnostics.svg"),
    ("Suggestions", "suggestions.html", "suggestions.svg"),
]


def render_search_form(prefix: str) -> str:
    return (
        f'<form class="doc-search" role="search" action="{prefix}search.html" method="get">'
        '<input class="doc-search-input" type="search" name="q" placeholder="Search docs, book, grammar" aria-label="Search documentation" autocomplete="off">'
        '<div class="doc-search-controls" aria-label="Search filters">'
        '<select class="doc-search-filter doc-search-section" name="section" aria-label="Filter by section">'
        '<option value="all">All sections</option><option value="docs">Docs</option><option value="book">Book</option><option value="grammar">Grammar</option>'
        '</select>'
        '<select class="doc-search-filter doc-search-lang" name="lang" aria-label="Filter by language">'
        '<option value="all">All languages</option><option value="en">EN</option><option value="fr">FR</option>'
        '</select>'
        '</div><div class="doc-search-results" hidden></div><div class="doc-search-footer" hidden></div></form>'
    )


def protect_document(text: str) -> tuple[str, list[tuple[str, str]]]:
    placeholders: list[tuple[str, str]] = []

    def add(kind: str, value: str) -> str:
        idx = len(placeholders)
        placeholders.append((kind, value))
        return f"@@{kind}{idx}@@"

    def protect_fence(match: re.Match[str]) -> str:
        return add("FENCE", match.group(0))

    def protect_code(match: re.Match[str]) -> str:
        return add("CODE", match.group(0))

    def protect_link(match: re.Match[str]) -> str:
        label, href = match.group(1), match.group(2)
        token = add("HREF", href)
        return f"[{label}]({token})"

    def protect_tag(match: re.Match[str]) -> str:
        return add("TAG", match.group(0))

    line_safe: list[str] = []
    for line in text.splitlines(keepends=True):
        newline = "\n" if line.endswith("\n") else ""
        core = line[:-1] if newline else line
        m = re.match(r"^(\s*(?:#{1,6}\s+|[-*+]\s+|\d+\.\s+|>\s+))(.+)$", core)
        if m:
            line_safe.append(add("MD", m.group(1)) + m.group(2) + newline)
        else:
            line_safe.append(line)
    safe = "".join(line_safe)
    safe = FENCE_BLOCK_RE.sub(protect_fence, safe)
    safe = INLINE_CODE_RE.sub(protect_code, safe)
    safe = LINK_RE.sub(protect_link, safe)
    safe = TAG_RE.sub(protect_tag, safe)
    return safe, placeholders


def restore_document(text: str, placeholders: list[tuple[str, str]]) -> str:
    for idx, (kind, value) in enumerate(placeholders):
        if kind != "code":
            text = text.replace(f"@@{kind.upper()}{idx}@@", value)
    for idx, (kind, value) in enumerate(placeholders):
        if kind == "code":
            text = text.replace(f"@@{kind.upper()}{idx}@@", value)
    return text


def translate_document_source(text: str) -> str:
    safe, placeholders = protect_document(text)
    safe = safe.replace("\n", "<vitte-nl/>")
    chunks = split_chunks(safe, limit=8000)
    translated = []
    for chunk in chunks:
        translated.append(translate_text(chunk))
    restored = restore_document("".join(translated), placeholders)
    return restored.replace("<vitte-nl/>", "\n")


class FragmentRenderer(HTMLParser):
    def __init__(self, translated: bool = False) -> None:
        super().__init__(convert_charrefs=False)
        self.parts: list[str] = []
        self._skip_translate_depth = 0
        self.translated = translated

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        attr_bits = []
        for name, value in attrs:
            if value is None:
                attr_bits.append(name)
            else:
                if name.lower() == "href":
                    value = rewrite_href(value)
                attr_bits.append(f'{name}="{html_escape(value)}"')
        if attr_bits:
            self.parts.append(f"<{tag} {' '.join(attr_bits)}>")
        else:
            self.parts.append(f"<{tag}>")
        if tag.lower() not in VOID_TAGS:
            if tag.lower() in {"code", "pre", "script", "style", "textarea"}:
                self._skip_translate_depth += 1

    def handle_endtag(self, tag: str) -> None:
        if tag.lower() not in VOID_TAGS:
            self.parts.append(f"</{tag}>")
            if tag.lower() in {"code", "pre", "script", "style", "textarea"}:
                self._skip_translate_depth = max(0, self._skip_translate_depth - 1)

    def handle_startendtag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        attr_bits = []
        for name, value in attrs:
            if value is None:
                attr_bits.append(name)
            else:
                if name.lower() == "href":
                    value = rewrite_href(value)
                attr_bits.append(f'{name}="{html_escape(value)}"')
        if attr_bits:
            self.parts.append(f"<{tag} {' '.join(attr_bits)} />")
        else:
            self.parts.append(f"<{tag} />")

    def handle_data(self, data: str) -> None:
        if self._skip_translate_depth:
            self.parts.append(html_escape(data))
        else:
            self.parts.append(data if self.translated else translate_text(data))

    def handle_entityref(self, name: str) -> None:
        self.parts.append(f"&{name};")

    def handle_charref(self, name: str) -> None:
        self.parts.append(f"&#{name};")

    def handle_comment(self, data: str) -> None:
        self.parts.append(f"<!--{data}-->")

    def get_html(self) -> str:
        return "".join(self.parts)


def translate_html_fragment(fragment: str, translated: bool = False) -> str:
    parser = FragmentRenderer(translated=translated)
    parser.feed(fragment)
    parser.close()
    return parser.get_html()


def inline_to_html(text: str, translated: bool = False) -> str:
    placeholders: list[tuple[str, str]] = []

    def protect_code(match: re.Match[str]) -> str:
        idx = len(placeholders)
        placeholders.append(("code", match.group(1)))
        return f"@@CODE{idx}@@"

    def protect_link(match: re.Match[str]) -> str:
        idx = len(placeholders)
        label = match.group(1)
        href = match.group(2)
        placeholders.append(("link", f"{label}\n{href}"))
        return f"@@LINK{idx}@@"

    safe = CODE_RE.sub(protect_code, text)
    safe = LINK_RE.sub(protect_link, safe)
    if not translated:
        safe = translate_text(safe)
    safe = EM_RE.sub(r"<strong>\1</strong>", safe)
    safe = EM2_RE.sub(r"<em>\1</em>", safe)
    for idx, (kind, payload) in enumerate(placeholders):
        if kind != "link":
            continue
        token = f"@@{kind.upper()}{idx}@@"
        label, href = payload.split("\n", 1)
        safe_label = label if translated else translate_text(label)
        safe_href = html_escape(rewrite_href(href))
        safe = safe.replace(token, f'<a href="{safe_href}">{html_escape(safe_label)}</a>')
    for idx, (kind, payload) in enumerate(placeholders):
        if kind != "code":
            continue
        token = f"@@{kind.upper()}{idx}@@"
        safe = safe.replace(token, f"<code>{html_escape(payload)}</code>")
    return safe


def render_table(rows: list[list[str]], translated: bool = False) -> str:
    if not rows:
        return ""
    header = rows[0]
    body = rows[1:] if len(rows) > 1 else []
    parts = ['<table border="1" cellpadding="6" cellspacing="0">']
    if header:
        parts.append("<tr>" + "".join(f"<th>{inline_to_html(cell, translated=translated)}</th>" for cell in header) + "</tr>")
    for row in body:
        parts.append("<tr>" + "".join(f"<td>{inline_to_html(cell, translated=translated)}</td>" for cell in row) + "</tr>")
    parts.append("</table>")
    return "\n".join(parts)


def parse_table(lines: list[str], start: int, translated: bool = False) -> tuple[str, int]:
    rows: list[list[str]] = []
    i = start
    while i < len(lines) and "|" in lines[i]:
        row = lines[i].strip().strip("|")
        rows.append([cell.strip() for cell in row.split("|")])
        i += 1
    if len(rows) >= 2 and TABLE_SEP_RE.match(lines[start + 1] if start + 1 < len(lines) else ""):
        rows = [rows[0]] + rows[2:]
    return render_table(rows, translated=translated), i


def render_markdown(text: str, translated: bool = False) -> str:
    lines = text.splitlines()
    out: list[str] = []
    paragraph: list[str] = []
    i = 0

    def flush_paragraph() -> None:
        nonlocal paragraph
        if paragraph:
            raw = " ".join(s.strip() for s in paragraph).strip()
            if raw:
                out.append(f"<p>{inline_to_html(raw, translated=translated)}</p>")
            paragraph = []

    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        fence = FENCE_RE.match(stripped)
        if fence:
            flush_paragraph()
            lang = fence.group(1) or ""
            code_lines: list[str] = []
            i += 1
            while i < len(lines) and not FENCE_RE.match(lines[i].strip()):
                code_lines.append(lines[i])
                i += 1
            out.append(
                f'<pre><code class="language-{html_escape(lang)}">{html_escape("\n".join(code_lines))}</code></pre>'
            )
            i += 1
            continue

        if not stripped:
            flush_paragraph()
            i += 1
            continue

        if HEADING_RE.match(stripped):
            flush_paragraph()
            hashes, content = HEADING_RE.match(stripped).groups()
            level = len(hashes)
            out.append(f"<h{level}>{inline_to_html(content, translated=translated)}</h{level}>")
            i += 1
            continue

        if HR_RE.match(stripped):
            flush_paragraph()
            out.append("<hr>")
            i += 1
            continue

        if i + 1 < len(lines) and "|" in line and TABLE_SEP_RE.match(lines[i + 1].strip()):
            flush_paragraph()
            table_html, new_i = parse_table(lines, i, translated=translated)
            out.append(table_html)
            i = new_i
            continue

        if UL_RE.match(line) or OL_RE.match(line):
            flush_paragraph()
            list_lines: list[str] = []
            list_tag = "ul" if UL_RE.match(line) else "ol"
            while i < len(lines) and (UL_RE.match(lines[i]) or OL_RE.match(lines[i])):
                list_lines.append(lines[i])
                i += 1
            out.append(f"<{list_tag}>")
            for item in list_lines:
                m = UL_RE.match(item) or OL_RE.match(item)
                assert m is not None
                out.append(f"<li>{inline_to_html(m.group(2).strip(), translated=translated)}</li>")
            out.append(f"</{list_tag}>")
            continue

        if stripped.startswith(">"):
            flush_paragraph()
            quote_lines: list[str] = []
            while i < len(lines) and lines[i].lstrip().startswith(">"):
                quote_lines.append(lines[i].lstrip()[1:].lstrip())
                i += 1
            quote_html = " ".join(quote_lines)
            out.append(f"<blockquote><p>{inline_to_html(quote_html, translated=translated)}</p></blockquote>")
            continue

        if line.lstrip().startswith("<") and not line.lstrip().startswith("<!"):
            flush_paragraph()
            raw_lines: list[str] = []
            while i < len(lines) and lines[i].strip():
                raw_lines.append(lines[i])
                i += 1
            out.append(translate_html_fragment("\n".join(raw_lines), translated=translated))
            continue

        paragraph.append(line)
        i += 1

    flush_paragraph()
    return "\n".join(out)


def extract_title(text: str, fallback: str) -> str:
    for line in text.splitlines():
        m = HEADING_RE.match(line.strip())
        if m and len(m.group(1)) == 1:
            return m.group(2).strip()
    return fallback


def site_shell(body: str, title: str, md_path: Path, root: Path) -> str:
    prefix = rel_prefix(md_path, root)
    nav_html = "".join(
        f'<li><a class="nav-chip" href="{prefix}{href}"><svg width="14" height="14" aria-hidden="true" focusable="false"><use href="{prefix}svg/sprite.svg#i-{"home" if icon == "home.svg" else "news" if icon == "news.svg" else "docs"}"></use></svg><span>{html_escape(label)}</span></a></li>'
        for label, href, icon in NAV_ITEMS
    )
    return "\n".join([
        "<!doctype html>",
        '<html lang="en">',
        "<head>",
        '<meta charset="utf-8">',
        '<meta name="viewport" content="width=device-width, initial-scale=1">',
        f"<title>{html_escape(title)}</title>",
        f'<link rel="icon" href="{prefix}svg/logo.svg" type="image/svg+xml">',
        f'<link rel="stylesheet" href="{prefix}css/site.css">',
        f'<link rel="stylesheet" href="{prefix}css/print.css" media="print">',
        "</head>",
        '<body class="classic-doc">',
        '<div class="site-shell">',
        '<header class="site-header">',
        f'<a class="site-brand" href="{prefix}index.html"><img class="site-brand-mark" src="{prefix}svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a>',
        '<nav class="site-nav" aria-label="Primary">',
        f'<ul class="nav-band">{nav_html}</ul>',
        "</nav>",
        render_search_form(prefix),
        "</header>",
        '<main id="main-content" class="site-main">',
        '<article class="doc-content">',
        body,
        "</article>",
        "</main>",
        '<footer class="site-footer">',
        f'<p class="site-footer-path">{html_escape(md_path.relative_to(root).with_suffix(".html").as_posix())}</p>',
        f'<p><a href="{prefix}index.html">Back to home</a></p>',
        "</footer>",
        "</div>",
        f'<script type="module" src="{prefix}js/main.js"></script>',
        "</body>",
        "</html>",
        "",
    ])


def build_html(md_path: Path, root: Path) -> tuple[Path, str]:
    src = md_path.read_text(encoding="utf-8", errors="ignore")
    body = render_markdown(src)
    title = extract_title(src, md_path.stem)
    rel = md_path.relative_to(root)
    out_path = root / rel.with_suffix(".html")
    html_doc = site_shell(body, title, md_path, root)
    return out_path, html_doc


def main() -> int:
    ap = argparse.ArgumentParser(description="Render docs markdown to raw HTML pages in English")
    ap.add_argument("--root", default="docs")
    ap.add_argument("--write", action="store_true", default=True)
    ap.add_argument("--shard-index", type=int, default=0)
    ap.add_argument("--shard-count", type=int, default=1)
    args = ap.parse_args()

    root = Path(args.root)
    md_files = sorted(root.rglob("*.md"))
    if args.shard_count < 1:
        raise SystemExit("--shard-count must be >= 1")
    if not (0 <= args.shard_index < args.shard_count):
        raise SystemExit("--shard-index must satisfy 0 <= index < count")
    md_files = [p for idx, p in enumerate(md_files) if idx % args.shard_count == args.shard_index]
    changed = 0
    for idx, md_path in enumerate(md_files, 1):
        out_path, html_doc = build_html(md_path, root)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        old = out_path.read_text(encoding="utf-8", errors="ignore") if out_path.exists() else None
        if old != html_doc:
            out_path.write_text(html_doc, encoding="utf-8")
            changed += 1
        if idx % 10 == 0:
            print(f"shard={args.shard_index}/{args.shard_count} processed={idx} written={changed}")
    print(f"html_written={changed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
