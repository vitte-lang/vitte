#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
COMPILER = ROOT / "bin/vitte"
MANIFEST = ROOT / "src/vitte/stdlib/modules.vitte.json"
REPORT = ROOT / "target/reports/stdlib_target_matrix.json"
TARGETS = (
    "x86_64-linux-gnu",
    "i386-linux-gnu",
    "aarch64-linux-gnu",
    "riscv64-linux-gnu",
)


def main() -> int:
    if not COMPILER.is_file():
        print("[stdlib-target-matrix][error] missing canonical bin/vitte", file=sys.stderr)
        return 1
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    modules = sorted(
        str(entry["path"])
        for entry in manifest.get("modules", [])
        if isinstance(entry, dict) and entry.get("kind") == "module" and isinstance(entry.get("path"), str)
    )
    failures: list[dict[str, object]] = []
    checked = 0
    for target in TARGETS:
        for source in modules:
            completed = subprocess.run(
                [str(COMPILER), "check", source, f"--target={target}"],
                cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False,
            )
            checked += 1
            if completed.returncode != 0:
                failures.append({"target": target, "source": source, "exit_code": completed.returncode, "output": completed.stdout[-2000:]})
    payload = {
        "schema": "vitte.stdlib.target-matrix.v1",
        "status": "pass" if not failures else "fail",
        "compiler": "bin/vitte",
        "targets": list(TARGETS),
        "modules": len(modules),
        "checks": checked,
        "failures": failures,
        "execution_evidence": "cross-target compiler checks; native execution remains owned by real target runners",
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failures:
        print(f"[stdlib-target-matrix][error] {len(failures)}/{checked} checks failed", file=sys.stderr)
        return 1
    print(f"[stdlib-target-matrix] OK modules={len(modules)} targets={len(TARGETS)} checks={checked}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
