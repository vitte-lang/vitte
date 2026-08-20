#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BOOTSTRAP = ROOT / "target/bootstrap-c17/vitte-bootstrap"
ENTRY = ROOT / "src/vitte/compiler/main.vit"
DRIVER = ROOT / "src/vitte/compiler/driver/compiler.vit"
STAGE1 = ROOT / "target/stage1/vitte"
PREDECESSOR = ROOT / "target/bootstrap-real/vitte"
SENSITIVITY_REPORT = ROOT / "target/reports/compiler_source_sensitivity.json"
REPORT_JSON = ROOT / "target/reports/driver_provenance_gate.json"
REPORT_MD = ROOT / "target/reports/driver_provenance_gate.md"
PROVENANCE_MARKER = b"COMPILER_DRIVER_SOURCE=src/vitte/compiler/driver/compiler.vit"
REQUIRED_SYMBOLS = ("compiler_driver_provenance", "run_cli_main_with_ice_boundary")
FORBIDDEN_MARKERS = (b"_command_build", b"_copy_file", b"vitte_stage0_clone_self")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def run(command: list[str], *, env: dict[str, str] | None = None) -> dict[str, Any]:
    completed = subprocess.run(
        command,
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    return {
        "command": command,
        "exit_code": completed.returncode,
        "output_tail": completed.stdout[-12000:],
    }


def load_sensitivity_report(failures: list[str]) -> dict[str, Any]:
    try:
        payload = json.loads(SENSITIVITY_REPORT.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        failures.append(f"missing or invalid source-sensitivity report: {exc}")
        return {}
    if payload.get("status") != "pass":
        failures.append("compiler source-sensitivity gate did not pass")
    evidence = payload.get("evidence", {})
    if not isinstance(evidence, dict) or evidence.get("output_changed") is not True:
        failures.append("driver perturbation did not change the rebuilt compiler")
    driver = evidence.get("driver", {}) if isinstance(evidence, dict) else {}
    if not isinstance(driver, dict) or driver.get("source_sha256") != sha256(DRIVER):
        failures.append("source-sensitivity evidence does not match the current driver")
    return payload


def write_report(status: str, evidence: dict[str, Any], failures: list[str]) -> None:
    payload = {
        "schema": "vitte.compiler.driver-provenance.v1",
        "status": status,
        "source_of_truth": rel(DRIVER),
        "entrypoint": rel(ENTRY),
        "stage1": rel(STAGE1),
        "evidence": evidence,
        "failures": failures,
    }
    REPORT_JSON.parent.mkdir(parents=True, exist_ok=True)
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    stage1 = evidence.get("stage1", {})
    predecessor = evidence.get("predecessor", {})
    lines = [
        "# Driver Provenance Gate",
        "",
        f"- status: {status}",
        f"- driver: {rel(DRIVER)}",
        f"- stage1: {rel(STAGE1)}",
        f"- driver_sha256: {evidence.get('driver_sha256', 'missing')}",
        f"- stage1_sha256: {stage1.get('sha256', 'missing')}",
        f"- predecessor_sha256: {predecessor.get('sha256', 'missing')}",
        f"- stage1_differs_from_predecessor: {evidence.get('stage1_differs_from_predecessor', False)}",
        f"- driver_perturbation_changes_compiler: {evidence.get('driver_perturbation_changes_compiler', False)}",
    ]
    if failures:
        lines.extend(["", "## Failures", *[f"- {failure}" for failure in failures]])
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    evidence: dict[str, Any] = {"driver_sha256": sha256(DRIVER)}
    sensitivity = load_sensitivity_report(failures)
    sensitivity_evidence = sensitivity.get("evidence", {})
    evidence["driver_perturbation_changes_compiler"] = (
        isinstance(sensitivity_evidence, dict) and sensitivity_evidence.get("output_changed") is True
    )

    if not BOOTSTRAP.is_file() or not os.access(BOOTSTRAP, os.X_OK):
        failures.append(f"missing executable bootstrap compiler: {rel(BOOTSTRAP)}")
    else:
        previous_sha = sha256(STAGE1) if STAGE1.is_file() else None
        STAGE1.parent.mkdir(parents=True, exist_ok=True)
        STAGE1.unlink(missing_ok=True)
        env = os.environ.copy()
        env["VITTE_C17_GENERIC_COMPILER"] = "1"
        build = run([str(BOOTSTRAP), "build", rel(ENTRY), "-o", str(STAGE1)], env=env)
        evidence["build"] = build
        evidence["previous_stage1_sha256"] = previous_sha
        if build["exit_code"] != 0 or not STAGE1.is_file():
            failures.append("bootstrap compiler failed to rebuild canonical stage1")

    if STAGE1.is_file():
        data = STAGE1.read_bytes()
        evidence["stage1"] = {
            "path": rel(STAGE1),
            "sha256": sha256(STAGE1),
            "size": STAGE1.stat().st_size,
            "carries_driver_provenance": PROVENANCE_MARKER in data,
        }
        if PROVENANCE_MARKER not in data:
            failures.append("stage1 does not carry canonical driver source provenance")
        forbidden = [marker.decode("ascii") for marker in FORBIDDEN_MARKERS if marker in data]
        evidence["stage1"]["forbidden_copy_markers"] = forbidden
        if forbidden:
            failures.append("stage1 contains binary-copy markers: " + ", ".join(forbidden))

        symbols_completed = subprocess.run(
            ["nm", str(STAGE1)],
            cwd=ROOT,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        present_symbols = [symbol for symbol in REQUIRED_SYMBOLS if symbol in symbols_completed.stdout]
        evidence["symbols"] = {
            "command": ["nm", str(STAGE1)],
            "exit_code": symbols_completed.returncode,
            "required": list(REQUIRED_SYMBOLS),
            "present": present_symbols,
            "output_tail": symbols_completed.stdout[-4000:],
        }
        if symbols_completed.returncode != 0:
            failures.append("unable to inspect stage1 symbols")
        else:
            missing = [symbol for symbol in REQUIRED_SYMBOLS if symbol not in present_symbols]
            if missing:
                failures.append("stage1 is missing driver symbols: " + ", ".join(missing))

        probes = {
            "version": run([str(STAGE1), "--version"]),
            "help": run([str(STAGE1), "--help"]),
        }
        evidence["runtime_probes"] = probes
        for name, probe in probes.items():
            if probe["exit_code"] != 0:
                failures.append(f"stage1 {name} probe failed")

    if PREDECESSOR.is_file() and STAGE1.is_file():
        predecessor_sha = sha256(PREDECESSOR)
        evidence["predecessor"] = {"path": rel(PREDECESSOR), "sha256": predecessor_sha}
        differs = predecessor_sha != evidence["stage1"]["sha256"]
        evidence["stage1_differs_from_predecessor"] = differs
        if not differs:
            failures.append("rebuilt stage1 is byte-for-byte identical to its predecessor")
    else:
        evidence["stage1_differs_from_predecessor"] = False
        failures.append(f"missing predecessor compiler: {rel(PREDECESSOR)}")

    status = "pass" if not failures else "fail"
    write_report(status, evidence, failures)
    for failure in failures:
        print(f"[driver-provenance][error] {failure}", file=sys.stderr)
    print(
        f"[driver-provenance] status={status} "
        f"stage1_changed={evidence.get('stage1_differs_from_predecessor', False)} "
        f"source_sensitive={evidence.get('driver_perturbation_changes_compiler', False)} "
        f"report={rel(REPORT_JSON)}"
    )
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
