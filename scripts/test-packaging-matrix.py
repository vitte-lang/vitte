#!/usr/bin/env python3
"""Check package-matrix entries against each package builder's target listing."""

from __future__ import annotations

import os
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MATRIX = ROOT / "scripts" / "package-matrix.sh"


def run(args: list[str], *, env: dict[str, str] | None = None) -> str:
    result = subprocess.run(
        args,
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if result.returncode != 0:
        raise AssertionError(f"command failed ({result.returncode}): {args!r}\n{result.stdout}")
    return result.stdout


def rows(output: str, columns: int, label: str) -> set[tuple[str, ...]]:
    parsed: set[tuple[str, ...]] = set()
    for line in output.splitlines():
        fields = tuple(line.split())
        if len(fields) != columns:
            raise AssertionError(f"{label}: malformed target row: {line!r}")
        if fields in parsed:
            raise AssertionError(f"{label}: duplicate target row: {line!r}")
        parsed.add(fields)
    if not parsed:
        raise AssertionError(f"{label}: target list is empty")
    return parsed


def normalize(kind: str, value: str) -> str:
    return run([str(MATRIX), f"normalize-{kind}", value]).strip()


def require_covered(
    matrix: set[tuple[str, str, str]],
    targets: set[tuple[str, str, str]],
    builder: str,
) -> None:
    missing = sorted(targets - matrix)
    if missing:
        raise AssertionError(f"{builder}: targets missing from package matrix: {missing}")


def main() -> None:
    matrix_rows = rows(run([str(MATRIX), "list"]), 3, "package matrix")
    matrix: set[tuple[str, str, str]] = set(matrix_rows)  # type: ignore[assignment]

    for os_name, arch, package_format in sorted(matrix):
        found = run([str(MATRIX), "lookup", os_name, arch, package_format]).strip()
        if found != f"{os_name} {arch} {package_format}":
            raise AssertionError(f"matrix lookup changed {os_name} {arch} {package_format}: {found!r}")

    alias_lookups = {
        ("Linux", "x86_64", "deb"): "linux amd64 deb",
        ("linux", "armhf", "deb"): "linux armv7 deb",
        ("Darwin", "aarch64", "pkg"): "macos arm64 pkg",
        ("Raspberry", "armv6l", "tar.gz"): "raspberry armv6 tar.gz",
        ("FreeBSD", "powerpc64le", "pkg"): "freebsd powerpc64le pkg",
    }
    for query, expected in alias_lookups.items():
        found = run([str(MATRIX), "lookup", *query]).strip()
        if found != expected:
            raise AssertionError(f"matrix alias {query!r}: expected {expected!r}, got {found!r}")

    linux_targets = {
        ("linux", normalize("arch", arch), "deb")
        for _, arch in rows(run([str(ROOT / "scripts" / "build-linux-debs.sh"), "--list-targets"]), 2, "Linux builder")
    }
    require_covered(matrix, linux_targets, "Linux deb builder")

    portable_targets = {
        (normalize("os", os_name), normalize("arch", arch), "tar.gz")
        for os_name, arch in rows(
            run([str(ROOT / "scripts" / "build-portable-tarball.sh"), "--list-targets"]),
            2,
            "portable archive builder",
        )
    }
    require_covered(matrix, portable_targets, "portable archive builder")

    raspberry_targets = rows(
        run([str(ROOT / "scripts" / "build-raspberry-installer.sh"), "--list-targets"]),
        3,
        "Raspberry Pi builder",
    )
    require_covered(matrix, raspberry_targets, "Raspberry Pi builder")  # type: ignore[arg-type]

    freebsd_targets = {
        ("freebsd", normalize("arch", arch), "pkg")
        for _, arch in rows(
            run([str(ROOT / "scripts" / "build-freebsd-packages.sh"), "--list-targets"]),
            2,
            "FreeBSD package builder",
        )
    }
    require_covered(matrix, freebsd_targets, "FreeBSD pkg builder")

    bsd_pairs = {
        (family, arch)
        for family, _release, arch in rows(
            run([str(ROOT / "scripts" / "build-bsd-installers.sh"), "--list-targets"]),
            3,
            "BSD installer builder",
        )
    }
    bsd_targets = {(family, arch, "tar.xz") for family, arch in bsd_pairs}
    require_covered(matrix, bsd_targets, "BSD installer builder")

    native_targets = rows(
        run([str(ROOT / "scripts" / "build-macos-installers.sh"), "--list-targets"]),
        3,
        "macOS installer builder",
    )
    require_covered(matrix, native_targets, "macOS installer builder")  # type: ignore[arg-type]

    solaris_targets = rows(
        run([str(ROOT / "scripts" / "build-solaris-package.sh"), "--list-targets"]),
        3,
        "Solaris package builder",
    )
    require_covered(matrix, solaris_targets, "Solaris package builder")  # type: ignore[arg-type]

    windows_expected = {
        ("windows-xp", "i386", "exe"),
        ("windows-vista", "i386", "exe"),
        ("windows-7", "amd64", "exe"),
        ("windows-8", "amd64", "exe"),
        ("windows-8.1", "amd64", "exe"),
        ("windows-10", "amd64", "exe"),
        ("windows-11", "amd64", "exe"),
        ("windows-11", "arm64", "exe"),
    }
    require_covered(matrix, windows_expected, "Windows compatibility targets")

    all_targets = run([str(ROOT / "scripts" / "build-all-installers.sh"), "--list-targets"])
    if all_targets != run([str(MATRIX), "list"]):
        raise AssertionError("build-all-installers --list-targets differs from package-matrix.sh")

    with tempfile.TemporaryDirectory(prefix="vitte-packaging-dry-run-") as directory:
        output_dir = Path(directory) / "pkgout"
        env = os.environ.copy()
        env.update({"FAMILY": "all", "OUT_DIR": str(output_dir)})
        dry_run = run(
            [str(ROOT / "scripts" / "build-all-installers.sh"), "--dry-run"],
            env=env,
        )
        if "[dry-run]" not in dry_run or output_dir.exists():
            raise AssertionError("installer dry-run must report the plan without creating output")

    print(f"packaging matrix passed ({len(matrix)} targets)")


if __name__ == "__main__":
    main()
