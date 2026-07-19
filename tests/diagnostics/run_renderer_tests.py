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
    source_shape_cases = (
        ("source-ascii.json", "ascii.vit:1:1", "proc main() -> int { give 0; }", "^^^^ ascii keyword is highlighted"),
        ("source-accents.json", "accents.vit:1:6", "proc café() -> int { give 1; }", "^^^^ accented identifier is highlighted"),
        ("source-unicode.json", "unicode.vit:1:25", "let π = 3", "^ unicode identifier is highlighted"),
        ("source-tabs.json", "tabs.vit:2:2", "\tgive 0;", "^^^^ tabbed line is highlighted"),
        ("source-long-line.json", "long-line.vit:1:24", "extremely_long_identifier_for_diagnostic_snapshot_stability", "long identifier is highlighted"),
        ("source-empty-file.json", "empty.vit:1:1", "empty source file diagnostic", "empty file diagnostic source case"),
        ("source-lf.json", "lf.vit:2:3", "  give 0;", "^^^^ lf line ending is handled"),
        ("source-crlf.json", "crlf.vit:2:3", "  give 0;", "^^^^ crlf line ending is handled"),
    )
    for fixture, location, source_fragment, marker_fragment in source_shape_cases:
        result = subprocess.run(
            [
                "python3", "tools/render_diagnostic.py", f"tests/diagnostics/schema/{fixture}",
                "--source-root", "tests/diagnostics/sources", "--color", "never",
            ],
            cwd=ROOT,
            check=True,
            text=True,
            capture_output=True,
        )
        rendered = result.stdout
        for expected_fragment in (location, source_fragment, marker_fragment):
            if expected_fragment not in rendered:
                raise SystemExit(f"diagnostic source-shape case {fixture} missed {expected_fragment!r}\n{rendered}")
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
