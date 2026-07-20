#!/usr/bin/env python3
"""Audit migration from free-form diagnostics to catalogued structured diagnostics."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BASELINE = ROOT / "schemas" / "diagnostics" / "legacy_audit.json"
STYLE_GUIDE = ROOT / "docs" / "compiler" / "diagnostic_style_guide.md"
CENTRAL_CATALOG = ROOT / "schemas" / "diagnostics" / "codes.json"
SCAN_ROOTS = (
    ROOT / "src" / "vitte" / "compiler",
    ROOT / "src" / "vitte" / "packages" / "compiler",
)

LEGACY_CALL_RE = re.compile(
    r"\b(?:"
    r"parse_error|parse_error_with_span|lex_error|lex_diag|with_diag|"
    r"semantic_diagnostic|semantic_error|semantic_warning|validation_error|"
    r"borrow_diagnostic|session_diagnostic|emit_(?:error|warning|note|fatal)|"
    r"driver_diagnostic"
    r")\s*\("
)
DIRECT_CONCAT_RE = re.compile(
    r"\b(?:with_diag|lex_diag|driver_diagnostic|semantic_diagnostic|"
    r"validation_error|borrow_diagnostic)\s*\([^\n]*\+"
)
CATCH_ALL_RE = re.compile(
    r"\b(?:"
    r"invalid ast expression|invalid ast statement|invalid ast item|"
    r"invalid hir expression|unknown hir expression kind|unknown hir statement kind|"
    r"invalid hir module|parser made no progress|statement parser made no progress|"
    r"codegen\.error|ir\.error|link\.error"
    r")\b",
    re.IGNORECASE,
)
UNSTRUCTURED_DIAGNOSTIC_RE = re.compile(
    r"\b(?:"
    r"form\s+(?:FrontendDiagnostic|AnalysisDiagnostic|CBackendDiagnostic|TypeDiagnostic)\b|"
    r"(?:freeform|raw|legacy)_(?:error|warning|diagnostic)\s*\(|"
    r"(?:error|warning|fatal)_message\s*:\s*string"
    r")"
)
STRUCTURED_DIAGNOSTIC_ALLOWLIST = {
    "src/vitte/compiler/frontend/diagnostics.vit",
    "src/vitte/compiler/frontend/pipeline.vit",
    "src/vitte/compiler/frontend/lexer/literals.vit",
    "src/vitte/compiler/analysis/report.vit",
    "src/vitte/compiler/backend/c/diagnostics.vit",
    "src/vitte/compiler/middle/typecheck/diagnostics.vit",
    "src/vitte/compiler/diagnostics/diagnostic.vit",
}

def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def compiler_sources() -> list[Path]:
    sources: list[Path] = []
    for root in SCAN_ROOTS:
        if not root.exists():
            continue
        sources.extend(
            path
            for path in root.rglob("*")
            if path.suffix in {".vit", ".c", ".h"}
            and "/tests/" not in path.as_posix()
            and not path.as_posix().endswith("/tests.vit")
        )
    return sorted(sources)


def count_by_file(pattern: re.Pattern[str]) -> dict[str, int]:
    counts: dict[str, int] = {}
    for path in compiler_sources():
        text = path.read_text(encoding="utf-8")
        count = len(pattern.findall(text))
        if count:
            counts[rel(path)] = count
    return counts


def compare_counts(name: str, current: dict[str, int], allowed: dict[str, int]) -> list[str]:
    failures: list[str] = []
    for path, count in sorted(current.items()):
        limit = int(allowed.get(path, 0))
        if count > limit:
            failures.append(f"{name}: {path} has {count} site(s), baseline allows {limit}")
    for path, count in sorted(allowed.items()):
        if path not in current:
            failures.append(f"{name}: {path} baseline has {count} site(s) but code now has 0; remove the stale baseline entry")
        elif current[path] < int(count):
            failures.append(f"{name}: {path} dropped from {count} to {current[path]}; lower the baseline in the same migration")
    return failures


def validate_four_question_catalog() -> list[str]:
    payload = json.loads(CENTRAL_CATALOG.read_text(encoding="utf-8"))
    failures: list[str] = []
    for entry in payload.get("codes", []):
        code = entry.get("code", "<missing>")
        documentation = entry.get("documentation")
        if not isinstance(documentation, dict):
            failures.append(f"{code}: documentation object is required")
            continue
        field_map = {
            "incorrect": "title",
            "location": "summary",
            "reason": "cause",
            "correction": "action",
        }
        for question, field in field_map.items():
            value = documentation.get(field)
            if not isinstance(value, str) or not value.strip():
                failures.append(f"{code}: documentation.{field} must answer {question!r} from the diagnostic style guide")
    return failures


def validate_review_rule() -> list[str]:
    text = STYLE_GUIDE.read_text(encoding="utf-8")
    required = (
        "Code review must reject",
        "does not answer all",
        "Do not concatenate user-facing error strings",
    )
    return [
        f"{rel(STYLE_GUIDE)}: missing required review rule text {item!r}"
        for item in required
        if item not in text
    ]

def validate_no_unstructured_diagnostics() -> list[str]:
    failures: list[str] = []
    for path in compiler_sources():
        path_rel = rel(path)
        if path_rel in STRUCTURED_DIAGNOSTIC_ALLOWLIST:
            continue
        text = path.read_text(encoding="utf-8")
        if UNSTRUCTURED_DIAGNOSTIC_RE.search(text):
            failures.append(f"unstructured diagnostic shape is forbidden outside canonical adapters: {path_rel}")
    return failures


def main() -> int:
    baseline = json.loads(BASELINE.read_text(encoding="utf-8"))
    failures: list[str] = []
    failures.extend(validate_review_rule())
    failures.extend(validate_four_question_catalog())
    failures.extend(compare_counts("legacy diagnostic call", count_by_file(LEGACY_CALL_RE), baseline.get("legacy_call_sites", {})))
    failures.extend(compare_counts("direct diagnostic message concatenation", count_by_file(DIRECT_CONCAT_RE), baseline.get("direct_message_concat_sites", {})))
    failures.extend(compare_counts("catch-all diagnostic message", count_by_file(CATCH_ALL_RE), baseline.get("catch_all_messages", {})))
    failures.extend(validate_no_unstructured_diagnostics())

    if failures:
        for failure in failures:
            print(f"[diagnostics-migration][error] {failure}", file=sys.stderr)
        return 1
    print("[diagnostics-migration] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
