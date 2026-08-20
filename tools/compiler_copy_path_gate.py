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
SOURCE_BOOTSTRAP = ROOT / "target/bootstrap-c17/vitte-bootstrap"
FIXTURE = ROOT / "tests/pipeline/hello_world.vit"
OUTPUT = ROOT / "target/compiler-copy-path/fixture"
REPORT_JSON = ROOT / "target/reports/compiler_copy_path_gate.json"
REPORT_MD = ROOT / "target/reports/compiler_copy_path_gate.md"
BOOTSTRAP_REAL = ROOT / "tools/bootstrap_real/bootstrap_real.py"
BOOTSTRAP_CHAIN = ROOT / "tools/bootstrap_real/bootstrap_chain.py"
STAGE1_GATE = ROOT / "tools/stage1_compiler_gate.sh"
BOOTSTRAP_MAX = ROOT / "tools/bootstrap_max_gate.py"
RUNTIME_PROVENANCE = ROOT / "tools/runtime_driver_provenance_gate.py"
FAIL_CLOSED_GATES = (
    ROOT / "tools/compiler_real_native_gate.sh",
    ROOT / "tools/stage2_project_gate.py",
    ROOT / "tools/release_binary_gate.py",
    ROOT / "tools/release_clean_selfhost_gate.py",
    ROOT / "tools/selfhost_stage_compare_gate.py",
    ROOT / "tools/selfhost_stage0_gate.py",
    ROOT / "tools/selfhost_stage_gates.py",
)
ENTRYPOINT = "src/vitte/compiler/main.vit"
RETIRED_MARKERS = (
    b"vitte_stage0_clone_self",
    b"vitte-bootstrap-payload",
    b"payload_source",
    b"write_payload_file",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def function_body(text: str, name: str) -> str:
    marker = f"def {name}("
    start = text.find(marker)
    if start < 0:
        return ""
    next_def = text.find("\ndef ", start + len(marker))
    return text[start:] if next_def < 0 else text[start:next_def]


def static_contract(failures: list[str]) -> dict[str, Any]:
    bootstrap_real = BOOTSTRAP_REAL.read_text(encoding="utf-8")
    chain = BOOTSTRAP_CHAIN.read_text(encoding="utf-8")
    stage1 = STAGE1_GATE.read_text(encoding="utf-8")
    maximum = BOOTSTRAP_MAX.read_text(encoding="utf-8")
    runtime_provenance = RUNTIME_PROVENANCE.read_text(encoding="utf-8")
    checks: dict[str, bool] = {
        "stage1_defaults_to_source_bootstrap": (
            'BOOTSTRAP_COMPILER="${VITTE_BOOTSTRAP_COMPILER:-$ROOT_DIR/target/bootstrap-c17/vitte-bootstrap}"'
            in stage1
        ),
        "stage1_enables_generic_source_compilation": "VITTE_C17_GENERIC_COMPILER=1" in stage1,
        "stage1_does_not_materialize_from_signed_stage0": 'bootstrap_real.py" --stage0' not in stage1,
        "chain_builds_bootstrap_from_source": "source_bootstrap_build_command()" in chain,
        "chain_never_executes_signed_stage0": (
            '[sys.executable, str(BOOTSTRAP_REAL), "--stage0"' not in chain
        ),
        "chain_marks_trust_artifact_non_executable": '"executed_as_compiler": False' in chain,
        "clean_room_uses_source_bootstrap": "[str(SOURCE_BOOTSTRAP), \"build\"" in maximum,
        "clean_room_never_builds_with_trusted_stage0": (
            '[str(STAGES["trusted_stage0"]), "build"' not in maximum
        ),
        "runtime_probe_skips_rejected_compiler": "compilation_probe_skipped" in runtime_provenance,
    }
    fail_closed = {
        rel(path): "vitte_stage0_clone_self" in path.read_text(encoding="utf-8")
        for path in FAIL_CLOSED_GATES
    }
    builders: dict[str, dict[str, bool]] = {}
    for name in ("stage1_build_command", "stage2_build_command", "release_build_command"):
        body = function_body(bootstrap_real, name)
        builders[name] = {
            "compiles_entrypoint": bool(body and '"build"' in body and "rel(ENTRYPOINT)" in body and '"-o"' in body),
            "copies_artifact": "copy" in body,
        }
        if not builders[name]["compiles_entrypoint"]:
            failures.append(f"{name} does not compile {ENTRYPOINT}")
        if builders[name]["copies_artifact"]:
            failures.append(f"{name} copies an artifact instead of compiling sources")
    for name, passed in checks.items():
        if not passed:
            failures.append(f"static copy-path contract failed: {name}")
    for path, passed in fail_closed.items():
        if not passed:
            failures.append(f"compiler gate lacks self-copy preflight: {path}")
    return {"checks": checks, "fail_closed_gates": fail_closed, "stage_builders": builders}


def runtime_contract(failures: list[str]) -> dict[str, Any]:
    evidence: dict[str, Any] = {
        "compiler": rel(SOURCE_BOOTSTRAP),
        "source": rel(FIXTURE),
        "output": rel(OUTPUT),
    }
    if not SOURCE_BOOTSTRAP.is_file() or not os.access(SOURCE_BOOTSTRAP, os.X_OK):
        failures.append(f"missing source bootstrap compiler: {rel(SOURCE_BOOTSTRAP)}")
        return evidence
    if not FIXTURE.is_file():
        failures.append(f"missing compilation fixture: {rel(FIXTURE)}")
        return evidence

    compiler_data = SOURCE_BOOTSTRAP.read_bytes()
    compiler_markers = [marker.decode("ascii") for marker in RETIRED_MARKERS if marker in compiler_data]
    evidence["compiler_sha256"] = sha256(SOURCE_BOOTSTRAP)
    evidence["compiler_retired_markers"] = compiler_markers
    if b"vitte_stage0_clone_self" in compiler_data:
        failures.append("source bootstrap contains the retired self-copy implementation")

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.unlink(missing_ok=True)
    env = os.environ.copy()
    env["VITTE_C17_GENERIC_COMPILER"] = "1"
    command = [str(SOURCE_BOOTSTRAP), "build", rel(FIXTURE), "-o", str(OUTPUT)]
    completed = subprocess.run(
        command,
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    evidence["build"] = {
        "command": command,
        "exit_code": completed.returncode,
        "output_tail": completed.stdout[-6000:],
    }
    if completed.returncode != 0 or not OUTPUT.is_file():
        failures.append("source bootstrap failed to compile the no-copy fixture")
        return evidence

    output_data = OUTPUT.read_bytes()
    output_markers = [marker.decode("ascii") for marker in RETIRED_MARKERS if marker in output_data]
    output_sha = sha256(OUTPUT)
    evidence.update(
        {
            "output_sha256": output_sha,
            "output_size": OUTPUT.stat().st_size,
            "output_retired_markers": output_markers,
            "output_differs_from_compiler": output_sha != evidence["compiler_sha256"],
        }
    )
    if output_sha == evidence["compiler_sha256"]:
        failures.append("compiled program is a byte-for-byte copy of the compiler")
    if output_markers:
        failures.append("compiled program contains retired copy/payload markers: " + ", ".join(output_markers))
    return evidence


def write_report(payload: dict[str, Any]) -> None:
    REPORT_JSON.parent.mkdir(parents=True, exist_ok=True)
    REPORT_JSON.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    runtime = payload["runtime"]
    lines = [
        "# Compiler Copy Path Gate",
        "",
        f"- status: {payload['status']}",
        f"- compiler: {runtime.get('compiler', 'missing')}",
        f"- source: {runtime.get('source', 'missing')}",
        f"- compiler_sha256: {runtime.get('compiler_sha256', 'missing')}",
        f"- output_sha256: {runtime.get('output_sha256', 'missing')}",
        f"- output_differs_from_compiler: {runtime.get('output_differs_from_compiler', False)}",
    ]
    if payload["failures"]:
        lines.extend(["", "## Failures", *[f"- {item}" for item in payload["failures"]]])
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    failures: list[str] = []
    payload: dict[str, Any] = {
        "schema": "vitte.compiler.copy-path-gate.v1",
        "status": "fail",
        "static": static_contract(failures),
        "runtime": runtime_contract(failures),
        "failures": failures,
    }
    payload["status"] = "pass" if not failures else "fail"
    write_report(payload)
    for failure in failures:
        print(f"[compiler-copy-path][error] {failure}", file=sys.stderr)
    print(
        f"[compiler-copy-path] status={payload['status']} "
        f"output_differs={payload['runtime'].get('output_differs_from_compiler', False)} "
        f"report={rel(REPORT_JSON)}"
    )
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
