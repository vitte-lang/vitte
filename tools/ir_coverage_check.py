#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
IR_DIR = ROOT / "src" / "vitte" / "compiler" / "backend" / "ir"
TESTS = ROOT / "src" / "vitte" / "compiler" / "tests" / "codegen_tests.vit"
MANIFEST = ROOT / "src" / "vitte" / "compiler" / "tests" / "ir_coverage_manifest.json"
REPORT_DIR = ROOT / "target" / "reports" / "ir_coverage"
SNAPSHOT_DIR = ROOT / "src" / "vitte" / "compiler" / "tests" / "snapshots" / "ir"
JSON_SNAPSHOT = SNAPSHOT_DIR / "coverage.json.snap"
TEXT_SNAPSHOT = SNAPSHOT_DIR / "coverage.txt.snap"
FIXTURE_JSON_SNAPSHOT = SNAPSHOT_DIR / "fixtures.json.snap"
FIXTURE_TEXT_SNAPSHOT = SNAPSHOT_DIR / "fixtures.txt.snap"

SUPPORT_STATUSES = {"supported", "hir_only", "ir_only", "planned", "not_supported"}
OWNER_PHASES = {"mir_to_ir", "ir", "ir_validate", "codegen", "linker", "diagnostics"}
EXTERNAL_TESTS = {"ir-fixtures", "ir-snapshots", "ir-gate", "backend-ir-contract-audit", "core-ir-golden-snapshots"}
REQUIRED_FIXTURES = {
    "tests/diag_snapshots/core_ir_golden.vit",
    "tests/diag_snapshots/core_ir_golden.must",
    "tests/diag_snapshots/core_ir_golden_manifest.txt",
    "tests/diagnostics/catalog/ir.catalog.json",
    "tests/ir/valid/basic.vit",
    "tests/ir/valid/control_flow.vit",
    "tests/ir/valid/calls.vit",
    "tests/ir/valid/generics.vit",
    "tests/ir/valid/borrows.vit",
    "tests/ir/valid/aggregates.vit",
    "tests/ir/invalid/bad_cfg.vit",
    "tests/ir/invalid/bad_call_arity.vit",
    "tests/ir/invalid/bad_borrow.vit",
    "tests/ir/invalid/bad_generic_arity.vit",
    "tests/ir/multifile/app.vit",
    "tests/ir/multifile/lib.vit",
}


def rel(path: Path) -> str:
    return str(path.relative_to(ROOT))


def read(path: Path) -> str:
    if not path.is_file():
        raise SystemExit(f"[ir-coverage][error] missing file: {rel(path)}")
    return path.read_text(encoding="utf-8", errors="replace")


def test_names(text: str) -> set[str]:
    return set(re.findall(r"(?m)^\s*proc\s+(test_[A-Za-z0-9_]+)\s*\(", text))


def module_name(path: Path) -> str:
    return path.relative_to(IR_DIR).with_suffix("").as_posix()


def declared_surfaces() -> list[str]:
    surfaces: list[str] = []
    for path in sorted(IR_DIR.glob("*.vit")):
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


def build_payload() -> tuple[dict[str, object], list[str], str, str]:
    tests_text = read(TESTS)
    manifest = json.loads(read(MANIFEST))
    entries = manifest.get("entries", [])
    known_tests = test_names(tests_text) | EXTERNAL_TESTS
    failures: list[str] = []

    for name in manifest.get("required_tests", []):
        if name not in known_tests:
            failures.append(f"missing required IR test `{name}`")
        elif name.startswith("test_") and f"if not {name}()" not in tests_text:
            failures.append(f"IR test `{name}` is not called by run_all_tests")

    covered: set[str] = set()
    rows: list[dict[str, object]] = []
    supported_total = 0
    supported_covered = 0
    status_totals = {status: 0 for status in sorted(SUPPORT_STATUSES)}
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
        else:
            status_totals[status] += 1
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
        failures.append("IR surfaces missing manifest entries: " + ", ".join(uncovered))
    if stale:
        failures.append("IR manifest entries without declared surface: " + ", ".join(stale))

    rows.sort(key=lambda row: str(row["surface"]))
    payload: dict[str, object] = {
        "schema": "vitte.compiler.ir_coverage",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "declared_surface_count": len(declared),
        "covered_surface_count": len([surface for surface in declared if surface in covered]),
        "manifest_entry_count": len(covered),
        "supported_total": supported_total,
        "supported_covered": supported_covered,
        "status_totals": status_totals,
        "uncovered_surfaces": uncovered,
        "stale_surfaces": stale,
        "coverage_table": rows,
        "failures": failures,
    }

    lines = [
        "# IR Coverage",
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
        "# IR Remaining Coverage",
        "",
        "This report is generated by `tools/ir_coverage_check.py`.",
        "",
        "## Current State",
        "",
        f"- Declared surfaces: {payload['declared_surface_count']}",
        f"- Manifest entries: {payload['manifest_entry_count']}",
        f"- Supported coverage: {payload['supported_covered']}/{payload['supported_total']}",
        f"- Planned surfaces: {status_totals['planned']}",
        f"- HIR-only surfaces: {status_totals['hir_only']}",
        f"- IR-only surfaces: {status_totals['ir_only']}",
        f"- Not-supported surfaces: {status_totals['not_supported']}",
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
        "- Expand compiler-emitted JSON snapshots beyond the current representative IR fixture set.",
        "- Add executable checks for every `tests/ir` source fixture once the CLI exposes stable per-fixture IR dumps.",
        "- Add structured IR diagnostic snapshots when IR validation emits diagnostics directly.",
        "- Audit legacy `src/vitte/compiler/ir/mir_extended.vit` and `src/vitte/compiler/ir/hir_to_mir_lowering.vit` against canonical middle IR.",
        "- Keep every `supported` surface backed by a named test and concrete assertion.",
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
        "status_totals",
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
        failures.append(f"missing IR JSON snapshot: {rel(JSON_SNAPSHOT)}")
    elif JSON_SNAPSHOT.read_text(encoding="utf-8") != json_snapshot:
        failures.append(f"IR JSON snapshot drift: {rel(JSON_SNAPSHOT)}")
    if not TEXT_SNAPSHOT.is_file():
        failures.append(f"missing IR text snapshot: {rel(TEXT_SNAPSHOT)}")
    elif TEXT_SNAPSHOT.read_text(encoding="utf-8") != text_snapshot:
        failures.append(f"IR text snapshot drift: {rel(TEXT_SNAPSHOT)}")
    fixture_json, fixture_text = fixture_snapshots()
    (REPORT_DIR / "fixtures.json.snap").write_text(fixture_json, encoding="utf-8")
    (REPORT_DIR / "fixtures.txt.snap").write_text(fixture_text, encoding="utf-8")
    if not FIXTURE_JSON_SNAPSHOT.is_file():
        failures.append(f"missing IR fixture JSON snapshot: {rel(FIXTURE_JSON_SNAPSHOT)}")
    elif FIXTURE_JSON_SNAPSHOT.read_text(encoding="utf-8") != fixture_json:
        failures.append(f"IR fixture JSON snapshot drift: {rel(FIXTURE_JSON_SNAPSHOT)}")
    if not FIXTURE_TEXT_SNAPSHOT.is_file():
        failures.append(f"missing IR fixture text snapshot: {rel(FIXTURE_TEXT_SNAPSHOT)}")
    elif FIXTURE_TEXT_SNAPSHOT.read_text(encoding="utf-8") != fixture_text:
        failures.append(f"IR fixture text snapshot drift: {rel(FIXTURE_TEXT_SNAPSHOT)}")
    return failures


def fixture_snapshots() -> tuple[str, str]:
    cases = []
    for name in sorted(REQUIRED_FIXTURES):
        path = ROOT / name
        text = path.read_text(encoding="utf-8", errors="replace") if path.is_file() else ""
        cases.append({
            "fixture": name,
            "lines": len(text.splitlines()),
            "has_namespace": "space " in text,
            "kind": "source" if name.endswith(".vit") else "snapshot",
        })
    payload = {
        "schema": "vitte.compiler.ir_fixture_snapshots",
        "schema_version": "1.0.0",
        "cases": cases,
    }
    json_text = json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n"
    lines = [
        "# IR Fixture Snapshots",
        "",
        "| Fixture | Kind | Lines | Namespace |",
        "| --- | --- | --- | --- |",
    ]
    for case in cases:
        lines.append(f"| {case['fixture']} | {case['kind']} | {case['lines']} | {case['has_namespace']} |")
    return json_text, "\n".join(lines) + "\n"


def check_fixtures() -> int:
    failures: list[str] = []
    if not TESTS.is_file():
        failures.append(f"missing IR test file: {rel(TESTS)}")
    for name in sorted(REQUIRED_FIXTURES):
        path = ROOT / name
        if not path.is_file():
            failures.append(f"missing IR fixture: {name}")
        elif name.endswith(".vit") and not path.read_text(encoding="utf-8", errors="replace").strip():
            failures.append(f"empty IR fixture: {name}")
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    report = ["# IR Fixtures", "", "| Fixture | Status |", "| --- | --- |"]
    for name in sorted(REQUIRED_FIXTURES):
        report.append(f"| {name} | present |")
    (REPORT_DIR / "fixtures.md").write_text("\n".join(report) + "\n", encoding="utf-8")
    for failure in failures:
        print(f"[ir-fixtures][error] {failure}", file=sys.stderr)
    print(f"[ir-fixtures] status={'fail' if failures else 'pass'} fixtures={len(REQUIRED_FIXTURES)} report={rel(REPORT_DIR / 'fixtures.md')}")
    return 1 if failures else 0


def check_snapshots() -> int:
    failures = write_reports()
    for failure in failures:
        print(f"[ir-snapshots][error] {failure}", file=sys.stderr)
    print(f"[ir-snapshots] status={'fail' if failures else 'pass'} report={rel(REPORT_DIR)}")
    return 1 if failures else 0


def check_coverage() -> int:
    failures = write_reports()
    payload = json.loads((REPORT_DIR / "coverage.json").read_text(encoding="utf-8"))
    print(
        "[ir-coverage] "
        f"status={'fail' if failures else 'pass'} covered={payload['covered_surface_count']} "
        f"declared={payload['declared_surface_count']} entries={payload['manifest_entry_count']} "
        f"supported={payload['supported_covered']}/{payload['supported_total']} "
        f"report={rel(REPORT_DIR / 'coverage.json')}"
    )
    for failure in failures:
        print(f"[ir-coverage][error] {failure}", file=sys.stderr)
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
