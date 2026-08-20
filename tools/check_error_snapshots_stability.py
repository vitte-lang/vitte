#!/usr/bin/env python3
"""Check that tracked error snapshots have complete, deterministic metadata."""

from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DIAG_ROOT = ROOT / "tests/diag_snapshots"
COMPILER_DIAG_ROOT = ROOT / "src/vitte/compiler/tests/diagnostics"
COMPILER_SNAPSHOT_ROOT = ROOT / "tests/compiler_snapshots/snapshots"
REPORT_DIR = ROOT / "target/reports"
REPORT = REPORT_DIR / "error_snapshots_stability.json"
MARKDOWN = REPORT_DIR / "error_snapshots_stability.md"


def main() -> int:
    failures: list[str] = []
    cli_cases = sorted(DIAG_ROOT.glob("*.vit"))
    for fixture in cli_cases:
        must = fixture.with_suffix(".must")
        exit_file = fixture.with_suffix(".exit")
        if not exit_file.is_file():
            failures.append(f"{fixture.relative_to(ROOT)} missing .exit snapshot")
        else:
            try:
                expected_exit = int(exit_file.read_text(encoding="utf-8").strip())
                if expected_exit != 0 and (not must.is_file() or not must.read_text(encoding="utf-8").strip()):
                    failures.append(f"{fixture.relative_to(ROOT)} missing non-empty .must snapshot for failing case")
            except ValueError:
                failures.append(f"{exit_file.relative_to(ROOT)} must contain an integer exit code")
        json_must = fixture.with_suffix(".json.must")
        if json_must.is_file() and not json_must.read_text(encoding="utf-8").strip():
            failures.append(f"{json_must.relative_to(ROOT)} must not be empty")

    compiler_snapshots = sorted(COMPILER_DIAG_ROOT.rglob("*.snap"))
    for snapshot in compiler_snapshots:
        content = snapshot.read_text(encoding="utf-8")
        if not content.strip():
            failures.append(f"{snapshot.relative_to(ROOT)} is empty")
        if str(ROOT) in content:
            failures.append(f"{snapshot.relative_to(ROOT)} contains an absolute workspace path")

    required_surfaces = {
        "diagnostics_text.snap",
        "diagnostics_json.snap",
        "diagnostics_lsp.snap",
        "mir.snap",
        "ir.snap",
        "object.snap",
    }
    present_surfaces = {path.name for path in COMPILER_SNAPSHOT_ROOT.glob("*.snap")}
    failures.extend(f"missing compiler snapshot surface: {name}" for name in sorted(required_surfaces - present_surfaces))

    report = {
        "schema": "vitte.compiler.error-snapshots-stability",
        "status": "error" if failures else "ok",
        "cli_case_count": len(cli_cases),
        "compiler_diagnostic_snapshot_count": len(compiler_snapshots),
        "compiler_snapshot_surface_count": len(present_surfaces),
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MARKDOWN.write_text(
        "# Error snapshot stability\n\n"
        f"- CLI cases: {report['cli_case_count']}\n"
        f"- compiler diagnostic snapshots: {report['compiler_diagnostic_snapshot_count']}\n"
        f"- compiler snapshot surfaces: {report['compiler_snapshot_surface_count']}\n"
        f"- status: {report['status']}\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[error-snapshots][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[error-snapshots] "
        f"cli_cases={report['cli_case_count']} diagnostics={report['compiler_diagnostic_snapshot_count']} "
        f"surfaces={report['compiler_snapshot_surface_count']} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
