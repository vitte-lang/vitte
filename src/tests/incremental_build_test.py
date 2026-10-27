#!/usr/bin/env python3
"""Check that source edits invalidate generated compiler outputs.

The current CLI accepts --incremental but does not yet provide a persistent
compilation cache. This regression test therefore checks the release-critical
contract that an incremental build never reuses output after its source changes.
"""

from __future__ import annotations

import os
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
VITTE = Path(os.environ.get("VITTE_BIN", ROOT / "build" / "bin" / ("vitte.exe" if os.name == "nt" else "vitte"))).resolve()
CC = shlex.split(os.environ.get("CC", "cc"))


def run(command: list[str], *, expected: int = 0) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, text=True, capture_output=True, timeout=120)
    if result.returncode != expected:
        raise AssertionError(
            f"command returned {result.returncode}, expected {expected}: {command!r}\n"
            f"stdout={result.stdout!r}\nstderr={result.stderr!r}"
        )
    return result


def source_for(value: int) -> str:
    return f"proc main() -> i64 {{ return {value}; }}\n"


def compile_vitte(source: Path, generated_c: Path, *, expected: int = 0) -> None:
    run(
        [
            str(VITTE), "compile", str(source), "--emit-c", "--incremental",
            "--quiet", "--no-color", "-o", str(generated_c),
        ],
        expected=expected,
    )


def compile_and_run_c(generated_c: Path, executable: Path, expected_exit: int) -> None:
    run([*CC, "-std=c17", str(generated_c), "-o", str(executable)])
    run([str(executable)], expected=expected_exit)


def main() -> int:
    if not VITTE.is_file():
        raise SystemExit(f"compiler binary not found: {VITTE}")
    if not CC:
        raise SystemExit("CC must name a C compiler")

    with tempfile.TemporaryDirectory(prefix="vitte-incremental-") as temporary:
        work = Path(temporary)
        source = work / "main.vit"
        generated_c = work / "main.c"
        executable = work / ("main.exe" if os.name == "nt" else "main")

        source.write_text(source_for(17), encoding="utf-8")
        compile_vitte(source, generated_c)
        first_output = generated_c.read_bytes()
        compile_and_run_c(generated_c, executable, 17)

        # A no-op rebuild remains deterministic.
        compile_vitte(source, generated_c)
        if generated_c.read_bytes() != first_output:
            raise AssertionError("unchanged source produced nondeterministic C output")

        # The same destination must reflect a source edit and execute its result.
        source.write_text(source_for(23), encoding="utf-8")
        compile_vitte(source, generated_c)
        second_output = generated_c.read_bytes()
        if second_output == first_output:
            raise AssertionError("incremental build reused stale output after a source edit")
        compile_and_run_c(generated_c, executable, 23)

        # A failed edit must fail the build, and a later valid edit must recover.
        source.write_text("proc main( -> i64 { return 31; }\n", encoding="utf-8")
        compile_vitte(source, generated_c, expected=1)
        source.write_text(source_for(31), encoding="utf-8")
        compile_vitte(source, generated_c)
        compile_and_run_c(generated_c, executable, 31)

    print("Incremental source invalidation passed: unchanged, edited, failed, and recovered builds.")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, subprocess.SubprocessError, AssertionError) as error:
        print(f"[incremental-build-test][error] {error}", file=sys.stderr)
        raise SystemExit(1)
