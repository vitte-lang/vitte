#!/usr/bin/env python3
"""Regression checks for the pinned Unicode XID table generator."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "scripts" / "generate-unicode-xid.py"
PINNED_VERSION = "16.0.0"


def run(*arguments: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(GENERATOR), *arguments],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def main() -> int:
    with TemporaryDirectory(prefix="vitte-unicode-") as directory:
        temporary = Path(directory)
        first = temporary / "unicode-first.h"
        second = temporary / "unicode-second.h"

        generated = run(
            "--expected-version", PINNED_VERSION, "--output", str(first)
        )
        if generated.returncode != 0:
            raise AssertionError(generated.stderr)
        generated = run(
            "--expected-version", PINNED_VERSION, "--output", str(second)
        )
        if generated.returncode != 0:
            raise AssertionError(generated.stderr)
        if first.read_bytes() != second.read_bytes():
            raise AssertionError("Unicode generation is not byte-for-byte deterministic")

        checked = run(
            "--expected-version", PINNED_VERSION,
            "--check", "--output", str(first),
        )
        if checked.returncode != 0:
            raise AssertionError(checked.stderr)

        # A platform line-ending conversion must be detected instead of being
        # silently accepted as the same generated artifact.
        first.write_bytes(first.read_bytes().replace(b"\n", b"\r\n"))
        line_endings = run(
            "--expected-version", PINNED_VERSION,
            "--check", "--output", str(first),
        )
        if line_endings.returncode == 0:
            raise AssertionError("Unicode check accepted CRLF table bytes")

        wrong_version = run(
            "--expected-version", "15.1.0", "--output", str(temporary / "wrong.h")
        )
        if wrong_version.returncode == 0:
            raise AssertionError("Unicode generator accepted an unfrozen version")

        header = second.read_text(encoding="utf-8")
        required = (
            '#define VITTE_UNICODE_GENERATED_DATA_VERSION_STRING "16.0.0"',
            '#define VITTE_UNICODE_GENERATED_SOURCE "Python unicodedata XID properties"',
        )
        for marker in required:
            if marker not in header:
                raise AssertionError(f"generated table is missing {marker!r}")

    print("[unicode] deterministic generation and pinned-version checks: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
