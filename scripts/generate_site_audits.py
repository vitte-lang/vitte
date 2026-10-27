#!/usr/bin/env python3
"""Generate and verify the static documentation audit reports."""

from __future__ import annotations

import argparse
import hashlib
import html
from html.parser import HTMLParser
import json
import re
from datetime import datetime, timezone
from pathlib import Path
from urllib.parse import unquote, urlsplit
from zoneinfo import ZoneInfo


ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / "docs"
REPORTS = {
    "accessibility-report.html",
    "broken-links.html",
    "seo-report.html",
    "site-statistics.html",
}
GENERATED = REPORTS | {
    "assets.json",
    "build-manifest.json",
    "status.json",
    "status.html",
    "status-public.html",
}
SEARCH_INDEXES = {
    "index.html",
    "doc.html",
    "demarrer.html",
    "start-here.html",
    "project-tutorials.html",
    "download.html",
    "diagnostics.html",
    "source.html",
    "source-guides.html",
    "sitemap.html",
    "docs-tree.html",
    "ressources-secondaires.html",
    "BOOTSTRAP_NATIVE_MIGRATION_FROM_RUST.html",
    "BOOTSTRAP_VITTE_FILE_MAPPING.html",
    "roadmap_161_200_pro.html",
    "roadmap_161_200_pro_en.html",
    "roadmap_161_200_quarterly.html",
    "reports/architecture-docs-summary.html",
    "COMPILER_DRIVER_MIGRATION.html",
    "MODULE_REFERENCE.html",
    "bootstrap/self_host.html",
    "MD_MIGRATION.html",
    "diagnostics.html",
    "suggestions.html",
    "book/index.html",
    "book/summary.html",
    "book/chapters/21-cli-project.html",
    "book/chapters/22-http-project.html",
    "book/chapters/23-system-project.html",
    "book/chapters/24-kv-project.html",
    "book/chapters/25-arduino-project.html",
    "book/chapters/26-editor-project.html",
    "compiler/backend.html",
    "compiler/pipeline.html",
    "compiler/architecture.html",
    "compiler/build.html",
    "compiler/selfhost_core_subset.html",
    "spec/grammar.html",
    "grammar/index.html",
    "book/grammar/railroad/README.html",
    "book/grammar/railroad/index.html",
    "grammar/railroad_index.html",
    "accessibility-report.html",
    "broken-links.html",
    "seo-report.html",
    "site-statistics.html",
    "status-public.html",
}
SOURCE_SUFFIXES = {".html", ".md", ".css", ".js", ".svg", ".json", ".xml", ".txt"}
VOID_TAGS = {
    "area", "base", "br", "col", "embed", "hr", "img", "input", "link",
    "meta", "param", "source", "track", "wbr",
}


class PageParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.stack: list[dict[str, object]] = []
        self.ids: dict[str, str] = {}
        self.images: list[dict[str, str]] = []
        self.controls: list[dict[str, object]] = []
        self.links: list[tuple[str, str]] = []
        self.meta: dict[str, str] = {}
        self.canonical = ""
        self.title = ""
        self.h1_text: list[str] = []
        self.lang = "en"

    def handle_startendtag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        self.handle_starttag(tag, attrs)
        if tag not in VOID_TAGS:
            self.handle_endtag(tag)

    def handle_endtag(self, tag: str) -> None:
        for index in range(len(self.stack) - 1, -1, -1):
            entry = self.stack[index]
            if entry["tag"] != tag:
                continue
            element_id = str(entry.get("id", ""))
            if element_id:
                self.ids[element_id] = str(entry.get("id_text", ""))
            del self.stack[index:]
            break

    def handle_data(self, data: str) -> None:
        for entry in self.stack:
            element_id = str(entry.get("id", ""))
            if element_id:
                entry["id_text"] = str(entry.get("id_text", "")) + data
            label_index = entry.get("label_index")
            if label_index is not None:
                self._labels[int(label_index)]["text"] += data
            control_index = entry.get("control_index")
            if control_index is not None:
                control = self.controls[int(control_index)]
                if control["tag"] == "button":
                    control["button_text"] = str(control["button_text"]) + data
            if entry["tag"] == "title":
                self.title += data
            elif entry["tag"] == "h1":
                self.h1_text[int(entry["h1_index"])] += data

    @property
    def _labels(self) -> list[dict[str, str]]:
        if not hasattr(self, "__labels"):
            self.__labels: list[dict[str, str]] = []
        return self.__labels

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:  # type: ignore[override]
        values = {key.lower(): value or "" for key, value in attrs}
        if tag == "html":
            self.lang = values.get("lang", "en")
        h1_index = None
        if tag == "h1":
            self.h1_text.append("")
            h1_index = len(self.h1_text) - 1
        if tag == "img":
            self.images.append(values)
        for attr in ("href", "src", "poster"):
            ref = values.get(attr, "").strip()
            if ref:
                self.links.append((attr, ref))
        if tag == "meta":
            name = values.get("name", "").lower()
            if name:
                self.meta[name] = values.get("content", "")
        if tag == "link" and "canonical" in values.get("rel", "").lower().split():
            self.canonical = values.get("href", "")

        label_index = None
        if tag == "label":
            self._labels.append({"for": values.get("for", ""), "text": ""})
            label_index = len(self._labels) - 1
        control_index = None
        control_type = values.get("type", "").lower()
        if tag in {"input", "select", "textarea", "button"} and not (
            tag == "input" and control_type == "hidden"
        ):
            implicit = next(
                (entry.get("label_index") for entry in reversed(self.stack)
                 if entry.get("tag") == "label"),
                None,
            )
            self.controls.append({
                "tag": tag,
                "type": control_type,
                "attrs": values,
                "button_text": "",
                "implicit_label": implicit,
            })
            control_index = len(self.controls) - 1
        self.stack.append({
            "tag": tag,
            "id": values.get("id", ""),
            "id_text": "",
            "label_index": label_index,
            "control_index": control_index,
            "h1_index": h1_index,
        })


def parse_page(path: Path) -> PageParser:
    parser = PageParser()
    parser.feed(path.read_text(encoding="utf-8"))
    return parser


def accessible_control_name(control: dict[str, object], page: PageParser) -> bool:
    attrs = control["attrs"]
    assert isinstance(attrs, dict)
    if attrs.get("aria-label", "").strip() or attrs.get("title", "").strip():
        return True
    labelledby = attrs.get("aria-labelledby", "").split()
    if any(page.ids.get(target, "").strip() for target in labelledby):
        return True
    control_id = attrs.get("id", "")
    if control_id and any(
        label["for"] == control_id and label["text"].strip()
        for label in page._labels
    ):
        return True
    implicit = control.get("implicit_label")
    if implicit is not None and page._labels[int(implicit)]["text"].strip():
        return True
    if control["tag"] == "button" and str(control.get("button_text", "")).strip():
        return True
    if control["tag"] == "input" and control["type"] in {"button", "submit", "reset", "image"}:
        return bool(attrs.get("value", "").strip() or attrs.get("alt", "").strip())
    return False


def local_target(source: Path, ref: str) -> tuple[Path | None, str]:
    parsed = urlsplit(ref.strip())
    if parsed.scheme.lower() in {"http", "https", "mailto", "tel", "data", "javascript", "blob"}:
        if parsed.hostname and parsed.hostname.lower() == "vitte-lang.org":
            pass
        else:
            return None, ""
    elif ref.startswith("//"):
        return None, ""
    raw_path = unquote(parsed.path)
    if not raw_path:
        target = source
    elif raw_path.startswith("/"):
        target = DOCS / raw_path.lstrip("/")
    else:
        target = source.parent / raw_path
    target = target.resolve()
    docs_resolved = DOCS.resolve()
    try:
        target.relative_to(docs_resolved)
    except ValueError:
        return None, ""
    if target.is_dir():
        target = target / "index.html"
    return target, unquote(parsed.fragment)


def svg_ids(path: Path) -> set[str]:
    text = path.read_text(encoding="utf-8", errors="replace")
    return set(re.findall(r'\bid=["\']([^"\']+)', text))


def audit_links(pages: dict[Path, PageParser]) -> tuple[list[str], list[str]]:
    anchors = {path: set(page.ids) for path, page in pages.items()}
    missing_targets: list[str] = []
    missing_fragments: list[str] = []
    for source, page in pages.items():
        for attr, ref in page.links:
            target, fragment = local_target(source, ref)
            if target is None:
                continue
            if not target.is_file():
                missing_targets.append(f"{source.relative_to(DOCS)}: {attr}={ref}")
                continue
            if fragment and target.suffix.lower() in {".html", ".svg"}:
                valid = anchors.get(target, set()) if target.suffix.lower() == ".html" else svg_ids(target)
                if fragment not in valid:
                    missing_fragments.append(f"{source.relative_to(DOCS)}: {ref}")
    return missing_targets, missing_fragments


def scan() -> dict[str, object]:
    html_paths = sorted(DOCS.rglob("*.html"))
    pages = {path: parse_page(path) for path in html_paths}
    missing_alt: list[str] = []
    unnamed_controls: list[str] = []
    missing_titles: list[str] = []
    missing_h1: list[str] = []
    missing_viewports: list[str] = []
    missing_canonicals: list[str] = []
    noindex: list[str] = []

    for path, page in pages.items():
        label = path.relative_to(DOCS).as_posix()
        if not page.title.strip():
            missing_titles.append(label)
        if not any(text.strip() for text in page.h1_text):
            missing_h1.append(label)
        if not page.meta.get("viewport", "").strip():
            missing_viewports.append(label)
        is_noindex = "noindex" in page.meta.get("robots", "").lower().split(",")
        if is_noindex:
            noindex.append(label)
        elif not page.canonical.strip():
            missing_canonicals.append(label)
        for image in page.images:
            if "alt" not in image:
                missing_alt.append(label)
        for control in page.controls:
            if not accessible_control_name(control, page):
                attrs = control["attrs"]
                assert isinstance(attrs, dict)
                desc = attrs.get("id") or attrs.get("name") or attrs.get("type") or control["tag"]
                unnamed_controls.append(f"{label}: {desc}")

    missing_targets, missing_fragments = audit_links(pages)
    files = [path for path in DOCS.rglob("*") if path.is_file()]
    by_ext = {ext: sum(path.suffix.lower() == ext for path in files) for ext in (".html", ".css", ".js", ".svg", ".json")}
    return {
        "pages": pages,
        "missing_alt": missing_alt,
        "unnamed_controls": unnamed_controls,
        "missing_titles": missing_titles,
        "missing_h1": missing_h1,
        "missing_viewports": missing_viewports,
        "missing_canonicals": missing_canonicals,
        "noindex": noindex,
        "missing_targets": missing_targets,
        "missing_fragments": missing_fragments,
        "file_counts": by_ext,
    }


def source_version() -> str:
    excluded = set(GENERATED)
    digest = hashlib.sha256()
    paths = [path for path in DOCS.rglob("*") if path.is_file()]
    paths.append(ROOT / "scripts/generate_site_audits.py")
    normalized_index_date = re.compile(
        r'(<strong>Manifest build:</strong>.*?<strong>Date:</strong> )[^<]+', re.S
    )
    for path in sorted(set(paths), key=lambda item: item.relative_to(ROOT).as_posix()):
        rel = path.relative_to(ROOT).as_posix()
        if path.parent == DOCS and path.name.startswith("search-index") and path.suffix == ".json":
            continue
        if rel.startswith("docs/"):
            docs_rel = path.relative_to(DOCS).as_posix()
            if docs_rel in excluded or path.suffix.lower() not in SOURCE_SUFFIXES:
                continue
        elif rel != "scripts/generate_site_audits.py":
            continue
        data = path.read_bytes()
        if rel == "docs/index.html":
            text = data.decode("utf-8")
            text = normalized_index_date.sub(r"\1@BUILD_TIMESTAMP@", text, count=1)
            data = text.encode("utf-8")
        digest.update(rel.encode("utf-8"))
        digest.update(b"\0")
        digest.update(data)
        digest.update(b"\0")
    return "sha256:" + digest.hexdigest()


def esc(value: object) -> str:
    return html.escape(str(value), quote=True)


def finding_list(items: list[str]) -> str:
    if not items:
        return "<p>None.</p>"
    shown = items[:80]
    note = (
        f"<p>Showing the first {len(shown)} of {len(items)} findings.</p>"
        if len(items) > len(shown)
        else ""
    )
    return note + "<ul>" + "".join(f"<li><code>{esc(item)}</code></li>" for item in shown) + "</ul>"


def report_shell(path: str, title: str, description: str, body: str, generated_date: str, version: str) -> str:
    date_text = datetime.strptime(generated_date, "%Y-%m-%d").strftime("%B %-d, %Y")
    return f'''<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(title)}</title>
<meta name="description" content="{esc(description)}">
<meta name="source-version" content="{esc(version)}">
<link rel="canonical" href="https://vitte-lang.org/{esc(path)}">
<link rel="stylesheet" href="css/site.css">
</head>
<body class="classic-doc">
<a class="skip-link" href="#main-content">Skip to content</a>
<div class="site-shell">
<header class="site-header">
<a class="site-brand" href="index.html"><img class="site-brand-mark" src="svg/logo.svg" alt="" width="32" height="32"><span>Vitte</span></a>
<nav class="site-nav" aria-label="Site"><ul class="nav-band"><li><a class="nav-chip" href="index.html">Home</a></li><li><a class="nav-chip" href="doc.html">Documentation</a></li><li><a class="nav-chip" href="search.html">Search</a></li><li><a class="nav-chip" href="sitemap.html">Site map</a></li></ul></nav>
</header>
<main id="main-content" class="site-main">
<article class="doc-content">
<h1>{esc(title)}</h1>
<p class="report-provenance">Generated <time datetime="{esc(generated_date)}">{esc(date_text)}</time> · Source version <code>{esc(version)}</code></p>
{body}
</article>
</main>
<footer class="site-footer"><p class="site-footer-path">{esc(path)}</p><p><a href="index.html">Back to home</a></p></footer>
</div>
</body>
</html>
'''


def reports_for(audit: dict[str, object], generated_date: str, version: str) -> dict[str, str]:
    pages = audit["pages"]
    assert isinstance(pages, dict)
    page_count = len(pages)
    missing_alt = audit["missing_alt"]
    unnamed = audit["unnamed_controls"]
    missing_titles = audit["missing_titles"]
    missing_h1 = audit["missing_h1"]
    missing_viewports = audit["missing_viewports"]
    missing_canonicals = audit["missing_canonicals"]
    noindex = audit["noindex"]
    missing_targets = audit["missing_targets"]
    missing_fragments = audit["missing_fragments"]
    counts = audit["file_counts"]
    assert all(isinstance(x, list) for x in (missing_alt, unnamed, missing_titles, missing_h1, missing_viewports, missing_canonicals, noindex, missing_targets, missing_fragments))
    assert isinstance(counts, dict)

    accessibility = f'''<h2>Results</h2>
<p>The source scan covered {page_count} HTML pages.</p>
<ul>
<li>Images missing an <code>alt</code> attribute: <strong>{len(missing_alt)}</strong></li>
<li>Native form controls without a detectable accessible name: <strong>{len(unnamed)}</strong></li>
<li>Pages missing a document title: <strong>{len(missing_titles)}</strong></li>
<li>Pages missing a level-one heading: <strong>{len(missing_h1)}</strong></li>
</ul>
<h2>Findings</h2>
<h3>Images missing <code>alt</code></h3>{finding_list(missing_alt)}
<h3>Controls without a detectable name</h3>{finding_list(unnamed)}
<h3>Pages without a title</h3>{finding_list(missing_titles)}
<h3>Pages without a level-one heading</h3>{finding_list(missing_h1)}
<h2>Scope</h2>
<p>This static scan checks image alternatives, native control labels and ARIA names, document titles, and level-one headings. It does not evaluate color contrast, keyboard interaction, or screen-reader output.</p>'''

    broken = f'''<h2>Results</h2>
<p>The local link scan checked <strong>{page_count}</strong> HTML pages and examined <code>href</code>, <code>src</code>, and <code>poster</code> targets and fragments.</p>
<ul>
<li>Missing local targets: <strong>{len(missing_targets)}</strong></li>
<li>Missing HTML or SVG fragments: <strong>{len(missing_fragments)}</strong></li>
</ul>
<h2>Missing targets</h2>{finding_list(missing_targets)}
<h2>Missing fragments</h2>{finding_list(missing_fragments)}
<p>External destinations and CSS <code>url()</code> references are outside this scan.</p>'''

    seo = f'''<h2>Results</h2>
<p>The metadata scan covered {page_count} HTML pages. Noindex pages are excluded from the canonical-URL requirement.</p>
<ul>
<li>Pages without a title: <strong>{len(missing_titles)}</strong></li>
<li>Pages without responsive viewport metadata: <strong>{len(missing_viewports)}</strong></li>
<li>Indexable pages without a canonical URL: <strong>{len(missing_canonicals)}</strong></li>
<li>Pages marked noindex: <strong>{len(noindex)}</strong></li>
</ul>
<h2>Findings</h2>
<h3>Pages without a title</h3>{finding_list(missing_titles)}
<h3>Pages without viewport metadata</h3>{finding_list(missing_viewports)}
<h3>Indexable pages without a canonical URL</h3>{finding_list(missing_canonicals)}
<h3>Pages marked noindex</h3>{finding_list(noindex)}'''

    stats = f'''<h2>Current file counts</h2>
<p>Counts from the current <code>docs/</code> tree.</p>
<ul>
<li>HTML pages: {counts['.html']}</li>
<li>CSS files: {counts['.css']}</li>
<li>JavaScript files, including the service worker: {counts['.js']}</li>
<li>SVG files: {counts['.svg']}</li>
<li>JSON files: {counts['.json']}</li>
<li>CSS and JavaScript assets: {counts['.css'] + counts['.js']}</li>
</ul>
<p>These counts are regenerated with the site audit reports.</p>'''

    return {
        "accessibility-report.html": report_shell("accessibility-report.html", "Accessibility Report", "Static accessibility findings for the Vitte documentation site.", accessibility, generated_date, version),
        "broken-links.html": report_shell("broken-links.html", "Broken Links", "Local link and fragment findings for the Vitte documentation site.", broken, generated_date, version),
        "seo-report.html": report_shell("seo-report.html", "SEO Report", "Search and page metadata findings for the Vitte documentation site.", seo, generated_date, version),
        "site-statistics.html": report_shell("site-statistics.html", "Site Statistics", "Current file counts for the Vitte static documentation site.", stats, generated_date, version),
    }


def text_content(source: str) -> str:
    class TextParser(HTMLParser):
        def __init__(self) -> None:
            super().__init__(convert_charrefs=True)
            self.hidden = 0
            self.parts: list[str] = []

        def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
            if tag in {"script", "style", "noscript", "svg"}:
                self.hidden += 1

        def handle_endtag(self, tag: str) -> None:
            if tag in {"script", "style", "noscript", "svg"}:
                self.hidden = max(0, self.hidden - 1)

        def handle_data(self, data: str) -> None:
            if not self.hidden and data.strip():
                self.parts.append(data.strip())

    parser = TextParser()
    parser.feed(source)
    return " ".join(" ".join(parser.parts).split())


def page_metadata(source: str) -> tuple[str, str]:
    title_match = re.search(r"<title\b[^>]*>(.*?)</title\s*>", source, re.I | re.S)
    lang_match = re.search(r"<html\b[^>]*\blang=[\"']([^\"']+)", source, re.I)
    title = re.sub(r"\s+", " ", re.sub(r"<[^>]+>", "", title_match.group(1))).strip() if title_match else ""
    return title, lang_match.group(1) if lang_match else "en"


def update_search_indexes(outputs: dict[str, str], check: bool) -> dict[Path, str]:
    updates: dict[Path, str] = {}
    failures: list[str] = []
    for path in sorted(DOCS.glob("search-index*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        pages = data.get("pages", []) if isinstance(data, dict) else []
        changed = False
        for entry in pages:
            if not isinstance(entry, dict) or entry.get("path") not in SEARCH_INDEXES:
                continue
            rel = entry["path"]
            source = outputs.get(rel)
            html_path = DOCS / rel
            if source is None:
                source = html_path.read_text(encoding="utf-8")
            title, lang = page_metadata(source)
            content = text_content(source)
            expected = {"title": title, "lang": lang, "content": content}
            if any(entry.get(key) != value for key, value in expected.items()):
                changed = True
                if check:
                    failures.append(f"{path.relative_to(ROOT)}: {rel}")
                else:
                    entry.update(expected)
        if changed and not check:
            updates[path] = json.dumps(data, ensure_ascii=False, indent=2) + "\n"
    if failures:
        raise ValueError("Stale search entries: " + ", ".join(failures))
    return updates


def generated_provenance() -> tuple[str, str]:
    dates: set[str] = set()
    versions: set[str] = set()
    for name in REPORTS:
        source = (DOCS / name).read_text(encoding="utf-8")
        date_match = re.search(r'<time\b[^>]*datetime="([0-9]{4}-[0-9]{2}-[0-9]{2})"', source)
        version_match = re.search(r'<meta\s+name="source-version"\s+content="([^"]+)"', source)
        if date_match:
            dates.add(date_match.group(1))
        if version_match:
            versions.add(version_match.group(1))
    build = json.loads((DOCS / "build-manifest.json").read_text(encoding="utf-8"))
    stamp = str(build.get("built_at_utc", ""))
    if len(dates) != 1 or len(versions) != 1 or not stamp:
        raise ValueError("Audit reports do not share a generated date and source version")
    return dates.pop(), stamp


def build_outputs(audit: dict[str, object], version: str, generated_date: str, stamp: str) -> dict[Path, str]:
    outputs: dict[str, str] = reports_for(audit, generated_date, version)
    file_counts = audit["file_counts"]
    assert isinstance(file_counts, dict)
    main_index_path = DOCS / "search-index.json"
    main_index = json.loads(main_index_path.read_text(encoding="utf-8"))
    entries = main_index.get("pages", [])
    section_counts: dict[str, int] = {}
    for entry in entries:
        section = str(entry.get("section", ""))
        section_counts[section] = section_counts.get(section, 0) + 1
    search_count = len(entries)

    assets_path = DOCS / "assets.json"
    build_path = DOCS / "build-manifest.json"
    assets = json.loads(assets_path.read_text(encoding="utf-8"))
    build = json.loads(build_path.read_text(encoding="utf-8"))
    for manifest in (assets, build):
        manifest["built_at_utc"] = stamp
        manifest["source_version"] = version
        for entry in manifest.get("assets", []):
            asset_path = DOCS / entry["path"]
            entry["sha256"] = hashlib.sha256(asset_path.read_bytes()).hexdigest()
            entry["size"] = asset_path.stat().st_size
    build.update({
        "html_pages": file_counts[".html"],
        "css_files": file_counts[".css"],
        "js_files": file_counts[".js"],
        "svg_files": file_counts[".svg"],
        "json_files": file_counts[".json"],
    })
    outputs["assets.json"] = json.dumps(assets, ensure_ascii=False, indent=2) + "\n"
    outputs["build-manifest.json"] = json.dumps(build, ensure_ascii=False, indent=2) + "\n"

    status_path = DOCS / "status.json"
    status = json.loads(status_path.read_text(encoding="utf-8"))
    status.update({
        "build_utc": stamp,
        "source_version": version,
        "pages": search_count,
        "total_search_entries": search_count,
        "sections": section_counts,
        "docs_pages": section_counts.get("docs", 0),
        "book_pages": section_counts.get("book", 0),
        "grammar_pages": section_counts.get("grammar", 0),
        "css_hash": hashlib.sha256((DOCS / "css/site.css").read_bytes()).hexdigest()[:10],
        "js_hash": hashlib.sha256((DOCS / "js/main.js").read_bytes()).hexdigest()[:10],
    })
    outputs["status.json"] = json.dumps(status, ensure_ascii=False, indent=2) + "\n"

    checksums = {entry["path"]: entry["sha256"] for entry in build["assets"]}
    status_payload = {
        "built_at_utc": stamp,
        "source_version": version,
        "pages": file_counts[".html"],
        "asset_count": len(build["assets"]),
        "checksums": checksums,
    }
    status_html_path = DOCS / "status.html"
    status_html = status_html_path.read_text(encoding="utf-8")
    pre = "\n" + json.dumps(status_payload, ensure_ascii=False, indent=2) + "\n"
    status_html, count = re.subn(r"<pre>.*?</pre>", "<pre>" + pre + "</pre>", status_html, count=1, flags=re.S)
    if count != 1:
        raise ValueError("Could not find the technical status JSON block")
    status_html = re.sub(r"(<strong>Date:</strong> )[^<]+", r"\g<1>" + stamp, status_html, count=1)
    source_line = f'<p><strong>Source version:</strong> <code>{esc(version)}</code></p>'
    if "<strong>Source version:</strong>" in status_html:
        status_html = re.sub(
            r'<p><strong>Source version:</strong> <code>.*?</code></p>',
            source_line,
            status_html,
            count=1,
        )
    else:
        status_html = status_html.replace("</div><pre>", "</div>" + source_line + "<pre>", 1)
    outputs["status.html"] = status_html

    public_path = DOCS / "status-public.html"
    public = public_path.read_text(encoding="utf-8")
    public = re.sub(r"(<strong>Last build \(UTC\):</strong> )[^<]+", r"\g<1>" + stamp, public, count=1)
    public = re.sub(r"(<strong>Generated pages:</strong> )\d+", r"\g<1>" + str(file_counts[".html"]), public, count=1)
    public = re.sub(r"(<strong>Assets tracked:</strong> )\d+", r"\g<1>" + str(len(build["assets"])), public, count=1)
    source_line = f'<p><strong>Source version:</strong> <code>{esc(version)}</code></p>'
    if "<strong>Source version:</strong>" in public:
        public = re.sub(
            r'<p><strong>Source version:</strong> <code>.*?</code></p>',
            source_line,
            public,
            count=1,
        )
    else:
        public = public.replace("</p><h2>Checks</h2>", "</p>" + source_line + "<h2>Checks</h2>", 1)
    outputs["status-public.html"] = public

    home_path = DOCS / "index.html"
    home = home_path.read_text(encoding="utf-8")
    home = re.sub(r"(<strong>Manifest build:</strong> [^<]*<strong>Date:</strong> )[^<]+", r"\g<1>" + stamp + " UTC", home, count=1)
    outputs["index.html"] = home

    output_stats = reports_for(audit, generated_date, version)["site-statistics.html"]
    outputs["site-statistics.html"] = output_stats
    for rel, source in list(outputs.items()):
        if rel.endswith(".html"):
            outputs[rel] = source

    result: dict[Path, str] = {DOCS / rel: source for rel, source in outputs.items()}
    result.update(update_search_indexes(outputs, check=False))
    return result


def check_outputs(expected: dict[Path, str]) -> list[str]:
    stale = []
    for path, content in expected.items():
        if not path.exists() or path.read_text(encoding="utf-8") != content:
            stale.append(path.relative_to(ROOT).as_posix())
    stale.extend(path.relative_to(ROOT).as_posix() for path in update_search_indexes({}, check=True))
    return sorted(set(stale))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true", help="regenerate reports and site metadata")
    mode.add_argument("--check", action="store_true", help="verify generated reports and metadata")
    args = parser.parse_args()

    try:
        audit = scan()
        version = source_version()
        if args.write:
            now = datetime.now(timezone.utc)
            stamp = now.isoformat(timespec="seconds")
            generated_date = now.astimezone(ZoneInfo("Europe/Paris")).date().isoformat()
        else:
            generated_date, stamp = generated_provenance()
            for name in REPORTS:
                source = (DOCS / name).read_text(encoding="utf-8")
                if version not in source:
                    raise ValueError(f"{name} records a different source version")
        outputs = build_outputs(audit, version, generated_date, stamp)
        if args.write:
            for path, content in outputs.items():
                path.write_text(content, encoding="utf-8")
            print(f"Generated site audits for {len(audit['pages'])} pages; source version {version}.")
            return 0
        stale = check_outputs(outputs)
        if stale:
            print("Generated site files are stale:")
            for path in stale:
                print(f"  {path}")
            return 1
        print(f"Site audits are current for {len(audit['pages'])} pages; source version {version}.")
        return 0
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as error:
        print(f"Site audit generation failed: {error}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
