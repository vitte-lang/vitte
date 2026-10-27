#!/usr/bin/env python3
"""Run the available C17 compiler probes and report platform boundaries."""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "src" / "bootstrap"
POSIX_HEADERS = {
    "dirent.h",
    "fcntl.h",
    "spawn.h",
    "sys/stat.h",
    "sys/types.h",
    "sys/wait.h",
    "termios.h",
    "unistd.h",
}


def run(command: list[str], cwd: Path | None = None) -> bool:
    try:
        result = subprocess.run(command, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, text=True, cwd=cwd)
    except OSError:
        return False
    return result.returncode == 0


def probe_compilers() -> int:
    probes = []
    for compiler in ("cc", "gcc", "clang"):
        path = shutil.which(compiler)
        if path:
            probes.append((compiler, [path, "-std=c17", "-Wall", "-Wextra",
                                     "-Wpedantic", "-Werror", "-x", "c",
                                     "-fsyntax-only", "-" ]))
    cl = shutil.which("cl")
    if cl:
        probes.append(("cl", [cl, "/nologo", "/std:c17", "/Zs", "/TC"]))

    source = "#include <stddef.h>\nint main(void) { return 0; }\n"
    bootstrap_sources = sorted(
        str(path) for path in SOURCE_ROOT.rglob("*.c")
        if " " not in path.name
    )
    portability_test_sources = bootstrap_sources + [
        str(ROOT / "src" / "tests" / "filesystem_portability_test.c")
    ]
    source_includes = [
        "-I", str(ROOT / "src"),
        "-I", str(SOURCE_ROOT),
        "-I", str(ROOT),
        "-I", str(ROOT / "include"),
        "-I", str(ROOT / "src" / "include"),
    ]
    cl_includes = [
        "/I" + str(ROOT / "src"),
        "/I" + str(SOURCE_ROOT),
        "/I" + str(ROOT),
        "/I" + str(ROOT / "include"),
        "/I" + str(ROOT / "src" / "include"),
    ]
    unicode_source = str(SOURCE_ROOT / "unicode" / "unicode.c")
    results = []
    for name, command in probes:
        if name == "cl":
            with tempfile.NamedTemporaryFile("w", suffix=".c", delete=False) as handle:
                handle.write(source)
                filename = handle.name
            ok = run(command + [filename])
            Path(filename).unlink(missing_ok=True)
        else:
            try:
                result = subprocess.run(command, input=source, text=True,
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE)
                ok = result.returncode == 0
            except OSError:
                ok = False
        print(f"[portability] {name}: {'OK' if ok else 'FAILED'}")
        results.append(ok)
        if name == "cl" and ok:
            with tempfile.TemporaryDirectory() as directory:
                source_result = run(
                    command + cl_includes + portability_test_sources,
                    Path(directory))
            print(f"[portability] {name} bootstrap sources and portability tests: "
                  f"{'OK' if source_result else 'FAILED'}")
            results.append(source_result)
        elif name != "cl" and ok:
            source_result = run(command[:-1] + source_includes + bootstrap_sources)
            print(f"[portability] {name} bootstrap sources: "
                  f"{'OK' if source_result else 'FAILED'}")
            results.append(source_result)
        if ok:
            if name == "cl":
                unicode_result = run(command + cl_includes + [unicode_source])
            else:
                unicode_result = run(command[:-1] + source_includes +
                                     [unicode_source])
            print(f"[portability] {name} Unicode tables: "
                  f"{'OK' if unicode_result else 'FAILED'}")
            results.append(unicode_result)
    return 0 if all(results) else 1


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--strict", action="store_true",
                        help="fail when neither MSVC nor Windows-target Clang is available")
    args = parser.parse_args()

    print("[portability] POSIX boundary inventory")
    for source in sorted(SOURCE_ROOT.rglob("*.c")):
        text = source.read_text(errors="replace")
        headers = sorted(
            header for header in POSIX_HEADERS
            if re.search(rf"^\s*#include\s*<{re.escape(header)}>",
                         text, re.MULTILINE))
        if headers:
            print(f"  {source.relative_to(ROOT)}: {', '.join(headers)}")

    if probe_compilers() != 0:
        return 1
    native_windows_compiler = shutil.which("cl") is not None
    clang = shutil.which("clang")
    if clang:
        command = [clang, "--target=x86_64-pc-windows-msvc", "-std=c17",
                   "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                   "-fsyntax-only", "-I", str(ROOT / "src"),
                   "-I", str(SOURCE_ROOT),
                   str(SOURCE_ROOT / "filesystem" / "filesystem.c"),
                   str(SOURCE_ROOT / "platform" / "platform.c"),
                   str(SOURCE_ROOT / "unicode" / "unicode.c")]
        ok = run(command)
        print(f"[portability] clang Windows probe: {'OK' if ok else 'UNAVAILABLE/FAILED'}")
        if args.strict and not ok and not native_windows_compiler:
            return 1
    else:
        print("[portability] clang Windows probe: skipped (clang not installed)")
        if args.strict and not native_windows_compiler:
            return 1
    print("[portability] compiler probe complete")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
