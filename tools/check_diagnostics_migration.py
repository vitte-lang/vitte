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
    r"(?:freeform|raw|legacy)_(?:error|warning|diagnostic)|"
    r"with_diag|semantic_error|semantic_warning|lex_error"
    r")\s*\("
)
DIRECT_CONCAT_RE = re.compile(
    r"\b(?:with_diag|(?:freeform|raw|legacy)_(?:error|warning|diagnostic)|"
    r"semantic_error|semantic_warning|lex_error)\s*\([^\n]*\+"
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
REMOVED_LEGACY_DIAGNOSTIC_PATHS = (
    "src/vitte/compiler/diagnostics/legacy.vit",
    "src/vitte/compiler/diagnostics/freeform.vit",
    "src/vitte/compiler/diagnostics/raw.vit",
    "src/vitte/compiler/infrastructure/diagnostics/legacy_emitter.vit",
)
REMOVED_LEGACY_ENTRYPOINT_RE = re.compile(
    r"\b(?:legacy_diagnostic_system|freeform_diagnostic_system|raw_diagnostic_emitter|"
    r"LegacyDiagnostic|FreeformDiagnostic|RawDiagnostic)\b"
)

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


def migration_debt_counts() -> dict[str, int]:
    legacy_calls = count_by_file(LEGACY_CALL_RE)
    direct_concats = count_by_file(DIRECT_CONCAT_RE)
    catch_all = count_by_file(CATCH_ALL_RE)
    return {
        "legacy_call_sites": sum(legacy_calls.values()),
        "direct_message_concat_sites": sum(direct_concats.values()),
        "catch_all_messages": sum(catch_all.values()),
    }


def render_migration_debt_counts(counts: dict[str, int]) -> str:
    total = counts["legacy_call_sites"] + counts["direct_message_concat_sites"] + counts["catch_all_messages"]
    return (
        "[diagnostics-migration] unmigrated="
        + str(total)
        + " legacy_call_sites="
        + str(counts["legacy_call_sites"])
        + " direct_message_concat_sites="
        + str(counts["direct_message_concat_sites"])
        + " catch_all_messages="
        + str(counts["catch_all_messages"])
    )


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
        "must state the cause, the",
        "location, the reason it violates Vitte, and the correction",
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


def validate_legacy_system_removed() -> list[str]:
    failures: list[str] = []
    for path in REMOVED_LEGACY_DIAGNOSTIC_PATHS:
        if (ROOT / path).exists():
            failures.append(f"removed legacy diagnostic system file must not exist: {path}")
    for path in compiler_sources():
        text = path.read_text(encoding="utf-8")
        if REMOVED_LEGACY_ENTRYPOINT_RE.search(text):
            failures.append(f"removed legacy diagnostic system entry point is forbidden: {rel(path)}")
    return failures


def main() -> int:
    baseline = json.loads(BASELINE.read_text(encoding="utf-8"))
    debt_counts = migration_debt_counts()
    failures: list[str] = []
    failures.extend(validate_review_rule())
    failures.extend(validate_four_question_catalog())
    failures.extend(compare_counts("legacy diagnostic call", count_by_file(LEGACY_CALL_RE), baseline.get("legacy_call_sites", {})))
    failures.extend(compare_counts("direct diagnostic message concatenation", count_by_file(DIRECT_CONCAT_RE), baseline.get("direct_message_concat_sites", {})))
    failures.extend(compare_counts("catch-all diagnostic message", count_by_file(CATCH_ALL_RE), baseline.get("catch_all_messages", {})))
    failures.extend(validate_no_unstructured_diagnostics())
    failures.extend(validate_legacy_system_removed())

    if failures:
        for failure in failures:
            print(f"[diagnostics-migration][error] {failure}", file=sys.stderr)
        return 1
    print(render_migration_debt_counts(debt_counts))
    print("[diagnostics-migration] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
