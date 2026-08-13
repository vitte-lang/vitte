#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CANDIDATE = ROOT / "target/compiler-source-sensitivity/stage1-candidate"
SENSITIVITY_REPORT = ROOT / "target/reports/compiler_source_sensitivity.json"
REPORT = ROOT / "target/reports/stage0_rotation_readiness.json"
FORBIDDEN_MARKERS = (
    b"vitte-bootstrap-payload",
    b"payload_source",
    b"write_payload_file",
    b"_command_build",
    b"_copy_file",
    b"vitte_stage0_clone_self",
)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_tree_sha256(root: Path) -> str:
    digest = hashlib.sha256()
    for path in sorted(root.rglob("*.vit")):
        digest.update(path.relative_to(root).as_posix().encode("utf-8"))
        digest.update(b"\0")
        digest.update(path.read_bytes())
        digest.update(b"\0")
    return digest.hexdigest()


def run(command: list[str], cwd: Path) -> dict[str, Any]:
    try:
        completed = subprocess.run(
            command,
            cwd=cwd,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=20,
            check=False,
        )
        return {"command": command, "exit_code": completed.returncode, "output": completed.stdout[-6000:]}
    except (OSError, subprocess.TimeoutExpired) as exc:
        return {"command": command, "exit_code": 124 if isinstance(exc, subprocess.TimeoutExpired) else 127, "output": str(exc)}


def main() -> int:
    failures: list[str] = []
    evidence: dict[str, Any] = {}
    try:
        sensitivity = json.loads(SENSITIVITY_REPORT.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        sensitivity = {}
        failures.append(f"missing or invalid source-sensitivity report: {exc}")

    if sensitivity.get("status") != "pass":
        failures.append("complete compiler source closure is not source-sensitive")
    if not sensitivity.get("runtime_selfhost_proven"):
        failures.append("source-sensitive compiler candidate is not runtime-ready")
    sensitivity_evidence = sensitivity.get("evidence", {})
    if not isinstance(sensitivity_evidence, dict):
        sensitivity_evidence = {}
    current_source_sha = source_tree_sha256(ROOT / "src/vitte/compiler")
    source_sha = sensitivity_evidence.get("compiler_source_sha256")
    module_count = sensitivity_evidence.get("compiler_module_count")
    output_changed = sensitivity_evidence.get("output_changed") is True
    baseline = sensitivity_evidence.get("baseline", {})
    baseline_sha = baseline.get("sha256") if isinstance(baseline, dict) else None
    if module_count != 971:
        failures.append(f"source-sensitivity report does not cover 971 compiler modules: {module_count}")
    if source_sha != current_source_sha:
        failures.append("source-sensitivity report does not match the current compiler source tree")
    if not output_changed:
        failures.append("source-sensitivity report does not prove a changed candidate")
    evidence["source_sensitivity"] = {
        "status": sensitivity.get("status", "missing"),
        "runtime_selfhost_proven": bool(sensitivity.get("runtime_selfhost_proven")),
        "compiler_module_count": module_count,
        "compiler_source_sha256": source_sha,
        "current_compiler_source_sha256": current_source_sha,
        "output_changed": output_changed,
        "baseline_sha256": baseline_sha,
    }

    if not CANDIDATE.is_file() or not os.access(CANDIDATE, os.X_OK):
        failures.append(f"missing executable candidate: {CANDIDATE.relative_to(ROOT)}")
    else:
        data = CANDIDATE.read_bytes()
        markers = [marker.decode("ascii") for marker in FORBIDDEN_MARKERS if marker in data]
        evidence["candidate"] = {
            "path": str(CANDIDATE.relative_to(ROOT)),
            "sha256": sha256(CANDIDATE),
            "size": CANDIDATE.stat().st_size,
            "forbidden_markers": markers,
        }
        if baseline_sha != evidence["candidate"]["sha256"]:
            failures.append("candidate hash does not match the source-sensitivity baseline")
        if markers:
            failures.append("candidate contains forbidden copy/payload markers: " + ", ".join(markers))

        with tempfile.TemporaryDirectory(prefix="vitte-stage0-readiness-") as raw_tmp:
            work = Path(raw_tmp)
            source = work / "main.vit"
            output = work / "hello"
            source.write_text('proc main() -> int { return 0; }\n', encoding="utf-8")
            probes = {
                "version": run([str(CANDIDATE), "--version"], ROOT),
                "check": run([str(CANDIDATE), "check", str(source)], ROOT),
                "build": run([str(CANDIDATE), "build", str(source), "-o", str(output)], ROOT),
            }
            probes["built_program_exists"] = output.is_file() and os.access(output, os.X_OK)
            if probes["built_program_exists"]:
                probes["built_program"] = run([str(output)], ROOT)
            evidence["runtime_probes"] = probes
            if probes["version"]["exit_code"] != 0:
                failures.append("candidate --version probe failed")
            if probes["check"]["exit_code"] != 0:
                failures.append("candidate check probe failed")
            if probes["build"]["exit_code"] != 0 or not probes["built_program_exists"]:
                failures.append("candidate build probe failed to produce an executable")
            elif probes["built_program"]["exit_code"] != 0:
                failures.append("program built by candidate failed to execute")

    payload = {
        "schema": "vitte.bootstrap.stage0-rotation-readiness.v1",
        "status": "ready" if not failures else "blocked",
        "signing_key_required_only_after_readiness": True,
        "candidate": str(CANDIDATE.relative_to(ROOT)),
        "evidence": evidence,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"[stage0-rotation-readiness][blocked] {failure}", file=sys.stderr)
        print(f"[stage0-rotation-readiness] status=blocked report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[stage0-rotation-readiness] status=ready candidate={CANDIDATE.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
