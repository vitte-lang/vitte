#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
REPORT_DIR = ROOT / "target/reports"
REPORT_JSON = REPORT_DIR / "vitte_in_vitte_gate.json"
REPORT_MD = REPORT_DIR / "vitte_in_vitte_gate.md"
ENTRYPOINT = "src/vitte/compiler/main.vit"
FORBIDDEN_STAGE_MARKERS = (
    "bin/vittec0",
    "toolchain/seed",
    "vittec0.seed",
    "scripts/seed/install_seed.sh",
    "scripts/seed/verify_seed.sh",
)
STAGES = {
    "stage0": ROOT / "target/release/vitte",
    "stage1": ROOT / "target/stage1/vitte",
    "stage2": ROOT / "target/stage2/vitte",
    "release": ROOT / "target/release/vitte",
}


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def file_kind(path: Path) -> str:
    proc = subprocess.run(
        ["file", "-b", str(path)],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    return proc.stdout.strip()


def is_script(path: Path) -> bool:
    try:
        return path.read_bytes()[:2] == b"#!"
    except OSError:
        return False


def validate_stage(name: str, path: Path, failures: list[str]) -> dict[str, Any]:
    item: dict[str, Any] = {"name": name, "path": rel(path)}
    if not path.is_file():
        failures.append(f"{name} compiler missing: {rel(path)}")
        item["missing"] = True
        return item
    item["executable"] = os.access(path, os.X_OK)
    item["script"] = is_script(path)
    item["kind"] = file_kind(path)
    if not item["executable"]:
        failures.append(f"{name} compiler is not executable: {rel(path)}")
    if item["script"]:
        failures.append(f"{name} compiler is a script, not native Vitte artifact: {rel(path)}")
    for marker in FORBIDDEN_STAGE_MARKERS:
        if marker in rel(path):
            failures.append(f"{name} compiler path is seed/bootstrap: {rel(path)}")
    version = subprocess.run(
        [str(path), "--version"],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    item["version_exit_code"] = version.returncode
    item["version"] = (version.stdout + version.stderr).strip()
    if version.returncode != 0:
        failures.append(f"{name} compiler --version failed: {item['version']}")
    return item


def load_json(path: Path, failures: list[str]) -> dict[str, Any]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        failures.append(f"cannot read report {rel(path)}: {exc}")
        return {}
    if not isinstance(payload, dict):
        failures.append(f"report is not an object: {rel(path)}")
        return {}
    return payload


def validate_reports(failures: list[str]) -> list[dict[str, Any]]:
    reports: list[dict[str, Any]] = []
    for name in ("stage1_compiler_gate.json", "stage2_project_gate.json", "release_binary_gate.json", "selfhost_completion.json"):
        path = REPORT_DIR / name
        if name == "selfhost_completion.json":
            path = ROOT / "target/selfhost_completion/selfhost_completion.json"
        payload = load_json(path, failures)
        if not payload:
            continue
        text = json.dumps(payload, sort_keys=True)
        reports.append({"path": rel(path), "status": payload.get("status"), "entrypoint": payload.get("entrypoint") or payload.get("compiler_source")})
        if ENTRYPOINT not in text:
            failures.append(f"report does not reference canonical compiler entrypoint: {rel(path)}")
        if "toolchain/src" in text:
            failures.append(f"report references removed toolchain compiler source: {rel(path)}")
        for marker in FORBIDDEN_STAGE_MARKERS:
            if marker in text:
                failures.append(f"report references seed/bootstrap marker {marker}: {rel(path)}")
    return reports


def main() -> int:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    failures: list[str] = []
    if (ROOT / "toolchain/src").exists():
        failures.append("removed duplicate compiler source tree exists: toolchain/src")
    for removed in ("toolchain/seed", "scripts/seed", "bin/vittec0"):
        if (ROOT / removed).exists():
            failures.append(f"removed seed artifact path exists: {removed}")
    if not (ROOT / ENTRYPOINT).is_file():
        failures.append(f"missing canonical compiler entrypoint: {ENTRYPOINT}")

    stages = [validate_stage(name, path, failures) for name, path in STAGES.items()]
    reports = validate_reports(failures)
    status = "fail" if failures else "pass"
    payload = {
        "schema": "vitte.in.vitte.gate.v1",
        "status": status,
        "canonical_compiler_entrypoint": ENTRYPOINT,
        "removed_compiler_source_roots": ["toolchain/src"],
        "stages": stages,
        "reports": reports,
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    REPORT_MD.write_text(
        "# Vitte in Vitte gate\n\n"
        f"- status: {status}\n"
        f"- canonical compiler entrypoint: `{ENTRYPOINT}`\n"
        f"- removed source roots: `toolchain/src`\n"
        f"- stages checked: {len(stages)}\n"
        f"- reports checked: {len(reports)}\n"
        + ("\n## Failures\n" + "\n".join(f"- {failure}" for failure in failures) + "\n" if failures else ""),
        encoding="utf-8",
    )
    if failures:
        print("[vitte-in-vitte-gate][error] gate failed", file=sys.stderr)
        for failure in failures:
            print(f" - {failure}", file=sys.stderr)
        return 1
    print("[vitte-in-vitte-gate] ok compiler_source=src/vitte/compiler stage0=Vitte stage1=Vitte stage2=Vitte release=Vitte")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
