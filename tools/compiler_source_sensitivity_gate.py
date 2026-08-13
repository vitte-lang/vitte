#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BOOTSTRAP = ROOT / "target/bootstrap-c17/vitte-bootstrap"
SOURCE_ROOT = ROOT / "src/vitte"
ENTRY = Path("src/vitte/compiler/main.vit")
DRIVER = Path("src/vitte/compiler/driver/compiler.vit")
REPORT = ROOT / "target/reports/compiler_source_sensitivity.json"
ORIGINAL_VERSION = 'const VERSION_TEXT: string = "vittec vitte-compiler 0.1.0"'
PROBE_VERSION = 'const VERSION_TEXT: string = "vittec vitte-compiler source-sensitivity-probe"'
FORBIDDEN_SYMBOLS = ("_command_build", "_copy_file", "_vitte_stage0_clone_self")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command: list[str], cwd: Path, env: dict[str, str] | None = None) -> dict[str, Any]:
    completed = subprocess.run(
        command,
        cwd=cwd,
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


def build_candidate(project: Path, output: Path) -> dict[str, Any]:
    env = os.environ.copy()
    env["VITTE_C17_GENERIC_COMPILER"] = "1"
    result = run([str(BOOTSTRAP), "build", str(project / ENTRY), "-o", str(output)], project, env)
    result["exists"] = output.is_file()
    result["sha256"] = sha256(output) if output.is_file() else ""
    result["size"] = output.stat().st_size if output.is_file() else 0
    return result


def main() -> int:
    failures: list[str] = []
    if not BOOTSTRAP.is_file():
        failures.append("missing C17 bootstrap compiler; run make bootstrap-c17")

    module_count = sum(1 for _ in (ROOT / "src/vitte/compiler").rglob("*.vit"))
    if module_count != 971:
        failures.append(f"compiler source closure changed: expected 971 .vit modules, found {module_count}")

    evidence: dict[str, Any] = {"compiler_module_count": module_count}
    if not failures:
        with tempfile.TemporaryDirectory(prefix="vitte-source-sensitivity-") as raw_tmp:
            work = Path(raw_tmp)
            baseline_project = work / "baseline"
            perturbed_project = work / "perturbed"
            shutil.copytree(SOURCE_ROOT, baseline_project / "src/vitte")
            shutil.copytree(SOURCE_ROOT, perturbed_project / "src/vitte")

            probe_driver = perturbed_project / DRIVER
            driver_text = probe_driver.read_text(encoding="utf-8")
            perturbed_text = driver_text.replace(ORIGINAL_VERSION, PROBE_VERSION, 1)
            if perturbed_text == driver_text:
                failures.append("driver sensitivity probe could not locate VERSION_TEXT")
            else:
                probe_driver.write_text(perturbed_text, encoding="utf-8")

            baseline_output = work / "baseline-stage1"
            perturbed_output = work / "perturbed-stage1"
            baseline = build_candidate(baseline_project, baseline_output)
            perturbed = build_candidate(perturbed_project, perturbed_output)
            evidence["baseline"] = baseline
            evidence["perturbed"] = perturbed

            if baseline["exit_code"] != 0 or not baseline["exists"]:
                failures.append("baseline compiler source closure did not build")
            if perturbed["exit_code"] != 0 or not perturbed["exists"]:
                failures.append("perturbed compiler source closure did not build")

            output_changed = bool(baseline["sha256"] and perturbed["sha256"] and baseline["sha256"] != perturbed["sha256"])
            evidence["output_changed"] = output_changed
            if baseline["sha256"] and perturbed["sha256"] and not output_changed:
                failures.append("modifying driver/compiler.vit did not change the rebuilt compiler candidate")

            if baseline_output.is_file():
                binary_data = baseline_output.read_bytes()
                forbidden = [symbol for symbol in FORBIDDEN_SYMBOLS if symbol.encode("ascii") in binary_data]
                evidence["forbidden_copy_symbols"] = forbidden
                if forbidden:
                    failures.append("compiler candidate contains binary-copy symbols: " + ", ".join(forbidden))

                runtime = run([str(baseline_output), "--version"], baseline_project)
                evidence["runtime_probe"] = runtime
                evidence["runtime_ready"] = runtime["exit_code"] == 0 and "vitte" in runtime["output_tail"].lower()

    payload = {
        "schema": "vitte.compiler.source-sensitivity.v1",
        "status": "fail" if failures else "pass",
        "scope": "C17 generic compilation of the complete Vitte compiler source closure",
        "runtime_selfhost_proven": bool(evidence.get("runtime_ready", False)),
        "evidence": evidence,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    for failure in failures:
        print(f"[compiler-source-sensitivity][error] {failure}", file=sys.stderr)
    print(
        f"[compiler-source-sensitivity] status={payload['status']} modules={module_count} "
        f"changed={evidence.get('output_changed', False)} runtime_ready={payload['runtime_selfhost_proven']} "
        f"report={REPORT.relative_to(ROOT)}"
    )
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
