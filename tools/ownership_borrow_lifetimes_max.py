#!/usr/bin/env python3
"""Max ownership, borrow, and lifetime compiler gate."""

from __future__ import annotations

import hashlib
import json
import re
import subprocess
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "target/borrowck/ownership_borrow_lifetimes_max"
REPORT_DIR = ROOT / "target/reports"
CATALOG = ROOT / "schemas/diagnostics/codes.json"
BORROWCK_MANIFEST = ROOT / "src/vitte/compiler/tests/borrowck_coverage_manifest.json"
BORROWCK_TESTS = ROOT / "src/vitte/compiler/tests/borrowck_tests.vit"
BORROWCK_REPORT = ROOT / "target/reports/borrowck_coverage/coverage.json"
VALID_FIXTURE = ROOT / "tests/borrowck/max/valid_surface.vit"
INVALID_CONTRACT_FIXTURE = ROOT / "tests/borrowck/max/invalid_contract_surface.vit"


COMMANDS: tuple[tuple[str, ...], ...] = (
    ("bin/vitte", "check", "src/vitte/compiler/tests/borrowck_tests.vit"),
    ("python3", "tools/borrowck_coverage_check.py", "--fixtures"),
    ("python3", "tools/borrowck_coverage_check.py", "--snapshots"),
    ("python3", "tools/borrowck_coverage_check.py"),
    ("python3", "tools/check_diagnostic_catalog.py"),
    ("python3", "tools/check_suggestion_quality.py"),
    ("bin/vitte", "check", "tests/borrowck/max/valid_surface.vit"),
    ("bin/vitte", "check", "tests/borrowck/max/invalid_contract_surface.vit"),
)


PIPELINE_NEEDLES: tuple[tuple[str, tuple[str, ...]], ...] = (
    (
        "src/vitte/compiler/analysis/pipeline.vit",
        ("borrow_check_hir_mir", "lower_hir_to_mir", "BorrowCheckResult"),
    ),
    (
        "src/vitte/compiler/driver/compile.vit",
        ("borrow_check_hir_mir", "diagnostic_report", "artifacts.typed_hir", "artifacts.mir"),
    ),
    (
        "src/vitte/compiler/tests/borrowck_tests.vit",
        (
            "borrow_check_frontend_hir_mir",
            "same_codes",
            "test_borrowck_hir_only_and_hir_mir_match_on_move_and_borrow",
            "test_borrowck_no_parasitic_diagnostics_for_valid_copy_and_scope",
            "test_borrowck_recovers_and_reports_multiple_errors",
        ),
    ),
)


ANALYSIS_NEEDLES: tuple[tuple[str, tuple[str, ...]], ...] = (
    ("src/vitte/compiler/analysis/borrowck/moves.vit", ("MoveEventKind", "MoveAfterMove", "UseAfterMove", "tracker_record_move", "tracker_record_use")),
    ("src/vitte/compiler/analysis/borrowck/loans.vit", ("LoanKind", "SharedWhileMutable", "MutableWhileShared", "add_loan_checked", "deactivate_expired_loans")),
    ("src/vitte/compiler/analysis/borrowck/lifetimes.vit", ("LifetimeFactKind", "Reborrow", "AsyncSuspend", "add_return_constraint", "finalize_lifetimes")),
    ("src/vitte/compiler/analysis/borrowck/ownership.vit", ("PlaceKind", "place_overlap", "mark_moved", "mark_reinitialized", "shared_borrow_count")),
    ("src/vitte/compiler/analysis/borrowck/regions.vit", ("RegionConstraintKind", "MemoryRegionKind", "solve_regions", "finalize_memory_regions")),
    ("src/vitte/compiler/analysis/borrowck/errors.vit", ("BorrowPhase", "borrow_error_temporal_story", "diagnostic_with_subject_borrow", "diagnostic_add_cause")),
    ("src/vitte/compiler/analysis/borrowck/mod.vit", ("borrow_check_hir_mir", "borrow_check_source", "BorrowMetadata", "result_fingerprint")),
)


REQUIRED_TESTS = (
    "test_valid_reinit_after_move",
    "test_move_after_move",
    "test_write_while_borrowed",
    "test_mutable_borrow_conflict",
    "test_return_borrow_of_local",
    "test_branch_borrow_conflict_after_join",
    "test_branch_move_after_join",
    "test_field_borrow_blocks_root_write",
    "test_copy_binding_does_not_move_source",
    "test_partial_move_preserves_disjoint_field",
    "test_borrow_param_call_does_not_move_source",
    "test_owned_param_call_moves_source",
    "test_lifetime_escape_detects_dangling_reference",
    "test_first_class_memory_regions_bind_places_to_declared_regions",
    "test_borrowck_diagnostic_order_is_stable",
    "test_borrowck_diagnostic_deduplication_keeps_one_duplicate",
    "test_borrowck_diagnostic_spans_are_exact",
    "test_borrowck_no_parasitic_diagnostics_for_valid_copy_and_scope",
    "test_borrowck_hir_only_and_hir_mir_match_on_move_and_borrow",
    "test_borrowck_closure_async_defer_and_loop_contracts_are_tracked",
    "test_borrowck_call_receiver_generic_and_copy_contracts",
    "test_borrowck_pattern_match_loop_and_multifile_contracts",
)


VAGUE_BORROW_TERMS = (
    "unknown error",
    "unexpected failure",
    "something went wrong",
    "fix the error",
    "try again",
)


def run(command: tuple[str, ...]) -> subprocess.CompletedProcess[str]:
    proc = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=120)
    if proc.returncode != 0:
        raise SystemExit(
            f"[ownership-borrow-lifetimes-max][error] command failed: {' '.join(command)}\n{proc.stdout}{proc.stderr}"
        )
    return proc


def read(path: Path) -> str:
    if not path.is_file():
        raise SystemExit(f"[ownership-borrow-lifetimes-max][error] missing file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8", errors="replace")


def load_json(path: Path) -> Any:
    return json.loads(read(path))


def digest(value: object) -> str:
    return hashlib.sha256(json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def validate_needles(groups: tuple[tuple[str, tuple[str, ...]], ...]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for rel, needles in groups:
        text = read(ROOT / rel)
        missing = [needle for needle in needles if needle not in text]
        if missing:
            raise SystemExit(f"[ownership-borrow-lifetimes-max][error] {rel} missing: {', '.join(missing)}")
        rows.append({"file": rel, "needles": list(needles), "fingerprint": digest({"file": rel, "needles": needles})[:16]})
    return rows


def public_borrow_codes() -> list[dict[str, Any]]:
    payload = load_json(CATALOG)
    codes = payload.get("codes")
    if not isinstance(codes, list):
        raise SystemExit("[ownership-borrow-lifetimes-max][error] diagnostics catalog is malformed")
    selected = [entry for entry in codes if isinstance(entry, dict) and entry.get("phase") == "borrowck"]
    selected.sort(key=lambda entry: str(entry["code"]))
    if len(selected) != 147 or selected[0]["code"] != "BOR0001" or selected[-1]["code"] != "BOR0147":
        raise SystemExit(
            "[ownership-borrow-lifetimes-max][error] expected BOR0001..BOR0147 public borrowck catalog"
        )
    return selected


def validate_borrow_codes(codes: list[dict[str, Any]]) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    for entry in codes:
        code = str(entry["code"])
        message_key = str(entry.get("message_key", ""))
        documentation = entry.get("documentation")
        tests = entry.get("tests")
        snapshot = ROOT / "tests/diagnostics/catalog/snapshots/borrowck" / f"{code}.snap"
        if entry.get("stable") is not True:
            raise SystemExit(f"[ownership-borrow-lifetimes-max][error] {code} is not stable")
        if entry.get("deprecated") not in {True, False}:
            raise SystemExit(f"[ownership-borrow-lifetimes-max][error] {code} lacks deprecated=true/false")
        if not isinstance(documentation, dict):
            raise SystemExit(f"[ownership-borrow-lifetimes-max][error] {code} lacks documentation")
        missing_doc = {"title", "summary", "cause", "action", "example", "url"} - set(documentation)
        if missing_doc:
            raise SystemExit(f"[ownership-borrow-lifetimes-max][error] {code} documentation misses {sorted(missing_doc)}")
        for field in ("title", "summary", "cause", "action"):
            text = str(documentation[field]).lower()
            vague = [term for term in VAGUE_BORROW_TERMS if term in text]
            if vague:
                raise SystemExit(f"[ownership-borrow-lifetimes-max][error] {code} documentation.{field} is vague: {vague}")
        if not isinstance(tests, list) or not tests:
            raise SystemExit(f"[ownership-borrow-lifetimes-max][error] {code} has no catalog tests")
        snapshot_text = read(snapshot)
        for needle in (code, "borrowck", message_key):
            if needle not in snapshot_text:
                raise SystemExit(f"[ownership-borrow-lifetimes-max][error] {snapshot.relative_to(ROOT)} misses {needle!r}")
        rows.append(
            {
                "code": code,
                "message_key": message_key,
                "title": documentation["title"],
                "documentation_url": documentation["url"],
                "snapshot": snapshot.relative_to(ROOT).as_posix(),
                "tests": tests,
                "stable": True,
                "deprecated": entry["deprecated"],
                "fingerprint": digest({"code": code, "message_key": message_key, "snapshot": snapshot_text, "tests": tests})[:16],
            }
        )
    return rows


def validate_coverage_report() -> dict[str, Any]:
    report = load_json(BORROWCK_REPORT)
    if report.get("status") != "pass":
        raise SystemExit("[ownership-borrow-lifetimes-max][error] borrowck coverage report is not pass")
    if report.get("supported_total") != report.get("supported_covered"):
        raise SystemExit("[ownership-borrow-lifetimes-max][error] supported borrowck surfaces are not fully covered")
    if report.get("uncovered_surfaces") or report.get("stale_surfaces"):
        raise SystemExit("[ownership-borrow-lifetimes-max][error] borrowck manifest has uncovered/stale surfaces")
    entries = report.get("coverage_table")
    if not isinstance(entries, list) or len(entries) < 60:
        raise SystemExit("[ownership-borrow-lifetimes-max][error] borrowck coverage table is too small")
    analysis_entries = [entry for entry in entries if str(entry.get("surface", "")).startswith("analysis/")]
    unsupported_analysis = [entry["surface"] for entry in analysis_entries if entry.get("support_status") != "supported"]
    if unsupported_analysis:
        raise SystemExit(
            "[ownership-borrow-lifetimes-max][error] production analysis borrowck has unsupported surfaces: "
            + ", ".join(map(str, unsupported_analysis))
        )
    return {
        "declared_surface_count": report["declared_surface_count"],
        "covered_surface_count": report["covered_surface_count"],
        "manifest_entry_count": report["manifest_entry_count"],
        "supported_total": report["supported_total"],
        "supported_covered": report["supported_covered"],
        "planned_non_production": len([entry for entry in entries if entry.get("support_status") == "planned"]),
    }


def validate_tests() -> dict[str, Any]:
    text = read(BORROWCK_TESTS)
    declared_tests = set(re.findall(r"(?m)^\s*proc\s+(test_[A-Za-z0-9_]+)\s*\(", text))
    called_tests = set(re.findall(r"if not (test_[A-Za-z0-9_]+)\(\)", text))
    missing = [name for name in REQUIRED_TESTS if name not in declared_tests or name not in called_tests]
    if missing:
        raise SystemExit(f"[ownership-borrow-lifetimes-max][error] missing called borrowck tests: {', '.join(missing)}")
    manifest = load_json(BORROWCK_MANIFEST)
    required_manifest_tests = manifest.get("required_tests")
    if not isinstance(required_manifest_tests, list) or len(required_manifest_tests) < 40:
        raise SystemExit("[ownership-borrow-lifetimes-max][error] borrowck manifest required_tests is too small")
    manifest_missing = [name for name in required_manifest_tests if str(name).startswith("test_") and name not in called_tests]
    if manifest_missing:
        raise SystemExit(
            "[ownership-borrow-lifetimes-max][error] manifest required tests are not called: "
            + ", ".join(map(str, manifest_missing))
        )
    return {
        "declared_tests": len(declared_tests),
        "called_tests": len(called_tests),
        "required_tests": len(required_manifest_tests),
        "required_max_tests": len(REQUIRED_TESTS),
    }


def validate_fixtures() -> dict[str, Any]:
    fixture_paths = [
        *sorted((ROOT / "tests/borrowck/valid").glob("*.vit")),
        *sorted((ROOT / "tests/borrowck/invalid").glob("*.vit")),
        *sorted((ROOT / "tests/borrowck/multifile").glob("*.vit")),
        VALID_FIXTURE,
        INVALID_CONTRACT_FIXTURE,
    ]
    required_terms = {
        VALID_FIXTURE: ("borrow", "owned", "set", "give"),
        INVALID_CONTRACT_FIXTURE: ("owned", "set", "borrow", "give"),
    }
    for path in fixture_paths:
        text = read(path)
        if "space " not in text:
            raise SystemExit(f"[ownership-borrow-lifetimes-max][error] fixture has no Vitte space: {path.relative_to(ROOT)}")
    for path, terms in required_terms.items():
        text = read(path)
        missing = [term for term in terms if term not in text]
        if missing:
            raise SystemExit(f"[ownership-borrow-lifetimes-max][error] {path.relative_to(ROOT)} missing {missing}")
    return {"fixture_count": len(fixture_paths), "max_valid": VALID_FIXTURE.relative_to(ROOT).as_posix(), "max_invalid_contract": INVALID_CONTRACT_FIXTURE.relative_to(ROOT).as_posix()}


def render_manifest(
    code_rows: list[dict[str, Any]],
    coverage: dict[str, Any],
    tests: dict[str, Any],
    fixtures: dict[str, Any],
    pipeline: list[dict[str, Any]],
    analysis: list[dict[str, Any]],
) -> dict[str, Any]:
    requirements = []
    for index, row in enumerate(code_rows, start=1):
        item = dict(row)
        item["requirement"] = f"OBL{index:03d}"
        item["status"] = "covered"
        item["explanation"] = f"{row['code']} is stable, documented, tested, and snapshot-backed for borrow checking."
        item["suggestion"] = "Keep ownership fixtures, borrowck tests, catalog entries, and snapshots synchronized."
        requirements.append(item)
    return {
        "schema": "vitte.ownership_borrow_lifetimes.max",
        "schema_version": "1.0.0",
        "coverage_percent": 100,
        "requirements_total": len(requirements),
        "requirements_covered": len(requirements),
        "borrowck_public_codes": len(requirements),
        "commands": [" ".join(command) for command in COMMANDS],
        "coverage": coverage,
        "tests": tests,
        "fixtures": fixtures,
        "pipeline_evidence": pipeline,
        "analysis_evidence": analysis,
        "matrix_fingerprint": digest({"requirements": requirements, "coverage": coverage, "tests": tests, "fixtures": fixtures, "pipeline": pipeline, "analysis": analysis}),
        "requirements": requirements,
    }


def write_outputs(manifest: dict[str, Any]) -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    (OUT_DIR / "snapshots").mkdir(parents=True, exist_ok=True)
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    manifest_text = json.dumps(manifest, ensure_ascii=True, indent=2, sort_keys=True) + "\n"
    text_snapshot = "\n".join(
        f"{row['requirement']} covered borrowck {row['code']} {row['fingerprint']} {row['title']}"
        for row in manifest["requirements"]
    ) + "\n"
    lsp = {
        "jsonrpc": "2.0",
        "method": "textDocument/publishDiagnostics",
        "params": {
            "uri": "file:///" + INVALID_CONTRACT_FIXTURE.relative_to(ROOT).as_posix(),
            "version": 1,
            "diagnostics": [
                {
                    "source": "vitte",
                    "severity": 1,
                    "code": row["code"],
                    "message": row["explanation"],
                    "data": {"requirement": row["requirement"], "phase": "borrowck", "fix_why": row["suggestion"]},
                }
                for row in manifest["requirements"]
            ],
        },
    }
    lsp_text = json.dumps(lsp, ensure_ascii=True, indent=2, sort_keys=True) + "\n"
    md_lines = [
        "# Ownership Borrow Lifetimes Max",
        "",
        f"Coverage: {manifest['requirements_covered']}/{manifest['requirements_total']} ({manifest['coverage_percent']}%)",
        f"Borrowck public codes: {manifest['borrowck_public_codes']}",
        f"Supported surfaces: {manifest['coverage']['supported_covered']}/{manifest['coverage']['supported_total']}",
        f"Fingerprint: {manifest['matrix_fingerprint']}",
        "",
        "| ID | Code | Status | Title |",
        "| --- | --- | --- | --- |",
    ]
    for row in manifest["requirements"]:
        md_lines.append(f"| {row['requirement']} | {row['code']} | {row['status']} | {row['title']} |")
    md = "\n".join(md_lines) + "\n"
    (OUT_DIR / "manifest.json").write_text(manifest_text, encoding="utf-8")
    (OUT_DIR / "coverage.md").write_text(md, encoding="utf-8")
    (OUT_DIR / "snapshots/text.snapshot").write_text(text_snapshot, encoding="utf-8")
    (OUT_DIR / "snapshots/json.snapshot").write_text(manifest_text, encoding="utf-8")
    (OUT_DIR / "snapshots/lsp.snapshot").write_text(lsp_text, encoding="utf-8")
    (REPORT_DIR / "ownership_borrow_lifetimes_max.json").write_text(manifest_text, encoding="utf-8")
    (REPORT_DIR / "ownership_borrow_lifetimes_max.md").write_text(md, encoding="utf-8")


def validate_outputs(manifest: dict[str, Any]) -> None:
    if manifest["requirements_total"] != 147 or manifest["coverage_percent"] != 100:
        raise SystemExit("[ownership-borrow-lifetimes-max][error] manifest must cover exactly 147 borrowck codes")
    for path in (
        OUT_DIR / "manifest.json",
        OUT_DIR / "coverage.md",
        OUT_DIR / "snapshots/text.snapshot",
        OUT_DIR / "snapshots/json.snapshot",
        OUT_DIR / "snapshots/lsp.snapshot",
        REPORT_DIR / "ownership_borrow_lifetimes_max.json",
        REPORT_DIR / "ownership_borrow_lifetimes_max.md",
    ):
        if not path.is_file() or path.stat().st_size == 0:
            raise SystemExit(f"[ownership-borrow-lifetimes-max][error] missing generated artifact: {path.relative_to(ROOT)}")


def main() -> int:
    for command in COMMANDS:
        run(command)
    pipeline = validate_needles(PIPELINE_NEEDLES)
    analysis = validate_needles(ANALYSIS_NEEDLES)
    code_rows = validate_borrow_codes(public_borrow_codes())
    coverage = validate_coverage_report()
    tests = validate_tests()
    fixtures = validate_fixtures()
    manifest = render_manifest(code_rows, coverage, tests, fixtures, pipeline, analysis)
    write_outputs(manifest)
    validate_outputs(manifest)
    print(
        "[ownership-borrow-lifetimes-max] OK "
        f"requirements={manifest['requirements_total']} "
        f"surfaces={coverage['supported_covered']}/{coverage['supported_total']} "
        f"fingerprint={manifest['matrix_fingerprint']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
