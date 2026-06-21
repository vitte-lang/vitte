#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys
import json
import re
from datetime import UTC, datetime


ROOT = Path(__file__).resolve().parent.parent
DOCS = [
    ROOT / "docs" / "compiler" / "architecture.md",
    ROOT / "docs" / "compiler" / "pipeline.md",
    ROOT / "docs" / "COMPILER_DRIVER_MIGRATION.md",
    ROOT / "docs" / "bootstrap_contracts.md",
    ROOT / "docs" / "bootstrap_migration_checklist.md",
    ROOT / "docs" / "MAKE_TARGETS.md",
]

FORBIDDEN = [
    "src/vitte/compiler/frontend/parser.vit",
    "src/vitte/compiler/frontend/lexer.vit",
    "src/vitte/compiler/frontend/token.vit",
    "src/vitte/compiler/frontend/expr_parser.vit",
    "src/vitte/compiler/frontend/diagnostics.vit",
    "ast_extended.vit",
    "hir_extended.vit",
    "architecture_integration_tests.vit",
    "C backend code generation",
    "Code Generation (C backend)",
    "generating .c/.h",
]

REQUIRED = [
    "toolchain/seed/vittec0.seed",
    "make bootstrap-native-contract",
    "make bootstrap-verify",
    "make build",
]

REQUIRED_SECTIONS = [
    "Bootstrap",
    "Pipeline",
    "Driver",
]

RECOMMENDED_REFERENCES = [
    "HIR",
    "MIR",
    "borrowck",
    "typeck",
    "diagnostics",
]

MIN_WORDS_PER_DOC = 300

RECOMMENDED_HEADINGS = [
    "Overview",
    "Responsibilities",
    "Invariants",
    "Data Flow",
    "Examples",
]

ARCHITECTURE_TERMS = [
    "Lexer",
    "Parser",
    "AST",
    "HIR",
    "Borrow Checker",
    "MIR",
    "IR",
    "Codegen",
    "Linker",
]

COMPILER_PIPELINE_TERMS = [
    "Lexer",
    "Parser",
    "AST",
    "HIR",
    "Sema",
    "Typeck",
    "Borrowck",
    "MIR",
    "IR",
    "Backend",
    "Linker",
]

REPORT_DIR = ROOT / "docs" / "reports"
REPORT_DIR.mkdir(parents=True, exist_ok=True)
HTML_REPORT = REPORT_DIR / "architecture-docs-report.html"
CSV_REPORT = REPORT_DIR / "architecture-docs-report.csv"
INDEX_REPORT = REPORT_DIR / "index.html"
TOP_SCORE_THRESHOLD = 95
GOOD_SCORE_THRESHOLD = 85
ACCEPTABLE_SCORE_THRESHOLD = 70

ORPHAN_SCOPE_GLOBS = [
    "docs/compiler/*.md",
    "docs/bootstrap/*.md",
    "docs/bootstrap_*.md",
    "docs/COMPILER_DRIVER_MIGRATION.md",
    "docs/MAKE_TARGETS.md",
]

def find_orphan_documents() -> list[Path]:
    all_docs: list[Path] = []
    for pattern in ORPHAN_SCOPE_GLOBS:
        all_docs.extend(ROOT.glob(pattern))
    all_docs = sorted({doc.resolve() for doc in all_docs})

    referenced: set[str] = set()

    for doc in all_docs:
        try:
            text = doc.read_text(encoding="utf-8")
        except Exception:
            continue

        for link in extract_markdown_links(text):
            if link.startswith("http") or link.startswith("#"):
                continue

            target_text = link.split("#")[0]
            if not target_text:
                continue
            target = (doc.parent / target_text).resolve()
            try:
                referenced.add(str(target.relative_to(ROOT)).replace("\\", "/"))
            except ValueError:
                continue

    orphans: list[Path] = []

    for doc in all_docs:
        if doc.name.lower() == "readme.md":
            continue

        rel = str(doc.relative_to(ROOT)).replace("\\", "/")

        if rel not in referenced and doc not in DOCS:
            orphans.append(doc)

    return orphans

def check_markdown_quality(path: Path, text: str) -> tuple[list[str], list[str]]:
    local_errors: list[str] = []
    local_warnings: list[str] = []

    headings = text.count("#")
    if headings < 3:
        local_warnings.append(
            f"{path.name}: very few headings ({headings})"
        )

    lines = text.splitlines()
    long_lines = [n + 1 for n, line in enumerate(lines) if len(line) > 160]

    if long_lines:
        local_warnings.append(
            f"{path.name}: {len(long_lines)} long lines (>160 chars)"
        )

    word_count = len(text.split())
    if word_count < MIN_WORDS_PER_DOC:
        local_warnings.append(
            f"{path.name}: small document ({word_count} words)"
        )

    fenced_blocks = text.count("```") // 2
    if fenced_blocks == 0:
        local_warnings.append(
            f"{path.name}: no code examples"
        )

    tables = text.count("|")
    if tables < 4:
        local_warnings.append(
            f"{path.name}: no visible table"
        )

    for section in REQUIRED_SECTIONS:
        if section.lower() not in text.lower():
            local_warnings.append(
                f"{path.name}: missing recommended section `{section}`"
            )

    for heading in RECOMMENDED_HEADINGS:
        if heading.lower() not in text.lower():
            local_warnings.append(
                f"{path.name}: missing heading `{heading}`"
            )

    return local_errors, local_warnings

def compute_quality_score(text: str) -> int:
    score = 100

    if len(text.split()) < MIN_WORDS_PER_DOC:
        score -= 15

    if text.count("#") < 3:
        score -= 10

    if "```" not in text:
        score -= 15

    if "|" not in text:
        score -= 10

    for heading in RECOMMENDED_HEADINGS:
        if heading.lower() not in text.lower():
            score -= 5

    return max(score, 0)


# --- Helper functions for report ---
def extract_markdown_links(text: str) -> list[str]:
    return re.findall(r'\[[^\]]+\]\(([^)]+)\)', text)


def architecture_coverage(text: str) -> float:
    found = 0
    low = text.lower()
    for term in ARCHITECTURE_TERMS:
        if term.lower() in low:
            found += 1
    return round((found / len(ARCHITECTURE_TERMS)) * 100.0, 1)

def compiler_pipeline_coverage(text: str) -> float:
    found = 0
    low = text.lower()

    for term in COMPILER_PIPELINE_TERMS:
        if term.lower() in low:
            found += 1

    return round((found / len(COMPILER_PIPELINE_TERMS)) * 100.0, 1)


def extract_vitte_code_blocks(text: str) -> list[str]:
    return re.findall(r"```(?:vit|vitte)?\n(.*?)```", text, re.S | re.I)

def extract_markdown_anchors(text: str) -> set[str]:
    anchors: set[str] = set()

    for line in text.splitlines():
        if line.startswith("#"):
            title = re.sub(r"^#+\s*", "", line).strip().lower()
            title = re.sub(r"[^a-z0-9\- ]", "", title)
            title = title.replace(" ", "-")
            anchors.add(title)

    return anchors


def extract_anchor_links(text: str) -> list[tuple[str, str]]:
    results = []

    for link in extract_markdown_links(text):
        if "#" not in link:
            continue

        target, anchor = link.split("#", 1)
        results.append((target, anchor.lower()))

    return results

def score_rating(score: float) -> str:
    if score >= TOP_SCORE_THRESHOLD:
        return "EXCELLENT"
    if score >= GOOD_SCORE_THRESHOLD:
        return "GOOD"
    if score >= ACCEPTABLE_SCORE_THRESHOLD:
        return "ACCEPTABLE"
    return "NEEDS WORK"

def main() -> int:
    failed = False
    warning_count = 0
    checked_files = 0
    combined = ""
    scores: list[int] = []
    report_entries = []
    total_vitte_examples = 0
    invalid_vitte_examples = 0
    invalid_anchor_links = 0
    anchor_map: dict[Path, set[str]] = {}
    for doc in DOCS:
        if not doc.is_file():
            print(f"[architecture-docs][error] missing documentation file `{doc.relative_to(ROOT)}`", file=sys.stderr)
            failed = True
            continue
        text = doc.read_text(encoding="utf-8")
        anchor_map[doc] = extract_markdown_anchors(text)
        scores.append(compute_quality_score(text))
        checked_files += 1

        local_errors, local_warnings = check_markdown_quality(doc, text)

        for err in local_errors:
            print(f"[architecture-docs][error] {err}", file=sys.stderr)
            failed = True

        for warn in local_warnings:
            print(f"[architecture-docs][warning] {warn}")
            warning_count += 1

        links = extract_markdown_links(text)

        vitte_blocks = extract_vitte_code_blocks(text)

        invalid_examples = 0
        total_vitte_examples += len(vitte_blocks)
        for block in vitte_blocks:
            low = block.lower()

            if "space " not in low:
                invalid_examples += 1

            if "proc " not in low:
                invalid_examples += 1
        invalid_vitte_examples += invalid_examples

        for link in links:
            if link.startswith("http") or link.startswith("#"):
                continue

            target = (doc.parent / link.split("#")[0]).resolve()
            if not target.exists():
                print(
                    f"[architecture-docs][warning] broken link in {doc.name}: {link}"
                )
                warning_count += 1

        for target_name, anchor in extract_anchor_links(text):
            if not target_name:
                target_doc = doc
            else:
                target_doc = (doc.parent / target_name).resolve()

            if target_doc in anchor_map:
                if anchor not in anchor_map[target_doc]:
                    print(
                        f"[architecture-docs][warning] missing anchor `{anchor}` in {target_doc.name}"
                    )
                    warning_count += 1
                    invalid_anchor_links += 1

        report_entries.append(
            {
                "file": str(doc.relative_to(ROOT)),
                "score": compute_quality_score(text),
                "words": len(text.split()),
                "links": len(links),
                "vitte_examples": len(vitte_blocks),
                "invalid_examples": invalid_examples,
            }
        )

        combined += text + "\n"
        for forbidden in FORBIDDEN:
            if forbidden in text:
                print(f"[architecture-docs][error] {doc.name}: stale reference `{forbidden}`", file=sys.stderr)
                failed = True
    for required in REQUIRED:
        if required not in combined:
            print(f"[architecture-docs][error] missing required reference `{required}`", file=sys.stderr)
            failed = True

    for reference in RECOMMENDED_REFERENCES:
        if reference.lower() not in combined.lower():
            print(
                f"[architecture-docs][warning] recommended reference `{reference}` not found"
            )
            warning_count += 1

    total_words = len(combined.split())
    average_score = round(sum(scores) / max(1, len(scores)), 1)
    rating = score_rating(average_score)
    print(f"[architecture-docs] words={total_words}")
    print(f"[architecture-docs] average-score={average_score}/100")

    coverage = architecture_coverage(combined)
    pipeline_coverage = compiler_pipeline_coverage(combined)
    print(f"[architecture-docs] architecture-coverage={coverage}%")
    print(f"[architecture-docs] pipeline-coverage={pipeline_coverage}%")
    print(f"[architecture-docs] vitte-examples={total_vitte_examples}")
    print(f"[architecture-docs] invalid-vitte-examples={invalid_vitte_examples}")
    print(f"[architecture-docs] invalid-anchor-links={invalid_anchor_links}")

    orphans = find_orphan_documents()

    for orphan in orphans:
        print(
            f"[architecture-docs][warning] orphan document: {orphan.relative_to(ROOT)}"
        )

    sorted_entries = sorted(
        report_entries,
        key=lambda x: x["score"],
        reverse=True,
    )

    json_report = REPORT_DIR / "architecture-docs-report.json"
    json_report.write_text(
        json.dumps(
            {
                "score": average_score,
                "rating": rating,
                "coverage": coverage,
                "pipeline_coverage": pipeline_coverage,
                "files": sorted_entries,
                "warnings": warning_count,
                "vitte_examples": total_vitte_examples,
                "invalid_vitte_examples": invalid_vitte_examples,
                "invalid_anchor_links": invalid_anchor_links,
                "orphans": [str(x.relative_to(ROOT)) for x in orphans],
            },
            indent=2,
        ) + "\n",
        encoding="utf-8",
    )

    CSV_REPORT.write_text(
        "file,score,words,links,vitte_examples,invalid_examples\n"
        + "\n".join(
            f"{e['file']},{e['score']},{e['words']},{e['links']},{e['vitte_examples']},{e['invalid_examples']}"
            for e in sorted_entries
        )
        + "\n",
        encoding="utf-8",
    )

    # HTML dashboard
    html_rows = "\n".join(
        f"<tr><td>{e['file']}</td>"
        f"<td>{e['score']}</td>"
        f"<td>{e['words']}</td>"
        f"<td>{e['links']}</td>"
        f"<td>{e['vitte_examples']}</td>"
        f"<td>{e['invalid_examples']}</td></tr>"
        for e in sorted_entries
    )
    html_table = (
        "<table border='1' cellspacing='0' cellpadding='4'>"
        "<thead><tr>"
        "<th>File</th><th>Score</th><th>Words</th><th>Links</th><th>Vitte Examples</th><th>Invalid Examples</th>"
        "</tr></thead><tbody>"
        f"{html_rows}"
        "</tbody></table>"
    )
    html_content = (
        "<html><head><meta charset='utf-8'><title>Architecture Docs Report</title></head><body>"
        "<h1>Architecture Documentation Report</h1>"
        f"<p><b>Score:</b> {average_score}/100<br>"
        f"<b>Rating:</b> {rating}<br>"
        f"<b>Architecture Coverage:</b> {coverage}%<br>"
        f"<b>Pipeline Coverage:</b> {pipeline_coverage}%<br>"
        f"<b>Vitte Examples:</b> {total_vitte_examples}<br>"
        f"<b>Invalid Vitte Examples:</b> {invalid_vitte_examples}<br>"
        f"<b>Invalid Anchor Links:</b> {invalid_anchor_links}<br>"
        f"<b>Warnings:</b> {warning_count}<br>"
        f"<b>Orphan Documents:</b> {len(orphans)}"
        "</p>"
        + html_table +
        "</body></html>"
    )
    HTML_REPORT.write_text(html_content, encoding="utf-8")

    portal = f"""
<html>
<head>
<meta charset='utf-8'>
<title>Vitte Documentation Reports</title>
</head>
<body>
<h1>Vitte Documentation Quality Portal</h1>
<ul>
<li><a href='architecture-docs-report.html'>HTML Report</a></li>
<li><a href='architecture-docs-report.json'>JSON Report</a></li>
<li><a href='architecture-docs-report.csv'>CSV Report</a></li>
<li><a href='architecture-docs-summary.md'>Summary</a></li>
<li><a href='architecture-docs-badge.svg'>Badge</a></li>
<li><a href='coverage-chart.svg'>Coverage Chart</a></li>
</ul>
<p>Score: {average_score}/100 ({rating})</p>
<p>Architecture Coverage: {coverage}%</p>
<p>Pipeline Coverage: {pipeline_coverage}%</p>
</body>
</html>
"""
    INDEX_REPORT.write_text(portal, encoding="utf-8")

    history_file = REPORT_DIR / "history.jsonl"
    with history_file.open("a", encoding="utf-8") as f:
        f.write(
            json.dumps(
                {
                    "date": datetime.now(UTC).isoformat(),
                    "score": average_score,
                    "coverage": coverage,
                    "pipeline_coverage": pipeline_coverage,
                    "warnings": warning_count,
                }
            )
            + "\n"
        )

    badge = REPORT_DIR / "architecture-docs-badge.svg"
    badge.write_text(
        f'''<svg xmlns="http://www.w3.org/2000/svg" width="220" height="20">
<rect width="120" height="20" fill="#555"/>
<rect x="120" width="100" height="20" fill="#4c1"/>
<text x="60" y="14" fill="white" text-anchor="middle">Docs Quality</text>
<text x="170" y="14" fill="white" text-anchor="middle">{average_score}/100</text>
</svg>
''',
        encoding="utf-8",
    )

    coverage_chart = REPORT_DIR / "coverage-chart.svg"
    coverage_chart.write_text(
        f'''<svg xmlns="http://www.w3.org/2000/svg" width="420" height="120">
<text x="10" y="20">Architecture Coverage</text>
<rect x="10" y="30" width="300" height="20" fill="#ddd"/>
<rect x="10" y="30" width="{coverage * 3}" height="20" fill="#4c1"/>
<text x="320" y="45">{coverage}%</text>
<text x="10" y="80">Pipeline Coverage</text>
<rect x="10" y="90" width="300" height="20" fill="#ddd"/>
<rect x="10" y="90" width="{pipeline_coverage * 3}" height="20" fill="#06c"/>
<text x="320" y="105">{pipeline_coverage}%</text>
</svg>''',
        encoding="utf-8",
    )

    summary_md = REPORT_DIR / "architecture-docs-summary.md"
    summary_md.write_text(
        "# Architecture Documentation Report\n\n"
        f"- Score: {average_score}/100\n"
        f"- Rating: {rating}\n"
        f"- Architecture Coverage: {coverage}%\n"
        f"- Pipeline Coverage: {pipeline_coverage}%\n"
        f"- Warnings: {warning_count}\n"
        f"- Invalid Anchor Links: {invalid_anchor_links}\n",
        encoding="utf-8",
    )

    print(f"[architecture-docs] report={json_report}")
    print(f"[architecture-docs] history={history_file}")
    print(f"[architecture-docs] badge={badge}")
    print(f"[architecture-docs] coverage-chart={coverage_chart}")
    print(f"[architecture-docs] summary={summary_md}")
    print(f"[architecture-docs] html={HTML_REPORT}")
    print(f"[architecture-docs] csv={CSV_REPORT}")

    if len(scores) > 1:
        best = max(scores)
        worst = min(scores)
        print(f"[architecture-docs] best-score={best}")
        print(f"[architecture-docs] worst-score={worst}")

    print(f"[architecture-docs] rating={rating}")
    print(f"[architecture-docs] portal={INDEX_REPORT}")

    if failed:
        return 1
    print(
        f"[architecture-docs] OK: files={checked_files} warnings={warning_count} score={average_score}/100"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
