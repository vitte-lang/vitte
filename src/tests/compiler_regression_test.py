#!/usr/bin/env python3
"""Compile and execute valid programs that cover known backend regressions."""

from __future__ import annotations

import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
TESTS = ROOT / "src/tests"

# The nested match fixture protects a native C17 lowering regression. The
# remaining acceptance fixtures exercise features shared by both compilers.
NATIVE_ONLY_FIXTURES = ("nested_match_codegen_regression.vit",)
SHARED_FIXTURES = (
    "generic_pick_runtime.vit",
    "generic_pick_multi_parameter_runtime.vit",
    "generic_trait_method_dispatch_runtime.vit",
    "generic_impl_trait_method_runtime.vit",
    "nullable_coalesce_runtime.vit",
    "range_for_runtime.vit",
    "defer_runtime.vit",
    "aggregate_runtime.vit",
)


def run(
    command: list[str], environment: dict[str, str]
) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        check=False,
        env=environment,
    )
    if result.returncode != 0:
        raise AssertionError(
            f"command failed ({result.returncode}): {shlex.join(command)}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    return result


def verify_fixture(
    compiler: Path,
    fixture_name: str,
    c_compiler: list[str],
    c_flags: list[str],
    environment: dict[str, str],
) -> None:
    source = TESTS / fixture_name
    if not source.is_file():
        raise AssertionError(f"valid-program regression fixture is missing: {source}")

    with tempfile.TemporaryDirectory(
        prefix=f"vitte-regression-{compiler.name}-"
    ) as directory:
        work = Path(directory)
        generated_c = work / "regression.c"
        executable = work / ("regression.exe" if os.name == "nt" else "regression")

        run(
            [str(compiler), "check", str(source), "--quiet", "--no-color"],
            environment,
        )
        run(
            [
                str(compiler),
                "compile",
                str(source),
                "--emit-c",
                "--quiet",
                "--no-color",
                "-o",
                str(generated_c),
            ],
            environment,
        )
        if not generated_c.is_file() or generated_c.stat().st_size == 0:
            raise AssertionError(
                f"{compiler.name} emitted no C17 source for {fixture_name}"
            )

        run(
            c_compiler
            + c_flags
            + ["-std=c17", str(generated_c), "-o", str(executable)],
            environment,
        )
        run([str(executable)], environment)


def verify_compiler(
    compiler: Path,
    fixture_names: tuple[str, ...],
    c_compiler: list[str],
    c_flags: list[str],
) -> None:
    environment = os.environ.copy()
    environment["VITTE_MODULE_PATH"] = os.pathsep.join(
        [str(ROOT / "modules/core/src"), str(ROOT / "src")]
    )
    for fixture_name in fixture_names:
        verify_fixture(
            compiler,
            fixture_name,
            c_compiler,
            c_flags,
            environment,
        )
        print(f"  passed: {fixture_name}")


def main() -> int:
    if len(sys.argv) != 3:
        print(
            f"usage: {Path(sys.argv[0]).name} NATIVE_COMPILER BOOTSTRAP_COMPILER",
            file=sys.stderr,
        )
        return 2

    c_compiler = shlex.split(os.environ.get("VITTE_C17_CC", "cc"))
    if not c_compiler:
        print("VITTE_C17_CC must name a C compiler", file=sys.stderr)
        return 2
    c_flags = shlex.split(os.environ.get("VITTE_C17_CFLAGS", ""))

    native = Path(sys.argv[1]).resolve()
    bootstrap = Path(sys.argv[2]).resolve()
    print(f"valid-program regressions: {native.name}")
    verify_compiler(
        native,
        NATIVE_ONLY_FIXTURES + SHARED_FIXTURES,
        c_compiler,
        c_flags,
    )
    print(f"valid-program regressions: {bootstrap.name}")
    verify_compiler(bootstrap, SHARED_FIXTURES, c_compiler, c_flags)
    print("compiler valid-program regressions passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
