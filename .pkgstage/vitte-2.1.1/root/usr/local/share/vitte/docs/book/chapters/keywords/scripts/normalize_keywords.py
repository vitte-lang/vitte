#!/usr/bin/env python3
"""Normalize docs/book/keywords to a strict editorial template."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import argparse
import re

SKIP = {
    "README.md",
    "couverture.md",
    "parcours.md",
    "packs-apprentissage.md",
    "non-utilises.md",
    "erreurs-compilateur.md",
}

ORDER = [
    "## Quick read (30s)",
    "## Why it matters",
    "## Definition",
    "## Syntax",
    "## Nominal example",
    "## Invalid example",
    "## Nearby differences",
    "## Quick refactor",
    "## Pitfalls",
    "## When to use it / When to avoid it",
    "## Common compiler errors",
    "## Neighbor keyword",
    "## Used in chapters",
    "## See also",
    "## Completeness score",
]

SECTION_RE = re.compile(r"^## .*$", re.MULTILINE)


def split_sections(text: str) -> tuple[str, str, dict[str, str]]:
    lines = text.splitlines()
    title = lines[0] if lines else ""
    level = ""
    for line in lines[1:6]:
        if line.startswith("Level:"):
            level = line
            break

    parts = SECTION_RE.split(text)
    heads = SECTION_RE.findall(text)
    sections: dict[str, str] = {}
    for i, head in enumerate(heads):
        body = parts[i + 1].strip("\n")
        sections[head.strip()] = body.strip()

    return title.strip(), level.strip(), sections


def ensure_section(sections: dict[str, str], heading: str, keyword: str) -> str:
    if heading in sections and sections[heading].strip():
        return sections[heading].strip()

    base = keyword.strip("`")
    if heading == "## Quick read (30s)":
        return (
            "- What it is: this keyword expresses a precise intent in Vitte flow.\n"
            "- When to use it: when it makes the contract easier to read.\n"
            "- Common error: using it at the wrong level (top-level vs block)."
        )
    if heading == "## Why it matters":
        return (
            f"`{base}` reduces ambiguity in production code.\n\n"
            "Use it to make the domain rule explicit at first reading.\n"
            "That simplifies reviews and speeds up diagnostics when errors appear.\n"
            "The goal is stable, readable, and testable behavior."
        )
    if heading == "## Nearby differences":
        return (
            "| Nearby keyword | Key difference |\n"
            "| --- | --- |\n"
            "| `To complete` | State the operational difference in one sentence. |"
        )
    if heading == "## Quick refactor":
        return (
            "Before:\n"
            "```vit\n"
            "# fragile usage to fix\n"
            "```\n\n"
            "After:\n"
            "```vit\n"
            "# clear and testable usage\n"
            "```"
        )
    if heading == "## Nominal example":
        return (
            "Input:\n"
            "- Simple nominal case.\n\n"
            "```vit\n"
            f"# minimal example with `{base}`\n"
            "```\n\n"
            "Observable output:\n"
            "- Stable and verifiable expected result."
        )
    if heading == "## Invalid example":
        return (
            "Input:\n"
            "- Minimal out-of-contract case.\n\n"
            "```vit\n"
            f"# counter-example with `{base}`\n"
            "```\n\n"
            "Expected diagnostic:\n"
            "- Code: `VITTE-XXXX` (or matching compiler code `E000X`).\n"
            "- Position: `line 1, column 1` (adjust to the real snippet).\n"
            "- Message: stable, correction-oriented pattern."
        )
    if heading == "## Completeness score":
        return "coverage: syntax/examples/invalid/diagnostics/links = 3/5"
    if heading == "## Used in chapters":
        return "- `docs/book/chapters/27-grammar.md`.\n- `docs/book/chapters/31-build-errors.md`.\n- `docs/book/chapters/07-control.md`."
    if heading == "## See also":
        return "- `docs/book/chapters/keywords/erreurs-compilateur.md`.\n- `docs/book/glossary.md`."
    if heading == "## Definition":
        return f"`{base}` is a Vitte keyword. This sheet gives its shape, usage, and common errors."
    if heading == "## Syntax":
        return "Canonical form: `to specify`."
    if heading == "## Pitfalls":
        return "- Using it out of context.\n- Hiding the domain intent.\n- Forgetting a testable counter-example."
    if heading == "## When to use it / When to avoid it":
        return "- When to use it: if the keyword clarifies the contract.\n- When to avoid it: if a simpler form already exists."
    if heading == "## Common compiler errors":
        return "| Typical message | Cause | Fix |\n| --- | --- | --- |\n| `VITTE-XXXX` | Common cause to specify. | Quick correction to apply. |"
    if heading == "## Neighbor keyword":
        return "| Keyword | Operational difference |\n| --- | --- |\n| `To complete` | Difference to specify. |"
    return "To complete."


def enrich_quick_read(body: str) -> str:
    lines = [l for l in body.splitlines() if l.strip()]
    bullets = [l for l in lines if l.lstrip().startswith("- ")]
    if len(bullets) >= 3:
        return body.strip()
    extra = [
        "- What it is: this keyword expresses a precise intent in Vitte flow.",
        "- When to use it: when it makes the contract easier to read.",
        "- Common error: using it at the wrong level (top-level vs block).",
    ]
    merged = bullets + [e for e in extra if e not in bullets]
    return "\n".join(merged[:3])


def enrich_invalid_example(body: str, keyword: str) -> str:
    text = body.strip()
    if "Diagnostic attendu:" not in text:
        text += (
            "\n\nExpected diagnostic:\n"
            "- Code: `VITTE-XXXX` (or matching compiler code `E000X`).\n"
            "- Position: `line 1, column 1` (adjust to the real snippet).\n"
            "- Message: stable, correction-oriented pattern."
        )
    if "VITTE-" not in text and "E000" not in text:
        text += "\n- Code: `VITTE-XXXX`."
    low = text.lower()
    if "line" not in low or "column" not in low:
        text += "\n- Expected position: `line 1, column 1`."
    return text


def normalize_chapter_links(body: str) -> str:
    links: list[str] = []
    for raw in body.splitlines():
        m = re.search(r"`((?:docs/)?docs/book/chapters/[0-9a-z\\-]+\\.md)`", raw)
        if not m:
            continue
        path = m.group(1)
        if path.startswith("docs/book/chapters/"):
            path = "docs/" + path
        if path not in links:
            links.append(path)
    defaults = [
        "docs/book/chapters/07-control.md",
        "docs/book/chapters/27-grammar.md",
        "docs/book/chapters/31-build-errors.md",
    ]
    for d in defaults:
        if len(links) >= 3:
            break
        if d not in links:
            links.append(d)
    links = links[:5]
    return "\n".join(f"- `{p}`." for p in links)


def normalize_see_also(body: str) -> str:
    lines = [l.strip() for l in body.splitlines() if l.strip().startswith("- ")]
    if not any("docs/book/chapters/keywords/" in l for l in lines):
        lines.insert(0, "- `docs/book/chapters/keywords/erreurs-compilateur.md`.")
    if not lines:
        lines = [
            "- `docs/book/chapters/keywords/erreurs-compilateur.md`.",
            "- `docs/book/glossary.md`.",
        ]
    return "\n".join(lines)


def compute_score(sections: dict[str, str]) -> str:
    checks = []
    checks.append("## Syntax" in sections and "Canonical form" in sections["## Syntax"])
    checks.append("## Nominal example" in sections and "```vit" in sections["## Nominal example"])
    checks.append("## Invalid example" in sections and "```vit" in sections["## Invalid example"])
    inv = sections.get("## Invalid example", "")
    checks.append("VITTE-" in inv or "E000" in inv)
    used = sections.get("## Used in chapters", "")
    checks.append(used.count("- ") >= 3)
    score = sum(1 for c in checks if c)
    return f"coverage: syntax/examples/invalid/diagnostics/links = {score}/5"


def normalize_file(path: Path) -> bool:
    text = path.read_text(encoding="utf-8")
    title, level, sections = split_sections(text)

    if not title:
        title = f"# Keyword `{path.stem}`"
    if not level:
        level = "Level: Intermediate."

    # Reuse existing "Neighbor keyword" section as fallback for "Nearby differences".
    if "## Nearby differences" not in sections and "## Neighbor keyword" in sections:
        sections["## Nearby differences"] = sections["## Neighbor keyword"]

    out = [title, "", level, ""]
    for heading in ORDER:
        body = ensure_section(sections, heading, path.stem)
        if heading == "## Quick read (30s)":
            body = enrich_quick_read(body)
        elif heading == "## Invalid example":
            body = enrich_invalid_example(body, path.stem)
        elif heading == "## Used in chapters":
            body = normalize_chapter_links(body)
        elif heading == "## See also":
            body = normalize_see_also(body)
        if heading == "## Completeness score":
            body = compute_score({**sections, heading: body})
        out.append(heading)
        out.append("")
        out.append(body.strip())
        out.append("")

    new_text = "\n".join(out).rstrip() + "\n"
    if new_text != text:
        path.write_text(new_text, encoding="utf-8")
        return True
    return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Normalize keyword docs to strict template")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[3]
    root = repo / "book" / "keywords"
    changed = 0
    for path in sorted(root.glob("*.md")):
        if path.name in SKIP:
            continue
        if normalize_file(path):
            changed += 1
    print(f"[keywords-normalize] changed={changed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
