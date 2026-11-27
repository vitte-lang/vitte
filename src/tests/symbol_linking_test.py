#!/usr/bin/env python3
"""Verify that C17 output from separate Vitte modules links and runs."""

from __future__ import annotations

import os
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
FIXTURES = ROOT / "src" / "tests" / "symbol_linking"
SOURCES = (FIXTURES / "module_a.vit", FIXTURES / "module_b.vit")
HARNESS = FIXTURES / "harness.c"
EXECUTABLE_SOURCE = FIXTURES / "executable.vit"


def run(command: list[str], *, cwd: Path | None = None) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        command,
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if result.returncode != 0:
        rendered = " ".join(shlex.quote(part) for part in command)
        raise RuntimeError(
            f"command failed ({result.returncode}): {rendered}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )
    return result


def verify_compiler(
    compiler: Path,
    cc: list[str],
    *,
    supports_object_output: bool,
) -> None:
    with tempfile.TemporaryDirectory(prefix="vitte-symbol-link-") as directory:
        work = Path(directory)
        executable = work / ("vitte-program.exe" if os.name == "nt" else "vitte-program")
        run(
            [
                str(compiler),
                "compile",
                str(EXECUTABLE_SOURCE),
                "-o",
                str(executable),
                "--quiet",
                "--no-color",
            ]
        )
        if not executable.is_file():
            raise RuntimeError(f"{compiler.name} did not create {executable}")
        executable_result = run([str(executable)])
        if executable_result.stdout != "42":
            raise RuntimeError(
                f"{compiler.name} executable printed {executable_result.stdout!r}; "
                "expected '42'"
            )

        generated_c_objects: list[str] = []
        emitted_objects: list[str] = []
        for index, source in enumerate(SOURCES):
            generated_c = work / f"module_{index}.c"
            c_object = work / f"module_{index}_c.o"
            emitted_object = work / f"module_{index}_emitted.o"
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
                ]
            )
            run(cc + ["-std=c17", "-c", str(generated_c), "-o", str(c_object)])
            generated_c_objects.append(str(c_object))
            if supports_object_output:
                run(
                    [
                        str(compiler),
                        "compile",
                        str(source),
                        "-c",
                        "--quiet",
                        "--no-color",
                        "-o",
                        str(emitted_object),
                    ]
                )
                if not emitted_object.is_file():
                    raise RuntimeError(
                        f"{compiler.name} did not create object {emitted_object}"
                    )
                emitted_objects.append(str(emitted_object))

        link_inputs = [("generated C", generated_c_objects)]
        if supports_object_output:
            link_inputs.append(("compiler object output", emitted_objects))
        # The native frontend qualifies exported C symbols with the module
        # path, while the compatibility bootstrap emits the source name.
        # Both object sets must remain linkable by a matching C consumer.
        harness_flags = [] if supports_object_output else ["-DVITTE_NATIVE_FRONTEND"]
        for label, objects in link_inputs:
            link_check = work / (
                f"link-check-{label.replace(' ', '-')}.exe"
                if os.name == "nt"
                else f"link-check-{label.replace(' ', '-')}"
            )
            run(
                cc
                + [
                    "-std=c17",
                    *harness_flags,
                    str(HARNESS),
                    *objects,
                    "-o",
                    str(link_check),
                ]
            )
            result = run([str(link_check)])
            if result.stdout != "402":
                raise RuntimeError(
                    f"{compiler.name} {label} linked and ran, but intrinsic "
                    f"output was {result.stdout!r}; expected '402'"
                )


def main() -> int:
    if len(sys.argv) < 2:
        print(f"usage: {Path(sys.argv[0]).name} VITTE_BINARY [VITTE_BINARY ...]", file=sys.stderr)
        return 2
    cc = shlex.split(os.environ.get("VITTE_C17_CC", "cc"))
    if not cc:
        print("VITTE_C17_CC must name a C compiler", file=sys.stderr)
        return 2
    try:
        for argument in sys.argv[1:]:
            compiler = Path(argument).resolve()
            verify_compiler(
                compiler,
                cc,
                supports_object_output=compiler.name.startswith("vitte-bootstrap"),
            )
            print(f"symbol generation and linking passed: {compiler}")
    except (OSError, RuntimeError) as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
