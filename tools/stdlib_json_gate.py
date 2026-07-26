#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
JSON_DIR = ROOT / "src/vitte/stdlib/json"
TEST_DIR = ROOT / "src/vitte/stdlib/tests"
MANIFEST = ROOT / "src/vitte/stdlib/modules.vitte.json"
SNAPSHOT = TEST_DIR / "snapshots/modules/json_exports.snap"
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "stdlib_json_gate.json"
REPORT_MD = REPORT_DIR / "stdlib_json_gate.md"
REQUIRED_TESTS = ["json_max_contracts.vit"]


def run(cmd: list[str]) -> tuple[int, str]:
    result = subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return result.returncode, result.stdout


def json_entries() -> list[dict[str, object]]:
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    entries = [
        entry for entry in data.get("modules", [])
        if isinstance(entry, dict)
        and isinstance(entry.get("path"), str)
        and entry["path"].startswith("src/vitte/stdlib/json/")
        and entry.get("kind") == "module"
    ]
    return sorted(entries, key=lambda entry: str(entry["path"]))


def snapshot_text(entries: list[dict[str, object]]) -> str:
    lines = ["vitte stdlib json exports snapshot v1", f"module_count={len(entries)}"]
    for entry in entries:
        exports = entry.get("exports", [])
        if not isinstance(exports, list):
            exports = []
        lines.append(f"{entry['module']}|{entry['path']}|exports={','.join(str(item) for item in exports)}")
    return "\n".join(lines) + "\n"


def main() -> int:
    failures: list[str] = []
    checked: list[str] = []
    sources = sorted(JSON_DIR.glob("*.vitl"))
    for source in sources:
        rel = source.relative_to(ROOT).as_posix()
        code, output = run([str(ROOT / "bin/vitte"), "check", rel])
        checked.append(rel)
        if code != 0:
            failures.append(f"{rel} failed: {output.strip()}")
    for name in REQUIRED_TESTS:
        source = TEST_DIR / name
        rel = source.relative_to(ROOT).as_posix()
        code, output = run([str(ROOT / "bin/vitte"), "check", rel])
        checked.append(rel)
        if code != 0:
            failures.append(f"{rel} failed: {output.strip()}")
    index = JSON_DIR / "index.vit"
    code, output = run([str(ROOT / "bin/vitte"), "check", index.relative_to(ROOT).as_posix()])
    checked.append(index.relative_to(ROOT).as_posix())
    if code != 0:
        failures.append(f"{index.relative_to(ROOT)} failed: {output.strip()}")
    entries = json_entries()
    expected = snapshot_text(entries)
    if not SNAPSHOT.is_file():
        failures.append(f"missing json exports snapshot: {SNAPSHOT.relative_to(ROOT)}")
    elif SNAPSHOT.read_text(encoding="utf-8") != expected:
        failures.append(f"json exports snapshot drift: {SNAPSHOT.relative_to(ROOT)}")
    manifest_vitl = len(entries) - 1 if any(str(entry["path"]).endswith("/index.vit") for entry in entries) else len(entries)
    if len(sources) != manifest_vitl:
        failures.append(f"json manifest count mismatch: sources={len(sources)} manifest_vitl={manifest_vitl}")
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    report = {
        "schema": "vitte.stdlib.json.gate.v1",
        "status": "fail" if failures else "pass",
        "json_sources": len(sources),
        "manifest_entries": len(entries),
        "checked": checked,
        "snapshot": SNAPSHOT.relative_to(ROOT).as_posix(),
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    REPORT_MD.write_text(
        "# stdlib json gate\n\n"
        f"- status: {report['status']}\n"
        f"- json sources: {len(sources)}\n"
        f"- manifest entries: {len(entries)}\n"
        f"- snapshot: {SNAPSHOT.relative_to(ROOT).as_posix()}\n",
        encoding="utf-8",
    )
    if failures:
        print("[stdlib-json-gate][error] json gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print(f"[stdlib-json-gate] ok sources={len(sources)} tests={len(REQUIRED_TESTS)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
