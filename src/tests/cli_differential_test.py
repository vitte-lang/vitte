#!/usr/bin/env python3
"""Compare fixed C and native Vitte CLI parser fixtures byte-for-byte."""

from __future__ import annotations

import difflib
import pathlib
import subprocess
import sys


EXPECTED_CASES = (
    "valid-options",
    "valid-switches",
    "duplicate-option",
    "conflicting-options",
    "forwarded-args",
    "input-limit",
    "forward-limit",
    "missing-value",
    "invalid-value",
    "unknown-option",
)


def run_runner(label: str, path: pathlib.Path) -> str:
    if not path.is_file() or not path.stat().st_mode & 0o111:
        raise RuntimeError(
            f"{label} runner is unavailable or not executable: {path}\n"
            "Build the native Vitte runner before running test-cli-differential."
        )
    result = subprocess.run(
        [str(path)], check=False, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    if result.returncode != 0:
        raise RuntimeError(
            f"{label} runner exited with status {result.returncode}:\n{result.stderr}"
        )
    lines = result.stdout.splitlines()
    names = tuple(line.partition("|")[0] for line in lines)
    if names != EXPECTED_CASES:
        raise RuntimeError(
            f"{label} runner emitted unexpected cases.\n"
            f"Expected: {EXPECTED_CASES}\nReceived: {names}"
        )
    for number, line in enumerate(lines, start=1):
        fields = line.partition("|")[2].split(";")
        if len(fields) != 18 or any(len(field) != 16 for field in fields):
            raise RuntimeError(
                f"{label} runner emitted malformed result at line {number}: {line}"
            )
    return result.stdout


def main() -> int:
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} C_RUNNER VITTE_RUNNER", file=sys.stderr)
        return 2
    c_runner, vitte_runner = map(pathlib.Path, sys.argv[1:])
    try:
        c_output = run_runner("C", c_runner)
        vitte_output = run_runner("Vitte", vitte_runner)
    except (OSError, RuntimeError) as error:
        print(error, file=sys.stderr)
        return 2

    if c_output != vitte_output:
        diff = difflib.unified_diff(
            c_output.splitlines(keepends=True),
            vitte_output.splitlines(keepends=True),
            fromfile="C parser",
            tofile="Vitte parser",
        )
        sys.stderr.writelines(diff)
        return 1

    print(f"CLI differential parity: {len(EXPECTED_CASES)} cases match")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
