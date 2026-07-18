#!/usr/bin/env python3
from __future__ import annotations

import json
import platform
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT_DIR = ROOT / "target" / "reports" / "bootstrap"
REPORT = REPORT_DIR / "hard_gate.json"


def run_step(name: str, command: list[str]) -> dict[str, object]:
    start = time.time()
    completed = subprocess.run(
        command,
        cwd=ROOT,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    duration = time.time() - start
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    (REPORT_DIR / f"{name}.log").write_text(completed.stdout or "", encoding="utf-8")
    return {
        "name": name,
        "command": command,
        "returncode": completed.returncode,
        "duration_sec": duration,
        "ok": completed.returncode == 0,
    }


def write_report(payload: dict[str, object]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    tmp = REPORT.with_suffix(".json.tmp")
    tmp.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    tmp.replace(REPORT)


def main() -> int:
    payload: dict[str, object] = {
        "schema": "vitte.bootstrap.hard_gate",
        "schema_version": "2.0.0",
        "trust_root": "toolchain/seed/vittec0.seed",
        "platform": {
            "system": platform.system(),
            "machine": platform.machine(),
            "release": platform.release(),
        },
        "status": "fail",
        "steps": [],
    }
    commands = [
        ("seed_verify", ["make", "--no-print-directory", "seed-verify"]),
        ("bootstrap_seed", ["make", "--no-print-directory", "bootstrap-seed"]),
        ("seed_artifact_contract", ["python3", "tools/check_bootstrap_seed_root.py", "--artifacts"]),
        ("bootstrap_native_snapshots", ["make", "--no-print-directory", "bootstrap-native-snapshots"]),
    ]
    steps: list[dict[str, object]] = []
    for name, command in commands:
        step = run_step(name, command)
        steps.append(step)
        print(f"[bootstrap-hard-gate] {name}: {'ok' if step['ok'] else 'fail'}")
        if not step["ok"]:
            payload["steps"] = steps
            payload["failed_step"] = name
            write_report(payload)
            return 1
    payload["steps"] = steps
    payload["status"] = "ok"
    write_report(payload)
    print(json.dumps({"status": "ok", "report": str(REPORT.relative_to(ROOT))}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
