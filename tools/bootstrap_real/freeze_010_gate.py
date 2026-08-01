#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ALLOWED_PREFIXES = (
    "tools/bootstrap_real/",
    "target/bootstrap-real/",
    "target/reports/bootstrap_real_gate.",
)


def git_status() -> list[str]:
    completed = subprocess.run(
        ["git", "status", "--short"],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if completed.returncode != 0:
        raise SystemExit(completed.stdout)
    return [line for line in completed.stdout.splitlines() if line.strip()]


def status_path(line: str) -> str:
    path = line[3:]
    if " -> " in path:
        path = path.split(" -> ", 1)[1]
    return path


def is_allowed(path: str) -> bool:
    return any(path.startswith(prefix) for prefix in ALLOWED_PREFIXES)


def main() -> int:
    blocked = [line for line in git_status() if not is_allowed(status_path(line))]
    if blocked:
        print("[bootstrap-freeze-0.1.0][error] feature freeze active; only bootstrap-real changes are allowed")
        for line in blocked:
            print(f" - {line}")
        return 1
    print("[bootstrap-freeze-0.1.0] ok: no out-of-scope changes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
