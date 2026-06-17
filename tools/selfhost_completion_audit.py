#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import argparse
import json
import os
import stat
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "target" / "selfhost_completion"
REPORT = ROOT / "target" / "reports" / "selfhost_completion.md"


def run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def is_shell(path: Path) -> bool:
    try:
        head = path.read_text(encoding="utf-8", errors="ignore")[:128]
    except UnicodeDecodeError:
        return False
    return head.startswith("#!/usr/bin/env sh") or head.startswith("#!/bin/sh")


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit the Vitte self-hosting completion chain")
    parser.add_argument(
        "--strict-complete",
        action="store_true",
        help="fail unless stage2/stage3 byte parity is reached and shell transition payloads are gone",
    )
    args = parser.parse_args()

    OUT.mkdir(parents=True, exist_ok=True)
    REPORT.parent.mkdir(parents=True, exist_ok=True)

    tmp = Path(tempfile.mkdtemp(prefix="vitte-selfhost-", dir=ROOT / "target"))
    stage1 = tmp / "vittec1"
    stage2 = tmp / "vittec2"
    stage3 = tmp / "vittec3"

    steps: list[dict[str, object]] = []
    commands = [
        ("vittec0_builds_vittec1", [str(ROOT / "bin" / "vittec0"), "build-native", "--src", str(ROOT / "toolchain/stage1/src/main.vit"), "--out", str(stage1)], stage1),
        ("vittec1_builds_vittec2", [str(stage1), "build-native", "--src", str(ROOT / "toolchain/stage2/src/main.vit"), "--out", str(stage2)], stage2),
        ("vittec2_builds_vittec3", [str(stage2), "build-native", "--src", str(ROOT / "toolchain/stage3/src/main.vit"), "--out", str(stage3)], stage3),
    ]
    ok = True
    for name, cmd, output in commands:
        completed = run(cmd)
        executable = output.exists() and os.access(output, os.X_OK)
        sidecar = Path(str(output) + ".bootstrap-bridge").exists()
        item = {
            "name": name,
            "returncode": completed.returncode,
            "executable": executable,
            "sidecar_bridge": sidecar,
            "shell_payload": is_shell(output) if output.exists() else False,
        }
        item["ok"] = completed.returncode == 0 and executable and not sidecar
        steps.append(item)
        ok = ok and bool(item["ok"])

    parity = {
        "available": stage2.exists() and stage3.exists(),
        "hash_equal": stage2.exists() and stage3.exists() and sha(stage2) == sha(stage3),
        "stage2_hash": sha(stage2) if stage2.exists() else "",
        "stage3_hash": sha(stage3) if stage3.exists() else "",
    }

    payload_free = bool(parity["available"]) and not is_shell(stage2) and not is_shell(stage3)
    status = "complete" if ok and parity["hash_equal"] and payload_free else "transition"
    payload = {
        "schema": "vitte.selfhost_completion",
        "status": status,
        "strict_complete": args.strict_complete,
        "steps": steps,
        "parity": parity,
        "payload_shell_transition_remaining": not payload_free,
    }
    (OUT / "selfhost_completion.json").write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    REPORT.write_text(
        "# Self-hosting Completion Audit\n\n"
        f"- stage0 -> stage1 -> stage2 -> stage3: {'PASS' if ok else 'FAIL'}\n"
        f"- vittec2 == vittec3 byte parity: {'PASS' if parity['hash_equal'] else 'FAIL'}\n"
        f"- payload shell transition removed: {'PASS' if payload_free else 'TRANSITION'}\n"
        f"- status: {status}\n",
        encoding="utf-8",
    )
    print(f"[selfhost-completion] status={status} chain={'ok' if ok else 'fail'} parity={parity['hash_equal']} payload_free={payload_free}")
    if args.strict_complete:
        return 0 if ok and parity["hash_equal"] and payload_free else 1
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
