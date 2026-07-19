#!/usr/bin/env python3
"""Snapshot tests for the canonical terminal diagnostic renderer."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from render_diagnostic import detect_color  # noqa: E402


class FakeStream:
    def __init__(self, tty: bool) -> None:
        self.tty = tty

    def isatty(self) -> bool:
        return self.tty


def main() -> int:
    assert detect_color(FakeStream(True), {})
    assert not detect_color(FakeStream(False), {})
    assert not detect_color(FakeStream(True), {"NO_COLOR": "1"})
    assert not detect_color(FakeStream(True), {"TERM": "dumb"})
    cases = (
        ("minimal.json", "location.txt"),
        ("multiple-labels.json", "labels.txt"),
        ("multiline-span.json", "help.txt"),
        ("message-quality.json", "message-quality.txt"),
    )
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
    base_command = [
        "python3", "tools/render_diagnostic.py", "tests/diagnostics/schema/minimal.json",
        "--source-root", "tests/diagnostics/sources",
    ]
    always = subprocess.run(base_command + ["--color", "always"], cwd=ROOT, check=True, text=True, capture_output=True)
    never = subprocess.run(base_command + ["--color", "never"], cwd=ROOT, check=True, text=True, capture_output=True)
    auto = subprocess.run(base_command + ["--color", "auto"], cwd=ROOT, check=True, text=True, capture_output=True)
    if "\x1b[" not in always.stdout:
        raise SystemExit("--color always did not emit ANSI colors")
    if "\x1b[" in never.stdout or "\x1b[" in auto.stdout:
        raise SystemExit("--color never/auto emitted ANSI colors to a non-TTY")
    print("terminal diagnostic renderer ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
