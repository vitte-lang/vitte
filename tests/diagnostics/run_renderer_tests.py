#!/usr/bin/env python3
"""Snapshot tests for the canonical terminal diagnostic renderer."""

from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main() -> int:
    cases = (("minimal.json", "location.txt"), ("multiple-labels.json", "labels.txt"))
    for fixture, snapshot in cases:
        result = subprocess.run(
            [
                "python3", "tools/render_diagnostic.py", f"tests/diagnostics/schema/{fixture}",
                "--source-root", "tests/diagnostics/sources",
            ],
            cwd=ROOT,
            check=True,
            text=True,
            capture_output=True,
        )
        expected = (ROOT / "tests/diagnostics/renderer" / snapshot).read_text(encoding="utf-8")
        if result.stdout != expected:
            raise SystemExit(f"terminal diagnostic snapshot drift ({fixture})\nexpected:\n{expected}\nactual:\n{result.stdout}")
    print("terminal diagnostic renderer ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
