#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ALLOC_DIR = ROOT / "src/vitte/stdlib/alloc"
TEST_DIR = ROOT / "src/vitte/stdlib/tests"
MANIFEST = ROOT / "src/vitte/stdlib/modules.vitte.json"
SNAPSHOT = TEST_DIR / "snapshots/modules/alloc_exports.snap"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "stdlib_alloc_gate.json"
REPORT_MD = REPORT_DIR / "stdlib_alloc_gate.md"

REQUIRED_TESTS = [
    "alloc_layout_contracts.vit",
    "alloc_pool_contracts.vit",
    "alloc_intern_contracts.vit",
    "alloc_stress_contracts.vit",
    "alloc_max_contracts.vit",
]


def run(cmd: list[str]) -> tuple[int, str]:
    result = subprocess.run(
        cmd,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    return result.returncode, result.stdout


def alloc_manifest_entries() -> list[dict[str, object]]:
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    entries = [
        entry for entry in data.get("modules", [])
        if isinstance(entry, dict)
        and isinstance(entry.get("path"), str)
        and entry["path"].startswith("src/vitte/stdlib/alloc/")
        and entry.get("kind") == "module"
    ]
    return sorted(entries, key=lambda entry: str(entry["path"]))


def build_snapshot(entries: list[dict[str, object]]) -> str:
    lines = [
        "vitte stdlib alloc exports snapshot v1",
        f"module_count={len(entries)}",
    ]
    for entry in entries:
        exports = entry.get("exports", [])
        if not isinstance(exports, list):
            exports = []
        lines.append(f"{entry['module']}|{entry['path']}|exports={','.join(str(item) for item in exports)}")
    return "\n".join(lines) + "\n"


def main() -> int:
    failures: list[str] = []
    checked_sources: list[str] = []

    alloc_sources = sorted(ALLOC_DIR.glob("*.vitl"))
    for source in alloc_sources:
        code, output = run([str(ROOT / "bin/vitte"), "check", source.relative_to(ROOT).as_posix()])
        checked_sources.append(source.relative_to(ROOT).as_posix())
        if code != 0:
            failures.append(f"{source.relative_to(ROOT)} failed: {output.strip()}")

    for name in REQUIRED_TESTS:
        source = TEST_DIR / name
        if not source.is_file():
            failures.append(f"missing alloc contract test: {source.relative_to(ROOT)}")
            continue
        code, output = run([str(ROOT / "bin/vitte"), "check", source.relative_to(ROOT).as_posix()])
        checked_sources.append(source.relative_to(ROOT).as_posix())
        if code != 0:
            failures.append(f"{source.relative_to(ROOT)} failed: {output.strip()}")

    index = ALLOC_DIR / "index.vit"
    code, output = run([str(ROOT / "bin/vitte"), "check", index.relative_to(ROOT).as_posix()])
    checked_sources.append(index.relative_to(ROOT).as_posix())
    if code != 0:
        failures.append(f"{index.relative_to(ROOT)} failed: {output.strip()}")

    entries = alloc_manifest_entries()
    expected_snapshot = build_snapshot(entries)
    if not SNAPSHOT.is_file():
        failures.append(f"missing alloc exports snapshot: {SNAPSHOT.relative_to(ROOT)}")
    elif SNAPSHOT.read_text(encoding="utf-8") != expected_snapshot:
        failures.append(f"alloc exports snapshot drift: {SNAPSHOT.relative_to(ROOT)}")

    source_count = len(alloc_sources)
    manifest_count = len(entries) - 1 if any(str(entry["path"]).endswith("/index.vit") for entry in entries) else len(entries)
    if source_count != manifest_count:
        failures.append(f"alloc manifest count mismatch: sources={source_count} manifest_vitl={manifest_count}")

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    report = {
        "schema": "vitte.stdlib.alloc.gate.v1",
        "status": "fail" if failures else "pass",
        "alloc_sources": source_count,
        "manifest_entries": len(entries),
        "checked": checked_sources,
        "snapshot": SNAPSHOT.relative_to(ROOT).as_posix(),
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    REPORT_MD.write_text(
        "\n".join([
            "# stdlib alloc gate",
            "",
            f"- status: {report['status']}",
            f"- alloc sources: {source_count}",
            f"- manifest entries: {len(entries)}",
            f"- snapshot: {SNAPSHOT.relative_to(ROOT).as_posix()}",
        ]) + "\n",
        encoding="utf-8",
    )

    if failures:
        print("[stdlib-alloc-gate][error] alloc gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print(f"[stdlib-alloc-gate] ok sources={source_count} tests={len(REQUIRED_TESTS)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
