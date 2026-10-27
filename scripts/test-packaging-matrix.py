#!/usr/bin/env python3
"""Validate package target inventory, aliases, and builder lookup behavior."""

from __future__ import annotations

import subprocess
import sys
import os
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "scripts" / "package-matrix.sh"
BUILDER = ROOT / "scripts" / "build-all-installers.sh"


def run(*command: str) -> str:
    result = subprocess.run(command, cwd=ROOT, text=True, capture_output=True, timeout=30)
    if result.returncode != 0:
        raise AssertionError(
            f"command failed ({result.returncode}): {command!r}\n"
            f"stdout={result.stdout!r}\nstderr={result.stderr!r}"
        )
    return result.stdout


def script_path(path: Path) -> str:
    if os.name != "nt":
        return str(path)
    result = subprocess.run(
        ["cygpath", "-u", str(path)], text=True, capture_output=True, timeout=10
    )
    if result.returncode != 0:
        raise AssertionError(f"cygpath failed for {path}: {result.stderr}")
    return result.stdout.strip()


def main() -> int:
    matrix_script = script_path(MATRIX)
    builder_script = script_path(BUILDER)
    matrix = run("sh", matrix_script, "list")
    builder = run("sh", builder_script, "--list-targets")
    if matrix != builder:
        raise AssertionError("build-all-installers target list drifted from package-matrix.sh")

    rows = [tuple(line.split()) for line in matrix.splitlines() if line.strip()]
    if len(rows) < 40:
        raise AssertionError(f"package target inventory unexpectedly small: {len(rows)} rows")
    if any(len(row) != 3 for row in rows):
        raise AssertionError("each package target must contain OS, architecture, and format")
    if len(rows) != len(set(rows)):
        raise AssertionError("package target inventory contains duplicates")

    required = {
        ("linux", "amd64", "deb"),
        ("macos", "arm64", "pkg"),
        ("macos", "arm64", "dmg"),
        ("windows-11", "amd64", "exe"),
    }
    missing = required.difference(rows)
    if missing:
        raise AssertionError(f"representative native packages missing from matrix: {sorted(missing)}")

    for os_name, arch, package_format in rows:
        actual = run("sh", matrix_script, "lookup", os_name, arch, package_format).strip()
        if actual != " ".join((os_name, arch, package_format)):
            raise AssertionError(f"matrix lookup failed for {(os_name, arch, package_format)}: {actual!r}")

    aliases = {
        ("normalize-arch", "x86_64"): "amd64",
        ("normalize-arch", "aarch64"): "arm64",
        ("normalize-os", "Darwin"): "macos",
        ("normalize-os", "Windows"): "windows",
    }
    for (operation, value), expected in aliases.items():
        actual = run("sh", matrix_script, operation, value).strip()
        if actual != expected:
            raise AssertionError(f"{operation} {value!r}: expected {expected!r}, got {actual!r}")

    print(f"Package matrix passed: {len(rows)} unique targets and {len(aliases)} aliases.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, subprocess.SubprocessError, AssertionError) as error:
        print(f"[packaging-matrix-test][error] {error}", file=sys.stderr)
        raise SystemExit(1)
