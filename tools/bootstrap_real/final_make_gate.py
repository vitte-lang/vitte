#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BOOTSTRAP_REAL = ROOT / "tools/bootstrap_real/bootstrap_real.py"


def main() -> int:
    command = [sys.executable, str(BOOTSTRAP_REAL), "--verify-chain"]
    completed = subprocess.run(
        command,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if completed.returncode != 0:
        print(completed.stdout, end="")
        sys.stdout.flush()
        print("[bootstrap-final-make][error] make must remain last: rebuild and verify stage1/stage2/release first")
        return completed.returncode
    print("[bootstrap-final-make] ok: stage1/stage2/release verified; final command may be `make`")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
