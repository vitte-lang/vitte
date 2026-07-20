#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ANALYSIS_DIR = ROOT / "src" / "vitte" / "compiler" / "analysis" / "borrowck"
MIDDLE_DIR = ROOT / "src" / "vitte" / "compiler" / "middle" / "borrow"
TESTS = ROOT / "src" / "vitte" / "compiler" / "tests" / "borrowck_tests.vit"
MANIFEST = ROOT / "src" / "vitte" / "compiler" / "tests" / "borrowck_coverage_manifest.json"
DIAG_FIXTURES = ROOT / "src" / "vitte" / "compiler" / "tests" / "diagnostics" / "borrowck"
REPORT_DIR = ROOT / "target" / "reports" / "borrowck_coverage"
SNAPSHOT_DIR = ROOT / "src" / "vitte" / "compiler" / "tests" / "snapshots" / "borrowck"
JSON_SNAPSHOT = SNAPSHOT_DIR / "coverage.json.snap"
TEXT_SNAPSHOT = SNAPSHOT_DIR / "coverage.txt.snap"

SUPPORT_STATUSES = {"supported", "hir_only", "mir_only", "planned", "not_supported"}
OWNER_PHASES = {"borrowck", "typeck", "mir", "lower_hir", "diagnostics"}
EXTERNAL_TESTS = {"borrowck-fixtures", "borrowck-snapshots", "borrowck-gate"}
REQUIRED_DIAGNOSTIC_FIXTURES = {
    "use_after_move.vit",
    "use_after_move.json.snap",
    "use_after_move.txt.snap",
    "complex_alias_path.vit",
    "complex_alias_path.json.snap",
    "complex_alias_path.txt.snap",
}
REQUIRED_BORROWCK_FIXTURES = {
    "valid/basic.vit": "valid basic ownership",
    "valid/reinit_after_move.vit": "valid reinitialization after move",
    "valid/copy_does_not_move.vit": "copy type does not move source",
    "valid/borrow_scope_end.vit": "borrow expires at scope end",
    "invalid/use_after_move.vit": "use after move",
    "invalid/move_after_move.vit": "move after move",
    "invalid/write_while_borrowed.vit": "write while borrowed",
    "invalid/mutable_borrow_conflict.vit": "mutable borrow conflict",
    "invalid/return_ref_to_local.vit": "return reference to local",
    "invalid/partial_move_root_use.vit": "partial move then root use",
    "multifile/app.vit": "multifile app using exported non-Copy type and taking ownership",
    "multifile/box.vit": "multifile exported non-Copy type",
    "multifile/take.vit": "multifile imported function taking ownership",
}


def rel(path: Path) -> str:
    return str(path.relative_to(ROOT))


def read(path: Path) -> str:
    if not path.is_file():
        raise SystemExit(f"[borrowck-coverage][error] missing file: {rel(path)}")
    return path.read_text(encoding="utf-8", errors="replace")


def test_names(text: str) -> set[str]:
    return set(re.findall(r"(?m)^\s*proc\s+(test_[A-Za-z0-9_]+)\s*\(", text))


def module_name(path: Path) -> str:
    if path.is_relative_to(ANALYSIS_DIR):
        return "analysis/" + path.relative_to(ANALYSIS_DIR).with_suffix("").as_posix()
    return "middle/" + path.relative_to(MIDDLE_DIR).with_suffix("").as_posix()


def declared_surfaces() -> list[str]:
    surfaces: list[str] = []
    for path in sorted([*ANALYSIS_DIR.glob("*.vit"), *MIDDLE_DIR.glob("*.vit")]):
        text = read(path)
        module = module_name(path)
        for pick in re.findall(r"(?m)^\s*pick\s+([A-Z][A-Za-z0-9_]+)\s*\{", text):
            surfaces.append(f"{module}.{pick}")
        for form in re.findall(r"(?m)^\s*form\s+([A-Z][A-Za-z0-9_]+)\s*\{", text):
            surfaces.append(f"{module}.{form}")
    return sorted(set(surfaces))


def evidence_tests(entry: dict[str, object]) -> list[str]:
    evidence = entry.get("evidence", {})
    if isinstance(evidence, dict):
        tests = evidence.get("tests", [])
        if isinstance(tests, list):
            return [str(test) for test in tests]
    return []


def load_manifest() -> dict[str, object]:
    return json.loads(read(MANIFEST))


def build_payload() -> tuple[dict[str, object], list[str], str, str]:
    tests_text = read(TESTS)
    manifest = load_manifest()
    entries = manifest.get("entries", [])
    known_tests = test_names(tests_text) | EXTERNAL_TESTS
    failures: list[str] = []

    required_tests = manifest.get("required_tests", [])
    for name in required_tests:
        if name not in known_tests:
            failures.append(f"missing required borrowck test `{name}`")
        elif name.startswith("test_") and f"if not {name}()" not in tests_text:
            failures.append(f"borrowck test `{name}` is not called by run_all_tests")

    covered: set[str] = set()
    rows: list[dict[str, object]] = []
    supported_total = 0
    supported_covered = 0
    for index, entry in enumerate(entries):
        surface = str(entry.get("surface", ""))
        tests = entry.get("tests", [])
        status = str(entry.get("support_status", ""))
        owner_phase = str(entry.get("owner_phase", ""))
        evidence = entry.get("evidence", {})
        if not surface:
            failures.append(f"entry {index} has no surface")
        if surface in covered:
            failures.append(f"duplicate manifest entry `{surface}`")
        covered.add(surface)
        if status not in SUPPORT_STATUSES:
            failures.append(f"{surface}: invalid or missing support_status `{status}`")
        if owner_phase not in OWNER_PHASES:
            failures.append(f"{surface}: invalid or missing owner_phase `{owner_phase}`")
        if not isinstance(tests, list):
            failures.append(f"{surface}: tests must be a list")
            tests = []
        if not isinstance(evidence, dict) or not evidence.get("assertion"):
            failures.append(f"{surface}: missing evidence assertion")
        if evidence_tests(entry) != tests:
            failures.append(f"{surface}: evidence tests must match tests")
        if status == "supported":
            supported_total += 1
            if tests:
                supported_covered += 1
            else:
                failures.append(f"{surface}: supported entry has no evidence test")
        for test in tests:
            if test not in known_tests:
                failures.append(f"{surface}: unknown test `{test}`")
        rows.append({
            "surface": surface,
            "source": entry.get("source", ""),
            "producer": entry.get("producer", ""),
            "consumer": entry.get("consumer", ""),
            "support_status": status,
            "owner_phase": owner_phase,
            "tests": tests,
            "evidence": evidence,
            "covered": bool(tests),
        })

    declared = declared_surfaces()
    uncovered = sorted(surface for surface in declared if surface not in covered)
    stale = sorted(surface for surface in covered if surface not in declared)
    if uncovered:
        failures.append("borrowck surfaces missing manifest entries: " + ", ".join(uncovered))
    if stale:
        failures.append("borrowck manifest entries without declared surface: " + ", ".join(stale))

    rows.sort(key=lambda row: str(row["surface"]))
    payload: dict[str, object] = {
        "schema": "vitte.compiler.borrowck_coverage",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "declared_surface_count": len(declared),
        "covered_surface_count": len([surface for surface in declared if surface in covered]),
        "manifest_entry_count": len(covered),
        "supported_total": supported_total,
        "supported_covered": supported_covered,
        "uncovered_surfaces": uncovered,
        "stale_surfaces": stale,
        "coverage_table": rows,
        "failures": failures,
    }

    lines = [
        "# Borrowck Coverage",
        "",
        "| Surface | Status | Owner | Source | Tests | Evidence |",
        "| --- | --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        evidence = row["evidence"].get("assertion", "") if isinstance(row["evidence"], dict) else ""
        lines.append(
            f"| {row['surface']} | {row['support_status']} | {row['owner_phase']} | "
            f"{row['source']} | {', '.join(row['tests'])} | {evidence} |"
        )
    lines.extend(["", "## Uncovered Surfaces", ""])
    lines.extend(f"- `{surface}`" for surface in uncovered) if uncovered else lines.append("- None.")
    text_snapshot = "\n".join(lines) + "\n"

    remaining = [
        "# Borrowck Remaining Coverage",
        "",
        "This report is generated by `tools/borrowck_coverage_check.py`.",
        "",
        "## Current State",
        "",
        f"- Declared surfaces: {payload['declared_surface_count']}",
        f"- Manifest entries: {payload['manifest_entry_count']}",
        f"- Supported coverage: {payload['supported_covered']}/{payload['supported_total']}",
        f"- Missing manifest entries: {len(uncovered)}",
        "",
        "## Missing Manifest Entries",
        "",
    ]
    remaining.extend(f"- `{surface}`" for surface in uncovered) if uncovered else remaining.append("- None.")
    remaining.extend([
        "",
        "## Remaining Hardening Tasks",
        "",
        "- Add compiler-emitted JSON snapshots for every borrowck diagnostic fixture.",
        "- Add real multifile borrowck fixtures for imported ownership and borrowed parameters.",
        "- Add exact span assertions for move, use, borrow, write, drop and return diagnostics.",
        "- Compare HIR-only and HIR-to-MIR borrowck paths for every supported ownership rule.",
        "- Audit and either retire or clearly separate the older `middle/borrow` helper layer.",
    ])
    return payload, failures, text_snapshot, "\n".join(remaining) + "\n"


def write_reports() -> list[str]:
    payload, failures, text_snapshot, remaining = build_payload()
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    SNAPSHOT_DIR.mkdir(parents=True, exist_ok=True)
    snapshot_payload = {key: payload[key] for key in (
        "schema",
        "schema_version",
        "declared_surface_count",
        "covered_surface_count",
        "manifest_entry_count",
        "supported_total",
        "supported_covered",
        "uncovered_surfaces",
        "stale_surfaces",
        "coverage_table",
    )}
    json_snapshot = json.dumps(snapshot_payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n"
    (REPORT_DIR / "coverage.json").write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    (REPORT_DIR / "coverage.md").write_text(text_snapshot, encoding="utf-8")
    (REPORT_DIR / "remaining.md").write_text(remaining, encoding="utf-8")
    (REPORT_DIR / "coverage.json.snap").write_text(json_snapshot, encoding="utf-8")
    (REPORT_DIR / "coverage.txt.snap").write_text(text_snapshot, encoding="utf-8")
    if not JSON_SNAPSHOT.is_file():
        failures.append(f"missing borrowck JSON snapshot: {rel(JSON_SNAPSHOT)}")
    elif JSON_SNAPSHOT.read_text(encoding="utf-8") != json_snapshot:
        failures.append(f"borrowck JSON snapshot drift: {rel(JSON_SNAPSHOT)}")
    if not TEXT_SNAPSHOT.is_file():
        failures.append(f"missing borrowck text snapshot: {rel(TEXT_SNAPSHOT)}")
    elif TEXT_SNAPSHOT.read_text(encoding="utf-8") != text_snapshot:
        failures.append(f"borrowck text snapshot drift: {rel(TEXT_SNAPSHOT)}")
    return failures


def check_fixtures() -> int:
    failures: list[str] = []
    if not TESTS.is_file():
        failures.append(f"missing test file: {rel(TESTS)}")
    for name in sorted(REQUIRED_DIAGNOSTIC_FIXTURES):
        path = DIAG_FIXTURES / name
        if not path.is_file():
            failures.append(f"missing diagnostic fixture: {rel(path)}")
        elif not path.read_text(encoding="utf-8", errors="replace").strip():
            failures.append(f"empty diagnostic fixture: {rel(path)}")
    borrowck_root = ROOT / "tests" / "borrowck"
    for name in sorted(REQUIRED_BORROWCK_FIXTURES):
        path = borrowck_root / name
        if not path.is_file():
            failures.append(f"missing borrowck fixture: {rel(path)}")
        else:
            text = path.read_text(encoding="utf-8", errors="replace")
            if not text.strip():
                failures.append(f"empty borrowck fixture: {rel(path)}")
            if "space " not in text:
                failures.append(f"fixture has no namespace: {rel(path)}")
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    report = ["# Borrowck Fixtures", "", "| Fixture | Status |", "| --- | --- |"]
    for name in sorted(REQUIRED_DIAGNOSTIC_FIXTURES):
        report.append(f"| {rel(DIAG_FIXTURES / name)} | present |")
    for name, purpose in sorted(REQUIRED_BORROWCK_FIXTURES.items()):
        report.append(f"| tests/borrowck/{name} | {purpose} |")
    (REPORT_DIR / "fixtures.md").write_text("\n".join(report) + "\n", encoding="utf-8")
    for failure in failures:
        print(f"[borrowck-fixtures][error] {failure}", file=sys.stderr)
    fixture_count = len(REQUIRED_DIAGNOSTIC_FIXTURES) + len(REQUIRED_BORROWCK_FIXTURES)
    print(f"[borrowck-fixtures] status={'fail' if failures else 'pass'} fixtures={fixture_count} report={rel(REPORT_DIR / 'fixtures.md')}")
    return 1 if failures else 0


def check_snapshots() -> int:
    failures = write_reports()
    for failure in failures:
        print(f"[borrowck-snapshots][error] {failure}", file=sys.stderr)
    print(f"[borrowck-snapshots] status={'fail' if failures else 'pass'} report={rel(REPORT_DIR)}")
    return 1 if failures else 0


def check_coverage() -> int:
    failures = write_reports()
    payload = json.loads((REPORT_DIR / "coverage.json").read_text(encoding="utf-8"))
    print(
        "[borrowck-coverage] "
        f"status={'fail' if failures else 'pass'} covered={payload['covered_surface_count']} "
        f"declared={payload['declared_surface_count']} entries={payload['manifest_entry_count']} "
        f"supported={payload['supported_covered']}/{payload['supported_total']} "
        f"report={rel(REPORT_DIR / 'coverage.json')}"
    )
    for failure in failures:
        print(f"[borrowck-coverage][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixtures", action="store_true")
    parser.add_argument("--snapshots", action="store_true")
    args = parser.parse_args()
    if args.fixtures:
        return check_fixtures()
    if args.snapshots:
        return check_snapshots()
    return check_coverage()


if __name__ == "__main__":
    raise SystemExit(main())
