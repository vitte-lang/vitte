#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "target" / "selfhost_completion"
REPORT = ROOT / "target" / "reports" / "selfhost_completion.md"
STAGE1_SOURCE = ROOT / "toolchain" / "stage1" / "src" / "main.vit"
COMPILER_SOURCE = ROOT / "src" / "vitte" / "compiler" / "main.vit"
BRIDGE_MARKER = b"vitte-bootstrap-payload-bridge"
SHELL_PREFIXES = (b"#!/usr/bin/env sh", b"#!/bin/sh")


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    environment = os.environ.copy()
    environment["VITTE_BOOTSTRAP_ALLOW_FULL_COMPILER_BRIDGE"] = "1"
    try:
        return subprocess.run(
            command,
            cwd=ROOT,
            env=environment,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=60,
        )
    except subprocess.TimeoutExpired as error:
        return subprocess.CompletedProcess(command, 124, error.stdout or "", error.stderr or "command timed out")
    except OSError as error:
        return subprocess.CompletedProcess(command, 127, "", str(error))


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def artifact_state(path: Path) -> dict[str, object]:
    if not path.is_file():
        return {
            "available": False,
            "executable": False,
            "size_bytes": 0,
            "shell_payload": False,
            "sidecar_bridge": False,
            "embedded_bridge": False,
            "sha256": "",
        }
    data = path.read_bytes()
    return {
        "available": True,
        "executable": os.access(path, os.X_OK),
        "size_bytes": len(data),
        "shell_payload": data.startswith(SHELL_PREFIXES),
        "sidecar_bridge": Path(str(path) + ".bootstrap-bridge").is_file(),
        "embedded_bridge": BRIDGE_MARKER in data,
        "sha256": hashlib.sha256(data).hexdigest(),
    }


def first_difference(left: Path, right: Path) -> int | None:
    left_data = left.read_bytes()
    right_data = right.read_bytes()
    for offset, (left_byte, right_byte) in enumerate(zip(left_data, right_data)):
        if left_byte != right_byte:
            return offset
    if len(left_data) != len(right_data):
        return min(len(left_data), len(right_data))
    return None


def step_result(name: str, command: list[str], output: Path, expected_version: str) -> dict[str, object]:
    completed = run(command)
    artifact = artifact_state(output)
    version_result = run([str(output), "--version"]) if artifact["executable"] else None
    version = "" if version_result is None else (version_result.stdout + version_result.stderr).strip()
    version_ok = version_result is not None and version_result.returncode == 0 and version == expected_version
    return {
        "name": name,
        "returncode": completed.returncode,
        "artifact": artifact,
        "version": version,
        "expected_version": expected_version,
        "version_ok": version_ok,
        "ok": completed.returncode == 0 and artifact["executable"] and version_ok,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit the Vitte self-hosting completion chain")
    parser.add_argument(
        "--strict-complete",
        action="store_true",
        help="fail unless the real compiler reaches stage2/stage3 byte parity without transition payloads",
    )
    args = parser.parse_args()

    OUT.mkdir(parents=True, exist_ok=True)
    REPORT.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="vitte-selfhost-", dir=OUT) as raw_tmp:
        tmp = Path(raw_tmp)
        stage1 = tmp / "vittec1"
        stage2 = tmp / "vittec2"
        stage3 = tmp / "vittec3"
        steps = [
            step_result(
                "vittec0_builds_vittec1",
                [str(ROOT / "bin" / "vittec0"), "build-native", "--src", str(STAGE1_SOURCE), "--out", str(stage1)],
                stage1,
                "vittec1 stage1-vitte 0.1.0",
            ),
            step_result(
                "vittec1_builds_real_compiler_stage2",
                [str(stage1), "build", str(COMPILER_SOURCE), "-o", str(stage2)],
                stage2,
                "vittec2 stage2-vitte 0.1.0",
            ),
            step_result(
                "vittec2_rebuilds_real_compiler_stage3",
                [str(stage2), "build", str(COMPILER_SOURCE), "-o", str(stage3)],
                stage3,
                "vittec2 stage2-vitte 0.1.0",
            ),
        ]

        stage2_state = steps[1]["artifact"]
        stage3_state = steps[2]["artifact"]
        parity_available = bool(stage2_state["available"] and stage3_state["available"])
        parity_equal = bool(parity_available and stage2_state["sha256"] == stage3_state["sha256"])
        parity = {
            "available": parity_available,
            "hash_equal": parity_equal,
            "size_equal": bool(
                parity_available and stage2_state["size_bytes"] == stage3_state["size_bytes"]
            ),
            "first_difference_offset": first_difference(stage2, stage3) if parity_available else None,
            "stage2_hash": stage2_state["sha256"],
            "stage3_hash": stage3_state["sha256"],
        }
        transition_remaining = any(
            bool(state[key])
            for state in (stage2_state, stage3_state)
            for key in ("shell_payload", "sidecar_bridge", "embedded_bridge")
        )
        chain_ok = all(bool(step["ok"]) for step in steps)
        complete = chain_ok and parity_equal and not transition_remaining
        status = "complete" if complete else ("transition" if chain_ok else "fail")

        payload = {
            "schema": "vitte.selfhost_completion",
            "schema_version": "1.1.0",
            "status": status,
            "strict_complete": args.strict_complete,
            "compiler_source": str(COMPILER_SOURCE.relative_to(ROOT)),
            "compiler_source_sha256": sha256(COMPILER_SOURCE),
            "steps": steps,
            "parity": parity,
            "transition_payload_remaining": transition_remaining,
        }

    (OUT / "selfhost_completion.json").write_text(
        json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    REPORT.write_text(
        "# Self-hosting Completion Audit\n\n"
        f"- compiler source: `{payload['compiler_source']}`\n"
        f"- stage0 -> stage1 -> compiler stage2 -> compiler stage3: {'PASS' if chain_ok else 'FAIL'}\n"
        f"- stage2 == stage3 byte parity: {'PASS' if parity_equal else 'FAIL'}\n"
        f"- first differing byte: {parity['first_difference_offset']}\n"
        f"- stage2 embedded bridge: {'PRESENT' if steps[1]['artifact']['embedded_bridge'] else 'ABSENT'}\n"
        f"- stage3 embedded bridge: {'PRESENT' if steps[2]['artifact']['embedded_bridge'] else 'ABSENT'}\n"
        f"- transition payload removed: {'PASS' if not transition_remaining else 'TRANSITION'}\n"
        f"- status: {status}\n",
        encoding="utf-8",
    )
    print(
        f"[selfhost-completion] status={status} chain={'ok' if chain_ok else 'fail'} "
        f"parity={parity_equal} transition_payload={transition_remaining}"
    )
    if args.strict_complete:
        return 0 if complete else 1
    return 0 if chain_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
