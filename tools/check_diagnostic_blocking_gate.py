#!/usr/bin/env python3
"""Blocking gate for diagnostic locale, explanation, suggestion, order, dedup.

This is intentionally catalog-wide. A diagnostic code is not release-ready when
it lacks a localized message, explain fields, correction guidance, stable-order
coverage, or deduplication coverage.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "schemas" / "diagnostics" / "codes.json"
CORE_CODES = ROOT / "tests" / "diag_snapshots" / "core_diagnostic_codes.txt"
SNAPSHOT_ROOT = ROOT / "tests" / "diagnostics" / "catalog" / "snapshots"
REPORT = ROOT / "target" / "reports" / "diagnostic_blocking_gate" / "coverage.json"

sys.path.insert(0, str(ROOT / "tools"))
from diagnostic_catalog_data import public_diagnostic_codes  # noqa: E402
from diagnostics_locales import supported_locale_codes  # noqa: E402


REQUIRED_EXPLAIN_SUFFIXES = ("summary", "cause", "step1", "fix", "example")
REQUIRED_ASSERTS = {
    "code",
    "title",
    "span",
    "notes",
    "suggestions",
    "no_parasitic_diagnostics",
    "stable_order",
    "deduplication",
    "recovery",
}
REQUIRED_SNAPSHOT_MARKERS = {
    "suggestions": "suggestions",
    "no_parasitic_diagnostics": "no_parasitic_diagnostics",
    "stable_order": "stable_order",
    "deduplication": "deduplication",
}
ACTION_WORDS = re.compile(
    r"\b(?:add|adjust|check|choose|compare|complete|declare|find|fix|import|"
    r"insert|inspect|look|move|provide|qualify|reduce|remove|rename|replace|"
    r"re-run|select|shorten|split|start|use|verify|write)\b",
    re.IGNORECASE,
)
VAGUE_SUGGESTIONS = (
    "fix this",
    "try changing this",
    "maybe use",
    "something else",
    "correct the error",
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_ftl(path: Path) -> dict[str, str]:
    entries: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        entries[key.strip()] = value.strip()
    return entries


def selected_locale_codes() -> list[str]:
    existing: list[str] = []
    if CORE_CODES.exists():
        existing = [
            line.strip()
            for line in CORE_CODES.read_text(encoding="utf-8").splitlines()
            if line.strip() and not line.strip().startswith("#")
        ]
    return public_diagnostic_codes(existing)


def catalog_entries(failures: list[str]) -> list[dict[str, Any]]:
    payload = load_json(CATALOG)
    entries = payload.get("codes")
    if not isinstance(entries, list):
        failures.append(f"{rel(CATALOG)}: codes array is required")
        return []
    return [entry for entry in entries if isinstance(entry, dict)]


def validate_locales(codes: list[str]) -> dict[str, Any]:
    failures: list[str] = []
    locales: dict[str, Any] = {}
    for locale in supported_locale_codes():
        messages_path = ROOT / "locales" / locale / "diagnostics.ftl"
        explain_path = ROOT / "locales" / locale / "diagnostics_explain.ftl"
        if not messages_path.is_file():
            failures.append(f"{rel(messages_path)} missing")
            continue
        if not explain_path.is_file():
            failures.append(f"{rel(explain_path)} missing")
            continue
        messages = parse_ftl(messages_path)
        explain = parse_ftl(explain_path)
        missing_messages = [code for code in codes if code not in messages]
        missing_explain = [
            f"{code}.{suffix}"
            for code in codes
            for suffix in REQUIRED_EXPLAIN_SUFFIXES
            if f"{code}.{suffix}" not in explain
        ]
        locales[locale] = {
            "missing_explanations": missing_explain,
            "missing_messages": missing_messages,
            "status": "pass" if not missing_messages and not missing_explain else "fail",
        }
        failures.extend(f"{rel(messages_path)} missing message {code}" for code in missing_messages[:50])
        failures.extend(f"{rel(explain_path)} missing explain {key}" for key in missing_explain[:50])
    return {"failures": failures, "locales": locales}


def validate_suggestion_text(code: str, documentation: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    action = documentation.get("action")
    example = documentation.get("example")
    cause = documentation.get("cause")
    for field, value in (("cause", cause), ("action", action), ("example", example)):
        if not isinstance(value, str) or not value.strip():
            failures.append(f"{code}: documentation.{field} is required")
    if isinstance(action, str):
        lowered = action.lower()
        if not ACTION_WORDS.search(action):
            failures.append(f"{code}: documentation.action must contain a concrete correction verb")
        for vague in VAGUE_SUGGESTIONS:
            if vague in lowered:
                failures.append(f"{code}: documentation.action contains vague suggestion text {vague!r}")
    return failures


def validate_catalog_requirements(entries: list[dict[str, Any]]) -> dict[str, Any]:
    failures: list[str] = []
    seen: set[str] = set()
    per_code: dict[str, Any] = {}
    for entry in entries:
        code = str(entry.get("code", ""))
        if not code:
            failures.append("catalog entry without code")
            continue
        if code in seen:
            failures.append(f"{code}: duplicate diagnostic code")
        seen.add(code)

        documentation = entry.get("documentation")
        if not isinstance(documentation, dict):
            failures.append(f"{code}: documentation is required")
            documentation = {}
        failures.extend(validate_suggestion_text(code, documentation))

        tests = entry.get("tests")
        if not isinstance(tests, list) or not tests:
            failures.append(f"{code}: tests are required")
            tests = []

        code_asserts: set[str] = set()
        snapshot_paths: list[str] = []
        for test in tests:
            if not isinstance(test, dict):
                failures.append(f"{code}: test entry must be an object")
                continue
            asserts = test.get("asserts")
            if isinstance(asserts, list):
                code_asserts.update(str(item) for item in asserts)
            else:
                failures.append(f"{code}: test entry missing asserts")
            path = test.get("path")
            case = test.get("case")
            if isinstance(path, str) and isinstance(case, str):
                manifest_path = ROOT / path
                if manifest_path.is_file():
                    manifest = load_json(manifest_path)
                    for item in manifest.get("cases", []):
                        if isinstance(item, dict) and item.get("code") == case and isinstance(item.get("snapshot"), str):
                            snapshot_paths.append(str(item["snapshot"]))
                else:
                    failures.append(f"{code}: test manifest missing: {path}")

        missing_asserts = sorted(REQUIRED_ASSERTS - code_asserts)
        if missing_asserts:
            failures.append(f"{code}: tests must assert {missing_asserts}")

        missing_snapshot_markers: list[str] = []
        for snapshot in snapshot_paths:
            path = ROOT / snapshot
            if not path.is_file():
                failures.append(f"{code}: snapshot missing: {snapshot}")
                continue
            text = path.read_text(encoding="utf-8")
            for name, marker in REQUIRED_SNAPSHOT_MARKERS.items():
                if marker not in text:
                    missing_snapshot_markers.append(f"{snapshot}:{name}")
        if not snapshot_paths:
            failures.append(f"{code}: no catalog snapshot evidence found")
        if missing_snapshot_markers:
            failures.append(f"{code}: snapshot evidence missing {', '.join(missing_snapshot_markers[:10])}")

        per_code[code] = {
            "asserts": sorted(code_asserts),
            "snapshots": sorted(snapshot_paths),
            "status": "pass" if not missing_asserts and not missing_snapshot_markers else "fail",
        }
    return {"failures": failures, "codes": per_code}


def validate_central_snapshots() -> list[str]:
    failures: list[str] = []
    central = SNAPSHOT_ROOT / "central"
    if not central.is_dir():
        return [f"{rel(central)}: central diagnostic snapshots are required"]
    for path in sorted(central.glob("*.snap")):
        text = path.read_text(encoding="utf-8")
        for name, marker in REQUIRED_SNAPSHOT_MARKERS.items():
            if marker not in text:
                failures.append(f"{rel(path)} missing {name} marker")
    return failures


def main() -> int:
    failures: list[str] = []
    entries = catalog_entries(failures)
    locale_result = validate_locales(selected_locale_codes())
    catalog_result = validate_catalog_requirements(entries)
    central_failures = validate_central_snapshots()
    failures.extend(locale_result["failures"])
    failures.extend(catalog_result["failures"])
    failures.extend(central_failures)

    report = {
        "schema": "vitte.diagnostic.blocking_gate.v1",
        "status": "pass" if not failures else "fail",
        "catalog_codes": len(entries),
        "locale_codes": len(selected_locale_codes()),
        "required_asserts": sorted(REQUIRED_ASSERTS),
        "locales": locale_result["locales"],
        "codes": catalog_result["codes"],
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if failures:
        for failure in failures[:100]:
            print(f"[diagnostic-blocking-gate][error] {failure}", file=sys.stderr)
        print(f"[diagnostic-blocking-gate] failures={len(failures)} report={rel(REPORT)}", file=sys.stderr)
        return 1

    print(
        "[diagnostic-blocking-gate] OK "
        f"catalog_codes={len(entries)} locale_codes={len(selected_locale_codes())} "
        f"locales={len(supported_locale_codes())} report={rel(REPORT)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
