#!/usr/bin/env python3
"""Strict attribution and color coverage for every compiler diagnostic."""

from __future__ import annotations

import json
import sys
from collections import Counter
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "schemas" / "diagnostics" / "codes.json"
CANONICAL = ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "diagnostic.vit"
RENDER = ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "render.vit"
REPORT = ROOT / "target" / "reports" / "diagnostic_attribution" / "coverage.json"

SEVERITY_COLORS = {
    "error": "red",
    "fatal": "red",
    "warning": "yellow",
    "help": "green",
    "note": "blue",
}
VAGUE = (
    "unknown error",
    "unexpected failure",
    "something went wrong",
    "semantic problem",
    "internal issue",
)
CAUSE_WORDS = (
    "because",
    "expected",
    "found",
    "missing",
    "required",
    "token",
    "type",
    "borrow",
    "module",
    "symbol",
    "target",
    "object",
    "linker",
    "span",
    "source",
)


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def has_precise_cause(text: str) -> bool:
    lowered = text.lower()
    return any(word in lowered for word in CAUSE_WORDS)


def validate_catalog() -> list[str]:
    payload = load_json(CATALOG)
    failures: list[str] = []
    seen: set[str] = set()
    for index, entry in enumerate(payload.get("codes", [])):
        code = str(entry.get("code", ""))
        if not code:
            failures.append(f"codes[{index}]: missing code")
            continue
        if code in seen:
            failures.append(f"{code}: duplicate code")
        seen.add(code)

        title = str(entry.get("title", "")).strip()
        message_key = str(entry.get("message_key", "")).strip()
        severity = str(entry.get("default_severity", "")).strip()
        documentation = entry.get("documentation")
        tests = entry.get("tests")
        aliases = entry.get("aliases")

        if not title:
            failures.append(f"{code}: title is required")
        if not message_key:
            failures.append(f"{code}: attributed message_key is required")
        if not isinstance(aliases, list) or message_key not in aliases:
            failures.append(f"{code}: aliases must include message_key {message_key!r}")
        if severity not in SEVERITY_COLORS:
            failures.append(f"{code}: severity {severity!r} has no canonical color")
        if not isinstance(tests, list) or not tests:
            failures.append(f"{code}: at least one test is required")
        if not isinstance(documentation, dict):
            failures.append(f"{code}: documentation object is required")
        else:
            for field in ("title", "summary", "cause", "action", "example", "url"):
                value = documentation.get(field)
                if not isinstance(value, str) or not value.strip():
                    failures.append(f"{code}: documentation.{field} is required")
            combined = " ".join(str(documentation.get(field, "")) for field in ("title", "summary", "cause", "action"))
            for term in VAGUE:
                if term in combined.lower() and not has_precise_cause(combined):
                    failures.append(f"{code}: vague term {term!r} lacks a precise cause")
    return failures


def validate_color_mapping() -> list[str]:
    canonical = CANONICAL.read_text(encoding="utf-8")
    render = RENDER.read_text(encoding="utf-8")
    checks = (
        (CANONICAL, canonical, "ansi_red()", "canonical red"),
        (CANONICAL, canonical, "ansi_yellow()", "canonical yellow"),
        (CANONICAL, canonical, "ansi_green()", "canonical green"),
        (CANONICAL, canonical, "DiagnosticSeverity.Warning", "canonical warning"),
        (CANONICAL, canonical, "DiagnosticSeverity.Help", "canonical help"),
        (RENDER, render, "ansi_red()", "renderer red"),
        (RENDER, render, "ansi_yellow()", "renderer yellow"),
        (RENDER, render, "ansi_green()", "renderer green"),
    )
    failures: list[str] = []
    for path, text, needle, label in checks:
        if needle not in text:
            failures.append(f"{path.relative_to(ROOT)} missing color contract {label!r}")
    return failures


def main() -> int:
    failures = [*validate_catalog(), *validate_color_mapping()]
    payload = load_json(CATALOG)
    entries = payload.get("codes", [])
    severity_counts = Counter(str(entry.get("default_severity", "")) for entry in entries)
    failed_codes = {failure.split(":", 1)[0] for failure in failures if failure and not failure.startswith("src/")}
    covered = len(entries) - len(failed_codes)
    total = len(entries)
    percent = 100 if not failures else int((covered * 100) / total)
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(
        json.dumps(
            {
                "status": "pass" if not failures else "fail",
                "covered": total if not failures else covered,
                "total": total,
                "percent": 100 if not failures else percent,
                "severity_colors": SEVERITY_COLORS,
                "severity_counts": dict(severity_counts),
                "failures": failures,
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures[:50]:
            print(f"[diagnostic-attribution][error] {failure}", file=sys.stderr)
        print(f"[diagnostic-attribution] coverage={covered}/{total} ({percent}%) report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[diagnostic-attribution] OK coverage={total}/{total} (100%) colors=red,yellow,green report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
