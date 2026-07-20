#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HIR = ROOT / "src" / "vitte" / "compiler" / "middle" / "hir" / "hir.vit"
TESTS = ROOT / "src" / "vitte" / "compiler" / "tests" / "hir_tests.vit"
MANIFEST = ROOT / "src" / "vitte" / "compiler" / "tests" / "hir_coverage_manifest.json"
REPORT_DIR = ROOT / "target" / "reports" / "hir_coverage"
SNAPSHOT_DIR = ROOT / "src" / "vitte" / "compiler" / "tests" / "snapshots" / "hir"
JSON_SNAPSHOT = SNAPSHOT_DIR / "coverage.json.snap"
COMPACT_SNAPSHOT = SNAPSHOT_DIR / "coverage.txt.snap"
SUPPORT_STATUSES = {"supported", "ast_only", "planned", "not_supported"}
OWNER_PHASES = {"parser", "lower_ast", "resolver", "typeck", "borrowck", "mir"}


def read(path: Path) -> str:
    if not path.is_file():
        raise SystemExit(f"[hir-coverage][error] missing file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8", errors="replace")


def braced_body(text: str, header_pattern: str) -> str:
    match = re.search(header_pattern, text)
    if match is None:
        raise SystemExit(f"[hir-coverage][error] missing declaration matching {header_pattern!r}")
    start = text.find("{", match.start())
    depth = 0
    for index in range(start, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1 : index]
    raise SystemExit(f"[hir-coverage][error] unterminated declaration matching {header_pattern!r}")


def pick_variants(text: str, pick_name: str) -> list[str]:
    body = braced_body(text, rf"\bpick\s+{re.escape(pick_name)}\s*\{{")
    return re.findall(r"(?m)^\s*([A-Z][A-Za-z0-9_]*)\s*,?\s*$", body)


def form_names(text: str) -> list[str]:
    return re.findall(r"(?m)^\s*form\s+(Hir[A-Za-z0-9_]+|SemanticDiagnostic)\s*\{", text)


def test_names(text: str) -> set[str]:
    return set(re.findall(r"(?m)^\s*proc\s+(test_[A-Za-z0-9_]+)\s*\(", text))


def evidence_tests(entry: dict[str, object]) -> list[str]:
    evidence = entry.get("evidence", {})
    if isinstance(evidence, dict):
        tests = evidence.get("tests", [])
        if isinstance(tests, list):
            return [str(name) for name in tests]
    return []


def main() -> int:
    hir_text = read(HIR)
    tests_text = read(TESTS)
    manifest = json.loads(read(MANIFEST))
    entries = manifest.get("entries", [])
    known_tests = test_names(tests_text)
    failures: list[str] = []

    required_tests = manifest.get("required_tests", [])
    for name in required_tests:
        if name not in known_tests:
            failures.append(f"missing required HIR test `{name}`")
        elif f"if not {name}()" not in tests_text:
            failures.append(f"HIR test `{name}` is not called by run_all_tests")

    for index, entry in enumerate(entries):
        variant = entry.get("variant", "")
        tests = entry.get("tests", [])
        status = entry.get("support_status", "")
        owner_phase = entry.get("owner_phase", "")
        evidence = entry.get("evidence", {})
        if not variant:
            failures.append(f"entry {index} has no variant")
        if status not in SUPPORT_STATUSES:
            failures.append(f"{variant}: invalid or missing support_status `{status}`")
        if owner_phase not in OWNER_PHASES:
            failures.append(f"{variant}: invalid or missing owner_phase `{owner_phase}`")
        if not isinstance(evidence, dict) or not evidence.get("assertion"):
            failures.append(f"{variant}: missing evidence assertion")
        if evidence_tests(entry) != tests:
            failures.append(f"{variant}: evidence tests must match tests")
        if status == "supported" and not tests:
            failures.append(f"{variant}: supported entry has no real HIR test")
        for name in tests:
            if name not in known_tests:
                failures.append(f"{variant}: unknown test `{name}`")

    declared_variants: list[str] = []
    for pick in (
        "HirExprKind",
        "HirStmtKind",
        "HirItemKind",
        "HirNominalMemberKind",
        "HirBindingKind",
        "HirValueMode",
        "HirPlaceCategory",
    ):
        declared_variants.extend(f"{pick}.{name}" for name in pick_variants(hir_text, pick))
    declared_variants.extend(form_names(hir_text))
    declared_variants.extend(("HirItem.where_clauses", "HirProcParam.zero_params", "HirProcParam.multiple_params"))

    covered = {entry.get("variant", "") for entry in entries}
    declared_covered = sorted(name for name in declared_variants if name in covered)
    uncovered = sorted(name for name in declared_variants if name not in covered)
    if uncovered:
        failures.append(
            "HIR variants missing manifest entries: " + ", ".join(uncovered)
        )

    rows = []
    categories = {
        "hir_real": [],
        "ast_indirect": [],
        "declared_not_supported": [],
        "supported_untested": [],
        "planned": [],
    }
    supported_total = 0
    supported_covered = 0
    for entry in entries:
        status = str(entry.get("support_status", ""))
        tests = entry.get("tests", [])
        if not isinstance(tests, list):
            tests = []
        variant = str(entry.get("variant", ""))
        if status == "supported":
            supported_total += 1
            if tests:
                supported_covered += 1
                categories["hir_real"].append(variant)
            else:
                categories["supported_untested"].append(variant)
        elif status == "ast_only":
            categories["ast_indirect"].append(variant)
        elif status == "planned":
            categories["planned"].append(variant)
        elif status == "not_supported":
            categories["declared_not_supported"].append(variant)
        rows.append(
            {
                "variant": entry["variant"],
                "source": entry["source"],
                "producer": entry["producer"],
                "consumer": entry["consumer"],
                "tests": tests,
                "support_status": status,
                "owner_phase": entry.get("owner_phase", ""),
                "evidence": entry.get("evidence", {}),
                "covered": bool(tests),
            }
        )
    if categories["supported_untested"]:
        failures.append(
            "supported HIR entries without real tests: "
            + ", ".join(categories["supported_untested"])
        )

    payload = {
        "schema": "vitte.compiler.hir_coverage",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "declared_variant_count": len(declared_variants),
        "covered_variant_count": len(declared_covered),
        "manifest_entry_count": len(covered),
        "supported_total": supported_total,
        "supported_covered": supported_covered,
        "coverage_categories": categories,
        "declared_covered_variants": declared_covered,
        "uncovered_variants": uncovered,
        "coverage_table": rows,
        "failures": failures,
    }

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    lines = [
        "# HIR Coverage",
        "",
        "| Variant | Status | Owner | Source | Producer | Consumer | Tests | Evidence |",
        "| --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for row in rows:
        lines.append(
            "| {variant} | {status} | {owner} | {source} | {producer} | {consumer} | {tests} | {evidence} |".format(
                variant=row["variant"],
                status=row["support_status"],
                owner=row["owner_phase"],
                source=row["source"],
                producer=row["producer"],
                consumer=row["consumer"],
                tests=", ".join(row["tests"]),
                evidence=row["evidence"].get("assertion", "") if isinstance(row["evidence"], dict) else "",
            )
        )
    lines.extend(["", "## Uncovered Variants", ""])
    lines.extend(f"- `{name}`" for name in uncovered)
    compact_text = "\n".join(lines) + "\n"
    (REPORT_DIR / "coverage.md").write_text(compact_text, encoding="utf-8")

    snapshot_payload = {
        "schema": payload["schema"],
        "schema_version": payload["schema_version"],
        "declared_variant_count": payload["declared_variant_count"],
        "covered_variant_count": payload["covered_variant_count"],
        "manifest_entry_count": payload["manifest_entry_count"],
        "supported_total": payload["supported_total"],
        "supported_covered": payload["supported_covered"],
        "coverage_categories": payload["coverage_categories"],
        "uncovered_variants": payload["uncovered_variants"],
        "coverage_table": payload["coverage_table"],
    }
    json_snapshot_text = json.dumps(snapshot_payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n"
    compact_snapshot_text = compact_text
    (REPORT_DIR / "coverage.json.snap").write_text(json_snapshot_text, encoding="utf-8")
    (REPORT_DIR / "coverage.txt.snap").write_text(compact_snapshot_text, encoding="utf-8")
    if not JSON_SNAPSHOT.is_file():
        failures.append(f"missing HIR JSON snapshot: {JSON_SNAPSHOT.relative_to(ROOT)}")
    elif JSON_SNAPSHOT.read_text(encoding="utf-8") != json_snapshot_text:
        failures.append(f"HIR JSON snapshot drift: {JSON_SNAPSHOT.relative_to(ROOT)}")
    if not COMPACT_SNAPSHOT.is_file():
        failures.append(f"missing HIR compact snapshot: {COMPACT_SNAPSHOT.relative_to(ROOT)}")
    elif COMPACT_SNAPSHOT.read_text(encoding="utf-8") != compact_snapshot_text:
        failures.append(f"HIR compact snapshot drift: {COMPACT_SNAPSHOT.relative_to(ROOT)}")
    payload["status"] = "fail" if failures else "pass"
    payload["failures"] = failures
    (REPORT_DIR / "coverage.json").write_text(
        json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )

    print(
        "[hir-coverage] "
        f"status={payload['status']} covered={payload['covered_variant_count']} "
        f"declared={payload['declared_variant_count']} entries={payload['manifest_entry_count']} "
        f"supported={payload['supported_covered']}/{payload['supported_total']} "
        f"report={REPORT_DIR.relative_to(ROOT)}/coverage.json"
    )
    for failure in failures:
        print(f"[hir-coverage][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
