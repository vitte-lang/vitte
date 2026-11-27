#!/usr/bin/env python3
"""Compile, link, and execute a program through both shipped compilers."""

from __future__ import annotations

import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
FIXTURE = ROOT / "src/tests/compiler_pipeline_integration.vit"


def run(
    command: list[str], environment: dict[str, str] | None = None
) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        command, capture_output=True, text=True, check=False, env=environment
    )
    if result.returncode != 0:
        raise AssertionError(
            f"command failed ({result.returncode}): {shlex.join(command)}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    return result


def expect_compiler_error(
    compiler: Path, source: Path, environment: dict[str, str]
) -> None:
    command = [str(compiler), "check", str(source), "--no-color"]
    result = subprocess.run(
        command, capture_output=True, text=True, check=False, env=environment
    )
    if (
        result.returncode != 1
        or "E0401" not in result.stderr
        or "unknown symbol `answr`" not in result.stderr
        or "Did you mean `answer`" not in result.stderr
    ):
        raise AssertionError(
            f"compiler did not report the expected source error: {shlex.join(command)}\n"
            f"exit: {result.returncode}\nstdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    sanitizer_markers = (
        "AddressSanitizer",
        "LeakSanitizer",
        "UndefinedBehaviorSanitizer",
        "runtime error:",
    )
    if any(marker in result.stderr for marker in sanitizer_markers):
        raise AssertionError(
            f"sanitizer reported a failure while rejecting invalid source:\n{result.stderr}"
        )


def verify_configured_module_root(compiler: Path, work: Path) -> None:
    if not compiler.name.endswith("bootstrap"):
        return

    project_root = work / "module-root-precedence"
    source_root = project_root / "project"
    conflicting_root = project_root / "probe"
    configured_root = project_root / "configured"
    for directory in (source_root, conflicting_root, configured_root / "probe"):
        directory.mkdir(parents=True, exist_ok=True)

    main_source = source_root / "main.vit"
    main_source.write_text(
        "space app::main;\n"
        "use probe::answer::{answer};\n"
        "proc main() -> int {\n"
        "    if answer() != 42 { return 1; }\n"
        "    return 0;\n"
        "}\n",
        encoding="utf-8",
    )
    (conflicting_root / "answer.vit").write_text(
        "space probe::answer;\n"
        "export proc answer() -> int { return 41; }\n",
        encoding="utf-8",
    )
    (configured_root / "probe" / "answer.vit").write_text(
        "space probe::answer;\n"
        "export proc answer() -> int { return 42; }\n",
        encoding="utf-8",
    )

    environment = os.environ.copy()
    environment["VITTE_MODULE_PATH"] = str(configured_root)
    executable = work / (
        "module-root-precedence-bin.exe"
        if os.name == "nt"
        else "module-root-precedence-bin"
    )
    run(
        [
            str(compiler),
            "compile",
            str(main_source),
            "--quiet",
            "--no-color",
            "-o",
            str(executable),
        ],
        environment,
    )
    run([str(executable)])


def verify(compiler: Path, c_compiler: list[str]) -> None:
    with tempfile.TemporaryDirectory(prefix=f"vitte-integration-{compiler.name}-") as directory:
        work = Path(directory)
        environment = os.environ.copy()
        environment["VITTE_MODULE_PATH"] = os.pathsep.join(
            [str(ROOT / "modules/core/src"), str(ROOT / "src")]
        )
        c_flags = shlex.split(os.environ.get("VITTE_C17_CFLAGS", ""))
        generated_c = work / "pipeline.c"
        executable = work / ("pipeline.exe" if os.name == "nt" else "pipeline")

        run(
            [str(compiler), "check", str(FIXTURE), "--quiet", "--no-color"],
            environment,
        )
        run(
            [
                str(compiler),
                "compile",
                str(FIXTURE),
                "--emit-c",
                "--quiet",
                "--no-color",
                "-o",
                str(generated_c),
            ],
            environment,
        )
        assert generated_c.is_file() and generated_c.stat().st_size > 0
        run(
            c_compiler
            + c_flags
            + ["-std=c17", str(generated_c), "-o", str(executable)]
        )
        run([str(executable)], environment)
        verify_configured_module_root(compiler, work)

        invalid_source = work / "invalid-name.vit"
        invalid_source.write_text(
            "proc main() -> i64 { let answer: i64 = 1; return answr; }\n",
            encoding="utf-8",
        )
        expect_compiler_error(compiler, invalid_source, environment)


def main() -> int:
    if len(sys.argv) < 3:
        print(
            f"usage: {Path(sys.argv[0]).name} NATIVE_COMPILER BOOTSTRAP_COMPILER",
            file=sys.stderr,
        )
        return 2
    c_compiler = shlex.split(os.environ.get("VITTE_C17_CC", "cc"))
    if not c_compiler:
        print("VITTE_C17_CC must name a C compiler", file=sys.stderr)
        return 2
    for argument in sys.argv[1:3]:
        compiler = Path(argument).resolve()
        verify(compiler, c_compiler)
        print(f"compiler pipeline integration passed: {compiler.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
