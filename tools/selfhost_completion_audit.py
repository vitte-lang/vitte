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
COMPILER_SOURCE = ROOT / "src" / "vitte" / "compiler" / "main.vit"
BRIDGE_MARKER = b"vitte-bootstrap-payload-bridge"
SHELL_PREFIXES = (b"#!/usr/bin/env sh", b"#!/bin/sh")


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    environment = os.environ.copy()
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


def step_result(name: str, command: list[str], output: Path) -> dict[str, object]:
    completed = run(command)
    artifact = artifact_state(output)
    version_result = run([str(output), "--version"]) if artifact["executable"] else None
    version = "" if version_result is None else (version_result.stdout + version_result.stderr).strip()
    version_ok = version_result is not None and version_result.returncode == 0 and version.startswith("vittec ")
    return {
        "name": name,
        "returncode": completed.returncode,
        "artifact": artifact,
        "version": version,
        "expected_version_prefix": "vittec ",
        "version_ok": version_ok,
        "ok": completed.returncode == 0 and artifact["executable"] and version_ok,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit the Vitte self-hosting completion chain")
    parser.add_argument(
        "--strict-complete",
        action="store_true",
        help="fail unless successive compiler generations reach byte parity without transition payloads",
    )
    parser.add_argument(
        "--require-parity",
        action="store_true",
        help="fail unless successive compiler generations reach byte parity",
    )
    args = parser.parse_args()

    OUT.mkdir(parents=True, exist_ok=True)
    REPORT.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="vitte-selfhost-", dir=OUT) as raw_tmp:
        tmp = Path(raw_tmp)
        generation1 = tmp / "vittec-generation1"
        generation2 = tmp / "vittec-generation2"
        steps = [
            step_result(
                "seed_builds_generation1",
                [
                    str(ROOT / "bin" / "vittec0"),
                    "build-native",
                    "--src",
                    str(COMPILER_SOURCE),
                    "--out",
                    str(generation1),
                ],
                generation1,
            ),
            step_result(
                "generation1_builds_generation2",
                [str(generation1), "build", str(COMPILER_SOURCE), "-o", str(generation2)],
                generation2,
            ),
        ]

        generation1_state = steps[0]["artifact"]
        generation2_state = steps[1]["artifact"]
        parity_available = bool(generation1_state["available"] and generation2_state["available"])
        parity_equal = bool(parity_available and generation1_state["sha256"] == generation2_state["sha256"])
        parity = {
            "available": parity_available,
            "hash_equal": parity_equal,
            "size_equal": bool(
                parity_available and generation1_state["size_bytes"] == generation2_state["size_bytes"]
            ),
            "first_difference_offset": first_difference(generation1, generation2) if parity_available else None,
            "generation1_hash": generation1_state["sha256"],
            "generation2_hash": generation2_state["sha256"],
        }
        transition_remaining = any(
            bool(state[key])
            for state in (generation1_state, generation2_state)
            for key in ("shell_payload", "sidecar_bridge", "embedded_bridge")
        )
        chain_ok = all(bool(step["ok"]) for step in steps)
        complete = chain_ok and parity_equal and not transition_remaining
        status = "complete" if complete else ("transition" if chain_ok else "fail")

        payload = {
            "schema": "vitte.selfhost_completion",
            "schema_version": "1.2.0",
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
    first_difference_text = parity["first_difference_offset"]
    if first_difference_text is None:
        first_difference_text = "none"
    REPORT.write_text(
        "# Self-hosting Completion Audit\n\n"
        f"- compiler source: `{payload['compiler_source']}`\n"
        f"- seed -> generation1 -> generation2: {'PASS' if chain_ok else 'FAIL'}\n"
        f"- generation1 == generation2 byte parity: {'PASS' if parity_equal else 'FAIL'}\n"
        f"- first differing byte: {first_difference_text}\n"
        f"- generation1 embedded bridge: {'PRESENT' if steps[0]['artifact']['embedded_bridge'] else 'ABSENT'}\n"
        f"- generation2 embedded bridge: {'PRESENT' if steps[1]['artifact']['embedded_bridge'] else 'ABSENT'}\n"
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
    if args.require_parity:
        return 0 if chain_ok and parity_equal else 1
    return 0 if chain_ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
