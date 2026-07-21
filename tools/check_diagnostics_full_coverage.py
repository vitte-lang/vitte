#!/usr/bin/env python3
"""Strict diagnostics coverage gate for the declared compiler surface.

This does not claim that every future language feature exists. It enforces that
every diagnostic currently declared by the public catalog is structured, tested,
snapshotted, and backed by zero legacy diagnostic debt.
"""

from __future__ import annotations

import json
import re
import sys
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "schemas" / "diagnostics" / "codes.json"
LEGACY_AUDIT = ROOT / "schemas" / "diagnostics" / "legacy_audit.json"
REPORT_DIR = ROOT / "target" / "reports" / "diagnostics_full_coverage"
JSON_REPORT = REPORT_DIR / "coverage.json"
MD_REPORT = REPORT_DIR / "coverage.md"

PUBLIC_PHASES = (
    "lexer",
    "parser",
    "resolver",
    "sema",
    "typeck",
    "borrowck",
    "mir",
    "ir",
    "codegen",
    "linker",
    "ice",
)
SNAPSHOT_ROOTS = (
    ROOT / "tests" / "diag_snapshots",
    ROOT / "tests" / "diagnostics" / "catalog" / "snapshots",
    ROOT / "src" / "vitte" / "compiler" / "tests" / "diagnostics",
)
CODE_RE = re.compile(
    r"\b(?:"
    r"[A-Z]{2,4}[0-9]{4}|"
    r"[A-Z][A-Z0-9_]+_(?:E|W|N|H)_[A-Z0-9_]+|"
    r"E_BOOTSTRAP_[A-Z0-9_]+|"
    r"E_STRICT_[A-Z0-9_]+|"
    r"P[0-9A-Z_]+|"
    r"BORROW_E_[A-Z0-9_]+|"
    r"LINKER_E_[A-Z0-9_]+"
    r")\b"
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def normalized_phase(phase: str, code: str) -> str:
    lowered = phase.lower()
    if lowered in {"module_resolution", "symbol_resolution", "resolver"}:
        return "resolver"
    if lowered in {"mir", "mir_lowering", "mir_verification"}:
        return "mir"
    if lowered in {"backend", "codegen"}:
        return "codegen"
    if lowered == "ice":
        return "ice"
    if lowered in PUBLIC_PHASES:
        return lowered
    if code.startswith("MOD_"):
        return "resolver"
    if code.startswith("CONST_EVAL_"):
        return "sema"
    if code.startswith("LINK"):
        return "linker"
    if code.startswith("BACKEND_") or code.startswith("CBACKEND_") or code.startswith("LLVM_"):
        return "codegen"
    return lowered


def catalog_entries() -> list[dict[str, Any]]:
    payload = load_json(CATALOG)
    entries = payload.get("codes", [])
    if not isinstance(entries, list):
        raise SystemExit("[diagnostics-full-coverage][error] codes.json must contain a codes array")
    return [entry for entry in entries if isinstance(entry, dict)]


def legacy_debt() -> dict[str, int]:
    payload = load_json(LEGACY_AUDIT)
    return {
        "legacy_call_sites": sum(int(v) for v in payload.get("legacy_call_sites", {}).values()),
        "direct_message_concat_sites": sum(int(v) for v in payload.get("direct_message_concat_sites", {}).values()),
        "catch_all_messages": sum(int(v) for v in payload.get("catch_all_messages", {}).values()),
    }


def catalog_aliases(entries: list[dict[str, Any]]) -> dict[str, str]:
    aliases: dict[str, str] = {}
    for entry in entries:
        code = str(entry.get("code", ""))
        if not code:
            continue
        aliases[code] = code
        message_key = entry.get("message_key")
        if isinstance(message_key, str) and message_key:
            aliases[message_key] = code
        for alias in entry.get("aliases", []):
            if isinstance(alias, str) and alias:
                aliases[alias] = code
    return aliases


def snapshot_codes(alias_to_code: dict[str, str]) -> dict[str, list[str]]:
    by_code: dict[str, list[str]] = defaultdict(list)
    suffixes = (".snap", ".must", ".ordered")
    for root in SNAPSHOT_ROOTS:
        if not root.exists():
            continue
        for path in sorted(root.rglob("*")):
            if not path.is_file():
                continue
            name = path.name
            if not (name.endswith(suffixes) or name.endswith(".json.must")):
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            for found in set(CODE_RE.findall(text)):
                code = alias_to_code.get(found, found)
                by_code[code].append(rel(path))
    return dict(sorted(by_code.items()))


def percent(done: int, total: int) -> int:
    if total == 0:
        return 100
    return int((done * 100) / total)


def main() -> int:
    entries = catalog_entries()
    snapshots = snapshot_codes(catalog_aliases(entries))
    debt = legacy_debt()
    failures: list[str] = []
    phase_totals: Counter[str] = Counter()
    phase_snapshots: Counter[str] = Counter()
    missing_tests: list[str] = []
    missing_snapshots: list[str] = []
    unknown_phases: list[str] = []

    for entry in entries:
        code = str(entry.get("code", ""))
        phase = normalized_phase(str(entry.get("phase", "")), code)
        tests = entry.get("tests")
        if phase not in PUBLIC_PHASES:
            unknown_phases.append(f"{code}:{entry.get('phase')}")
            continue
        phase_totals[phase] += 1
        if not isinstance(tests, list) or len(tests) == 0:
            missing_tests.append(code)
        if code in snapshots:
            phase_snapshots[phase] += 1
        else:
            missing_snapshots.append(code)

    for key, count in debt.items():
        if count != 0:
            failures.append(f"{key} must be 0, got {count}")
    if unknown_phases:
        failures.append("unknown catalog phase(s): " + ", ".join(unknown_phases[:20]))
    if missing_tests:
        failures.append("catalog code(s) without tests: " + ", ".join(missing_tests[:20]))
    if missing_snapshots:
        failures.append("catalog code(s) without snapshot proof: " + ", ".join(missing_snapshots[:20]))
    for phase in PUBLIC_PHASES:
        if phase_totals[phase] == 0:
            failures.append(f"phase {phase} has no catalog entries")
        if phase_snapshots[phase] == 0:
            failures.append(f"phase {phase} has no diagnostic snapshot proof")

    catalog_total = len(entries)
    tests_total = catalog_total - len(missing_tests)
    snapshots_total = catalog_total - len(missing_snapshots)
    migration_total = 1 if sum(debt.values()) == 0 else 0
    full = not failures
    report = {
        "status": "pass" if full else "fail",
        "catalog_entries": catalog_total,
        "catalog_tests": {
            "covered": tests_total,
            "total": catalog_total,
            "percent": percent(tests_total, catalog_total),
        },
        "snapshots": {
            "covered": snapshots_total,
            "total": catalog_total,
            "percent": percent(snapshots_total, catalog_total),
        },
        "migration": {
            "covered": migration_total,
            "total": 1,
            "percent": migration_total * 100,
            "debt": debt,
        },
        "phase_snapshot_coverage": {
            phase: {
                "snapshotted_codes": phase_snapshots[phase],
                "catalog_codes": phase_totals[phase],
                "percent": percent(phase_snapshots[phase], phase_totals[phase]),
            }
            for phase in PUBLIC_PHASES
        },
        "snapshot_files_by_code": snapshots,
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    JSON_REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# Diagnostics Full Coverage",
        "",
        f"- status: {report['status']}",
        f"- catalog tests: {tests_total}/{catalog_total} ({percent(tests_total, catalog_total)}%)",
        f"- snapshots: {snapshots_total}/{catalog_total} ({percent(snapshots_total, catalog_total)}%)",
        f"- migration legacy debt: {sum(debt.values())}",
        "",
        "| phase | snapshotted codes | catalog codes | percent |",
        "| --- | ---: | ---: | ---: |",
    ]
    for phase in PUBLIC_PHASES:
        lines.append(
            f"| {phase} | {phase_snapshots[phase]} | {phase_totals[phase]} | "
            f"{percent(phase_snapshots[phase], phase_totals[phase])}% |"
        )
    if failures:
        lines.extend(["", "## Failures", ""])
        lines.extend(f"- {failure}" for failure in failures)
    MD_REPORT.write_text("\n".join(lines) + "\n", encoding="utf-8")

    if failures:
        for failure in failures:
            print(f"[diagnostics-full-coverage][error] {failure}", file=sys.stderr)
        print(f"[diagnostics-full-coverage] report={rel(JSON_REPORT)}", file=sys.stderr)
        return 1
    print(
        "[diagnostics-full-coverage] OK "
        f"catalog={catalog_total}/{catalog_total} "
        f"snapshots={snapshots_total}/{catalog_total} "
        f"legacy_debt=0 report={rel(JSON_REPORT)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
